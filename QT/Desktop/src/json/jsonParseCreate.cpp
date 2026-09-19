#include "jsonParseCreate.h"
#include "protocol.h"

#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace JsonCmd {

// 构造通用命令 JSON
static QByteArray buildCommand(uint32_t mod, uint32_t cmd, uint32_t type, const QJsonObject &param)
{
    QJsonObject root;
    root["ver"] = 0;
    root["mod"] = static_cast<int>(mod);
    root["cmd"] = static_cast<int>(cmd);
    root["type"] = static_cast<int>(type);
    root["param"] = param;
    return QJsonDocument(root).toJson(QJsonDocument::Compact);
}

QByteArray buildQueryFilesCommand()
{
    return buildCommand(MODULE_ID_COMMAND, CMD_STORAGE_QUERY_FILES, 1, QJsonObject());
}

QByteArray buildUploadDbCommand()
{
    return buildCommand(MODULE_ID_LOGGER, CMD_LOG_UPLOAD_DB, 0, QJsonObject());
}

QByteArray buildDownloadFileCommand(const QString &fileName)
{
    QJsonObject param;
    param["file"] = fileName;
    return buildCommand(MODULE_ID_STORAGE, CMD_STORAGE_DOWNLOAD_FILE, 2, param);
}

FileListResult parseFileListResponse(const QByteArray &json)
{
    FileListResult r;
    QJsonParseError err;
    QJsonDocument doc = QJsonDocument::fromJson(json, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        r.status = QStringLiteral("parse_error");
        r.errMsg = err.errorString();
        return r;
    }

    QJsonObject root = doc.object();
    r.status = root.value("status").toString();
    r.errCode = root.value("err_code").toInt();
    r.errMsg = root.value("err_msg").toString();

    if (r.status != QStringLiteral("ok")) {
        return r;
    }

    QJsonArray arr = root.value("files").toArray();
    for (const QJsonValue &v : arr)
        r.files.append(v.toString());
    r.count = root.value("count").toInt(r.files.size());
    r.ok = true;
    return r;
}

} // namespace JsonCmd
