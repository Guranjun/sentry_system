#include "protocol.h"
#include "net_shared.h"

#include <cstring>

// CRC16-CCITT/XMODEM 逐位算法。
// 与下位机 crc16_ccitt()（查表法）结果完全一致，也与 Python 端一致。
uint16_t crc16Xmodem(const uint8_t *data, size_t len)
{
    uint16_t crc = 0x0000;
    for (size_t i = 0; i < len; ++i) {
        crc ^= static_cast<uint16_t>(data[i] << 8);
        for (int b = 0; b < 8; ++b) {
            if (crc & 0x8000)
                crc = static_cast<uint16_t>((crc << 1) ^ 0x1021);
            else
                crc = static_cast<uint16_t>(crc << 1);
        }
    }
    return crc;
}

uint16_t crc16Xmodem(const QByteArray &data)
{
    return crc16Xmodem(reinterpret_cast<const uint8_t *>(data.constData()),
                       static_cast<size_t>(data.size()));
}

QByteArray buildFrameHeader(uint32_t type, uint32_t frameId, uint16_t pkgCnt,
                            uint16_t pkgId, uint16_t dataLen, uint64_t timestampUs)
{
    Frame_Header hdr;
    std::memset(&hdr, 0, sizeof(hdr));

    hdr.magic     = FRAME_MAGIC;
    hdr.data_len  = dataLen;
    hdr.frame_id  = frameId;
    hdr.pkg_cnt   = pkgCnt;
    hdr.pkg_id    = pkgId;
    hdr.type      = type;
    hdr.timestamp = timestampUs;
    hdr.reserved1 = 0;
    hdr.reserved2 = 0;
    // CRC 对“除 crc 字段之外”的前 30 字节计算
    hdr.crc = crc16Xmodem(reinterpret_cast<const uint8_t *>(&hdr), sizeof(Frame_Header) - 2);

    return QByteArray(reinterpret_cast<const char *>(&hdr), sizeof(Frame_Header));
}

bool validateFrameHeader(const uint8_t *header)
{
    const Frame_Header *hdr = reinterpret_cast<const Frame_Header *>(header);
    if (hdr->magic != FRAME_MAGIC)
        return false;
    uint16_t crc = crc16Xmodem(header, sizeof(Frame_Header) - 2);
    return crc == hdr->crc;
}
