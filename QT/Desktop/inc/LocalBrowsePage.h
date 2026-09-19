#ifndef LOCALBROWSEPAGE_H
#define LOCALBROWSEPAGE_H

#include <QWidget>
#include <QSqlDatabase>

class QListWidget;
class QLabel;
class QTableView;
class QStackedWidget;
class QSqlQueryModel;

// 子页面3：本地历史文件查看。
// 扫描本地保存目录(与远程下载同一目录)下的 .avi/.db 文件；
// 点击 .avi 调用系统播放器回放，点击 .db 解析 logs 表并以表格展示。
class LocalBrowsePage : public QWidget
{
    Q_OBJECT
public:
    explicit LocalBrowsePage(const QString &dir, QWidget *parent = nullptr);

public slots:
    void refresh(); // 重新扫描本地目录

private slots:
    void onItemClicked(const QString &path);

private:
    QString m_dir;
    QListWidget *m_list = nullptr;
    QStackedWidget *m_previewStack = nullptr;
    QLabel *m_placeholder = nullptr;
    QTableView *m_dbTable = nullptr;
    QSqlQueryModel *m_dbModel = nullptr;
    QSqlDatabase m_db;

    void openAvi(const QString &path);
    void openDb(const QString &path);
};

#endif // LOCALBROWSEPAGE_H
