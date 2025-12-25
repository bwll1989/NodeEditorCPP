#include "HttpServer.hpp"
#include <QDebug>
#include <QJsonObject>
#include <QJsonDocument>
#include <QMetaObject>
#include <QCoreApplication>
#include "../ExternalControl/ExternalControler.hpp"
#include "../../Common/Devices/OSCSender/OSCSender.h"
#include "ConstantDefines.h"
#include "OSCMessage.h"
#include <Poco/Net/HTTPServerResponse.h>
#include <Poco/Net/HTTPServerRequest.h>
#include <Poco/Net/NetException.h>
#include <Poco/URI.h>
#include <Poco/FileStream.h>
#include <Poco/Path.h>
#include <Poco/StreamCopier.h>
#include <sstream>
#include <QMetaType>
Q_DECLARE_METATYPE(OSCMessage)
using namespace Poco::Net;
using namespace Poco;
using namespace NodeStudio;

// ===== PageWebSocketHandler =====
PageWebSocketHandler::PageWebSocketHandler(NodeHttpServer& server)
    : _server(server) {}

void PageWebSocketHandler::handleRequest(HTTPServerRequest& request,
                                         HTTPServerResponse& response) {
    try {
        Poco::Net::WebSocket ws(request, response);
        _ws = &ws;
        _server.registerWebSocket(this);
        
        char buffer[1024];
        int flags;
        int n;
        
        // 循环读取数据，保持连接活跃，直到客户端关闭或发生错误
        do {
            n = ws.receiveFrame(buffer, sizeof(buffer), flags);
            // 这里可以处理客户端发送的心跳或指令，目前仅作保持连接用
        } while (n > 0 && (flags & WebSocket::FRAME_OP_BITMASK) != WebSocket::FRAME_OP_CLOSE);
        
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
            response.setStatusAndReason(HTTPResponse::HTTP_BAD_REQUEST);
            response.setContentLength(0);
            response.send();
            break;
        }
    } catch (const Poco::Exception& exc) {
        _server.unregisterWebSocket(this);
        _ws = nullptr;
        // qWarning() << "WebSocket Poco Exception: " << exc.displayText().c_str();
    }
}

void PageWebSocketHandler::send(const std::string& message) {
    if (_ws) {
        try {
            _ws->sendFrame(message.data(), (int)message.size(), WebSocket::FRAME_TEXT);
        } catch (const Poco::Exception& e) {
            qWarning() << "WebSocket send failed:" << e.displayText().c_str();
        }
    }
}

// ===== StaticRequestHandler =====
void StaticRequestHandler::sendJsonResponse(HTTPServerResponse& response, const std::string& json, HTTPResponse::HTTPStatus status) {
    response.setStatus(status);
    response.setContentType("application/json; charset=utf-8");
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
        msg.port = AppConstants::EXTRA_CONTROL_PORT;
        msg.address = addr;
        msg.value = value;
        auto* controller = ExternalControler::instance();
        qRegisterMetaType<OSCMessage>("OSCMessage");
        QMetaObject::invokeMethod(controller,
                                  "parseOSC",
                                  Qt::QueuedConnection,
                                  Q_ARG(OSCMessage, msg));

        sendJsonResponse(response, "{\"ok\":true}");
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
        qDebug() << json;
    }
    std::ostringstream oss;
    oss << "{\"ok\":true,\"path\":\"" << path << "\",\"query\":\"" << cmd << "\"}";
    sendJsonResponse(response, oss.str());
}

void StaticRequestHandler::handleLayoutSave(HTTPServerRequest& request, HTTPServerResponse& response) {
    // 函数级注释：接收前端提交的布局JSON，仅保存到服务器内存，避免写入磁盘
    try {
        std::istream& in = request.stream();
        std::ostringstream body;
        StreamCopier::copyStream(in, body);
        // 同步到服务器内存布局
        {
            const QByteArray payload = QByteArray::fromStdString(body.str());
            QJsonParseError err;
            QJsonDocument doc = QJsonDocument::fromJson(payload, &err);
            if (err.error == QJsonParseError::NoError && doc.isObject()) {
                _server.load(doc.object());
            }
        }
        sendJsonResponse(response, "{\"ok\":true}");
    } catch (const Poco::Exception& e) {
        sendJsonResponse(response, "{\"ok\":false,\"error\":\"" + e.displayText() + "\"}", HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
    }
}

void StaticRequestHandler::handleLayoutLoad(HTTPServerRequest& request, HTTPServerResponse& response) {
    // 函数级注释：仅从服务器内存返回当前布局；不读取磁盘文件
    // 优先使用服务器内存布局
    {
        QJsonObject obj = _server.save();
        if (!obj.isEmpty()) {
            QJsonDocument doc(obj);
            sendJsonResponse(response, doc.toJson(QJsonDocument::Compact).toStdString(), HTTPResponse::HTTP_OK);
            return;
        }
    }
    // 返回空布局（兼容旧格式 items）
    sendJsonResponse(response, "{\"ok\":true,\"items\":[]}");
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
            std::ostream& ostr = response.send();
            Poco::FileInputStream fis(indexFile.path());
            Poco::StreamCopier::copyStream(fis, ostr);
            return;
        }

        // 回退：返回内置首页
        const std::string html = builtInIndexHtml();
        response.setStatus(HTTPResponse::HTTP_OK);
        response.setContentType("text/html; charset=utf-8");
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
        std::ostream& ostr = response.send();
        ostr << "403 Forbidden";
        return;
    }
    Path relPath(rel);
    if (relPath.isAbsolute()) {
        response.setStatus(HTTPResponse::HTTP_FORBIDDEN);
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
        std::ostream& ostr = response.send();
        ostr << "403 Forbidden";
        return;
    }

    File file(absPath);
    if (!file.exists() || file.isDirectory()) {
        response.setStatus(HTTPResponse::HTTP_NOT_FOUND);
        std::ostream& ostr = response.send();
        ostr << "404 Not Found";
        return;
    }

    // 读取文件并发送
    std::string ext = absPath.getExtension();
    response.setStatus(HTTPResponse::HTTP_OK);
    response.setContentType(guessContentType(ext));
    std::ostream& ostr = response.send();
    FileInputStream fis(file.path());
    StreamCopier::copyStream(fis, ostr);
}

