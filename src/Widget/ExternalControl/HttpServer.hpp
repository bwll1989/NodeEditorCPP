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
#include <atomic>
#include <QMutex>
#include <QThread>
#include <QByteArray>
#include <QJsonArray>
#include <QJsonObject>
#include "OSCMessage.h"
#include "Poco/Net/HTTPResponse.h"
#include "StatusContainer/StatusItem.h"
#include "ActionRegistry.h"

namespace Flow {

    class NodeHttpServer; // Forward declaration

    /**
     * @brief 单个 WebSocket 会话（shared_ptr 管理），收发在 Poco 工作线程；
     *        广播线程只通过 send() 排队写入，避免主线程阻塞。
     */
    class WsSession {
    public:
        /** @return false 表示套接字已失效，调用方应注销并 forceClose */
        bool send(const std::string& message);
        /** @brief 回复 PING（与 send 共用发送锁） */
        bool sendPong(const char* data, int len);
        void forceClose();
        void bindSocket(Poco::Net::WebSocket* ws);
        void clearSocket();
        bool alive() const { return _alive.load(std::memory_order_acquire); }

    private:
        Poco::Net::WebSocket* _ws = nullptr;
        QMutex _sendMutex;
        std::atomic<bool> _alive{true};
    };

    class PageWebSocketHandler : public Poco::Net::HTTPRequestHandler {
    public:
        // 函数级注释：构造 WebSocket 处理器，传入服务器实例以便注册
        explicit PageWebSocketHandler(NodeHttpServer& server);
        
        // 函数级注释：处理 WebSocket 连接
        void handleRequest(Poco::Net::HTTPServerRequest& request,
                           Poco::Net::HTTPServerResponse& response) override;
        
    private:
        NodeHttpServer& _server;
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
        void handleApiAuthSetting(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response);
        void handleApiCommand(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response);
        void handleApiExec(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response, const std::string& query);
        void handleLayoutSave(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response);
        void handleLayoutLoad(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response);
        void handleActions(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response, const std::string& subPath);
        // 函数级注释：处理媒体文件上传（octet-stream，query中携带filename）
        void handleUploadMedia(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response);
        // 函数级注释：处理.flow项目文件上传（octet-stream，query中携带filename，仅允许.flow扩展名）
        void handleUploadFlow(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response);
        // 函数级注释：下载当前Flow文件（取最近文件列表首项）
        void handleDownloadCurrentFlow(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response);
        // 函数级注释：获取当前Flow文件信息（返回JSON）
        void handleGetCurrentFlowInfo(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response);
        // 函数级注释：获取软件名称与版本（返回JSON）
        void handleGetAppInfo(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response);
        void handleApiLogs(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response, const std::string& subPath);
        void handleApiMedia(Poco::Net::HTTPServerRequest& request, Poco::Net::HTTPServerResponse& response, const std::string& subPath);
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
        ~NodeHttpServer() override;
        
        // 函数级注释：注册 WebSocket 会话
        void registerWebSocket(const std::shared_ptr<WsSession>& session);
        
        // 函数级注释：注销 WebSocket 会话
        void unregisterWebSocket(const std::shared_ptr<WsSession>& session);

        /** @brief 在广播线程执行实际 sendFrame（由 WsBroadcastWorker 调用） */
        void deliverWsBroadcast(const QByteArray& jsonUtf8);

        // 函数级注释：设置静态文件文档根目录
        void setDocRoot(const std::string& docRoot);
        // 函数级注释：启动HTTP服务器，监听指定端口
        bool start(int port);
        // 函数级注释：停止HTTP服务器，释放资源
        void stop();
        // 函数级注释：查询服务器是否正在运行
        bool running() const { return _running.load(std::memory_order_acquire); }
        // 函数级注释：获取当前监听端口
        int port() const { return _port; }
        // 函数级注释：获取当前布局配置（含 actions）
        QJsonObject save() const;
        // 函数级注释：设置布局配置
        void load(const QJsonObject& layout);

        /** @brief 保存布局并同步动作库 used 标记 */
        void applyLayoutSave(const QJsonObject& layout);

        ActionRegistry& actionRegistry() { return _actionRegistry; }
        const ActionRegistry& actionRegistry() const { return _actionRegistry; }

        /**
         * @brief 添加动作到库（主键 entity）
         * @return entity；无效时返回空字符串
         */
        QString addAction(const QJsonObject& binding);

        bool patchAction(const QString& entity, const QJsonObject& patch);
        bool removeAction(const QString& entity);

        /** @brief 向所有 WebSocket 客户端广播 JSON */
        void broadcastJson(const QJsonObject& payload);

        // 函数级注释：通知Flow文件上传完成
        void notifyFlowFileUploaded(const QString& path) {
            emit flowFileUploaded(path);
        }

    signals:
        // 函数级注释：Flow文件上传完成信号
        void flowFileUploaded(const QString& path);

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
        std::atomic<bool> _running{false};
        
        std::set<std::shared_ptr<WsSession>> _wsHandlers;
        QMutex _wsMutex;
        QJsonObject _layout;
        ActionRegistry _actionRegistry;

        QThread* _wsBroadcastThread = nullptr;
        QObject* _wsBroadcastWorker = nullptr; // 广播线程上下文，仅作 QueuedConnection 目标
        std::atomic<int> _wsBroadcastPending{0};

        void notifyActionsChanged(const QString& action, const QJsonObject& item);
        void enqueueWsBroadcast(const QByteArray& jsonUtf8);
        void startWsBroadcastThread();
        void stopWsBroadcastThread();
    };
}
