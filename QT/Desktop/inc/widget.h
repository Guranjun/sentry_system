#ifndef WIDGET_H
#define WIDGET_H

#include <QWidget>

class QStackedWidget;
class QButtonGroup;
class VideoPage;
class FileManagePage;
class LocalBrowsePage;
class TcpControlWorker;

// 主窗口：顶部导航栏(实时视频 / 远程文件管理 / 本地历史查看) + 子页面堆栈。
// 持有共享的 TcpControlWorker(异步事件驱动，主线程)，并注入文件管理页。
class Widget : public QWidget
{
    Q_OBJECT
public:
    explicit Widget(QWidget *parent = nullptr);
    ~Widget() override;

private slots:
    void onNavChanged(int index);

private:
    QButtonGroup *m_navGroup = nullptr;
    QStackedWidget *m_stack = nullptr;

    VideoPage *m_videoPage = nullptr;
    FileManagePage *m_filePage = nullptr;
    LocalBrowsePage *m_localPage = nullptr;

    TcpControlWorker *m_tcpWorker = nullptr;
    QString m_saveDir;

    void buildUi();
};

#endif // WIDGET_H
