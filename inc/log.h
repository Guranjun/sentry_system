#ifndef __LOG_H
#define __LOG_H
#include "common.h"
typedef enum{
    DEBUG = 0,//调试信息
    INFO ,//程序运行信息
    WARN ,//警告，有异常出现
    ERROR //出现错误
     
}LOG_LEVEL;
typedef struct{
    uint32_t timestamp;
    LOG_LEVEL level;
    Module_ID_e module;
    char content[64];//日志内容
}Log_Msg_t;

#endif // __LOG_H