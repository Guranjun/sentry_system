#include "widget.h"
#include "VideoPage.h"
#include "FileManagePage.h"
#include "LocalBrowsePage.h"
#include "TcpControlWorker.h"
#include "protocol.h"

#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QPushButton>
#include <QStackedWidget>
#include <QButtonGroup>
#include <QCoreApplication>
#include <QStringList>

Widget::Widget(QWidget *parent) : QWidget(parent)
{
    setWindowTitle(QStringLiteral("智能哨兵系统上位机"));
    resize(960, 640);

    // 本地保存目录：与远程下载共用，便于“下载后本地查看”联动
    m_saveDir = QCoreApplication::applicationDirPath() + QStringLiteral("/received_files");

    buildUi();

    // TCP 服务端：监听 8080，等待下位机连接（QTcpServer 异步，不阻塞 UI）
    m_tcpWorker = new TcpControlWorker(this);
    m_tcpWorker->startServer(TCP_PORT);
    m_filePage->setTcpWorker(m_tcpWorker);
}

Widget::~Widget() = default;

void Widget::buildUi()
{
    // 顶部导航按钮
    const QStringList navLabels = {QStringLiteral("实时视频"),
                                   QStringLiteral("远程文件管理"),
                                   QStringLiteral("本地历史查看")};
    m_navGroup = new QButtonGroup(this);
    m_navGroup->setExclusive(true);

    QHBoxLayout *navLayout = new QHBoxLayout;
    for (int i = 0; i < navLabels.size(); ++i) {
        QPushButton *btn = new QPushButton(navLabels.at(i));
        btn->setCheckable(true);
        m_navGroup->addButton(btn, i);
        navLayout->addWidget(btn);
    }
    navLayout->addStretch();

    // 子页面堆栈
    m_videoPage = new VideoPage(this);
    m_filePage = new FileManagePage(m_saveDir, this);
    m_localPage = new LocalBrowsePage(m_saveDir, this);

    m_stack = new QStackedWidget(this);
    m_stack->addWidget(m_videoPage);   // 0
    m_stack->addWidget(m_filePage);    // 1
    m_stack->addWidget(m_localPage);   // 2

    QVBoxLayout *mainLayout = new QVBoxLayout(this);
    mainLayout->addLayout(navLayout);
    mainLayout->addWidget(m_stack, 1);

    // 导航切换
    connect(m_navGroup, &QButtonGroup::idClicked, this, &Widget::onNavChanged);

    // 默认选中第一个
    if (m_navGroup->button(0))
        m_navGroup->button(0)->setChecked(true);
    m_stack->setCurrentIndex(0);
}

void Widget::onNavChanged(int index)
{
    m_stack->setCurrentIndex(index);
    // 进入本地浏览页时刷新文件列表（显示刚下载的文件）
    if (index == 2)
        m_localPage->refresh();
}
