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

DllExport cv::Mat cvw32capscr(const cv::Rect &rct, const cv::Size &sz);

}

#endif // __CVW32CAPTURESCREEN_H__
