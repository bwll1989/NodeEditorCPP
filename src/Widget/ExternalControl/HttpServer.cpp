#include "HttpServer.hpp"
#include "ActionRegistry.h"
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMetaObject>
#include <QCoreApplication>
#include <QJsonArray>
#include <QDateTime>
#include <QUuid>

#include "Common/Devices/StatusContainer/StatusContainer.h"
#include "Common/Devices/OSCSender/OSCSender.h"
#include "Common/AppConfig/ConfigManager.h"
#include "Common/AppConfig/ConstantDefines.h"
#include "Common/Log/LogRingBuffer.hpp"
#include "OSCMessage.h"
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/NetException.h>
#include <Poco/Exception.h>
#include <Poco/Timespan.h>
#include <Poco/URI.h>
#include <Poco/FileStream.h>
#include <Poco/Path.h>
#include <Poco/StreamCopier.h>
#include <sstream>
#include <vector>
#include <algorithm>
#include <cctype>
#include <QMetaType>
#include <QFileInfo>
#include <QDir>
Q_DECLARE_METATYPE(OSCMessage)
using namespace Poco::Net;
using namespace Poco;
using namespace Flow;

namespace {

void setDynamicCacheControl(HTTPServerResponse& response) {
    response.set("Cache-Control", "no-cache");
}

bool isBenignWebSocketDisconnect(const Poco::Exception& exc) {
    const std::string msg = exc.displayText();
    return msg.find("Connection reset by peer") != std::string::npos
        || msg.find("Broken pipe") != std::string::npos
        || msg.find("Connection aborted") != std::string::npos;
}

void logWebSocketPocoException(const char* context, const Poco::Exception& exc) {
    if (isBenignWebSocketDisconnect(exc)) {
        qDebug() << context << exc.displayText().c_str();
    } else {
        qWarning() << context << exc.displayText().c_str();
    }
}

/** @brief 解析并校验磁盘日志文件名，防止路径穿越 */
QString resolveLogFilePath(const QString& name)
{
    if (name.isEmpty()) {
        return {};
    }

    const QString base = QFileInfo(name.trimmed()).fileName();
    if (base.isEmpty() || base != name.trimmed()) {
        return {};
    }
    if (!base.startsWith(QStringLiteral("log")) || !base.endsWith(QStringLiteral(".txt"))) {
        return {};
    }
    if (base.contains(QLatin1Char('/')) || base.contains(QLatin1Char('\\'))) {
        return {};
    }

    QDir logsDir(AppConstants::LOGS_STORAGE_DIR);
    if (!logsDir.exists()) {
        return {};
    }

    const QString absPath = logsDir.absoluteFilePath(base);
    const QFileInfo info(absPath);
    if (!info.exists() || !info.isFile()) {
        return {};
    }

    const QString canonicalLogs = logsDir.canonicalPath();
    const QString canonicalFile = info.canonicalFilePath();
    if (canonicalFile.isEmpty() || !canonicalFile.startsWith(canonicalLogs)) {
        return {};
    }

    return canonicalFile;
}

/** @brief 解析并校验媒体库文件名，防止路径穿越 */
QString resolveMediaFilePath(const QString& name)
{
    if (name.isEmpty()) {
        return {};
    }

    const QString base = QFileInfo(name.trimmed()).fileName();
    if (base.isEmpty() || base != name.trimmed()) {
        return {};
    }
    if (base == QStringLiteral(".") || base == QStringLiteral("..")) {
        return {};
    }
    if (base.contains(QLatin1Char('/')) || base.contains(QLatin1Char('\\'))) {
        return {};
    }

    QDir mediaDir(AppConstants::MEDIA_LIBRARY_STORAGE_DIR);
    if (!mediaDir.exists()) {
        return {};
    }

    const QString absPath = mediaDir.absoluteFilePath(base);
    const QFileInfo info(absPath);
    if (!info.exists() || !info.isFile()) {
        return {};
    }

    const QString canonicalRoot = mediaDir.canonicalPath();
    const QString canonicalFile = info.canonicalFilePath();
    if (canonicalFile.isEmpty() || !canonicalFile.startsWith(canonicalRoot)) {
        return {};
    }

    return canonicalFile;
}

} // namespace

/** @brief WebSocket 长连接超时：读侧保持长等待；发送必须有上限，避免退出/广播卡死主线程 */
void configureWebSocketTimeouts(Poco::Net::WebSocket& ws)
{
    // 读超时：周期醒来以便连接被对端/本端关闭后能尽快退出循环（0=无限，会导致 stop 卡住）
    ws.setReceiveTimeout(Poco::Timespan(30, 0));
    // 发送超时：半开连接上 SO_SNDTIMEO=0 可能永久阻塞 broadcastJson（Qt 主线程）
    ws.setSendTimeout(Poco::Timespan(2, 0));
}

// ===== PageWebSocketHandler =====
PageWebSocketHandler::PageWebSocketHandler(NodeHttpServer& server)
    : _server(server) {}

