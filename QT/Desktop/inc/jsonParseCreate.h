#ifndef JSONPARSECREATE_H
#define JSONPARSECREATE_H

#include <QByteArray>
#include <QStringList>

// 与下位机 cJSON 指令/响应协议一致的 JSON 构造与解析工具。
//
// 指令格式(上位机 -> 下位机):
//   {"ver":0, "mod":<模块ID>, "cmd":<命令ID>, "type":<0控制/1查询/2上传>, "param":{...}}
//
// 查询文件列表响应(下位机 -> 上位机):
//   {"ver":0, "type":1, "status":"ok", "files":["a.avi", ...], "count":N}
// 错误响应:
//   {"ver":0, "type":1, "status":"error", "err_code":N, "err_msg":"..."}

namespace JsonCmd {

// 构造查询文件列表命令 (mod=COMMAND, cmd=CMD_STORAGE_QUERY_FILES, type=1查询)
QByteArray buildQueryFilesCommand();

// 构造上传日志数据库命令 (mod=LOGGER, cmd=CMD_LOG_UPLOAD_DB, type=0控制)
QByteArray buildUploadDbCommand();

// 构造下载指定文件命令 (mod=STORAGE, cmd=CMD_STORAGE_DOWNLOAD_FILE, type=2上传)
// 注意：下位机尚未实现该命令，仅预留格式。
QByteArray buildDownloadFileCommand(const QString &fileName);

// 解析文件列表响应，成功返回文件名列表，失败返回空列表
struct FileListResult {
    bool ok = false;
    QString status;
    int errCode = 0;
    QString errMsg;
    QStringList files;
    int count = 0;
};
FileListResult parseFileListResponse(const QByteArray &json);

} // namespace JsonCmd

#endif // JSONPARSECREATE_H
