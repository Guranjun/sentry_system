#include "image_process.hpp"
#include "msg_about.h"

using namespace std;
using namespace cv;
static Mat prev_frame_gray;
Alarm_Data Move_Detect(Mat* input_frame)
{
    //运动检测算法实现
    static bool is_first_frame = true;
    static Alarm_Data alarm_data;
    Mat current_frame_gray, frame_diff, thresh;
    // 将当前帧转换为灰度图像
    cvtColor(*input_frame, current_frame_gray, COLOR_BGR2GRAY);
    if (is_first_frame) {
        prev_frame_gray = current_frame_gray.clone();
        is_first_frame = false;
        alarm_data.status = SAFE;
        return alarm_data;// 第一帧没有前一帧可比较，直接返回
    }
    // 计算当前帧与上一帧的差异
    absdiff(current_frame_gray, prev_frame_gray, frame_diff);
    threshold(frame_diff, thresh, 25, 255, THRESH_BINARY);
    double movement_percentage = (double)countNonZero(thresh) / (thresh.rows * thresh.cols);
    if(movement_percentage > 0.1f){
        alarm_data.status = MOVED;
    }
    else{
        alarm_data.status = SAFE;
    }
    current_frame_gray.copyTo(prev_frame_gray); // 更新上一帧的灰度图像
    return alarm_data;
}
/*
void Move_Detectiom(Mat* input_frame)
{
    //static Mat prev_frame_gray; // 上一帧的灰度图像
    static bool is_first_frame = true; // 是否是第一帧的标志
    static Alarm_Data alarm_data;
    Mat current_frame_gray, frame_diff, thresh;
    // 将当前帧转换为灰度图像
    cvtColor(*input_frame, current_frame_gray, COLOR_BGR2GRAY);
    if (is_first_frame) {
        prev_frame_gray = current_frame_gray.clone();
        is_first_frame = false;
        return ;// 第一帧没有前一帧可比较，直接返回
    }
    // 计算当前帧与上一帧的差异
    absdiff(current_frame_gray, prev_frame_gray, frame_diff);
    // 对差异图像进行二值化处理，得到运动区域
    threshold(frame_diff, thresh, 25, 255, THRESH_BINARY);
    double movement_percentage = (double)countNonZero(thresh) / (thresh.rows * thresh.cols);
    if (movement_percentage > 0.02) { // 如果运动区域占比超过2%，认为有运动
        //putText(*input_frame, "Motion Detected", Point(10, 30), FONT_HERSHEY_SIMPLEX, 1, Scalar(0, 0, 255), 2);
        //发告警消息给存储模块
        if(alarm_data.status == SAFE){
            log_make(&process_data.log_msg, INFO, time(NULL), MODULE_ID_ALARM, "Detect Moved!");
            msg_dispatch(MODULE_ID_ALARM, MODULE_ID_LOGGER, sizeof(process_data.log_msg), MSG_TYPE_LOG, &process_data.log_msg);
        }
        alarm_data.status = MOVED;
        //printf("MOVED!!\n");
    }
    else{
        if(alarm_data.status == MOVED){
            log_make(&process_data.log_msg, INFO, time(NULL), MODULE_ID_ALARM, "Safe!");
            msg_dispatch(MODULE_ID_ALARM, MODULE_ID_LOGGER, sizeof(process_data.log_msg), MSG_TYPE_LOG, &process_data.log_msg);
        }
        alarm_data.status = SAFE;
    }
#ifdef MSG_ENABLE_PRIORITY
    msg_dispatch_with_priority(MODULE_ID_ALARM, MODULE_ID_STORAGE, sizeof(alarm_data), MSG_TYPE_ALARM, MSG_PRIORITY_HIGH, &alarm_data);
#else
    msg_dispatch(MODULE_ID_ALARM, MODULE_ID_STORAGE, sizeof(alarm_data), MSG_TYPE_ALARM, &alarm_data);
#endif
    current_frame_gray.copyTo(prev_frame_gray); // 更新上一帧的灰度图像
    
}*/
bool alarm_data_diff(Alarm_Data data)
{
    static Alarm_Data alarm_data;
    if(alarm_data.status != data.status){
        alarm_data.status = data.status;
        return true;
    }
    return false;
}
