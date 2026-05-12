#include "sqlite3.h"
#include "common.h"
#include "sqlite_about.h"
//#include "cJSON.h"
//#include <corecrt_search.h>
//#include <cstdint>
#include "log.h"
#include <string.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#define DB_PATH "/mnt/flash/syslogs.db"
#define MAX_DB_SIZE 16384   //应该设为16384
#define MAX_LOG_COUNT 50
#define DELETE_LOG_NUM 512
typedef struct{
    Log_Msg_t items[MAX_LOG_COUNT];
    int count;
}Log_Buffer_t;
typedef struct{
    Log_Buffer_t Buffer_A;
    Log_Buffer_t Buffer_B;
    Log_Buffer_t* input_ptr;//指向接收数据的日志块
    Log_Buffer_t* process_ptr;//指向待存入数据库的日志块
    pthread_mutex_t lock;
    pthread_cond_t cond;
    sqlite3* db;
}LOG_DATA_BUF;

//声明日志队列
static LOG_DATA_BUF log_data_buf;

static void log_init(void)
{   
    log_data_buf.Buffer_A.count = 0;
    log_data_buf.Buffer_B.count = 0;
    log_data_buf.input_ptr = &log_data_buf.Buffer_A;
    log_data_buf.process_ptr = &log_data_buf.Buffer_B;

    pthread_mutex_init( &log_data_buf.lock, NULL);
    pthread_cond_init( &log_data_buf.cond, NULL);
} 

static void log_deinit(void)
{
    pthread_mutex_destroy( &log_data_buf.lock);
    pthread_cond_destroy( &log_data_buf.cond);
}

void export_logs_on_demand(int count)
{
    /*如果上位机发送命令要提交日志*/
    /*采用动态的数据链路
    *从数据库采集count条数据，然后再以json的格式组成一个字符串，调用udp/tcp上传日志
    */
}

void log_make(Log_Msg_t* log_msg, LOG_LEVEL level, time_t timestamp, Module_ID_e module, char* content)
{
    log_msg->level = level;
    log_msg->timestamp = timestamp;
    log_msg->module = module;
    memcpy(log_msg->content, content, sizeof(log_msg->content));
}
void* logger_process_thread(void* arg)
{
    log_init();
    DB_Init(log_data_buf.db);
    uint16_t db_sql_count;
    while(running){
        pthread_mutex_lock(&log_data_buf.lock);
        while(log_data_buf.input_ptr->count < 30 && running){
            /*为防止一直没有新的日志到来，设置了一个定时检查写入sql的语句，防止日志在缓冲区持续堆积
            */
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 5;
            pthread_cond_timedwait(&log_data_buf.cond, &log_data_buf.lock, &ts);
            if(log_data_buf.input_ptr->count > 0){
                break;
            }
        }
        if(get_db_count(log_data_buf.db, &db_sql_count) == 0){
            if(db_sql_count >= MAX_DB_SIZE){
                delete_db_msg(log_data_buf.db, DELETE_LOG_NUM);
            }
        }
        Log_Buffer_t* temp = log_data_buf.input_ptr;
        log_data_buf.input_ptr = log_data_buf.process_ptr;
        log_data_buf.process_ptr = temp;
        pthread_mutex_unlock(&log_data_buf.lock);
        db_save_batch(log_data_buf.db, log_data_buf.process_ptr);
        
    }
    pthread_mutex_lock(&log_data_buf.lock);
    db_save_batch(log_data_buf.db, log_data_buf.input_ptr);//防止程序退出时还有没写的日志
    pthread_mutex_unlock(&log_data_buf.lock);
    
    sqlite3_close(log_data_buf.db);
    log_deinit();
    return NULL;

}
void logger_msg_handler(Common_Msg_t* msg)
{

        switch(msg->msg_type){
            case MSG_TYPE_IMAGE:
                //处理图像数据消息
                break;
            case MSG_TYPE_ALARM:
                //处理告警数据消息
                break;
            case MSG_TYPE_LOG:{
                //处理日志数据消息
                Log_Msg_t *data = msg->data ;
                pthread_mutex_lock(&log_data_buf.lock);
                memcpy(&log_data_buf.input_ptr->items[log_data_buf.input_ptr->count] , data , sizeof(Log_Msg_t));
                /*处理逻辑还未完成*/
                log_data_buf.input_ptr->items[log_data_buf.input_ptr->count].timestamp = time(NULL);
                log_data_buf.input_ptr->count++;
                if(log_data_buf.input_ptr->count > 30){
                    pthread_cond_signal(&log_data_buf.cond);
                }
                pthread_mutex_unlock(&log_data_buf.lock);
                break;
            }
            case MSG_TYPE_COMMAND:
                //处理命令数据消息
                break;
            default:
                break;
        }
}
void logger_thread_wakeup(void)
{
    pthread_mutex_lock(&log_data_buf.lock);
    pthread_cond_signal(&log_data_buf.cond);
    pthread_mutex_unlock(&log_data_buf.lock);
}