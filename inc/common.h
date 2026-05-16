#ifndef __COMMON_H
#define __COMMON_H

#include <cstdint>
#include <stdbool.h>
#include <pthread.h>
#include <stdint.h>
#include <time.h>

#define FRAME_HIGH 240
#define FRAME_WIDTH 320
#define V4L2_BUF_COUNT 2

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MODULE_ID_V4L2 = 0,
    MODULE_ID_UDP,
    MODULE_ID_ALARM,
    MODULE_ID_LOGGER,
    MODULE_ID_STORAGE,
    MODULE_ID_TCP,
    MODULE_ID_COMMAND,
    MODULE_ID_MAX
} Module_ID_e;

typedef enum {
    MSG_TYPE_IMAGE = 0,
    MSG_TYPE_ALARM,
    MSG_TYPE_LOG,
    MSG_TYPE_COMMAND,
    MSG_TYPE_BIGDATA
} Msg_Type_e;

typedef enum {
    SAFE = 0,
    MOVED
} Alarm_Level;

#ifdef MSG_ENABLE_PRIORITY
typedef enum {
    MSG_PRIORITY_LOW = 0,
    MSG_PRIORITY_NORMAL,
    MSG_PRIORITY_HIGH,
    MSG_PRIORITY_URGENT
} Msg_Priority_e;
#endif

typedef struct {
    Module_ID_e src_module;
    Module_ID_e dst_module;
    uint32_t data_len;
    Msg_Type_e msg_type;
    uint8_t count;
#ifdef MSG_ENABLE_PRIORITY
    Msg_Priority_e priority;
#endif
    void *data;
} Common_Msg_t;

typedef struct {
    void *data_ptr;
    uint32_t total_len;
    Msg_Type_e msg_type;
    int fd;
} BigData_Msg_t;

typedef void (*MsgHandler_t)(Common_Msg_t *msg);
typedef void (*MsgReleaseHandler_t)(Common_Msg_t *msg);

typedef enum {
    DEBUG = 0,
    INFO,
    WARN,
    ERROR
} LOG_LEVEL;

typedef struct {
    LOG_LEVEL level;
    uint64_t timestamp;
    Module_ID_e module;
    char content[64];
} Log_Msg_t;

typedef struct {
    uint64_t timestamps;
    uint32_t len;
    uint8_t *data;
    uint8_t index;
} Image_Data;

typedef struct {
} Log_Data;

typedef struct {
    Alarm_Level status;
} Alarm_Data;

typedef struct {
    Image_Data data[2];
    int latest_index;
    int status;
    pthread_mutex_t lock;
    pthread_cond_t cond;
} Camera_Udp_SharedBuffer;

typedef struct {
    uint64_t timestamp;
    uint32_t frame_id;
    uint16_t magic;
    uint16_t pkg_cnt;
    uint16_t pkg_id;
    uint16_t data_len;
    
} __attribute__((packed)) Frame_Header;

extern int running;
extern Camera_Udp_SharedBuffer camera_udp_shared_buffer;

void msg_init(void);
Common_Msg_t msg_make(Module_ID_e src, Module_ID_e dst, uint32_t len, Msg_Type_e type, void *data);
#ifdef MSG_ENABLE_PRIORITY
Common_Msg_t msg_make_with_priority(Module_ID_e src, Module_ID_e dst, uint32_t len, Msg_Type_e type, Msg_Priority_e priority, void *data);
void msg_set_priority(Common_Msg_t *msg, Msg_Priority_e priority);
#endif
int msg_send(Common_Msg_t *msg);


void V4L2_msg_release_handler(Common_Msg_t *msg);
void udp_msg_handler(Common_Msg_t *msg);
void storage_msg_handler(Common_Msg_t *msg);
void alarm_msg_release_handler(Common_Msg_t *msg);
void alarm_msg_handler(Common_Msg_t *msg);
void logger_msg_handler(Common_Msg_t *msg);
void log_make(Log_Msg_t *log_msg, LOG_LEVEL level, uint64_t timestamp, Module_ID_e module, const char *content);

#ifdef __cplusplus
}
#endif

#endif
