# 1.日志模块
## 1.cJSON.c
    json创建和解析文件
## 2.sqlite3.c
    sqlite相关数据库操作
## 3.sqlite_about.c
    日志模块调用数据库相关api的操作
### DB_Init
    数据库初始化
### get_db_count
    获取当前数据库条数
### delete_db_msg
    删除输入条数的数据
### db_save_batch
    存储数据进数据库
## 4.log.c
    日志线程与内部私有数据初始化释放实现
### log_init
    私有数据初始化
### log_deinit
    私有数据释放
### export_logs_on_demand
    上传日志
### log_make
    日志消息创建
### logger_process_thread
    日志线程
### logger_msg_handler
    日志模块接收消息函数
### logger_msg_release_handler
    日志模块发送消息释放函数
### logger_thread_wakeup
    保证日志模块退出时资源释放