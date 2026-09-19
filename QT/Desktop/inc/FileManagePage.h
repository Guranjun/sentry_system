#ifndef FILEMANAGEPAGE_H
#define FILEMANAGEPAGE_H

#include <QWidget>
#include <QStringList>
#include <cstdint>

class QPushButton;
class QTableWidget;
class QLabel;
class QProgressBar;
class TcpControlWorker;

// 子页面2：远程文件检索与下载。
// 通过 TcpControlWorker(由主窗口注入) 与下位机交互：
//  1. 下发查询文件列表命令 -> 解析 JSON 响应 -> 渲染表格；
//  2. 下发上传日志数据库命令 -> 接收 DB 文件 -> 保存本地；
//  3. 下发下载指定文件命令(预留，下位机未实现)。
class FileManagePage : public QWidget
{
    Q_OBJECT
public:
    explicit FileManagePage(const QString &saveDir, QWidget *parent = nullptr);

    // 由主窗口注入共享的 TCP worker
    void setTcpWorker(TcpControlWorker *worker);

private slots:
    void onQueryFilesClicked();
    void onUploadDbClicked();
    void onJsonResponse(const QByteArray &json);
    void onFileReceived(uint32_t type, const QByteArray &data, uint64_t tsUs);
    void onClientConnected();
    void onClientDisconnected();
    void onError(const QString &msg);

private:
    TcpControlWorker *m_worker = nullptr;
    QString m_saveDir;

    QPushButton *m_queryBtn = nullptr;
    QPushButton *m_uploadDbBtn = nullptr;
    QTableWidget *m_table = nullptr;
    QLabel *m_statusLabel = nullptr;

    QStringList m_fileNames;

    void setStatus(const QString &text);
    void saveFile(const QString &name, const QByteArray &data);
    QString fileExtName(uint32_t type) const;
};

#endif // FILEMANAGEPAGE_H
