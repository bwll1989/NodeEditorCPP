// ============================================================================
//  http_server —— HTTP 服务器骨架：请求分发 + 静态文件 + Poco 生命周期。
//
//  （收编自 MMC，改造点：AppContext → StandaloneApi 单入口；原 CORS/静态
//  托管/Poco 起停语义原样保留——见文件头。）
// ============================================================================

#include "http_server.hpp"

#include "standalone_api.hpp"
#include "http_util.hpp"
#include "json_util.hpp"
#include "path_utf8.hpp"

#include "Poco/Net/HTTPRequestHandler.h"
#include "Poco/Net/HTTPRequestHandlerFactory.h"
#include "Poco/Net/HTTPServer.h"
#include "Poco/Net/HTTPServerParams.h"
#include "Poco/Net/HTTPServerRequest.h"
#include "Poco/Net/HTTPServerResponse.h"
#include "Poco/Net/ServerSocket.h"
#include "Poco/Path.h"
#include "Poco/URI.h"

#include <memory>

namespace media_api {
namespace {

class ApiHandler : public Poco::Net::HTTPRequestHandler {
public:
    explicit ApiHandler(StandaloneApi& api) : api_(api) {}

    void handleRequest(Poco::Net::HTTPServerRequest& req, Poco::Net::HTTPServerResponse& resp) override {
        try {
            if (req.getMethod() == Poco::Net::HTTPRequest::HTTP_OPTIONS) {
                resp.setStatus(Poco::Net::HTTPResponse::HTTP_OK);
                resp.set("Access-Control-Allow-Origin", "*");
                resp.set("Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
                resp.set("Access-Control-Allow-Headers", "Content-Type, X-Filename");
                resp.send();
                return;
            }

            Poco::URI uri(req.getURI());
            const std::string path = uri.getPath();
            const std::string method = req.getMethod();

            if (path.rfind("/api/v1/", 0) == 0) {
                RouteCtx ctx{api_, method, path, uri, req, resp};
                if (api_.handle(ctx)) return;
                send_json(resp, 404, err_json("NOT_FOUND", "unknown api"));
                return;
            }
            handle_static(path, resp);
        } catch (const std::exception& ex) {
            // [S1] std::filesystem 等标准库异常消息是本地化 ACP(GBK) 字节，
            // 直出 JSON 会乱码——先转 UTF-8 再下发
            send_json(resp, 500, err_json("INTERNAL", media_api::acp_to_utf8(ex.what())));
        }
    }

private:
    void handle_static(const std::string& path, Poco::Net::HTTPServerResponse& resp) {
        std::string rel = path;
        if (rel.empty() || rel == "/") rel = "/index.html";
        if (rel.find("..") != std::string::npos) {
            send_json(resp, 400, err_json("PATH_DENIED", "invalid path"));
            return;
        }
        Poco::Path p(api_.webDir());
        p.append(rel.substr(1));
        std::string body;
        if (!read_file(p.toString(), body)) {
            send_json(resp, 404, err_json("NOT_FOUND", "file not found"));
            return;
        }
        send_text(resp, 200, mime_for(rel), body);
    }

    StandaloneApi& api_;
};

class HandlerFactory : public Poco::Net::HTTPRequestHandlerFactory {
public:
    explicit HandlerFactory(StandaloneApi& api) : api_(api) {}
    Poco::Net::HTTPRequestHandler* createRequestHandler(const Poco::Net::HTTPServerRequest&) override {
        return new ApiHandler(api_);
    }

private:
    StandaloneApi& api_;
};

}  // namespace

class HttpServer::Impl {
public:
    std::unique_ptr<Poco::Net::HTTPServer> server;
    std::unique_ptr<Poco::Net::ServerSocket> socket;
};

HttpServer::HttpServer(StandaloneApi& api) : api_(api), impl_(std::make_unique<Impl>()) {}

HttpServer::~HttpServer() { stop(); }

int HttpServer::boundPort() const {
    if (impl_ && impl_->socket) return static_cast<int>(impl_->socket->address().port());
    return 0;
}

bool HttpServer::start(const std::string& bind, int port, std::string& err) {
    if (running_.exchange(true)) return true;  // 已在运行：幂等成功
    try {
        Poco::Net::SocketAddress addr(bind, static_cast<Poco::UInt16>(port));
        // S2-2 端口独占（2026-11-11 测试清单）：禁止 ServerSocket(addr)
        // 构造直绑——Poco 该构造内部 bind(address, reuse=true)（SO_REUSEADDR
        // 置位），Windows 下第二实例可静默双绑同一端口（请求随机分流，
        // netstat 实测两行 LISTENING）。改显式 bind(reuse=false) 不落
        // REUSEADDR 语义，再以"再绑必须失败"自检独占性成立。
        auto socket = std::make_unique<Poco::Net::ServerSocket>();
        socket->bind(addr, false);
        socket->listen();
        // 独占性自检（实测防线）：对**实际绑定的地址**（port=0 时由系统
        // 分配，须读回 socket->address()）再绑一次必须失败；若成功说明
        // 当前平台/选项组合下端口未独占（含外来程序以 SO_REUSEADDR 劫持
        // 的场景），主动放弃本次启动——双绑分流比启动失败难排查得多。
        const Poco::Net::SocketAddress bound = socket->address();
        bool double_bindable = false;
        try {
            Poco::Net::ServerSocket probe;
            probe.bind(bound, false);
            probe.close();
            double_bindable = true;
        } catch (const Poco::Exception&) {
            // 再绑失败 = 独占成立（预期路径，吞掉绑定异常）
        }
        if (double_bindable) {
            err = "端口未被独占（同端口再次 bind 成功——存在双绑分流风险）: " +
                  bind + ":" + std::to_string(bound.port());
            running_.store(false);
            return false;  // socket 尚未移交 HTTPServer，析构即释放
        }
        auto* params = new Poco::Net::HTTPServerParams;
        params->setMaxThreads(16);
        params->setMaxQueued(100);
        params->setKeepAlive(true);
        // [S2] Slowloris 防线（round7 S2-1）：Poco 默认连接超时 60s——
        // 谎报 Content-Length 的半开连接可占住全部 16 个工作线程 60s，
        // 正常请求饿死（实测 25s 无响应）。收紧为 5s 空闲收包超时：
        // 超时按"单次 recv 空闲"计（setReceiveTimeout），持续慢速发送
        //（大媒体上传/慢客户端逐块发）不受影响；完全静默的半开连接
        // 5s 内被回收。keep-alive 空闲同步收紧（默认 10s）。
        params->setTimeout(Poco::Timespan(5, 0));          // 连接收包空闲超时
        params->setKeepAliveTimeout(Poco::Timespan(5, 0)); // keep-alive 空闲
        impl_->server = std::make_unique<Poco::Net::HTTPServer>(
                new HandlerFactory(api_), *socket, params);
        impl_->server->start();
        // HTTPServer 持引用拷贝（SocketAddress→StreamSocket 已在建连时
        // 使用完毕），socket 生命周期移交本 Impl 兜底——start 成功后无法
        // 再交回，故存到 Impl：stop 时随 server 一起析构（顺序：先 server
        // 后 socket，与 Poco 析构约定一致）。
        impl_->socket = std::move(socket);
        return true;
    } catch (const Poco::Exception& ex) {
        err = ex.displayText();
    } catch (const std::exception& ex) {
        err = ex.what();
    }
    running_.store(false);
    return false;
}

void HttpServer::stop() {
    if (!running_.exchange(false)) {
        if (impl_ && impl_->server) {
            impl_->server->stop();
            impl_->server.reset();
            impl_->socket.reset();
        }
        return;
    }
    if (impl_ && impl_->server) {
        impl_->server->stop();
        impl_->server.reset();
        impl_->socket.reset();
    }
}

}  // namespace media_api
