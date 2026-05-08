#include "sqlite3.h"
#include "common.h"
#include "cJSON.h"
#include <corecrt_search.h>
#include <cstdint>
#include <stdint.h>
#include <stdlib.h>
#include <time.h>
#define DB_PATH "/mnt/flash/syslogs.db"
#define MAX_DB_SIZE 16384
#define MAX_LOG_COUNT 50
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
    pthread_mutex_destroy( &log_data_buf.lock, NULL);
    pthread_cond_destroy( &log_data_buf.cond, NULL);
}
static void DB_Init(void)
{
    if(sqlite3_open(DB_PATH, &log_data_buf.db) != SQLITE_OK){
        fprintf(stderr, "Can't open the database:%s", sqlite3_errmsg(log_data_buf.db));
        return;
    }
    const char* sql = "CREATE TABLE IF NOT EXISTS logs("
                      "id INTEGER PRIMARY KEY AUTOINCREMENT,"
                      "level INTEGER,"
                      "timestamp INTEGER,"
                      "module INTEGER,"
                      "content TEXT);";
    sqlite3_exec(log_data_buf.db, sql, NULL, NULL, NULL);
}
static uint8_t get_db_count(sqlite3* db, uint16_t* out_count)
{  
    sqlite3_stmt* stmt = NULL;
    int count = 0;
    const char* sql = "SELECT COUNT(*) FROM logs;";
    int rc = sqlite3_prepare_v2(db, sql, -1, &stmt, NULL);
    if(rc == SQLITE_OK){
        if(sqlite3_step(stmt) == SQLITE_ROW){
            count = sqlite3_column_int(stmt, 0);
        }
        else{
            fprintf(stderr, "SQL step error: %s\n", sqlite3_errmsg(db));
        }
    }
    else{
        fprintf(stderr, "SQL prepare error: %s (code: %d)\n", sqlite3_errmsg(db), rc);
    }
    sqlite3_finalize(stmt);
    *out_count = count;
    return 0;
}
static uint8_t delete_db_msg(sqlite3* db, uint16_t delete_num)
{
    /*写一个除level = error外删除delete_num行的语句，删除普通的调试、程序信息
    不过最好分表先，建两个表，error一个表，其它的一个表
    */
}
void export_logs_on_demand(int count)
{
    /*如果上位机发送命令要提交日志*/
    /*采用动态的数据链路
    *从数据库采集count条数据，然后再以json的格式组成一个字符串，调用udp/tcp上传日志
    */
}
static void db_save_batch(Log_Buffer_t* buffer)
{
    if(buffer->count <=0){
        return;
    }
    sqlite3_exec(log_data_buf.db, "BEGIN TRANSACTION;", NULL, NULL,NULL);
    sqlite3_stmt* stmt;
    const char* sql = "INSERT INTO logs (level, timestamp, module, content) VALUES (?, ?, ?, ?);";
    sqlite3_prepare_v2(log_data_buf.db, sql, -1, &stmt, NULL);

    for(int i = 0; i < buffer->count; i++){
        sqlite3_bind_int(stmt, 1, buffer->items[i].level);
        sqlite3_bind_int64(stmt, 2, buffer->items[i].timestamp);
        sqlite3_bind_int(stmt, 3, buffer->items[i].module);
        sqlite3_bind_text(stmt, 4, buffer->items[i].content, -1, SQLITE_STATIC);
        sqlite3_step(stmt);
        sqlite3_reset(stmt);
    }
    sqlite3_finalize(stmt);
    sqlite3_exec(log_data_buf.db, "COMMIT;", NULL, NULL, NULL);
    buffer->count = 0;
}
void* logger_process_thread(void* arg)
{
    log_init();
    DB_Init();
    while(running){
        pthread_mutex_lock(&log_data_buf.lock);
        while(log_data_buf.input_ptr->count < 30 && running){
            struct timespec ts;
            clock_gettime(CLOCK_REALTIME, &ts);
            ts.tv_sec += 5;
            pthread_cond_timedwait(&log_data_buf.cond, &log_data_buf.lock, &ts);
            if(log_data_buf.input_ptr->count > 0){
                break;
            }
        }
        Log_Buffer_t* temp = log_data_buf.input_ptr;
        log_data_buf.input_ptr = log_data_buf.process_ptr;
        log_data_buf.process_ptr = temp;
        pthread_mutex_unlock(&log_data_buf.lock);
        db_save_batch(log_data_buf.process_ptr);
        
    }
    pthread_mutex_lock(&log_data_buf.lock);
    db_save_batch(log_data_buf.input_ptr);
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