void StaticRequestHandler::handleRequest(HTTPServerRequest& request,
                                         HTTPServerResponse& response) {
    try {
        URI uri(request.getURI());
        std::string path = uri.getPath();

        // 路由分发
        if (path == "/api/command") {
            handleApiCommand(request, response);
        } else if (path.rfind("/api/exec", 0) == 0) {
            handleApiExec(request, response, uri.getQuery());
        } else if (path == "/api/layout/save") {
            handleLayoutSave(request, response);
        } else if (path == "/api/layout/load") {
            handleLayoutLoad(request, response);
        } else {
            handleStaticFile(request, response, path);
        }
    } catch (const Poco::Exception& e) {
        response.setStatus(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
        std::ostream& ostr = response.send();
        ostr << "500 Internal Server Error: " << e.displayText();
    } catch (const std::exception& e) {
        response.setStatus(HTTPResponse::HTTP_INTERNAL_SERVER_ERROR);
        std::ostream& ostr = response.send();
        ostr << "500 Internal Server Error: " << e.what();
    }
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
        "<title>NodeStudio HTTP Server</title>"
        "<style>body{font-family:Segoe UI,Arial; margin:40px;}h1{color:#2b6cb0;}code{background:#f6f8fa;padding:2px 4px;border-radius:4px;}</style>"
        "</head><body>"
        "<h1>NodeStudio HTTP Server</h1>"
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
        params->setMaxThreads(4);
        
        _server = std::make_unique<HTTPServer>(new StaticRequestHandlerFactory(_docRoot, *this), svs, params);
        _server->start();
        _running = true;
        emit serverStarted(_port);
        
        // 连接 OSCSender 信号
        connect(OSCSender::instance(), &OSCSender::messageSent, this, &NodeHttpServer::onOscMessageSent, Qt::UniqueConnection);
        
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
    _server->stop();
    _server.reset();
    _running = false;
    emit serverStopped();
    
    // 断开 OSCSender 信号
    disconnect(OSCSender::instance(), &OSCSender::messageSent, this, &NodeHttpServer::onOscMessageSent);
    
    // 清理 WebSocket 处理器
    QMutexLocker locker(&_wsMutex);
    _wsHandlers.clear();
    
    // 清空内存中的布局，避免新项目继承旧布局
    _layout = QJsonObject();
}

void NodeHttpServer::registerWebSocket(PageWebSocketHandler* handler) {
    QMutexLocker locker(&_wsMutex);
    _wsHandlers.insert(handler);
}

void NodeHttpServer::unregisterWebSocket(PageWebSocketHandler* handler) {
    QMutexLocker locker(&_wsMutex);
    _wsHandlers.erase(handler);
}

void NodeHttpServer::onOscMessageSent(const OSCMessage &message) {
    // 将 OSC 消息转为 JSON
    QJsonObject json;
    json["addr"] = message.address;
    
    // 简化值处理
    QVariant val = message.value;
    if (val.typeId() == QMetaType::Double || val.typeId() == QMetaType::Float) {
        json["value"] = val.toDouble();
    } else if (val.typeId() == QMetaType::Int || val.typeId() == QMetaType::UInt || val.typeId() == QMetaType::LongLong) {
        json["value"] = val.toInt();
    } else if (val.typeId() == QMetaType::Bool) {
        json["value"] = val.toBool();
    } else {
        json["value"] = val.toString();
    }
    
    QJsonDocument doc(json);
    std::string jsonStr = doc.toJson(QJsonDocument::Compact).toStdString();

    // 广播给所有 WebSocket 连接
    QMutexLocker locker(&_wsMutex);
    for (auto* handler : _wsHandlers) {
        handler->send(jsonStr);
    }
}

// 函数级注释：加载并设置当前布局；仅保存于内存，不写入磁盘
void NodeHttpServer::load(const QJsonObject& layout) {
    _layout = layout;
}
