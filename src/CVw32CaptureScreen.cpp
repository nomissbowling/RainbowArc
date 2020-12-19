/*
  CVw32CaptureScreen.cpp

  test with OpenCV 3.4.12 https://github.com/opencv/opencv/releases/tag/3.4.12
    x64/vc15/bin
      opencv_world3412.dll
      opencv_ffmpeg3412_64.dll

  use Microsoft Visual Studio 2017
  x64 compiler/linker
  "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.16.27023\bin\Hostx64\x64\cl.exe"
   -source-charset:utf-8 -execution-charset:utf-8
   -EHsc

   // for make dll
    -D__MAKE_DLL__ -LD -Fe..\bin\CVw32CapScr.dll

   // for testing exe
    -Fe..\bin\CVw32CaptureScreen.exe

   ..\src\CVw32CaptureScreen.cpp
   -I..\include
   -IC:\OpenCV3\include
  (-I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.17763.0\ucrt")
  (-I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.17763.0\um")
  (-I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.17763.0\winrt")
  (-I"C:\Program Files (x86)\Windows Kits\10\Include\10.0.17763.0\shared")
   -link
   /LIBPATH:C:\OpenCV3\x64\vc15\lib
   /LIBPATH:"C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.16.27023\lib\x64"
   /LIBPATH:"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.17763.0\ucrt\x64"
   /LIBPATH:"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.17763.0\um\x64"
   /LIBPATH:"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.17763.0\winrt\x64"
   opencv_world3412.lib kernel32.lib user32.lib gdi32.lib

   // for testing exe
    ..\bin\CVw32CapScr.lib

  del ..\bin\CVw32CapScr.exp
  del ..\bin\CVw32CaptureScreen.obj

  open writer fps = 30.0 fixed (may be faster than capture cycle)
  after write fps = (frame count) * CLOCKS_PER_SEC / (end clk - start clk)
  may be CLOCKS_PER_SEC == cv::getTickFrequency()
  cv::TickMeter (version ok ?) https://note.nkmk.me/python-opencv-fps-measure/
  cv::getTickCount https://www.atmarkit.co.jp/ait/articles/1701/16/news141.html
*/

#include <windows.h>

#include "CVw32CaptureScreen.h"

using namespace cvw32capturescreen;

#ifdef __MAKE_DLL__

