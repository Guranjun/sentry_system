# 1.告警模块
## 文件组成
### 1.alarm.cpp
    告警模块的线程及各种资源建立和释放相关
#### Process_Data_Init
    模块私有数据初始化
#### process_image_thread
    告警模块线程
#### alarm_msg_release_handler
    告警模块发送消息释放函数
#### alarm_msg_handler
    告警模块接收消息函数
#### alarm_thread_wakeup
    告警模块线程退出，保证资源释放函数，在main.c调用
### 2.image_process.cpp
    图像处理相关函数
#### Move_Detect
    通过帧差法实现的运动检测函数
#### alarm_data_diff
    告警线程调用，用于对比目前警告状态，若有差异则写日志