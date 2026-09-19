#include "FileManagePage.h"
#include "TcpControlWorker.h"
#include "protocol.h"
#include "jsonParseCreate.h"

#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QTableWidget>
#include <QHeaderView>
#include <QAbstractItemView>
#include <QLabel>
#include <QDir>
#include <QDateTime>
#include <QFile>
#include <QFileInfo>

FileManagePage::FileManagePage(const QString &saveDir, QWidget *parent)
    : QWidget(parent), m_saveDir(saveDir)
{
    m_queryBtn = new QPushButton(QStringLiteral("查询下位机文件"));
    m_uploadDbBtn = new QPushButton(QStringLiteral("下载日志数据库"));
    m_statusLabel = new QLabel(QStringLiteral("未连接"));

    QHBoxLayout *btnLayout = new QHBoxLayout;
    btnLayout->addWidget(m_queryBtn);
    btnLayout->addWidget(m_uploadDbBtn);
    btnLayout->addStretch();

    m_table = new QTableWidget(0, 4, this);
    m_table->setHorizontalHeaderLabels({QStringLiteral("序号"),
                                        QStringLiteral("文件名称"),
                                        QStringLiteral("类型"),
                                        QStringLiteral("操作")});
    m_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_table->setSelectionBehavior(QAbstractItemView::SelectRows);
    m_table->setEditTriggers(QAbstractItemView::NoEditTriggers);

    QVBoxLayout *layout = new QVBoxLayout(this);
    layout->addLayout(btnLayout);
    layout->addWidget(m_table, 1);
    layout->addWidget(m_statusLabel);

    connect(m_queryBtn, &QPushButton::clicked, this, &FileManagePage::onQueryFilesClicked);
    connect(m_uploadDbBtn, &QPushButton::clicked, this, &FileManagePage::onUploadDbClicked);
}

void FileManagePage::setTcpWorker(TcpControlWorker *worker)
{
    m_worker = worker;
    if (!m_worker)
        return;
    connect(m_worker, &TcpControlWorker::jsonResponseReceived, this, &FileManagePage::onJsonResponse);
    connect(m_worker, &TcpControlWorker::fileReceived, this, &FileManagePage::onFileReceived);
    connect(m_worker, &TcpControlWorker::clientConnected, this, &FileManagePage::onClientConnected);
    connect(m_worker, &TcpControlWorker::clientDisconnected, this, &FileManagePage::onClientDisconnected);
    connect(m_worker, &TcpControlWorker::errorOccurred, this, &FileManagePage::onError);
}

void FileManagePage::setStatus(const QString &text)
{
    m_statusLabel->setText(text);
}

void FileManagePage::onClientConnected()
{
    setStatus(QStringLiteral("下位机已连接"));
}

void FileManagePage::onClientDisconnected()
{
    setStatus(QStringLiteral("下位机断开连接"));
}

void FileManagePage::onError(const QString &msg)
{
    setStatus(msg);
}

void FileManagePage::onQueryFilesClicked()
{
    if (!m_worker || !m_worker->isConnected()) {
        setStatus(QStringLiteral("下位机未连接，无法查询"));
        return;
    }
    m_table->setRowCount(0);
    m_fileNames.clear();
    setStatus(QStringLiteral("已发送查询命令，等待响应..."));
    m_worker->sendCommandFrame(FILE_TYPE_COMMAND, JsonCmd::buildQueryFilesCommand());
}

void FileManagePage::onUploadDbClicked()
{
    if (!m_worker || !m_worker->isConnected()) {
        setStatus(QStringLiteral("下位机未连接，无法下载数据库"));
        return;
    }
    setStatus(QStringLiteral("已发送下载日志数据库命令，等待文件..."));
    m_worker->sendCommandFrame(FILE_TYPE_COMMAND, JsonCmd::buildUploadDbCommand());
}

void FileManagePage::onJsonResponse(const QByteArray &json)
{
    JsonCmd::FileListResult r = JsonCmd::parseFileListResponse(json);
    if (!r.ok) {
        setStatus(QStringLiteral("响应异常: status=%1 err=%2")
                      .arg(r.status, r.errMsg.isEmpty() ? QStringLiteral("-") : r.errMsg));
        return;
    }

    m_fileNames = r.files;
    m_table->setRowCount(r.files.size());
    for (int i = 0; i < r.files.size(); ++i) {
        const QString &name = r.files.at(i);
        QFileInfo fi(name);
        QString ext = fi.suffix().isEmpty() ? QStringLiteral("-") : fi.suffix();

        m_table->setItem(i, 0, new QTableWidgetItem(QString::number(i + 1)));
        m_table->setItem(i, 1, new QTableWidgetItem(name));
        m_table->setItem(i, 2, new QTableWidgetItem(ext));

        QPushButton *dl = new QPushButton(QStringLiteral("点击下载"));
        connect(dl, &QPushButton::clicked, this, [this, name]() {
            if (!m_worker || !m_worker->isConnected()) {
                setStatus(QStringLiteral("下位机未连接"));
                return;
            }
            // 下发下载命令(下位机暂未实现 CMD_STORAGE_DOWNLOAD_FILE)
            m_worker->sendCommandFrame(FILE_TYPE_COMMAND, JsonCmd::buildDownloadFileCommand(name));
            setStatus(QStringLiteral("已发送下载命令: %1 (下位机需实现对应指令)").arg(name));
        });
        m_table->setCellWidget(i, 3, dl);
    }
    setStatus(QStringLiteral("下位机文件列表 (共 %1 个文件)").arg(r.files.size()));
}

void FileManagePage::onFileReceived(uint32_t type, const QByteArray &data, uint64_t tsUs)
{
    QDateTime dt = QDateTime::fromMSecsSinceEpoch(static_cast<qint64>(tsUs / 1000));
    QString ts = dt.toString(QStringLiteral("yyyyMMdd_HHmmss"));
    QString name = QStringLiteral("%1_%2.%3").arg(ts).arg(type).arg(fileExtName(type));
    saveFile(name, data);
    setStatus(QStringLiteral("已接收并保存: %1 (%2 字节)").arg(name).arg(data.size()));
}

QString FileManagePage::fileExtName(uint32_t type) const
{
    switch (type) {
    case FILE_TYPE_DB:    return QStringLiteral("db");
    case FILE_TYPE_VIDEO: return QStringLiteral("avi");
    case FILE_TYPE_IMAGE: return QStringLiteral("jpg");
    default:              return QStringLiteral("bin");
    }
}

void FileManagePage::saveFile(const QString &name, const QByteArray &data)
{
    QDir().mkpath(m_saveDir);
    QString path = QDir(m_saveDir).filePath(name);
    QFile f(path);
    if (f.open(QIODevice::WriteOnly)) {
        f.write(data);
        f.close();
    } else {
        setStatus(QStringLiteral("保存文件失败: %1").arg(path));
    }
}
