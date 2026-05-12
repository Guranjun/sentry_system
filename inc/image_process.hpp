#ifndef __IMAGE_PROCESS_HPP
#define __IMAGE_PROCESS_HPP
#include "common.h"
#include <stdbool>
Alarm_Data Move_Detect(Mat* input_frame);
bool alarm_data_diff(Alarm_Data data);
#endif //__IMAGE_PROCESS_HPP