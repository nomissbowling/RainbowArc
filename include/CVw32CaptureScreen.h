/*
  CVw32CaptureScreen.h
*/

#ifndef __CVW32CAPTURESCREEN_H__
#define __CVW32CAPTURESCREEN_H__

// use CV_PI instead of M_PI
// #define _USE_MATH_DEFINES
#include <opencv2/opencv.hpp>

#include <iomanip>
#include <iostream>
#include <sstream>
#include <vector>

// #pragma comment(linker, "/export:<alias>=<decorated_name>")
#ifdef __MAKE_DLL__
#define DllExport __declspec(dllexport)
#else
#define DllExport __declspec(dllimport)
#endif

namespace cvw32capturescreen {

using namespace std;

class DllExport CVw32CapScr {
private:
  cv::Rect rct;
  void *fakeptr;
public:
  CVw32CapScr(const cv::Rect &_rct) : rct(_rct), fakeptr(NULL) { Init(); }
  virtual ~CVw32CapScr(){ Dispose(); }
  void Init();
  void Dispose();
  cv::Mat cap(const cv::Size &sz);
};

DllExport cv::Mat pinp(cv::Mat &frm, const cv::Mat &pic);
DllExport cv::Mat pinp(cv::Mat &frm, cv::VideoCapture &cap);
DllExport cv::Mat pinp(cv::VideoCapture &cap, const cv::Mat &pic);

DllExport vector<int> getHalfMoonROI(int r);
DllExport void drawCircularROI(cv::Mat &im, const cv::Point &o, int r,
  const cv::Vec3b &bgr);

}

#endif // __CVW32CAPTURESCREEN_H__
