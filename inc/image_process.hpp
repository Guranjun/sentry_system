#ifndef __IMAGE_PROCESS_HPP
#define __IMAGE_PROCESS_HPP
#include "common.h"
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>    // 专门用于 cvtColor, threshold, putText 等图像处理
#include <opencv2/imgcodecs.hpp>  // 专门用于 imdecode 和 imencode (JPG 编解码)
using namespace cv;
Alarm_Data Move_Detect(Mat* input_frame);
bool alarm_data_diff(Alarm_Data data);
#endif //__IMAGE_PROCESS_HPP