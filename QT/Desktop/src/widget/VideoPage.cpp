#include "VideoPage.h"
#include "UdpReceiverWorker.h"

#include <QLabel>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QThread>
#include <QImage>
#include <QPixmap>
#include <QShowEvent>
#include <QHideEvent>

VideoPage::VideoPage(QWidget *parent) : QWidget(parent)
{
    m_videoLabel = new QLabel(QStringLiteral("等待视频流..."));
    m_videoLabel->setMinimumSize(640, 480);
    m_videoLabel->setAlignment(Qt::AlignCenter);
    m_videoLabel->setStyleSheet("background-color: black; color: white;");
    m_videoLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_statusLabel = new QLabel(QStringLiteral("未开始接收"));
    m_statusLabel->setAlignment(Qt::AlignCenter);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addWidget(m_videoLabel, 1);
    layout->addWidget(m_statusLabel);

    // worker 放入独立线程，UDP 收发不阻塞 UI
    m_worker = new UdpReceiverWorker;
    m_thread = new QThread(this);
    m_worker->moveToThread(m_thread);

    connect(this, &VideoPage::sigInitUdp, m_worker, &UdpReceiverWorker::initSocket);
    connect(this, &VideoPage::sigCloseUdp, m_worker, &UdpReceiverWorker::closeSocket);
    connect(m_worker, &UdpReceiverWorker::image_dataReceived, this, &VideoPage::onImageData);
    connect(m_worker, &UdpReceiverWorker::errorReceived, this, &VideoPage::onError);

    m_thread->start();
}

VideoPage::~VideoPage()
{
    emit sigCloseUdp();
    m_thread->quit();
    m_thread->wait();
    delete m_worker; // 线程已停止，安全回收
}

// 页面显示：开始接收
void VideoPage::showEvent(QShowEvent *event)
{
    if (!m_running) {
        m_running = true;
        emit sigInitUdp();
        m_statusLabel->setText(QStringLiteral("正在接收数据流... 端口: 8080"));
    }
    QWidget::showEvent(event);
}

// 页面隐藏：暂停接收，释放 CPU
void VideoPage::hideEvent(QHideEvent *event)
{
    if (m_running) {
        m_running = false;
        emit sigCloseUdp();
        m_statusLabel->setText(QStringLiteral("已暂停接收"));
    }
    QWidget::hideEvent(event);
}

void VideoPage::onImageData(const QByteArray &jpegData)
{
    // JPEG 字节流 -> QImage -> QPixmap -> 显示
    QImage img = QImage::fromData(jpegData, "JPEG");
    if (img.isNull())
        return;
    QPixmap pm = QPixmap::fromImage(img);
    m_videoLabel->setPixmap(pm.scaled(m_videoLabel->size(),
                                      Qt::KeepAspectRatio,
                                      Qt::SmoothTransformation));
}

void VideoPage::onError(const QString &msg)
{
    m_statusLabel->setText(msg);
}
