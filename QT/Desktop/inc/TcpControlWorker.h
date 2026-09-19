#ifndef TCPCONTROLWORKER_H
#define TCPCONTROLWORKER_H

#include <QObject>
#include <QTcpServer>
#include <QTcpSocket>
#include <QMap>
#include <QList>
#include <QByteArray>
#include <cstdint>

// TCP 控制 Worker（上位机作为服务端）：
// 下位机(板子)是 TCP 客户端，主动 connect 到本机 TCP_PORT。
// 本类负责：
//  1. 监听并接受下位机连接；
//  2. 向下位机下发命令帧(查询列表 / 上传DB / 下载文件)；
//  3. 接收下位机回传的数据帧，按 frame_id/pkg_id 重组，
//     区分 JSON 响应与文件数据后通过信号发出。
// QTcpServer/QTcpSocket 均为异步事件驱动，不会阻塞 UI。
class TcpControlWorker : public QObject
{
    Q_OBJECT
public:
    explicit TcpControlWorker(QObject *parent = nullptr);
    bool isConnected() const;

public slots:
    void startServer(quint16 port);                       // 开始监听
    void stopServer();                                    // 停止监听
    bool sendCommandFrame(uint32_t type, const QByteArray &payload); // 发送一帧命令

signals:
    void listeningChanged(bool listening);
    void clientConnected();
    void clientDisconnected();
    void jsonResponseReceived(const QByteArray &json);                       // type==JSON
    void fileReceived(uint32_t type, const QByteArray &data, uint64_t tsUs); // 文件数据(DB/VIDEO/IMAGE)
    void errorOccurred(const QString &msg);

private slots:
    void onNewConnection();
    void onReadyRead();
    void onDisconnected();

private:
    QTcpServer *m_server = nullptr;
    QTcpSocket *m_socket = nullptr;

    QByteArray m_buffer;               // 接收滑动缓冲
    typedef QMap<uint16_t, QByteArray> FramePackets;
    QMap<uint32_t, FramePackets> m_frameBuffer;  // 分片重组
    uint32_t m_lastCompleteFrameId = 0;
    uint32_t m_nextFrameId = 0;        // 发送帧ID自增

    void processBuffer();
    void dispatchFrame(uint32_t type, const QByteArray &data, uint64_t tsUs);
    void cleanExpiredFrames(uint32_t currentFrameId);
};

#endif // TCPCONTROLWORKER_H