void PageWebSocketHandler::handleRequest(HTTPServerRequest& request,
                                         HTTPServerResponse& response) {
    setDynamicCacheControl(response);
    try {
        Poco::Net::WebSocket ws(request, response);
        configureWebSocketTimeouts(ws);
        _ws = &ws;
        _server.registerWebSocket(this);
        
        std::vector<char> chunk(65536);
        int flags = 0;
        int n = 0;
        std::string accum;
        for (;;) {
            try {
                n = ws.receiveFrame(chunk.data(), (int)chunk.size(), flags);
            } catch (const Poco::TimeoutException&) {
                // 读超时：继续等；若 stop()/forceClose() 已关掉套接字，下次会抛其它异常退出
                if (!_ws) {
                    break;
                }
                continue;
            }
            if (n <= 0 || (flags & WebSocket::FRAME_OP_BITMASK) == WebSocket::FRAME_OP_CLOSE) {
                break;
            }
            const int op = flags & WebSocket::FRAME_OP_BITMASK;
            if (op == WebSocket::FRAME_OP_PING) {
                QMutexLocker locker(&_sendMutex);
                ws.sendFrame(chunk.data(), n, WebSocket::FRAME_OP_PONG | WebSocket::FRAME_FLAG_FIN);
                continue;
            }
            accum.append(chunk.data(), n);
            if (flags & WebSocket::FRAME_FLAG_FIN) {
                QByteArray payload(accum.data(), (int)accum.size());
                QJsonParseError err;
                QJsonDocument doc = QJsonDocument::fromJson(payload, &err);
                if (err.error == QJsonParseError::NoError && doc.isObject()) {
                    QJsonObject obj = doc.object();
                    if (obj.contains("query") && obj["query"].isArray()) {
                        QJsonArray queries = obj["query"].toArray();
                        for (const auto& val : queries) {
                            QString addr = val.toString();
                            if (StatusContainer::instance()->contains(addr)) {
                                StatusItem item = StatusContainer::instance()->last(addr);
                                QJsonObject resp = item.toJsonObject();
                                std::string msg = QJsonDocument(resp).toJson(QJsonDocument::Compact).toStdString();
                                send(msg);
                            }
                        }
                    }
                    QString addr = obj.value("address").toString();
                    if (addr.isEmpty()) addr = obj.value("addr").toString();
                    if (!addr.isEmpty()) {
                        OSCMessage msg;
                        msg.host = "127.0.0.1";
                        msg.port = ConfigManager::instance().getExtraControlPort();
                        msg.address = addr;
                        msg.value = obj["value"].toVariant();
                        QMetaObject::invokeMethod(StatusContainer::instance(),
                                                  "parseOSC",
                                                  Qt::QueuedConnection,
                                                  Q_ARG(OSCMessage, msg));
                    }
                }
                accum.clear();
            }
        }
        
        _server.unregisterWebSocket(this);
        _ws = nullptr;
    } catch (const Poco::Net::WebSocketException& exc) {
        _server.unregisterWebSocket(this);
        _ws = nullptr;
        // 日志记录异常
        qWarning() << "WebSocket Exception: " << exc.displayText().c_str();
        switch (exc.code()) {
        case WebSocket::WS_ERR_HANDSHAKE_UNSUPPORTED_VERSION:
            response.set("Sec-WebSocket-Version", WebSocket::WEBSOCKET_VERSION);
            // fallthrough
        case WebSocket::WS_ERR_NO_HANDSHAKE:
        case WebSocket::WS_ERR_HANDSHAKE_NO_VERSION:
        case WebSocket::WS_ERR_HANDSHAKE_NO_KEY:
            setDynamicCacheControl(response);
            response.setStatusAndReason(HTTPResponse::HTTP_BAD_REQUEST);
            response.setContentLength(0);
            response.send();
            break;
        }
    } catch (const Poco::Exception& exc) {
        _server.unregisterWebSocket(this);
        _ws = nullptr;
        logWebSocketPocoException("WebSocket Poco Exception:", exc);
    }
}

void PageWebSocketHandler::send(const std::string& message) {
    QMutexLocker locker(&_sendMutex);
    if (!_ws) {
        return;
    }
    try {
        _ws->sendFrame(message.data(), (int)message.size(), WebSocket::FRAME_TEXT);
    } catch (const Poco::Exception& e) {
        logWebSocketPocoException("WebSocket send failed:", e);
    }
}

void PageWebSocketHandler::forceClose() {
    QMutexLocker locker(&_sendMutex);
    if (!_ws) {
        return;
    }
    try {
        _ws->shutdown();
    } catch (...) {
    }
    try {
        _ws->close();
    } catch (...) {
    }
    _ws = nullptr;
}

// ===== StaticRequestHandler =====
void StaticRequestHandler::sendJsonResponse(HTTPServerResponse& response, const std::string& json, HTTPResponse::HTTPStatus status) {
    response.setStatus(status);
    response.setContentType("application/json; charset=utf-8");
    setDynamicCacheControl(response);
    std::ostream& ostr = response.send();
    ostr << json;
}

