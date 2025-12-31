#pragma once
#include <string>
#include <memory>
#include <QObject>
#include <Poco/Net/HTTPServer.h>
#include <Poco/Net/HTTPRequestHandler.h>
#include <Poco/Net/HTTPRequestHandlerFactory.h>
#include <Poco/Net/HTTPServerParams.h>
#include <Poco/Net/ServerSocket.h>
#include <Poco/Path.h>
#include <Poco/File.h>
#include <Poco/Buffer.h>
#include <Poco/StreamCopier.h>
#include <Poco/Net/WebSocket.h>
#include <Poco/Net/NetException.h>
#include <set>
#include <QMutex>
#include <QJsonObject>
#include "OSCMessage.h"
#include "Poco/Net/HTTPResponse.h"
#include "StatusContainer/StatusItem.h"

namespace NodeStudio {

    class NodeHttpServer; // Forward declaration

    class PageWebSocketHandler : public Poco::Net::HTTPRequestHandler {
    public:
        // 函数级注释：构造 WebSocket 处理器，传入服务器实例以便注册
        explicit PageWebSocketHandler(NodeHttpServer& server);
        
        // 函数级注释：处理 WebSocket 连接
        void handleRequest(Poco::Net::HTTPServerRequest& request,
                           Poco::Net::HTTPServerResponse& response) override;
                           
        // 函数级注释：发送数据给客户端
        void send(const std::string& message);
        
    private:
        NodeHttpServer& _server;
        Poco::Net::WebSocket* _ws = nullptr;
    };

    class StaticRequestHandler final : public Poco::Net::HTTPRequestHandler {
    public:
        // 函数级注释：构造请求处理器，指定文档根目录与服务器引用（静态文件与布局API）
        explicit StaticRequestHandler(const std::string& docRoot, NodeHttpServer& server)
            : _docRoot(docRoot), _server(server) {}

        // 函数级注释：处理HTTP请求，返回静态文件或内置首页
        void handleRequest(Poco::Net::HTTPServerRequest& request,
                           Poco::Net::HTTPServerResponse& response) override;
    private:
        std::string _docRoot;
        NodeHttpServer& _server;
        // 函数级注释：根据文件扩展名推断Content-Type
        static std::string guessContentType(const std::string& ext);
        // 函数级注释：生成内置首页HTML
        static std::string builtInIndexHtml();

        // Helper methods for request handling
        void handleApiCommand(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response);
        void handleApiExec(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response, const std::string& query);
        void handleLayoutSave(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response);
        void handleLayoutLoad(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response);
        // 函数级注释：处理媒体文件上传（octet-stream，query中携带filename）
        void handleUploadMedia(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response);
        // 函数级注释：处理.flow项目文件上传（octet-stream，query中携带filename，仅允许.flow扩展名）
        void handleUploadFlow(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response);
        void handleStaticFile(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response, const std::string& path);
        
        // Utility to send JSON response
        void sendJsonResponse(Poco::Net::HTTPServerResponse& response, const std::string& json, Poco::Net::HTTPResponse::HTTPStatus status = Poco::Net::HTTPResponse::HTTP_OK);
        // 函数级注释：从查询串中解析指定键的值（例如 filename），未找到返回空字符串
        static std::string parseQueryParam(const std::string& query, const std::string& key);
        // 函数级注释：对文件名进行安全过滤，移除路径分隔与非法字符
        static std::string sanitizeFilename(const std::string& name);
    };

    class StaticRequestHandlerFactory final : public Poco::Net::HTTPRequestHandlerFactory {
    public:
        // 函数级注释：构造工厂，保存文档根目录和服务器实例
        explicit StaticRequestHandlerFactory(const std::string& docRoot, NodeHttpServer& server)
            : _docRoot(docRoot), _server(server) {}
        // 函数级注释：创建请求处理器实例
        Poco::Net::HTTPRequestHandler* createRequestHandler(
            const Poco::Net::HTTPServerRequest& request) override;
    private:
        std::string _docRoot;
        NodeHttpServer& _server;
    };

    class NodeHttpServer final : public QObject {
        Q_OBJECT
    public:
        // 函数级注释：构造函数，初始化服务器状态（QObject基类）
        explicit NodeHttpServer(QObject* parent = nullptr);
        
        // 函数级注释：注册 WebSocket 处理器
        void registerWebSocket(PageWebSocketHandler* handler);
        
        // 函数级注释：注销 WebSocket 处理器
        void unregisterWebSocket(PageWebSocketHandler* handler);

        // 函数级注释：设置静态文件文档根目录
        void setDocRoot(const std::string& docRoot);
        // 函数级注释：启动HTTP服务器，监听指定端口
        bool start(int port);
        // 函数级注释：停止HTTP服务器，释放资源
        void stop();
        // 函数级注释：查询服务器是否正在运行
        bool running() const { return _running; }
        // 函数级注释：获取当前监听端口
        int port() const { return _port; }
        // 函数级注释：获取当前布局配置
        QJsonObject save() const { return _layout; }
        // 函数级注释：设置布局配置
        void load(const QJsonObject& layout);

    signals:
        // 函数级注释：服务器启动信号
        void serverStarted(int port);
        // 函数级注释：服务器停止信号
        void serverStopped();

    public slots:
        // 函数级注释：处理 OSC 消息发送，广播给所有 WebSocket 连接
        void onOscMessageSent(const StatusItem& message);

    private:
        std::unique_ptr<Poco::Net::HTTPServer> _server;
        std::string _docRoot;
        int _port = 0;
        bool _running = false;
        
        std::set<PageWebSocketHandler*> _wsHandlers;
        QMutex _wsMutex;
        QJsonObject _layout;
    };
}
