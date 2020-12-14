/*
  RainbowArc.cpp

  test with OpenCV 3.4.12 https://github.com/opencv/opencv/releases/tag/3.4.12
    x64/vc15/bin
      opencv_world3412.dll
      opencv_ffmpeg3412_64.dll

  use Microsoft Visual Studio 2017
  x64 compiler/linker
  "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.16.27023\bin\Hostx64\x64\cl.exe"
   -source-charset:utf-8 -execution-charset:utf-8
   -EHsc -Fe..\bin\RainbowArc.exe ..\src\RainbowArc.cpp
   -I..\include
   -IC:\OpenCV3\include
   -link
   /LIBPATH:C:\OpenCV3\x64\vc15\lib
   /LIBPATH:"C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\VC\Tools\MSVC\14.16.27023\lib\x64"
   /LIBPATH:"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.17763.0\ucrt\x64"
   /LIBPATH:"C:\Program Files (x86)\Windows Kits\10\Lib\10.0.17763.0\um\x64"
   opencv_world3412.lib
   ..\bin\CVw32CapScr.lib
  del ..\bin\RainbowArc.obj
*/

#include "RainbowArc.h"
#include "CVw32CaptureScreen.h"

using namespace rainbowarc;
using namespace cvw32capturescreen;

string rainbow(int ac, char **av)
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
  cv::VideoWriter wr("Rainbow.mp4", fourcc, fps, cv::Size(width, height), col);
  int cnt = 0;
  cv::Mat frm;
#if 0
  while(cap.read(frm)){
#else
  CVw32CapScr cvw32cs(cv::Rect(800, 200, 320, 240));
  while(true){
#if 0 // fake input from image
    frm = cv::imread("Rainbow_pen.png", -1);
    // cv::cvtColor(frm, frm, CV_BGR2RGB);
    cv::resize(frm, frm, cv::Size(width, height), 0, 0, cv::INTER_LANCZOS4);
#else // fake input from screen
    frm = cvw32cs.cap(cv::Size(width, height));
    frm = pinp(frm, cap);
    // frm = pinp(cap, frm);
#endif
#endif
    cv::imshow(wn[0], frm);
    cv::Mat gr, hsv;
    cv::cvtColor(frm, gr, CV_BGR2GRAY);
    cv::GaussianBlur(gr, gr, cv::Size(7, 7), 1.5, 1.5);
    // cv::LUT(gr, gammaLUT, gr);
    cv::imshow(wn[1], gr);
    cv::cvtColor(frm, hsv, CV_BGR2HSV);
    vector<cv::Mat> pl; // H S V planes
    cv::split(hsv, pl);
#if 1
    uchar *u = gr.data;
    for(int j = 0; j < hsv.rows; ++j){
      uchar *h = pl[0].ptr<uchar>(j);
      uchar *s = pl[1].ptr<uchar>(j);
      uchar *v = pl[2].ptr<uchar>(j);
      for(int i = 0; i < hsv.cols; ++i){
        uchar q = *u++;
        bool o = true;
        uchar hue = h[i];
#if 1 // straight cloud arc fan
        uchar t = (uchar)(179 * ((hsv.rows - j) + i) / (hsv.rows + hsv.cols));
#endif
        hue = 179 - ((t + 15) % 180);
        // if(hue > 135) o = false; // 270 < Hue < 360
        h[i] = cv::saturate_cast<uchar>((o ? cnt + hue : h[i]) % 180); // H
        s[i] = cv::saturate_cast<uchar>(255 - q); // s[i]); // S
        v[i] = cv::saturate_cast<uchar>(64 + 191 * q / 255); // v[i]); // V
      }
    }
#endif
    cv::merge(pl, hsv);
    cv::cvtColor(hsv, hsv, CV_HSV2BGR); // assume BGR as HSV
    cv::imshow(wn[2], hsv); // hsv.channels() == 3
#if 1
    cv::Mat im(frm.rows, frm.cols, CV_8UC4);
    vector<cv::Mat> pa; // B G R A planes
    cv::split(im, pa);
    cv::split(hsv, pl);
    pa[0] = pl[0];
    pa[1] = pl[1];
    pa[2] = pl[2];
    pa[3] = 255;
    cv::merge(pa, im);
#else
    cv::Mat im(frm);
    hsv.copyTo(im, pl[1]);
#endif
#if 1 // PinP
    cv::cvtColor(im, im, CV_BGRA2BGR); // to be same as cap channels (CV_8UC3)
    // im = pinp(im, cap);
    im = pinp(cap, im);
#endif
    wr << im;
    cv::imshow(wn[3], im);
    ++cnt;
    int k = cv::waitKey(1); // 1ms > 15ms ! on Windows
    if(k == 'q' || k == '\x1b') break;
  }
  wr.release();
  cap.release();
  cv::destroyAllWindows();
  return "ok";
}

int main(int ac, char **av)
{
  cout << rainbow(ac, av) << endl;
  return 0;
}
