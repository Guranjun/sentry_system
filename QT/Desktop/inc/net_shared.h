#ifndef NET_SHARED_H
#define NET_SHARED_H

#include <stdint.h>

// 与下位机 inc/common.h 的 Frame_Header 完全一致（32 字节，1 字节对齐）。
// 协议约定整帧使用“小端字节序”传输：下位机 ARM 与上位机 x86 均为小端，
// 结构体按内存布局直接读写即可，无需 hton/ntoh 转换。
//
// 编译环境为 MinGW(GCC)/Linux(GCC)，用 __attribute__((packed)) 做 1 字节对齐。
// 若未来改用 MSVC 编译，需改用 #pragma pack(push,1) / #pragma pack(pop) 包裹。

struct Frame_Header {
    uint16_t magic;      // 帧头标志 0xABCD
    uint16_t data_len;   // 载荷数据长度
    uint32_t frame_id;   // 帧ID
    uint16_t pkg_cnt;    // 分包总数
    uint16_t pkg_id;     // 分包ID
    uint32_t type;       // 数据类型 FILE_TYPE
    uint64_t timestamp;  // 时间戳(微秒)
    uint32_t reserved1;  // 4字节保留位
    uint16_t reserved2;  // 2字节保留位
    uint16_t crc;        // CRC16-CCITT/XMODEM(对前30字节计算)
} __attribute__((packed));

#endif // NET_SHARED_H
