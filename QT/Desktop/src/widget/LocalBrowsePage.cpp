#include "LocalBrowsePage.h"

#include <QHBoxLayout>
#include <QListWidget>
#include <QLabel>
#include <QTableView>
#include <QStackedWidget>
#include <QDir>
#include <QFileInfo>
#include <QDesktopServices>
#include <QUrl>
#include <QSqlQueryModel>
#include <QSqlQuery>
#include <QSqlError>
#include <QDebug>

LocalBrowsePage::LocalBrowsePage(const QString &dir, QWidget *parent)
    : QWidget(parent), m_dir(dir)
{
    m_list = new QListWidget(this);
    m_list->setMinimumWidth(220);

    m_placeholder = new QLabel(QStringLiteral("点击左侧文件查看 / 播放"));
    m_placeholder->setAlignment(Qt::AlignCenter);

    m_dbTable = new QTableView(this);

    m_previewStack = new QStackedWidget(this);
    m_previewStack->addWidget(m_placeholder);   // index 0
    m_previewStack->addWidget(m_dbTable);       // index 1

    QHBoxLayout *layout = new QHBoxLayout(this);
    layout->addWidget(m_list, 0);
    layout->addWidget(m_previewStack, 1);

    connect(m_list, &QListWidget::itemClicked, this, [this](QListWidgetItem *item) {
        onItemClicked(item->data(Qt::UserRole).toString());
    });

    refresh();
}

void LocalBrowsePage::refresh()
{
    m_list->clear();
    QDir dir(m_dir);
    if (!dir.exists())
        return;

    // 过滤出 .avi 与 .db 文件
    QStringList filters;
    filters << QStringLiteral("*.avi") << QStringLiteral("*.db");
    QFileInfoList files = dir.entryInfoList(filters, QDir::Files, QDir::Name);

    for (const QFileInfo &fi : files) {
        QString icon = fi.suffix() == QStringLiteral("avi")
                           ? QStringLiteral("📄 ")
                           : QStringLiteral("📊 ");
        QListWidgetItem *item = new QListWidgetItem(icon + fi.fileName(), m_list);
        item->setData(Qt::UserRole, fi.absoluteFilePath());
        item->setToolTip(fi.absoluteFilePath());
    }
}

void LocalBrowsePage::onItemClicked(const QString &path)
{
    QFileInfo fi(path);
    if (!fi.exists())
        return;

    if (fi.suffix().compare(QStringLiteral("avi"), Qt::CaseInsensitive) == 0) {
        openAvi(path);
    } else if (fi.suffix().compare(QStringLiteral("db"), Qt::CaseInsensitive) == 0) {
        openDb(path);
    }
}

// 调用系统默认播放器回放 .avi（QtMultimedia 未作为初版依赖）
void LocalBrowsePage::openAvi(const QString &path)
{
    m_previewStack->setCurrentIndex(0);
    m_placeholder->setText(QStringLiteral("正在调用系统播放器打开:\n%1").arg(path));
    QDesktopServices::openUrl(QUrl::fromLocalFile(path));
}

// 解析 .db 的 logs 表并展示
void LocalBrowsePage::openDb(const QString &path)
{
    const char *kConn = "localbrowse";
    if (QSqlDatabase::contains(kConn))
        m_db = QSqlDatabase::database(kConn);
    else
        m_db = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), kConn);

    // 若上一次已打开连接，先关闭再切换数据库文件
    if (m_db.isValid() && m_db.isOpen())
        m_db.close();

    m_db.setDatabaseName(path);
    if (!m_db.open()) {
        m_previewStack->setCurrentIndex(0);
        m_placeholder->setText(QStringLiteral("打开数据库失败:\n%1").arg(m_db.lastError().text()));
        return;
    }

    m_dbModel = new QSqlQueryModel(this);
    m_dbModel->setQuery(QStringLiteral("SELECT id, level, timestamp, module, content FROM logs"), m_db);
    if (m_dbModel->lastError().isValid()) {
        m_previewStack->setCurrentIndex(0);
        m_placeholder->setText(QStringLiteral("查询 logs 表失败:\n%1").arg(m_dbModel->lastError().text()));
        return;
    }

    m_dbTable->setModel(m_dbModel);
    m_dbTable->resizeColumnsToContents();
    m_previewStack->setCurrentIndex(1);
}
