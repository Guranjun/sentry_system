#include "UdpReceiverWorker.h"
#include "protocol.h"
#include "net_shared.h"

#include <QDebug>

UdpReceiverWorker::UdpReceiverWorker(QObject *parent)
    : QObject(parent)
{
}

void UdpReceiverWorker::initSocket()
{
    m_udpsocket = new QUdpSocket(this);
    // ShareAddress + ReuseAddressHint 允许多个接收端/快速重启不报端口占用
    bool ok = m_udpsocket->bind(m_port, QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint);
    if (ok) {
        qDebug() << "[UDP] 监听成功, 端口:" << m_port;
        connect(m_udpsocket, &QUdpSocket::readyRead, this, &UdpReceiverWorker::onReadyRead);
    } else {
        emit errorReceived(QStringLiteral("UDP 端口 %1 绑定失败: %2")
                               .arg(m_port).arg(m_udpsocket->errorString()));
    }
}

void UdpReceiverWorker::closeSocket()
{
    if (m_udpsocket == nullptr)
        return;
    m_udpsocket->disconnect();
    m_udpsocket->close();
    m_udpsocket->deleteLater();
    m_udpsocket = nullptr;
    m_frameBuffer.clear();
}

void UdpReceiverWorker::onReadyRead()
{
    while (m_udpsocket->hasPendingDatagrams()) {
        QNetworkDatagram datagram = m_udpsocket->receiveDatagram();
        QByteArray raw = datagram.data();
        if (raw.size() < FRAME_HEADER_SIZE)
            continue;

        const Frame_Header *hdr = reinterpret_cast<const Frame_Header *>(raw.constData());

        // 1. 校验帧头 magic + CRC
        if (!validateFrameHeader(reinterpret_cast<const uint8_t *>(raw.constData())))
            continue;

        uint16_t dataLen = hdr->data_len;
        uint32_t frameId = hdr->frame_id;
        uint16_t pkgCnt  = hdr->pkg_cnt;
        uint16_t pkgId   = hdr->pkg_id;
        uint32_t type    = hdr->type;

        // 2. 载荷长度与整包大小校验
        if (raw.size() < FRAME_HEADER_SIZE + dataLen)
            continue;

        QByteArray payload = raw.mid(FRAME_HEADER_SIZE, dataLen);

        // 3. 过期帧过滤（只关注 >= 已完整帧的帧）
        if (m_lastCompleteFrameId > 0 && frameId < m_lastCompleteFrameId)
            continue;

        cleanExpiredFrames(frameId);

        if (pkgCnt <= 1) {
            // 单包帧，直接派发
            dispatchFrame(type, payload);
            m_lastCompleteFrameId = frameId;
        } else {
            // 多包帧，按 frame_id/pkg_id 重组
            m_frameBuffer[frameId][pkgId] = payload;
            if (m_frameBuffer[frameId].size() == pkgCnt) {
                QByteArray complete;
                FramePackets &pkgs = m_frameBuffer[frameId];
                for (uint16_t i = 0; i < pkgCnt; ++i)
                    complete.append(pkgs.value(i));
                dispatchFrame(type, complete);
                m_frameBuffer.remove(frameId);
                m_lastCompleteFrameId = frameId;
            }
        }
    }
}

void UdpReceiverWorker::cleanExpiredFrames(uint32_t currentFrameId)
{
    if (m_frameBuffer.size() <= 3)
        return;
    QList<uint32_t> ids = m_frameBuffer.keys();
    for (uint32_t id : ids) {
        if (id < currentFrameId - 3) {
            m_frameBuffer.remove(id);
            qDebug() << "[UDP] 丢弃积压的过期帧:" << id;
        }
    }
}

void UdpReceiverWorker::dispatchFrame(uint32_t type, const QByteArray &data)
{
    switch (type) {
    case FILE_TYPE_IMAGE:
        emit image_dataReceived(data); // JPEG 数据，交给 UI 解码显示
        break;
    default:
        break;
    }
}
