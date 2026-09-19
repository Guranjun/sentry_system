#ifndef UDPRECEIVERWORKER_H
#define UDPRECEIVERWORKER_H

#include <QObject>
#include <QUdpSocket>
#include <QNetworkDatagram>
#include <QMap>
#include <QByteArray>
#include <QList>
#include <cstdint>

// UDP 视频流接收 Worker：
// 监听 UDP_PORT 端口，接收下位机分片的 MJPEG 图像帧，按 frame_id/pkg_id 重组，
// 校验 magic + CRC 后，把完整一帧 JPEG 数据通过 image_dataReceived 信号发出。
// 运行在独立 QThread 中，避免阻塞 UI。
class UdpReceiverWorker : public QObject
{
    Q_OBJECT
public:
    explicit UdpReceiverWorker(QObject *parent = nullptr);

public slots:
    void initSocket();   // 绑定端口，开始接收
    void closeSocket();  // 停止接收并释放 socket

private slots:
    void onReadyRead();

signals:
    void image_dataReceived(const QByteArray &jpegData); // 一帧完整 JPEG
    void errorReceived(const QString &errorMsg);

private:
    QUdpSocket *m_udpsocket = nullptr;
    const quint16 m_port = 8080;

    // 分片重组：frame_id -> (pkg_id -> 数据)
    typedef QMap<uint16_t, QByteArray> FramePackets;
    QMap<uint32_t, FramePackets> m_frameBuffer;
    uint32_t m_lastCompleteFrameId = 0;

    void cleanExpiredFrames(uint32_t currentFrameId);
    void dispatchFrame(uint32_t type, const QByteArray &data);
};

#endif // UDPRECEIVERWORKER_H
