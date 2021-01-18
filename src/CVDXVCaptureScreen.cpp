/*
  CVDXVCaptureScreen.cpp

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
    -D__MAKE_DLL__ -LD -Fe..\bin\CVDXVCapScr.dll

   // for testing exe
    -Fe..\bin\CVDXVCaptureScreen.exe

   ..\src\CVDXVCaptureScreen.cpp
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

   // for make dll
    ole32.lib strmiids.lib

   // for testing exe
    ..\bin\CVDXVCapScr.lib

  del ..\bin\CVDXVCapScr.exp
  del ..\bin\CVDXVCaptureScreen.obj
*/

#define UNICODE
#define _UNICODE
#include <wchar.h>

#define WIN32_LEAN_AND_MEAN
#define _WIN32_DCOM
#define _CRT_SECURE_NO_DEPRECATE
#include <windows.h>
#include <dshow.h>
// download qedit.h and edit it
#include <qedit.h> // CLSID_SampleGrabber ISampleGrabber IID_ISampleGrabber

#include "CVDXVCaptureScreen.h"

using namespace cvdxvcapturescreen;

#ifdef __MAKE_DLL__

namespace cvdxvcapturescreen {

typedef struct _CVDXVCapScrFake {
  BITMAPINFO bmi;
  AM_MEDIA_TYPE amt;
  IBaseFilter *pbf;
  IGraphBuilder *pGraph;
  IBaseFilter *pF;
  ISampleGrabber *pGrab;
  ICaptureGraphBuilder2 *pCapture;
  IMediaControl *pMC;
} CVDXVCapScrFake;

void CVDXVCapScr::Init()
{
  if(fakeptr) return;
  fakeptr = (void *)new CVDXVCapScrFake;
  CVDXVCapScrFake *p = (CVDXVCapScrFake *)fakeptr;
  ZeroMemory(p, sizeof(CVDXVCapScrFake));

  HRESULT hr = CoInitialize(NULL);
  if(FAILED(hr)){
    fprintf(stderr, "CoInitialize Failed\n");
    delete p; fakeptr = NULL; return;
  }
  ICreateDevEnum *pDevEnum = NULL;
  CoCreateInstance(CLSID_SystemDeviceEnum, NULL, CLSCTX_INPROC,
    IID_ICreateDevEnum, (void **)&pDevEnum);
  IEnumMoniker *pClassEnum = NULL;
  pDevEnum->CreateClassEnumerator(CLSID_VideoInputDeviceCategory, &pClassEnum, 0);
  if(!pClassEnum){
    fprintf(stderr, "Device does not exist\n");
    pDevEnum->Release();
    CoUninitialize();
    delete p; fakeptr = NULL; return;
  }
  // find Nth filter
  // p->pbf = NULL;
  for(int c = 0; c < devId + 1; ++c){
    if(p->pbf){ p->pbf->Release(); p->pbf = NULL; }
    ULONG cFetched;
    IMoniker *pMoniker = NULL;
    if(pClassEnum->Next(1, &pMoniker, &cFetched) == S_OK){
      pMoniker->BindToObject(0, 0, IID_IBaseFilter, (void**)&p->pbf);
      pMoniker->Release();
    }
  }
  pClassEnum->Release();
  pDevEnum->Release();
  if(!p->pbf){
    fprintf(stderr, "Video Capture Filter not found\n");
    CoUninitialize();
    delete p; fakeptr = NULL; return;
  }

  // p->pGraph = NULL;
  CoCreateInstance(CLSID_FilterGraph, NULL, CLSCTX_INPROC,
    IID_IGraphBuilder, (void **)&p->pGraph);
  p->pGraph->AddFilter(p->pbf, L"Video Capture");

  // p->pF = NULL;
  CoCreateInstance(CLSID_SampleGrabber, NULL, CLSCTX_INPROC_SERVER,
    IID_IBaseFilter, (LPVOID *)&p->pF);
  // p->pGrab = NULL;
  p->pF->QueryInterface(IID_ISampleGrabber, (void **)&p->pGrab);
  // ZeroMemory(&p->amt, sizeof(AM_MEDIA_TYPE));
  p->amt.majortype = MEDIATYPE_Video;
  p->amt.subtype = MEDIASUBTYPE_RGB24; // check
  p->amt.formattype = FORMAT_VideoInfo;
  p->pGrab->SetMediaType(&p->amt);
  p->pGraph->AddFilter(p->pF, L"Grabber");

  // p->pCapture = NULL;
  CoCreateInstance(CLSID_CaptureGraphBuilder2, NULL, CLSCTX_INPROC,
    IID_ICaptureGraphBuilder2, (void **)&p->pCapture);
  p->pCapture->SetFiltergraph(p->pGraph);
  p->pCapture->RenderStream(&PIN_CATEGORY_PREVIEW, &MEDIATYPE_Video,
    p->pbf, NULL, p->pF);

  // p->pMC = NULL;
  p->pGraph->QueryInterface(IID_IMediaControl, (LPVOID *)&p->pMC);
  p->pMC->Run(); // start render
  p->pGrab->SetBufferSamples(TRUE); // Grab

  p->pGrab->GetConnectedMediaType(&p->amt);
  fprintf(stdout, "amt.lSampleSize = %dbytes\n", p->amt.lSampleSize);
  VIDEOINFOHEADER *pVIH = (VIDEOINFOHEADER *)p->amt.pbFormat;
  // ZeroMemory(&p->bmi, sizeof(p->bmi));
  BITMAPINFOHEADER *pH = &p->bmi.bmiHeader;
  CopyMemory(pH, &pVIH->bmiHeader, sizeof(BITMAPINFOHEADER));
  fprintf(stdout, "%dx%d %dbits\n", pH->biWidth, pH->biHeight, pH->biBitCount);
}

void CVDXVCapScr::Dispose()
{
  if(fakeptr){
    CVDXVCapScrFake *p = (CVDXVCapScrFake *)fakeptr;
    if(p->pMC){ p->pMC->Release(); p->pMC = NULL; } // Filter Graph
    if(p->pCapture){ p->pCapture->Release(); p->pCapture = NULL; } // Capture Graph
    if(p->pGrab){ p->pGrab->Release(); p->pGrab = NULL; }
    if(p->pF){ p->pF->Release(); p->pF = NULL; } // Sample Grabber
    if(p->pGraph){ p->pGraph->Release(); p->pGraph = NULL; }
    if(p->pbf){ p->pbf->Release(); p->pbf = NULL; } // Capture Filter
    CoUninitialize();
    delete p;
    fakeptr = NULL;
  }
}

cv::Mat CVDXVCapScr::cap(const cv::Size &sz)
{
  if(!fakeptr) return cv::Mat(sz.height, sz.width, CV_8UC3); // or CV_8UC4
  cv::Mat im(sz.height, sz.width, CV_8UC3); // or CV_8UC4
  CVDXVCapScrFake *p = (CVDXVCapScrFake *)fakeptr;
  BITMAPINFOHEADER *pH = &p->bmi.bmiHeader;
  HRESULT hr = p->pGrab->GetCurrentBuffer(
    (long *)&pH->biSizeImage, (long *)im.data);
  if(FAILED(hr)) return cv::Mat(sz.height, sz.width, CV_8UC3); // or CV_8UC4
  cv::flip(im, im, 0); // reverse top <-> bottom
  // cv::cvtColor(im, im, CV_BGR2RGB);
  cv::resize(im, im, sz, 0, 0, cv::INTER_LANCZOS4);
  return im;
}

}

