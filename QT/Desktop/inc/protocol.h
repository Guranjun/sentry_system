#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <QByteArray>
#include <cstdint>

// ============ 与下位机 inc/common.h 保持一致的枚举 ============

// 文件类型 FILE_TYPE（对应 common.h）
enum FileType : uint32_t {
    FILE_TYPE_NORMAL  = 0,  // 普通数据
    FILE_TYPE_IMAGE   = 1,  // 图像(JPEG)
    FILE_TYPE_DB      = 2,  // 数据库文件
    FILE_TYPE_VIDEO   = 3,  // 视频文件
    FILE_TYPE_COMMAND = 4,  // 命令
    FILE_TYPE_JSON    = 5,  // JSON 报文
};

// 模块 ID Module_ID_e（对应 common.h）
enum ModuleId : uint32_t {
    MODULE_ID_V4L2     = 0,
    MODULE_ID_UDP      = 1,
    MODULE_ID_ALARM    = 2,
    MODULE_ID_LOGGER   = 3,
    MODULE_ID_STORAGE  = 4,
    MODULE_ID_TCP_SEND = 5,
    MODULE_ID_TCP_RECV = 6,
    MODULE_ID_COMMAND  = 7,
    MODULE_ID_MAX      = 8,
};

// 命令 ID CMD_ID（对应 common.h）
enum CmdId : uint32_t {
    CMD_NOR                 = 0x0000,
    CMD_V4L2_               = 0x0100,
    CMD_STORAGE_            = 0x0200,
    CMD_STORAGE_QUERY_FILES = 0x0201,  // 查询文件列表(下位机已实现)
    CMD_STORAGE_DOWNLOAD_FILE = 0x0202,// 下载指定文件(预留，下位机未实现)
    CMD_LOG_                = 0x0300,
    CMD_LOG_UPLOAD_DB       = 0x0301,  // 上传日志数据库(下位机已实现)
    CMD_UDP                 = 0x0400,
    CMD_TCP                 = 0x0500,
};

// ============ 协议常量 ============
#define FRAME_MAGIC       0xABCD   // 帧头魔数
#define FRAME_HEADER_SIZE 32       // 帧头固定 32 字节
#define NET_CHUNK_SIZE    1400     // 分包大小(避开以太网 MTU 1500)
#define UDP_PORT          8080     // 下位机 UDP 图传目标端口
#define TCP_PORT          8080     // 下位机 TCP 上传/指令端口

// ============ 协议工具函数 ============

// CRC16-CCITT/XMODEM，与下位机 src/crc/crc.c 及 Python 端一致(初值 0x0000，多项式 0x1021)
uint16_t crc16Xmodem(const uint8_t *data, size_t len);

// 构造一个 32 字节帧头(含 CRC)，用于发送命令帧
QByteArray buildFrameHeader(uint32_t type, uint32_t frameId, uint16_t pkgCnt,
                            uint16_t pkgId, uint16_t dataLen, uint64_t timestampUs);

// 校验帧头：magic == 0xABCD 且 CRC 正确(header 至少 32 字节)
bool validateFrameHeader(const uint8_t *header);

#endif // PROTOCOL_H