namespace cvw32capturescreen {

typedef struct _CVw32CapScrFake {
  BITMAPINFO bmi;
  LPDWORD buf; // set by CreateDIBSection later
  HWND wnd;
  HDC dc, mem;
  HBITMAP bmp, obmp;
} CVw32CapScrFake;

void CVw32CapScr::Init()
{
  if(!fakeptr){
    fakeptr = (void *)new CVw32CapScrFake;
    CVw32CapScrFake *p = (CVw32CapScrFake *)fakeptr;
    p->bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    p->bmi.bmiHeader.biWidth = rct.width;
    p->bmi.bmiHeader.biHeight = -rct.height; // reverse top bottom
    p->bmi.bmiHeader.biPlanes = 1;
    p->bmi.bmiHeader.biBitCount = 24; // or 32
    p->bmi.bmiHeader.biCompression = BI_RGB;
    p->wnd = GetDesktopWindow();
    p->dc = GetDC(p->wnd);
    p->mem = CreateCompatibleDC(p->dc);
    p->bmp = CreateDIBSection(p->dc, &p->bmi, DIB_RGB_COLORS, // or RGBA
      (void **)&p->buf, NULL, 0);
    p->obmp = (HBITMAP)SelectObject(p->mem, p->bmp);
  }
}

void CVw32CapScr::Dispose()
{
  if(fakeptr){
    CVw32CapScrFake *p = (CVw32CapScrFake *)fakeptr;
    SelectObject(p->mem, p->obmp);
    DeleteObject(p->bmp);
    DeleteDC(p->mem);
    ReleaseDC(p->wnd, p->dc);
    delete fakeptr;
    fakeptr = NULL;
  }
}

cv::Mat CVw32CapScr::cap(const cv::Size &sz)
{
  if(!fakeptr) return cv::Mat(sz.height, sz.width, CV_8UC3); // or CV_8UC4
  // cv::Mat im = cv::imread("Rainbow_pen.png", -1);
  cv::Mat im(rct.height, rct.width, CV_8UC3); // or CV_8UC4
  CVw32CapScrFake *p = (CVw32CapScrFake *)fakeptr;
  BitBlt(p->mem, 0, 0, rct.width, rct.height, p->dc, rct.x, rct.y, SRCCOPY);
  GetDIBits(p->mem, p->bmp, 0, rct.height, im.data, &p->bmi, DIB_RGB_COLORS); // or RGBA
  // cv::cvtColor(im, im, CV_BGR2RGB);
  cv::resize(im, im, sz, 0, 0, cv::INTER_LANCZOS4);
  return im;
}

DllExport cv::Mat pinp(cv::Mat &frm, const cv::Mat &pic)
{
  cv::Size sz(frm.cols / 2, frm.rows / 2);
  cv::Size thinfrm(4, 3);
  cv::Size szPinP(sz.width - 2 * thinfrm.width, sz.height - 2 * thinfrm.height);
  cv::Point ptPinP(sz.width + thinfrm.width, sz.height + thinfrm.height);
#if 0
  cv::Rect roi(ptPinP, szPinP);
  cv::Mat frm_roi = frm(roi);
#if 0 // PinP 1/4:1/4 (cut by same scale roi)
  cv::Mat pic_roi = pic(roi);
  pic_roi.copyTo(frm_roi);
#else // PinP 1:1/4 (all area)
  cv::Mat tmp;
  cv::resize(pic, tmp, roi.size(), 0, 0, cv::INTER_LANCZOS4);
  tmp.copyTo(frm_roi);
#endif
#else
  cv::Size fsz = frm.size();
#if 0
  cv::Size psz = pic.size();
  vector<cv::Point2f> src({cv::Point(0, 0), cv::Point(psz.width, 0),
    cv::Point(psz.width, psz.height)});
  vector<cv::Point2f> dst({ptPinP, ptPinP + cv::Point(szPinP.width, 0),
    ptPinP + cv::Point(szPinP.width, szPinP.height)});
  cv::Mat m = cv::getAffineTransform(src, dst);
  cv::warpAffine(pic, frm, m, fsz, cv::INTER_LANCZOS4, cv::BORDER_TRANSPARENT);
#else
  double r = (double)szPinP.width / pic.size().width; // same with height ratio
#if 1
  cv::Mat m = (cv::Mat_<double>(2, 3) << r, 0, ptPinP.x, 0, r, ptPinP.y);
  cv::warpAffine(pic, frm, m, fsz, cv::INTER_LANCZOS4, cv::BORDER_TRANSPARENT);
#else
  cv::Mat m = (cv::Mat_<double>(3, 3) << r, 0, ptPinP.x, 0, r, ptPinP.y, 0, 0, 1);
  cv::warpPerspective(pic, frm, m, fsz, cv::INTER_LANCZOS4, cv::BORDER_TRANSPARENT);
#endif
#endif
#endif
  return frm;
}

DllExport cv::Mat pinp(cv::Mat &frm, cv::VideoCapture &cap)
{
  cv::Mat tmp;
  cap >> tmp;
  return pinp(frm, tmp);
}

DllExport cv::Mat pinp(cv::VideoCapture &cap, const cv::Mat &pic)
{
  cv::Mat tmp;
  cap >> tmp;
  return pinp(tmp, pic);
}

DllExport cv::Mat rotROI(const cv::Mat &im, const cv::RotatedRect &rrct,
  int mode, const cv::Vec3b &bgr)
{
  cv::Mat m = cv::getRotationMatrix2D(rrct.center, rrct.angle, 1.0);
  cv::Mat rot;
  if(mode == cv::BORDER_TRANSPARENT) im.copyTo(rot);
  cv::warpAffine(im, rot, m, im.size(), cv::INTER_LANCZOS4, mode, bgr);
  cv::Mat crp;
  cv::getRectSubPix(rot, rrct.size, rrct.center, crp);
  return crp;
}

DllExport cv::Mat rotRectPts(const cv::RotatedRect &rrct)
{
  const int npts = 4;
  cv::Point2f vert2f[npts];
  rrct.points(vert2f);
#if 1
  vector<cv::Point> vec(npts);
  for(int i = 0; i < npts; ++i) vec[i] = vert2f[i];
  return cv::Mat(vec, true); // copy vec memory
#else
  cv::Mat_<cv::Point> vm_(npts, 1);
//  for(int i = 0; i < npts; ++i) vm_ << vert2f[i]; // BAD (no compile error)
  vm_ << vert2f[0], vert2f[1], vert2f[2], vert2f[3]; // OK
//  vm_ << vert2f; // BAD compile error
  return vm_;
#endif
}

DllExport void drawRotRect(cv::Mat &im, const cv::RotatedRect &rrct, int th,
  const cv::Vec3b &bgr)
{
  cv::Mat vm = rotRectPts(rrct);
  cv::Point *ppts[] = {(cv::Point *)vm.data};
  int pnpts[] = {vm.rows};
  cv::polylines(im, ppts, pnpts, sizeof(ppts)/sizeof(ppts[0]), true, bgr, th);
}

DllExport void drawRotRectFill(cv::Mat &im, const cv::RotatedRect &rrct,
  const cv::Vec3b &bgr)
{
  cv::Mat vm = rotRectPts(rrct);
  cv::fillConvexPoly(im, (cv::Point *)vm.data, vm.rows, bgr);
}

DllExport vector<int> getHalfMoonROI(int r)
{
  vector<int> vec(r + 1);
  for(int y = 0; y <= r; ++y) vec[y] = cvRound(sqrt((double)r * r - y * y));
  return vec;
}

DllExport void drawCircularROI(cv::Mat &im, const cv::Point &o, int r,
  const cv::Vec3b &bgr)
{
  vector<int> xvec = getHalfMoonROI(r);
  cv::Mat_<cv::Vec3b> &pix = (cv::Mat_<cv::Vec3b> &)im;
  for(int y = -r; y <= r; ++y){
    int xc = xvec[abs(y)];
    for(int x = -xc; x <= xc; ++x) pix(o.y + y, o.x + x) = bgr;
  }
}

}

