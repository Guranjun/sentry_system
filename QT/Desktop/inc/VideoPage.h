#ifndef VIDEOPAGE_H
#define VIDEOPAGE_H

#include <QWidget>

class QLabel;
class QThread;
class UdpReceiverWorker;

// 子页面1：实时视频流。
// 持有 UdpReceiverWorker(运行在独立线程)，页面显示时自动开始接收 UDP 视频流，
// 页面隐藏时自动暂停，避免离开页面后仍占用 CPU 接收/解码。
class VideoPage : public QWidget
{
    Q_OBJECT
public:
    explicit VideoPage(QWidget *parent = nullptr);
    ~VideoPage() override;

protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;

signals:
    void sigInitUdp();   // 通知 worker 线程开始接收
    void sigCloseUdp();  // 通知 worker 线程停止接收

private slots:
    void onImageData(const QByteArray &jpegData);
    void onError(const QString &msg);

private:
    QLabel *m_videoLabel = nullptr;
    QLabel *m_statusLabel = nullptr;

    QThread *m_thread = nullptr;
    UdpReceiverWorker *m_worker = nullptr;
    bool m_running = false;
};

#endif // VIDEOPAGE_H
