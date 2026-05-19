#include <cstring>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h> // socket, sendmsg, setsockopt
#include <netinet/in.h> // struct sockaddr_in
#include <arpa/inet.h>  // inet_addr
#include <sys/uio.h>    // 用于 readv/writev 和 struct iovec (sendmsg 依赖)

#include "common.h"
#include "my_time.h"
#include "tcp_send.h"

#include <stdio.h>      // 用于 printf 和 perror
#include <stdlib.h>     // 用于 exit, malloc, free
#include <string.h>     // 用于 memset, memcpy
#include <unistd.h>     // 用于 close() 函数
#include <stdint.h>     // 用于 uint8_t, uint32_t 等标准类型定义
#include <time.h>       // 用于 time() 获取时间戳
#include <stdbool.h>
typedef struct{
    int Sock;
    struct sockaddr_in dest_addr;
    uint32_t current_frame_id;
    bool is_sending;
    unsigned char* send_buf;
    uint32_t send_buf_len;
    Log_Msg_t log_msg;
    pthread_mutex_t lock;
    pthread_cond_t cond;
}Tcp_Data_Buffer;
static Tcp_Data_Buffer tcp_data_buffer;
static int Tcp_Init(Tcp_Data_Buffer* tcp_config, const char* ip, uint16_t port)
{
    memset(&tcp_config->log_msg, 0, sizeof(tcp_config->log_msg));
    tcp_config->Sock = socket(AF_INET, SOCK_STREAM, 0);
    if(tcp_config->Sock < 0){
        perror("socket");
        //log_make(&tcp_config->log_msg, ERROR, gettime_us(), "TCP init ERROR !");
        //msg_dispatch(MODULE_ID_TCP, MODULE_ID_LOGGER, sizeof(tcp_config->log_msg), MSG_TYPE_LOG, &tcp_config->log_msg);
        return -1;
    }
    memset(&tcp_config->dest_addr, 0, sizeof(tcp_config->dest_addr));
    tcp_config->dest_addr.sin_family = AF_INET;
	tcp_config->dest_addr.sin_port = htons(port);
	tcp_config->dest_addr.sin_addr.s_addr = inet_addr(ip);
    tcp_config->current_frame_id = 0;
    int send_buf_size = 1024 * 1024;
    setsockopt(tcp_config->Sock, SOL_SOCKET, SO_SNDBUF, &send_buf_size, sizeof(send_buf_size));
    printf("TCP Init Success: Target %s:%d\n", ip, port);
    tcp_config->is_sending = false;
    tcp_config->send_buf = malloc(1024 * 256);
    tcp_config->send_buf_len = 0;
    pthread_mutex_init(&tcp_config->lock, NULL);
    pthread_cond_init(&tcp_config->cond, NULL);
    return 0;
} 
void tcp_deinit(void)
{
    free(tcp_data_buffer.send_buf);
    pthread_mutex_destroy(&tcp_data_buffer.lock);
    pthread_cond_destroy(&tcp_data_buffer.cond);
    close(tcp_data_buffer.Sock);
}
void* tcp_send_thread(void* arg)
{

}
void tcp_msg_handler(Common_Msg_t* msg)
{
    //根据消息类型处理消息数据
    switch(msg->msg_type){
        case MSG_TYPE_IMAGE:{
            //处理图像数据消息
            
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
        case MSG_TYPE_BIGDATA:
            //处理大数据消息
            /*由于大数据消息占用的内存一定很大，所以udp私有的缓冲区一定不够用，这时需采用零拷贝策略，或让产生这个大数据的模块分块发送至udp发送模块*/
            break;
        default:
            break;
    }
}