void StaticRequestHandler::handleApiCommand(HTTPServerRequest& request, HTTPServerResponse& response) {
    // 函数级注释：接收前端发送的 {addr/address, value} JSON，构建 OSCMessage 并交由 ExternalControler 处理
    try {
        std::istream& in = request.stream();
        std::ostringstream body;
        StreamCopier::copyStream(in, body);
        const QByteArray payload = QByteArray::fromStdString(body.str());
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(payload, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            sendJsonResponse(response, "{\"ok\":false,\"error\":\"invalid_json\"}", HTTPResponse::HTTP_BAD_REQUEST);
            return;
        }
        const QJsonObject obj = doc.object();
        const QString addr = obj.value("addr").toString(obj.value("address").toString());
        if (addr.isEmpty()) {
            sendJsonResponse(response, "{\"ok\":false,\"error\":\"missing_addr\"}", HTTPResponse::HTTP_BAD_REQUEST);
            return;
        }
        QVariant value;
        const QJsonValue v = obj.value("value");
        if (v.isBool()) value = v.toBool();
        else if (v.isDouble()) value = v.toDouble();
        else if (v.isString()) value = v.toString();
        else if (v.isNull()) value = QVariant();
        else {
            sendJsonResponse(response, "{\"ok\":false,\"error\":\"unsupported_value_type\"}", HTTPResponse::HTTP_BAD_REQUEST);
            return;
        }

        OSCMessage msg;
        msg.host = "127.0.0.1";
        msg.port = ConfigManager::instance().getExtraControlPort();
        msg.address = addr;
        msg.value = value;
        // auto* controller = ExternalControler::instance();
        qRegisterMetaType<OSCMessage>("OSCMessage");
        QMetaObject::invokeMethod(StatusContainer::instance(),
                                  "parseOSC",
                                  Qt::QueuedConnection,
                                  Q_ARG(OSCMessage, msg));

        sendJsonResponse(response, "{\"ok\":true}");
    } catch (const Poco::Exception& e) {
        sendJsonResponse(response, "{\"ok\":false,\"error\":\"" + e.displayText() + "\"}", HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
    }
}

void StaticRequestHandler::handleApiAuthSetting(HTTPServerRequest& request, HTTPServerResponse& response) {
    // 函数级注释：校验设置页访问密码（POST JSON: {"password":"..."}）
    if (request.getMethod() != "POST") {
        sendJsonResponse(response, "{\"ok\":false,\"error\":\"method_not_allowed\"}", HTTPResponse::HTTP_METHOD_NOT_ALLOWED);
        return;
    }

    try {
        std::istream& in = request.stream();
        std::ostringstream body;
        StreamCopier::copyStream(in, body);
        const QByteArray payload = QByteArray::fromStdString(body.str());

        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(payload, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            sendJsonResponse(response, "{\"ok\":false,\"error\":\"invalid_json\"}", HTTPResponse::HTTP_BAD_REQUEST);
            return;
        }

        const QJsonObject obj = doc.object();
        const QString input = obj.value("password").toString();
        const QString expected = ConfigManager::instance().getWebAccessPassword();

        if (expected.isEmpty() || input == expected) {
            sendJsonResponse(response, "{\"ok\":true}", HTTPResponse::HTTP_OK);
            return;
        }

        // 密码错误用 200 + ok:false：属于业务校验失败，避免浏览器控制台刷 401 红字
        sendJsonResponse(response, "{\"ok\":false,\"error\":\"unauthorized\"}", HTTPResponse::HTTP_OK);
    } catch (const Poco::Exception& e) {
        sendJsonResponse(response, "{\"ok\":false,\"error\":\"" + e.displayText() + "\"}", HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
    }
}

void StaticRequestHandler::handleApiExec(HTTPServerRequest& request, HTTPServerResponse& response, const std::string& cmd) {
    // 函数级注释：执行命令API（示例），回显请求参数，便于前端联调
    URI uri(request.getURI());
    std::string path = uri.getPath();
    // 函数级注释：使用 qDebug 打印请求与返回内容（便于调试）
    {
        const QString json = QString("{\"ok\":true,\"path\":\"%1\",\"query\":\"%2\"}")
            .arg(QString::fromStdString(path),
                 QString::fromStdString(cmd));

    }
    std::ostringstream oss;
    oss << "{\"ok\":true,\"path\":\"" << path << "\",\"query\":\"" << cmd << "\"}";
    sendJsonResponse(response, oss.str());
}

void StaticRequestHandler::handleLayoutSave(HTTPServerRequest& request, HTTPServerResponse& response) {
    // 函数级注释：接收前端提交的布局JSON，保存到服务器内存并同步动作库 used 标记
    try {
        std::istream& in = request.stream();
        std::ostringstream body;
        StreamCopier::copyStream(in, body);
        const QByteArray payload = QByteArray::fromStdString(body.str());
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(payload, &err);
        if (err.error == QJsonParseError::NoError && doc.isObject()) {
            _server.applyLayoutSave(doc.object());
        }
        sendJsonResponse(response, "{\"ok\":true}");
    } catch (const Poco::Exception& e) {
        sendJsonResponse(response, "{\"ok\":false,\"error\":\"" + e.displayText() + "\"}", HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
    }
}

void StaticRequestHandler::handleLayoutLoad(HTTPServerRequest& request, HTTPServerResponse& response) {
    Q_UNUSED(request);
    QJsonObject obj = _server.save();
    if (!obj.isEmpty()) {
        QJsonDocument doc(obj);
        sendJsonResponse(response, doc.toJson(QJsonDocument::Compact).toStdString(), HTTPResponse::HTTP_OK);
        return;
    }
    sendJsonResponse(response, "{\"ok\":true,\"items\":[]}");
}

void StaticRequestHandler::handleActions(HTTPServerRequest& request, HTTPServerResponse& response, const std::string& subPath)
{
    const std::string method = request.getMethod();
    auto& registry = _server.actionRegistry();

    auto readBodyObject = [&request]() -> QJsonObject {
        std::istream& in = request.stream();
        std::ostringstream body;
        StreamCopier::copyStream(in, body);
        const QByteArray payload = QByteArray::fromStdString(body.str());
        QJsonParseError err;
        QJsonDocument doc = QJsonDocument::fromJson(payload, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            return {};
        }
        return doc.object();
    };

    if (subPath == "patch" && method == "POST") {
        try {
            const QJsonObject body = readBodyObject();
            const QString entity = body.value(QStringLiteral("entity")).toString().trimmed();
            if (entity.isEmpty()) {
                sendJsonResponse(response, "{\"ok\":false,\"error\":\"missing_entity\"}", HTTPResponse::HTTP_BAD_REQUEST);
                return;
            }
            const bool ok = _server.patchAction(entity, body);
            QJsonObject json;
            json[QStringLiteral("ok")] = ok;
            if (!ok) {
                json[QStringLiteral("error")] = QStringLiteral("not_found");
                sendJsonResponse(response, QJsonDocument(json).toJson(QJsonDocument::Compact).toStdString(), HTTPResponse::HTTP_NOT_FOUND);
                return;
            }
            json[QStringLiteral("item")] = registry.findByEntity(entity);
            sendJsonResponse(response, QJsonDocument(json).toJson(QJsonDocument::Compact).toStdString());
        } catch (const Poco::Exception& e) {
            sendJsonResponse(response, "{\"ok\":false,\"error\":\"" + e.displayText() + "\"}", HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
        }
        return;
    }

    if (subPath == "remove" && method == "POST") {
        try {
            const QJsonObject body = readBodyObject();
            const QString entity = body.value(QStringLiteral("entity")).toString().trimmed();
            if (entity.isEmpty()) {
                sendJsonResponse(response, "{\"ok\":false,\"error\":\"missing_entity\"}", HTTPResponse::HTTP_BAD_REQUEST);
                return;
            }
            const bool ok = _server.removeAction(entity);
            QJsonObject json;
            json[QStringLiteral("ok")] = ok;
            if (!ok) {
                json[QStringLiteral("error")] = QStringLiteral("not_found");
                sendJsonResponse(response, QJsonDocument(json).toJson(QJsonDocument::Compact).toStdString(), HTTPResponse::HTTP_NOT_FOUND);
                return;
            }
            sendJsonResponse(response, QJsonDocument(json).toJson(QJsonDocument::Compact).toStdString());
        } catch (const Poco::Exception& e) {
            sendJsonResponse(response, "{\"ok\":false,\"error\":\"" + e.displayText() + "\"}", HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
        }
        return;
    }

    if (!subPath.empty()) {
        sendJsonResponse(response, "{\"ok\":false,\"error\":\"not_found\"}", HTTPResponse::HTTP_NOT_FOUND);
        return;
    }

    if (method == "GET") {
        QJsonObject json;
        json[QStringLiteral("ok")] = true;
        json[QStringLiteral("items")] = registry.all();
        QJsonDocument doc(json);
        sendJsonResponse(response, doc.toJson(QJsonDocument::Compact).toStdString());
        return;
    }
    if (method == "POST") {
        try {
            const QJsonObject body = readBodyObject();
            const QString entity = _server.addAction(body);
            if (entity.isEmpty()) {
                sendJsonResponse(response, "{\"ok\":false,\"error\":\"missing_entity\"}", HTTPResponse::HTTP_BAD_REQUEST);
                return;
            }
            QJsonObject json;
            json[QStringLiteral("ok")] = true;
            json[QStringLiteral("entity")] = entity;
            json[QStringLiteral("item")] = registry.findByEntity(entity);
            QJsonDocument out(json);
            sendJsonResponse(response, out.toJson(QJsonDocument::Compact).toStdString());
        } catch (const Poco::Exception& e) {
            sendJsonResponse(response, "{\"ok\":false,\"error\":\"" + e.displayText() + "\"}", HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
        }
        return;
    }
    sendJsonResponse(response, "{\"ok\":false,\"error\":\"method_not_allowed\"}", HTTPResponse::HTTP_METHOD_NOT_ALLOWED);
}

void StaticRequestHandler::handleStaticFile(HTTPServerRequest& request, HTTPServerResponse& response, const std::string& path) {
    if (path.empty() || path == "/") {
        // 优先尝试返回文档根目录下的 index.html
        Poco::Path base(_docRoot);
        base.makeDirectory();
        Poco::Path indexPath(base);
        indexPath.append("index.html");
        Poco::File indexFile(indexPath);
        if (indexFile.exists() && !indexFile.isDirectory()) {
            response.setStatus(HTTPResponse::HTTP_OK);
            response.setContentType("text/html; charset=utf-8");
            response.set("Cache-Control", "no-cache");
            std::ostream& ostr = response.send();
            Poco::FileInputStream fis(indexFile.path());
            Poco::StreamCopier::copyStream(fis, ostr);
            return;
        }

        // 回退：返回内置首页
        const std::string html = builtInIndexHtml();
        response.setStatus(HTTPResponse::HTTP_OK);
        response.setContentType("text/html; charset=utf-8");
        response.set("Cache-Control", "no-cache");
        std::ostream& ostr = response.send();
        ostr << html;
        return;
    }

    // 防御：禁止路径穿越，计算基于文档根的安全路径
    Path base(_docRoot);
    base.makeDirectory();
    // 去掉前导斜杠
    std::string rel = path;
    if (!rel.empty() && (rel[0] == '/' || rel[0] == '\\')) rel.erase(0, 1);
    // 禁止使用 .. 穿越
    if (rel.find("..") != std::string::npos) {
        response.setStatus(HTTPResponse::HTTP_FORBIDDEN);
        setDynamicCacheControl(response);
        std::ostream& ostr = response.send();
        ostr << "403 Forbidden";
        return;
    }
    Path relPath(rel);
    if (relPath.isAbsolute()) {
        response.setStatus(HTTPResponse::HTTP_FORBIDDEN);
        setDynamicCacheControl(response);
        std::ostream& ostr = response.send();
        ostr << "403 Forbidden";
        return;
    }
    Path absPath(base);
    absPath.append(relPath);

    // 确保仍在 docRoot 下（前缀匹配）
    std::string absStr = absPath.toString();
    std::string baseStr = base.toString();
    // 统一大小写以适配 Windows 驱动器大小写差异
    std::transform(absStr.begin(), absStr.end(), absStr.begin(), ::tolower);
    std::transform(baseStr.begin(), baseStr.end(), baseStr.begin(), ::tolower);
    if (absStr.compare(0, baseStr.size(), baseStr) != 0) {
        response.setStatus(HTTPResponse::HTTP_FORBIDDEN);
        setDynamicCacheControl(response);
        std::ostream& ostr = response.send();
        ostr << "403 Forbidden";
        return;
    }

    File file(absPath);
    if (!file.exists() || file.isDirectory()) {
        response.setStatus(HTTPResponse::HTTP_NOT_FOUND);
        setDynamicCacheControl(response);
        std::ostream& ostr = response.send();
        ostr << "404 Not Found";
        return;
    }

    // 读取文件并发送（带缓存/ETag）
    std::string ext = absPath.getExtension();

    const std::string ifNoneMatch = request.get("If-None-Match", "");
    const std::string sizeStr = std::to_string(file.getSize());
    const std::string mtimeStr = std::to_string((long long)file.getLastModified().epochTime());
    const std::string etag = "\"" + sizeStr + "-" + mtimeStr + "\"";

    response.set("ETag", etag);

    const bool isServiceWorker = (rel == "service-worker.js");
    if (isServiceWorker) {
        response.set("Service-Worker-Allowed", "/");
        response.set("Cache-Control", "no-cache");
    } else if (ext == "html" || ext == "htm") {
        response.set("Cache-Control", "no-cache");
    } else {
        response.set("Cache-Control", "public, max-age=31536000, immutable");
    }

    if (!ifNoneMatch.empty() && ifNoneMatch == etag) {
        response.setStatus(HTTPResponse::HTTP_NOT_MODIFIED);
        response.setContentLength(0);
        response.send();
        return;
    }

    response.setStatus(HTTPResponse::HTTP_OK);
    response.setContentType(guessContentType(ext));
    response.setContentLength(file.getSize());

    std::ostream& ostr = response.send();
    FileInputStream fis(file.path());
    StreamCopier::copyStream(fis, ostr);
}

void StaticRequestHandler::handleApiLogs(HTTPServerRequest& request, HTTPServerResponse& response, const std::string& subPath)
{
    URI uri(request.getURI());
    const std::string query = uri.getQuery();

    if (subPath == "files") {
        if (request.getMethod() != "GET") {
            sendJsonResponse(response, "{\"ok\":false,\"error\":\"method_not_allowed\"}", HTTPResponse::HTTP_METHOD_NOT_ALLOWED);
            return;
        }

        QDir logsDir(AppConstants::LOGS_STORAGE_DIR);
        QJsonArray items;
        if (logsDir.exists()) {
            const QStringList names = logsDir.entryList(
                QStringList{QStringLiteral("log*.txt")},
                QDir::Files,
                QDir::Time | QDir::Reversed);

            for (const QString& name : names) {
                const QFileInfo info(logsDir.absoluteFilePath(name));
                if (!info.isFile()) {
                    continue;
                }
                QJsonObject item;
                item[QStringLiteral("name")] = name;
                item[QStringLiteral("size")] = static_cast<double>(info.size());
                item[QStringLiteral("modified")] = info.lastModified().toString(Qt::ISODate);
                items.append(item);
            }
        }

        QJsonObject payload;
        payload[QStringLiteral("ok")] = true;
        payload[QStringLiteral("items")] = items;
        payload[QStringLiteral("count")] = items.size();
        payload[QStringLiteral("dir")] = AppConstants::LOGS_STORAGE_DIR;

        QJsonDocument doc(payload);
        sendJsonResponse(response, doc.toJson(QJsonDocument::Compact).toStdString());
        return;
    }

    if (subPath == "download") {
        if (request.getMethod() != "GET") {
            sendJsonResponse(response, "{\"ok\":false,\"error\":\"method_not_allowed\"}", HTTPResponse::HTTP_METHOD_NOT_ALLOWED);
            return;
        }

        const std::string fileParam = parseQueryParam(query, "file");
        const QString filePath = resolveLogFilePath(QString::fromStdString(fileParam));
        if (filePath.isEmpty()) {
            sendJsonResponse(response, "{\"ok\":false,\"error\":\"file_not_found\"}", HTTPResponse::HTTP_NOT_FOUND);
            return;
        }

        Poco::File file(filePath.toStdString());
        if (!file.exists() || !file.isFile()) {
            sendJsonResponse(response, "{\"ok\":false,\"error\":\"file_not_found\"}", HTTPResponse::HTTP_NOT_FOUND);
            return;
        }

        response.setStatus(HTTPResponse::HTTP_OK);
        response.setContentType("text/plain; charset=utf-8");
        setDynamicCacheControl(response);
        response.setContentLength(file.getSize());
        response.set("Content-Disposition",
                       "attachment; filename=\"" + QFileInfo(filePath).fileName().toStdString() + "\"");

        std::ostream& ostr = response.send();
        Poco::FileInputStream fis(file.path());
        StreamCopier::copyStream(fis, ostr);
        return;
    }

    if (subPath != "tail") {
        sendJsonResponse(response, "{\"ok\":false,\"error\":\"not_found\"}", HTTPResponse::HTTP_NOT_FOUND);
        return;
    }

    if (request.getMethod() != "GET") {
        sendJsonResponse(response, "{\"ok\":false,\"error\":\"method_not_allowed\"}", HTTPResponse::HTTP_METHOD_NOT_ALLOWED);
        return;
    }

    int limit = 200;
    QString levelFilter = QStringLiteral("All");
    qint64 sinceSeq = 0;

    const std::string limitStr = parseQueryParam(query, "limit");
    if (!limitStr.empty()) {
        bool ok = false;
        const int parsed = QString::fromStdString(limitStr).toInt(&ok);
        if (ok) {
            limit = parsed;
        }
    }

    const std::string levelStr = parseQueryParam(query, "level");
    if (!levelStr.empty()) {
        levelFilter = QString::fromStdString(levelStr);
    }

    const std::string sinceStr = parseQueryParam(query, "since");
    if (!sinceStr.empty()) {
        bool ok = false;
        const qint64 parsed = QString::fromStdString(sinceStr).toLongLong(&ok);
        if (ok) {
            sinceSeq = parsed;
        }
    }

    const QJsonArray items = LogRingBuffer::instance().tail(limit, levelFilter, sinceSeq);
    QJsonObject payload;
    payload[QStringLiteral("ok")] = true;
    payload[QStringLiteral("items")] = items;
    payload[QStringLiteral("count")] = items.size();

    QJsonDocument doc(payload);
    sendJsonResponse(response, doc.toJson(QJsonDocument::Compact).toStdString());
}

void StaticRequestHandler::handleApiMedia(HTTPServerRequest& request, HTTPServerResponse& response, const std::string& subPath)
{
    URI uri(request.getURI());
    const std::string query = uri.getQuery();

    if (subPath == "files") {
        if (request.getMethod() != "GET") {
            sendJsonResponse(response, "{\"ok\":false,\"error\":\"method_not_allowed\"}", HTTPResponse::HTTP_METHOD_NOT_ALLOWED);
            return;
        }

        QDir mediaDir(AppConstants::MEDIA_LIBRARY_STORAGE_DIR);
        QJsonArray items;
        if (mediaDir.exists()) {
            const QStringList names = mediaDir.entryList(QDir::Files, QDir::Time | QDir::Reversed);
            for (const QString& name : names) {
                const QFileInfo info(mediaDir.absoluteFilePath(name));
                if (!info.isFile()) {
                    continue;
                }
                QJsonObject item;
                item[QStringLiteral("name")] = name;
                item[QStringLiteral("size")] = static_cast<double>(info.size());
                item[QStringLiteral("modified")] = info.lastModified().toString(Qt::ISODate);
                items.append(item);
            }
        }

        QJsonObject payload;
        payload[QStringLiteral("ok")] = true;
        payload[QStringLiteral("items")] = items;
        payload[QStringLiteral("count")] = items.size();
        payload[QStringLiteral("dir")] = AppConstants::MEDIA_LIBRARY_STORAGE_DIR;

        QJsonDocument doc(payload);
        sendJsonResponse(response, doc.toJson(QJsonDocument::Compact).toStdString());
        return;
    }

    if (subPath == "download") {
        if (request.getMethod() != "GET") {
            sendJsonResponse(response, "{\"ok\":false,\"error\":\"method_not_allowed\"}", HTTPResponse::HTTP_METHOD_NOT_ALLOWED);
            return;
        }

        const std::string fileParam = parseQueryParam(query, "file");
        const QString filePath = resolveMediaFilePath(QString::fromStdString(fileParam));
        if (filePath.isEmpty()) {
            sendJsonResponse(response, "{\"ok\":false,\"error\":\"file_not_found\"}", HTTPResponse::HTTP_NOT_FOUND);
            return;
        }

        Poco::File file(filePath.toStdString());
        if (!file.exists() || !file.isFile()) {
            sendJsonResponse(response, "{\"ok\":false,\"error\":\"file_not_found\"}", HTTPResponse::HTTP_NOT_FOUND);
            return;
        }

        Poco::Path p(filePath.toStdString());
        response.setStatus(HTTPResponse::HTTP_OK);
        response.setContentType(guessContentType(p.getExtension()));
        setDynamicCacheControl(response);
        response.setContentLength(file.getSize());
        response.set("Content-Disposition",
                       "attachment; filename=\"" + p.getFileName() + "\"");

        std::ostream& ostr = response.send();
        Poco::FileInputStream fis(file.path());
        StreamCopier::copyStream(fis, ostr);
        return;
    }

    sendJsonResponse(response, "{\"ok\":false,\"error\":\"not_found\"}", HTTPResponse::HTTP_NOT_FOUND);
}

void StaticRequestHandler::handleRequest(HTTPServerRequest& request,
                                         HTTPServerResponse& response) {
    try {
        URI uri(request.getURI());
        std::string path = uri.getPath();

        // 路由分发
        if (path == "/api/auth/setting") {
            handleApiAuthSetting(request, response);
        } else if (path == "/api/command") {
            handleApiCommand(request, response);
        } else if (path.rfind("/api/exec", 0) == 0) {
            handleApiExec(request, response, uri.getQuery());
        } else if (path == "/api/layout/save") {
            handleLayoutSave(request, response);
        } else if (path == "/api/layout/load") {
            handleLayoutLoad(request, response);
        } else if (path == "/api/actions") {
            handleActions(request, response, "");
        } else if (path.rfind("/api/actions/", 0) == 0) {
            const std::string subPath = path.substr(std::string("/api/actions/").size());
            handleActions(request, response, subPath);
        } else if (path == "/api/upload/media") {
            handleUploadMedia(request, response);
        } else if (path == "/api/upload/flow") {
            handleUploadFlow(request, response);
        } else if (path == "/api/download/current_flow") {
            handleDownloadCurrentFlow(request, response);
        } else if (path == "/api/info/current_flow") {
            handleGetCurrentFlowInfo(request, response);
        } else if (path == "/api/info/app") {
            handleGetAppInfo(request, response);
        } else if (path.rfind("/api/logs/", 0) == 0) {
            const std::string subPath = path.substr(std::string("/api/logs/").size());
            handleApiLogs(request, response, subPath);
        } else if (path.rfind("/api/media/", 0) == 0) {
            const std::string subPath = path.substr(std::string("/api/media/").size());
            handleApiMedia(request, response, subPath);
        } else {
            handleStaticFile(request, response, path);
        }
    } catch (const Poco::Exception& e) {
        response.setStatus(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
        setDynamicCacheControl(response);
        std::ostream& ostr = response.send();
        ostr << "500 Internal Server Error: " << e.displayText();
    } catch (const std::exception& e) {
        response.setStatus(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
        setDynamicCacheControl(response);
        std::ostream& ostr = response.send();
        ostr << "500 Internal Server Error: " << e.what();
    }
}

// 函数级注释：解析查询参数中指定键的值（简单实现，不处理重复键）
std::string StaticRequestHandler::parseQueryParam(const std::string& query, const std::string& key) {
    if (query.empty() || key.empty()) return "";
    std::string k = key + "=";
    auto pos = query.find(k);
    if (pos == std::string::npos) return "";
    pos += k.size();
    auto end = query.find('&', pos);
    std::string val = end == std::string::npos ? query.substr(pos) : query.substr(pos, end - pos);
    // URL 解码（简化，仅处理 %20 和 + 为空格）
    std::string out;
    out.reserve(val.size());
    for (size_t i = 0; i < val.size(); ++i) {
        if (val[i] == '+') out.push_back(' ');
        else if (val[i] == '%' && i + 2 < val.size()) {
            std::string hex = val.substr(i + 1, 2);
            char c = (char)strtol(hex.c_str(), nullptr, 16);
            out.push_back(c);
            i += 2;
        } else {
            out.push_back(val[i]);
        }
    }
    return out;
}

// 函数级注释：对文件名进行安全过滤，移除路径分隔与非法字符
std::string StaticRequestHandler::sanitizeFilename(const std::string& name) {
    std::string n;
    n.reserve(name.size());
    for (char ch : name) {
        if (ch == '/' || ch == '\\') continue;
        if (ch == ':' || ch == '*' || ch == '?' || ch == '"' || ch == '<' || ch == '>' || ch == '|') continue;
        if ((unsigned char)ch < 32) continue;
        n.push_back(ch);
    }
    // 防止 .. 穿越
    if (n == "..") n = "";
    return n;
}

// 函数级注释：处理媒体文件上传（以二进制流写入到文档根 uploads/media）
void StaticRequestHandler::handleUploadMedia(HTTPServerRequest& request, HTTPServerResponse& response) {
    try {
        if (request.getMethod() != "POST") {
            sendJsonResponse(response, "{\"ok\":false,\"error\":\"method_not_allowed\"}", HTTPResponse::HTTP_METHOD_NOT_ALLOWED);
            return;
        }
        URI uri(request.getURI());
        const std::string rawName = parseQueryParam(uri.getQuery(), "filename");
        const std::string safeName = sanitizeFilename(rawName);
        if (safeName.empty()) {
            sendJsonResponse(response, "{\"ok\":false,\"error\":\"missing_or_invalid_filename\"}", HTTPResponse::HTTP_BAD_REQUEST);
            return;
        }
        // 目标目录：使用应用常量 MEDIA_LIBRARY_STORAGE_DIR
        const QString mediaDir = AppConstants::MEDIA_LIBRARY_STORAGE_DIR;
        QDir().mkpath(mediaDir);
        const QString qFilePath = QDir(mediaDir).filePath(QString::fromStdString(safeName));
        const std::string absPath = qFilePath.toStdString();
        // 写入文件：必须 out|trunc|binary，否则同名较短文件覆盖后会残留旧尾部数据
        std::istream& in = request.stream();
        Poco::FileOutputStream fos(absPath, std::ios::out | std::ios::trunc | std::ios::binary);
        Poco::StreamCopier::copyStream(in, fos);
        fos.close();
        // 返回
        std::ostringstream oss;
        oss << "{\"ok\":true,\"path\":\"" << absPath << "\"}";
        sendJsonResponse(response, oss.str());
    } catch (const Poco::Exception& e) {
        sendJsonResponse(response, "{\"ok\":false,\"error\":\"" + e.displayText() + "\"}", HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
    } catch (const std::exception& e) {
        sendJsonResponse(response, std::string("{\"ok\":false,\"error\":\"") + e.what() + "\"}", HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
    }
}

// 函数级注释：处理.flow项目文件上传（限制扩展名为.flow）
void StaticRequestHandler::handleUploadFlow(HTTPServerRequest& request, HTTPServerResponse& response) {
    try {
        if (request.getMethod() != "POST") {
            sendJsonResponse(response, "{\"ok\":false,\"error\":\"method_not_allowed\"}", HTTPResponse::HTTP_METHOD_NOT_ALLOWED);
            return;
        }
        URI uri(request.getURI());
        const std::string rawName = parseQueryParam(uri.getQuery(), "filename");
        const std::string safeName = sanitizeFilename(rawName);
        if (safeName.empty()) {
            sendJsonResponse(response, "{\"ok\":false,\"error\":\"missing_or_invalid_filename\"}", HTTPResponse::HTTP_BAD_REQUEST);
            return;
        }
        Poco::Path tmp(safeName);
        if (Poco::icompare(tmp.getExtension(), "flow") != 0) {
            sendJsonResponse(response, "{\"ok\":false,\"error\":\"invalid_extension\"}", HTTPResponse::HTTP_BAD_REQUEST);
            return;
        }
        // 目标目录：使用应用常量 MEDIA_LIBRARY_FLOW_DIR（取消 docRoot/uploads/projects 双份保存）
        const QString mediaDir = AppConstants::MEDIA_LIBRARY_FLOW_DIR;
        QDir().mkpath(mediaDir);
        const QString qFilePath = QDir(mediaDir).filePath(QString::fromStdString(safeName));
        const std::string absPath = qFilePath.toStdString();
        // 写入文件：必须 out|trunc|binary，否则同名较短文件覆盖后会残留旧尾部数据，
        // 解析时表现为 "garbage at the end of the document"
        std::istream& in = request.stream();
        Poco::FileOutputStream fos(absPath, std::ios::out | std::ios::trunc | std::ios::binary);
        Poco::StreamCopier::copyStream(in, fos);
        fos.close();

        // 通知主窗口加载文件
        _server.notifyFlowFileUploaded(qFilePath);
        // 返回
        std::ostringstream oss;
        oss << "{\"ok\":true,\"path\":\"" << absPath << "\"}";
        sendJsonResponse(response, oss.str());
    } catch (const Poco::Exception& e) {
        sendJsonResponse(response, "{\"ok\":false,\"error\":\"" + e.displayText() + "\"}", HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
    } catch (const std::exception& e) {
        sendJsonResponse(response, std::string("{\"ok\":false,\"error\":\"") + e.what() + "\"}", HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
    }
}

// 函数级注释：下载当前Flow文件（取最近文件列表首项）
void StaticRequestHandler::handleDownloadCurrentFlow(HTTPServerRequest& request, HTTPServerResponse& response) {
    if (request.getMethod() != "GET") {
        sendJsonResponse(response, "{\"ok\":false,\"error\":\"method_not_allowed\"}", HTTPResponse::HTTP_METHOD_NOT_ALLOWED);
        return;
    }

    QString recentFile = ConfigManager::instance().getCurrentFlowPath();
    if (recentFile.isEmpty()) {
        sendJsonResponse(response, "{\"ok\":false,\"error\":\"no_file_running\"}", HTTPResponse::HTTP_NOT_FOUND);
        return;
    }

 
    Poco::File file(recentFile.toStdString());
    if (!file.exists() || !file.isFile()) {
        sendJsonResponse(response, "{\"ok\":false,\"error\":\"file_not_found\"}", HTTPResponse::HTTP_NOT_FOUND);
        return;
    }

    response.setStatus(HTTPResponse::HTTP_OK);
    response.setContentType("application/octet-stream");
    setDynamicCacheControl(response);

    // Set filename in Content-Disposition
    Poco::Path p(recentFile.toStdString());
    std::string filename = p.getFileName();
    response.set("Content-Disposition", "attachment; filename=\"" + filename + "\"");

    std::ostream& ostr = response.send();
    Poco::FileInputStream fis(file.path());
    Poco::StreamCopier::copyStream(fis, ostr);
}

// 函数级注释：获取当前Flow文件信息（返回JSON）
void StaticRequestHandler::handleGetCurrentFlowInfo(HTTPServerRequest& request, HTTPServerResponse& response) {
    if (request.getMethod() != "GET") {
        sendJsonResponse(response, "{\"ok\":false,\"error\":\"no_file_running\"}", HTTPResponse::HTTP_METHOD_NOT_ALLOWED);
        return;
    }

    QString recentFile = ConfigManager::instance().getCurrentFlowPath();
    QJsonObject json;
    
    if (recentFile.isEmpty()) {
        json["ok"] = false;
        json["error"] = "no_recent_file";
    } else {
        QString filePath = recentFile;
        Poco::File file(filePath.toStdString());
        if (!file.exists() || !file.isFile()) {
            json["ok"] = false;
            json["error"] = "file_not_found";
        } else {
            json["ok"] = true;
            json["path"] = filePath;
            json["filename"] = QFileInfo(filePath).fileName();
        }
    }
    
    QJsonDocument doc(json);
    sendJsonResponse(response, doc.toJson(QJsonDocument::Compact).toStdString());
}

// 函数级注释：返回软件名称与版本（与 ConstantDefines.h / 关于窗口同源）
void StaticRequestHandler::handleGetAppInfo(HTTPServerRequest& request, HTTPServerResponse& response) {
    if (request.getMethod() != "GET") {
        sendJsonResponse(response, "{\"ok\":false,\"error\":\"method_not_allowed\"}", HTTPResponse::HTTP_METHOD_NOT_ALLOWED);
        return;
    }

    QJsonObject json;
    json["ok"] = true;
    json["name"] = AppConstants::PRODUCT_NAME;
    json["version"] = QStringLiteral(PRODUCT_VERSION);
    QJsonDocument doc(json);
    sendJsonResponse(response, doc.toJson(QJsonDocument::Compact).toStdString());
}

std::string StaticRequestHandler::guessContentType(const std::string& ext) {
    if (ext == "html" || ext == "htm") return "text/html; charset=utf-8";
    if (ext == "css") return "text/css";
    if (ext == "js") return "application/javascript";
    if (ext == "json") return "application/json";
    if (ext == "png") return "image/png";
    if (ext == "jpg" || ext == "jpeg") return "image/jpeg";
    if (ext == "gif") return "image/gif";
    if (ext == "svg") return "image/svg+xml";
    return "application/octet-stream";
}

std::string StaticRequestHandler::builtInIndexHtml() {
    std::ostringstream ss;
    ss <<
        "<!doctype html><html><head><meta charset='utf-8'/>"
        "<title>FLOW HTTP Server</title>"
        "<style>body{font-family:Segoe UI,Arial; margin:40px;}h1{color:#2b6cb0;}code{background:#f6f8fa;padding:2px 4px;border-radius:4px;}</style>"
        "</head><body>"
        "<h1>FLOW HTTP Server</h1>"
        "<p>服务器正在运行。你可以将静态文件放在文档根目录并通过浏览器访问。</p>"
        "<p>默认首页由内置内容提供。如需自定义，请在文档根目录创建 <code>index.html</code>。</p>"
        "</body></html>";
    return ss.str();
}

// ===== StaticRequestHandlerFactory =====
HTTPRequestHandler* StaticRequestHandlerFactory::createRequestHandler(
    const HTTPServerRequest& request) {
    
    // 检查 WebSocket 升级请求
    if (request.has("Upgrade") && 
        Poco::icompare(request.get("Upgrade"), "websocket") == 0) {
        return new PageWebSocketHandler(_server);
    }
    
    return new StaticRequestHandler(_docRoot, _server);
}

// ===== NodeHttpServer =====
// 函数级注释：构造函数，初始化QObject基类与内部状态
NodeHttpServer::NodeHttpServer(QObject* parent) : QObject(parent) {}

void NodeHttpServer::setDocRoot(const std::string& docRoot) {
    _docRoot = docRoot;
}

bool NodeHttpServer::start(int port) {
    if (_running) return true;
    // 函数级注释：启动HTTP服务器
    try {
        _port = port;
        // 若未设置文档根，默认使用当前工作目录下的 www 目录
        if (_docRoot.empty()) {
            Poco::Path cwd = Poco::Path::current();
            cwd.makeDirectory();
            cwd.append("www");
            _docRoot = cwd.toString();
        }
        ServerSocket svs(static_cast<Poco::UInt16>(_port));
        auto params = new HTTPServerParams();
        params->setMaxQueued(64);
        params->setMaxThreads(16);
        // 勿设为 Timespan(0,0)：会导致 poll 立即超时，WebSocket 握手失败
        // WebSocket 读写超时见 configureWebSocketTimeouts()
        params->setTimeout(Poco::Timespan(3600, 0));
        params->setKeepAliveTimeout(Poco::Timespan(3600, 0));
        
        _server = std::make_unique<HTTPServer>(new StaticRequestHandlerFactory(_docRoot, *this), svs, params);
        _server->start();
        _running = true;
        emit serverStarted(_port);
        
        // 连接 OSCSender 信号
        connect(StatusContainer::instance(), &StatusContainer::statusUpdated, this, &NodeHttpServer::onOscMessageSent, Qt::UniqueConnection);

        LogRingBuffer::instance().setBroadcastCallback([this](const QJsonObject& payload) {
            QMetaObject::invokeMethod(
                this,
                [this, payload]() { broadcastJson(payload); },
                Qt::QueuedConnection);
        });
        
        return true;
    } catch (const Poco::Exception& e) {
        _server.reset();
        _running = false;
        qWarning() << "Failed to start HTTP server:" << e.displayText().c_str();
        return false;
    }
}

void NodeHttpServer::stop() {
    if (!_running) return;
    _running = false;

    // 先切断日志广播，避免退出期 qDebug 再排队同步 sendFrame 卡住主线程
    LogRingBuffer::instance().setBroadcastCallback({});
    disconnect(StatusContainer::instance(), &StatusContainer::statusUpdated, this, &NodeHttpServer::onOscMessageSent);

    // 先拷贝再关连接：forceClose 可能唤醒 worker，worker 会抢 _wsMutex 做 unregister
    std::vector<PageWebSocketHandler*> handlers;
    {
        QMutexLocker locker(&_wsMutex);
        handlers.assign(_wsHandlers.begin(), _wsHandlers.end());
    }
    for (auto* handler : handlers) {
        if (handler) {
            handler->forceClose();
        }
    }
    {
        QMutexLocker locker(&_wsMutex);
        _wsHandlers.clear();
    }

    if (_server) {
        // abortCurrent=true：关闭活跃连接底层套接字，避免 stop 等待长连接收尾
        try {
            _server->stopAll(true);
        } catch (...) {
        }
        try {
            _server->stop();
        } catch (...) {
        }
        _server.reset();
    }
    emit serverStopped();

    // 清空内存中的布局，避免新项目继承旧布局
    _layout = QJsonObject();
    _actionRegistry.clear();
}

void NodeHttpServer::registerWebSocket(PageWebSocketHandler* handler) {
    QMutexLocker locker(&_wsMutex);
    _wsHandlers.insert(handler);
}

void NodeHttpServer::unregisterWebSocket(PageWebSocketHandler* handler) {
    QMutexLocker locker(&_wsMutex);
    _wsHandlers.erase(handler);
}

void NodeHttpServer::onOscMessageSent(const StatusItem& message) {
    if (!_running) {
        return;
    }
    // 将 OSC 消息转为 JSON
    QJsonObject json;
    json= message.toJsonObject();
    QJsonDocument doc(json);
    std::string jsonStr = doc.toJson(QJsonDocument::Compact).toStdString();
    // 广播给所有 WebSocket 连接
    QMutexLocker locker(&_wsMutex);
    for (auto* handler : _wsHandlers) {
        handler->send(jsonStr);
    }
}

QJsonObject NodeHttpServer::save() const
{
    QJsonObject obj = _layout;
    obj[QStringLiteral("actions")] = _actionRegistry.save();
    return obj;
}

void NodeHttpServer::load(const QJsonObject& layout)
{
    if (layout.contains(QStringLiteral("actions")) && layout.value(QStringLiteral("actions")).isArray()) {
        _actionRegistry.load(layout.value(QStringLiteral("actions")).toArray());
    }
    _layout = layout;
    _actionRegistry.syncUsedFromLayout(layout);
}

void NodeHttpServer::applyLayoutSave(const QJsonObject& layout)
{
    load(layout);
    notifyActionsChanged(QStringLiteral("sync"), QJsonObject());
}

QString NodeHttpServer::addAction(const QJsonObject& binding)
{
    const QString entity = _actionRegistry.addOrUpdate(binding);
    if (entity.isEmpty()) {
        return {};
    }
    const QJsonObject item = _actionRegistry.findByEntity(entity);
    notifyActionsChanged(QStringLiteral("add"), item);
    return entity;
}

bool NodeHttpServer::removeAction(const QString& entity)
{
    const QString key = _actionRegistry.resolveEntityKey(entity);
    const QJsonObject item = _actionRegistry.findByEntity(key);
    if (item.isEmpty()) {
        return false;
    }
    if (!_actionRegistry.remove(key)) {
        return false;
    }
    notifyActionsChanged(QStringLiteral("remove"), item);
    return true;
}

bool NodeHttpServer::patchAction(const QString& entity, const QJsonObject& patch)
{
    const QString key = _actionRegistry.resolveEntityKey(entity);
    if (!_actionRegistry.patch(key, patch)) {
        return false;
    }
    const QJsonObject item = _actionRegistry.findByEntity(key);
    notifyActionsChanged(QStringLiteral("patch"), item);
    return true;
}

void NodeHttpServer::broadcastJson(const QJsonObject& payload)
{
    if (!_running) {
        return;
    }
    QJsonDocument doc(payload);
    const std::string jsonStr = doc.toJson(QJsonDocument::Compact).toStdString();
    QMutexLocker locker(&_wsMutex);
    for (auto* handler : _wsHandlers) {
        handler->send(jsonStr);
    }
}

void NodeHttpServer::notifyActionsChanged(const QString& action, const QJsonObject& item)
{
    QJsonObject evt;
    evt[QStringLiteral("event")] = QStringLiteral("actions_changed");
    evt[QStringLiteral("action")] = action;
    if (!item.isEmpty()) {
        evt[QStringLiteral("item")] = item;
    }
    evt[QStringLiteral("count")] = _actionRegistry.all().size();
    broadcastJson(evt);
}
