# 1.UDP模块
## 1.模块内部函数说明
### 1.send_packet_optimized
    拼接待发送的数据和数据帧头
### 2.Udp_Init
    udp相关设置初始化，udp模块私有数据初始化
### 3.Udp_Send_Frame
    udp发送私有数据缓冲区的数据，如果大小超过MTU限制，分帧发送
### 4.udp_send_thread
    udp发送线程
### 5.udp_msg_handler
    当有数据通过消息模块发送给udp模块时，消息模块调用这个函数，并把数据存入udp私有数据区
### 6.udp_thread_wakeup
    当进程结束时回收线程
# 2.TCP模块
## 1.模块内部函数说明
### 1.Tcp_Close_Socket
    关闭套接字
### 2.Tcp_Open_And_Connect
    tcp套接字打开并连接
### 3.Tcp_Init
    tcp模块私有数据初始化
### 4.Tcp_Deinit
    tcp模块私有数据释放
### 5.Tcp_Ensure_Send_Buffer
    保证当前数据缓冲区足够接收待发送的数据
### 6.Tcp_Send_All
    保证待发送的数据完全被发送，防止中断导致数据混乱
### 7.Tcp_Send_Packet
    发送数据，调用Tcp_Send_All
### 8.Tcp_Send_Frame
    分帧发送数据
### 9.tcp_send_thread
    tcp发送线程
### 10.tcp_msg_handler
    tcp模块接收消息处理函数
### 11.tcp_thread_wakeup
    保证tcp模块安全退出