#else

string test_cvdxvcapscr(int ac, char **av)
{
  vector<string> wn({"original", "gray", "Hue", "Alpha"});
  for(vector<string>::iterator i = wn.begin(); i != wn.end(); ++i)
    cv::namedWindow(*i, CV_WINDOW_AUTOSIZE | CV_WINDOW_FREERATIO);
  int cam_id = 1; // 1; // 0; // may be 'ManyCam Virtual Webcam'
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
  cv::VideoWriter wr("CapSDXV.mp4", fourcc, fps, cv::Size(width, height), col);
//  CVtickFPS tfps(10);
  cv::Mat frm, scr, usbcam, im;
#if 0
  while(cap.read(frm)){
#else // fake input from DirectShow Filter Video Capture
  CVDXVCapScr cvdxvcs2(2); // 640 x 480 24bits
  CVDXVCapScr cvdxvcs1(1); // 320 x 240 24bits (size miss match)
  CVDXVCapScr cvdxvcs0(0); // 640 x 480 24bits
  while(true){
    frm = cvdxvcs2.cap(cv::Size(width, height)); // grab ok
    scr = cvdxvcs1.cap(cv::Size(width, height)); // grab size miss match
    usbcam = cvdxvcs0.cap(cv::Size(width, height)); // grab ok
#endif
//    int cnt = tfps.getCnt();
//    tfps.update();
//    tfps.dsp(frm);
    cv::imshow(wn[0], frm);
    cv::imshow(wn[1], scr);
    cv::imshow(wn[2], usbcam);
    cap.read(im);
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
  cout << test_cvdxvcapscr(ac, av) << endl;
  return 0;
}

#endif