#else

string test_cvw32capscr(int ac, char **av)
{
  vector<string> wn({"original", "gray", "Hue", "Alpha"});
  for(vector<string>::iterator i = wn.begin(); i != wn.end(); ++i)
    cv::namedWindow(*i, CV_WINDOW_AUTOSIZE | CV_WINDOW_FREERATIO);
  int cam_id = 0; // 1; // 0; // may be 'ManyCam Virtual Webcam'
  int width = 640, height = 480, fourcc;
  double fps = 30.0;
  cv::VideoCapture cap(cv::CAP_DSHOW + cam_id); // use DirectShow
  if(!cap.isOpened()) return "error: open camera";
  // cout << cv::getBuildInformation() << endl;
  if(!cap.set(cv::CAP_PROP_FRAME_WIDTH, width)) cout << "width err" << endl;
  if(!cap.set(cv::CAP_PROP_FRAME_HEIGHT, height)) cout << "height err" << endl;
  if(!cap.set(cv::CAP_PROP_FPS, fps)) cout << "fps err" << endl;
  // fourcc = cv::VideoWriter::fourcc('M', 'J', 'P', 'G');
  // fourcc = cv::VideoWriter::fourcc('m', 'p', '4', 'v');
  // fourcc = cv::VideoWriter::fourcc('X', 'V', 'I', 'D');
  // fourcc = cv::VideoWriter::fourcc('D', 'I', 'V', 'X');
  // fourcc = cv::VideoWriter::fourcc('X', '2', '6', '4');
  fourcc = 0x00000020; // fallback tag
  bool col = true;
  cv::VideoWriter wr("CapScr.mp4", fourcc, fps, cv::Size(width, height), col);
  CVtickFPS tfps(10);
  cv::Mat frm;
#if 0
  while(cap.read(frm)){
#else // fake input from screen
  CVw32CapScr cvw32cs(cv::Rect(960, 512, 320, 240));
  while(true){
    frm = cvw32cs.cap(cv::Size(width, height));
#endif
    int cnt = tfps.getCnt();
    tfps.update();
    tfps.dsp(frm);
    cv::Point ct(300, 220);
    drawCircularROI(frm, ct, 32, cv::Vec3b(32, 192, 240));
    drawCircularROI(frm, cv::Point(320, 240), 8, cv::Vec3b(240, 32, 192));
    int rot = cnt % 360;
    cv::RotatedRect rf = cv::RotatedRect(ct, cv::Size(24, 16), 60 + rot);
    drawRotRectFill(frm, rf, cv::Vec3b(192, 240, 32));
    cv::RotatedRect rr = cv::RotatedRect(ct, cv::Size(16, 8), 30);
    drawRotRect(frm, rr, 2, cv::Vec3b(32, 240, 192));
    cv::RotatedRect rrct = cv::RotatedRect(ct, cv::Size(480, 360), rot);
    cv::Mat rroi;
#if 1
    rroi = rotROI(frm, rrct, cv::BORDER_TRANSPARENT);
#else
#if 0
    rroi = rotROI(frm, rrct, cv::BORDER_CONSTANT, cv::Vec3b(192, 32, 240));
#else
    rroi = rotROI(frm, rrct);
#endif
#endif
    frm = pinp(frm, rroi);
    cv::imshow(wn[0], frm);
#if 1 // PinP
    // cv::Mat im = pinp(frm, cap);
    cv::Mat im = pinp(cap, frm);
#else
    cv::Mat im(frm);
#endif
    wr << im;
    cv::imshow(wn[3], im);
    int k = cv::waitKey(1); // 1ms > 15ms ! on Windows
    if(k == 'q' || k == '\x1b') break;
  }
  wr.release();
  cap.release();
  cv::destroyAllWindows();
  // should be rewrite with true fps (re open writer and copy all frames)
  // but average fps may not be same for each frames
  return "ok";
}

int main(int ac, char **av)
{
  cout << test_cvw32capscr(ac, av) << endl;
  return 0;
}

#endif
