#include "TcpControlWorker.h"
#include "protocol.h"
#include "net_shared.h"

#include <QDebug>
#include <QDateTime>
#include <QHostAddress>

TcpControlWorker::TcpControlWorker(QObject *parent) : QObject(parent) {}

bool TcpControlWorker::isConnected() const
{
    return m_socket && m_socket->state() == QAbstractSocket::ConnectedState;
}

void TcpControlWorker::startServer(quint16 port)
{
    if (m_server)
        return;

    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &TcpControlWorker::onNewConnection);

    if (!m_server->listen(QHostAddress::Any, port)) {
        emit errorOccurred(QStringLiteral("TCP 监听端口 %1 失败: %2")
                               .arg(port).arg(m_server->errorString()));
        emit listeningChanged(false);
        return;
    }
    qDebug() << "[TCP] 开始监听, 端口:" << port;
    emit listeningChanged(true);
}

void TcpControlWorker::stopServer()
{
    if (m_socket) {
        m_socket->disconnectFromHost();
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    if (m_server) {
        m_server->close();
        m_server->deleteLater();
        m_server = nullptr;
    }
    m_buffer.clear();
    m_frameBuffer.clear();
    emit listeningChanged(false);
}

void TcpControlWorker::onNewConnection()
{
    while (m_server && m_server->hasPendingConnections()) {
        QTcpSocket *sock = m_server->nextPendingConnection();
        if (m_socket) {
            // 已有连接，拒绝新连接（当前设计单客户端）
            qDebug() << "[TCP] 已有连接，拒绝新的连接";
            sock->disconnectFromHost();
            sock->deleteLater();
            continue;
        }
        m_socket = sock;
        m_buffer.clear();
        m_frameBuffer.clear();
        m_lastCompleteFrameId = 0;
        connect(m_socket, &QTcpSocket::readyRead, this, &TcpControlWorker::onReadyRead);
        connect(m_socket, &QTcpSocket::disconnected, this, &TcpControlWorker::onDisconnected);
        qDebug() << "[TCP] 下位机已连接:" << m_socket->peerAddress().toString();
        emit clientConnected();
    }
}

void TcpControlWorker::onDisconnected()
{
    if (m_socket) {
        m_socket->deleteLater();
        m_socket = nullptr;
    }
    m_buffer.clear();
    m_frameBuffer.clear();
    qDebug() << "[TCP] 下位机断开连接";
    emit clientDisconnected();
}

void TcpControlWorker::onReadyRead()
{
    if (!m_socket)
        return;
    m_buffer.append(m_socket->readAll());
    processBuffer();
}

// 发送一帧命令：单包(pkg_cnt=1)，帧头 32 字节 + payload。
// 下位机 tcp_recv 对命令只校验 magic+CRC+data_len，不关心 type 与分片。
bool TcpControlWorker::sendCommandFrame(uint32_t type, const QByteArray &payload)
{
    if (!isConnected())
        return false;

    if (payload.size() > 512) {
        // 下位机 tcp_recv 的 MAX_FRAME_DATA_LEN = 512，命令载荷必须更小
        emit errorOccurred(QStringLiteral("命令载荷过大(%1 字节)").arg(payload.size()));
        return false;
    }

    uint64_t tsUs = static_cast<uint64_t>(QDateTime::currentMSecsSinceEpoch()) * 1000;
    QByteArray hdr = buildFrameHeader(type, m_nextFrameId++, 1, 0,
                                      static_cast<uint16_t>(payload.size()), tsUs);
    QByteArray frame = hdr + payload;
    qint64 n = m_socket->write(frame);
    m_socket->flush();
    if (n < 0) {
        emit errorOccurred(m_socket->errorString());
        return false;
    }
    return true;
}

// 从字节流中滑动解析完整帧，处理 TCP 粘包/半包
void TcpControlWorker::processBuffer()
{
    while (m_buffer.size() >= FRAME_HEADER_SIZE) {
        const uint8_t *raw = reinterpret_cast<const uint8_t *>(m_buffer.constData());

        // 帧头不对（magic 或 CRC），滑动 1 字节继续找下一帧
        if (!validateFrameHeader(raw)) {
            m_buffer.remove(0, 1);
            continue;
        }

        const Frame_Header *hdr = reinterpret_cast<const Frame_Header *>(raw);
        uint32_t frameId = hdr->frame_id;
        uint16_t pkgCnt  = hdr->pkg_cnt;
        uint16_t pkgId   = hdr->pkg_id;
        uint32_t type    = hdr->type;
        uint64_t tsUs    = hdr->timestamp;
        uint16_t dataLen = hdr->data_len;

        // 半包：载荷未收全，等待更多数据
        if (m_buffer.size() < FRAME_HEADER_SIZE + dataLen)
            break;

        QByteArray payload = m_buffer.mid(FRAME_HEADER_SIZE, dataLen);
        m_buffer.remove(0, FRAME_HEADER_SIZE + dataLen);

        // 过期帧过滤
        if (m_lastCompleteFrameId > 0 && frameId < m_lastCompleteFrameId)
            continue;
        cleanExpiredFrames(frameId);

        if (pkgCnt <= 1) {
            dispatchFrame(type, payload, tsUs);
            m_lastCompleteFrameId = frameId;
        } else {
            m_frameBuffer[frameId][pkgId] = payload;
            if (m_frameBuffer[frameId].size() == pkgCnt) {
                QByteArray complete;
                FramePackets &pkgs = m_frameBuffer[frameId];
                for (uint16_t i = 0; i < pkgCnt; ++i)
                    complete.append(pkgs.value(i));
                dispatchFrame(type, complete, tsUs);
                m_frameBuffer.remove(frameId);
                m_lastCompleteFrameId = frameId;
            }
        }
    }
}

void TcpControlWorker::cleanExpiredFrames(uint32_t currentFrameId)
{
    if (m_frameBuffer.size() <= 3)
        return;
    QList<uint32_t> ids = m_frameBuffer.keys();
    for (uint32_t id : ids) {
        if (id < currentFrameId - 3) {
            m_frameBuffer.remove(id);
            qDebug() << "[TCP] 丢弃积压的过期帧:" << id;
        }
    }
}

void TcpControlWorker::dispatchFrame(uint32_t type, const QByteArray &data, uint64_t tsUs)
{
    switch (type) {
    case FILE_TYPE_JSON:
        emit jsonResponseReceived(data);
        break;
    case FILE_TYPE_DB:
    case FILE_TYPE_VIDEO:
    case FILE_TYPE_IMAGE:
        emit fileReceived(type, data, tsUs);
        break;
    default:
        qDebug() << "[TCP] 忽略未知类型帧:" << type;
        break;
    }
}
