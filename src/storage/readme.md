# 存储模块
## ffmpeg_muxer.cpp
    相关ffmpeg操作
### 1.ffmpeg_muxer_init
    写文件头
### 2.ffmpeg_muxer_write
    写入图像数据到文件中
### 3.ffmpeg_muxer_close
    写入文件尾
## storage_video.cpp
    存储模块具体实现
### 1.Storage_Data_Init
    存储模块私有数据初始化
### 2.storage_video_thread
    存储线程
### 3.storage_msg_handler
    存储模块消息接收处理函数
### 4.storage_thread_wakeup
    保证存储模块安全退出