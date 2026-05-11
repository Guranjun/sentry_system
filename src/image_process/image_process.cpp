#include "image_process.hpp"
#include "common.h"
#include <cstdint>
#include <opencv2/opencv.hpp>
#include <opencv2/imgproc.hpp>    // 专门用于 cvtColor, threshold, putText 等图像处理
#include <opencv2/imgcodecs.hpp>  // 专门用于 imdecode 和 imencode (JPG 编解码)
#include <iostream>
#include <string.h>
#include <vector>
using namespace cv;
using namespace std;
//进行运动检测所需的全局变量
//static Mat prev_frame_gray; // 上一帧的灰度图像
//static bool is_first_frame = true; // 是否是第一帧的标志
/*图像处理功能*/
typedef struct{
    uint8_t frame_buffer[128 * 1024];
    uint32_t frame_len;
    Common_Msg_t msg;
    Log_Msg_t log_msg;
    bool is_updated;
    pthread_mutex_t lock;
    pthread_cond_t cond;
}Process_Data;
static Process_Data process_data; 
void Process_Data_Init(void)
{
    //process_data.frame_buffer = malloc(128 * 1024);
    process_data.frame_len = 0;
    process_data.is_updated = false;
    memset(&process_data.log_msg, 0, sizeof(process_data.log_msg));
    memset(&process_data.msg, 0, sizeof(process_data.msg));
    pthread_mutex_init(&process_data.lock, NULL);
    pthread_cond_init(&process_data.cond, NULL);
}
void Move_Detectiom(Mat* input_frame)
{
    static Mat prev_frame_gray; // 上一帧的灰度图像
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
        alarm_data.status = MOVED;
        log_make(&process_data.log_msg, INFO, time(NULL), MODULE_ID_ALARM, "Detect Moved!");
        process_data.msg = msg_make(MODULE_ID_ALARM, MODULE_ID_LOGGER, sizeof(process_data.log_msg), MSG_TYPE_LOG, &process_data.log_msg);
        msg_send(&process_data.msg);
        //printf("MOVED!!\n");
    }
    else{
        alarm_data.status = SAFE;
        log_make(&process_data.log_msg, INFO, time(NULL), MODULE_ID_ALARM, "Safe!");
        process_data.msg = msg_make(MODULE_ID_ALARM, MODULE_ID_LOGGER, sizeof(process_data.log_msg), MSG_TYPE_LOG, &process_data.log_msg);
        msg_send(&process_data.msg);
    }
    static Common_Msg_t msg = msg_make(MODULE_ID_ALARM, MODULE_ID_STORAGE, sizeof(alarm_data), MSG_TYPE_ALARM, &alarm_data);
    msg_send(&msg);
    current_frame_gray.copyTo(prev_frame_gray); // 更新上一帧的灰度图像
    
}
#ifdef __cplusplus
extern "C" 
{
#endif
void* process_image_thread(void* arg) 
{    
    Process_Data_Init();
    while (running) {
        pthread_mutex_lock(&process_data.lock);
        while(!process_data.is_updated && running){
            pthread_cond_wait(&process_data.cond, &process_data.lock);
        }
        if(!running){
            pthread_mutex_unlock(&process_data.lock);
            break; // 如果应用正在停止，提前退出循环
        }
        process_data.is_updated = false;
        uint8_t* data_to_process = process_data.frame_buffer;
        uint32_t frame_len = process_data.frame_len;
        pthread_mutex_unlock(&process_data.lock);
        /*第二步：进行图像处理逻辑*/
        /*先将jpg数据转成cv2能读懂的图像数据形式*/
        Mat raw_data_mat( 1, frame_len, CV_8UC1, data_to_process);
        Mat img = imdecode(raw_data_mat, IMREAD_COLOR);
        if(img.empty()){
            cerr << "Failed to decode image" << endl;
            //写个日志
            continue;
        }

        // 这里可以添加任何图像处理算法
        Move_Detectiom(&img);

    }
    return nullptr;
}

void alarm_msg_release_handler(Common_Msg_t* msg)
{
    (void*) msg;
}
void alarm_msg_handler(Common_Msg_t* msg)
{
     switch(msg->msg_type){
        case MSG_TYPE_IMAGE:{
            //处理图像数据消息
            Image_Data* img_data = (Image_Data*)msg->data;
            pthread_mutex_lock(&process_data.lock);
            memcpy(process_data.frame_buffer, img_data->data, img_data->len);
            process_data.frame_len = img_data->len;
            process_data.is_updated = true;
            pthread_cond_signal(&process_data.cond);
            pthread_mutex_unlock(&process_data.lock);
            break;
        }
        case MSG_TYPE_ALARM:
            //处理告警数据消息
            break;
        case MSG_TYPE_LOG:
            //处理日志数据消息
            break;
        case MSG_TYPE_COMMAND:
            //处理命令数据消息
            break;
        default:
            break;
    }
}
void alarm_thread_wakeup(void)
{
    pthread_mutex_lock(&process_data.lock);
    pthread_cond_signal(&process_data.cond);
    pthread_mutex_unlock(&process_data.lock);
}
#ifdef __cplusplus
}
#endif