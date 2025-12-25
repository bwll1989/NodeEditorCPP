#pragma once
#include <QObject>
#include <QWebSocket>
#include <QJsonObject>
#include <QJsonDocument>
#include <QJsonValue>
#include "Common/Devices/StatusContainer/StatusContainer.h"
#include "Common/Devices/WebSocketServer/WebSocketServer.h"
#include "Widget/ExternalControl/ExternalControler.hpp"
#include "OSCMessage.h"

#ifdef STATUSCONTAINER_LIBRARY
#define STATUSCONTAINER_EXPORT Q_DECL_EXPORT
#else
#define STATUSCONTAINER_EXPORT Q_DECL_IMPORT
#endif

class STATUSCONTAINER_EXPORT ExternalGateway : public QObject {
    Q_OBJECT
public:
    /**
     * @brief 获取单例实例
     */
    static ExternalGateway* instance();

    /**
     * @brief 启动 WebSocket 服务并建立桥接
     * @param port WebSocket 监听端口，默认 2003（与现有 WebSocketServer 保持一致）
     */
    void start(quint16 port = 2003);

    /**
     * @brief 停止 WebSocket 服务并断开桥接
     */
    void stop();

    /**
     * @brief 将当前最新状态快照通过 WebSocket 广播一次
     * 用于客户端新连接后主动同步已有状态
     * @param prefix 地址前缀过滤，默认 "/dataflow/"
     */
    void broadcastSnapshot(const QString& prefix = "/dataflow/");

private:
    /**
     * @brief 构造函数（私有），初始化对象与信号桥接
     */
    explicit ExternalGateway(QObject* parent = nullptr);

    /**
     * @brief 解析来自 WebSocket 的消息并转交 ExternalControler
     * 支持 JSON 文本：{"address": "...", "value": any}
     * 非 JSON 文本将忽略
     * @param socket 发送该消息的客户端套接字
     * @param payload 原始消息字节
     */
    void handleWebSocketMessage(QWebSocket* socket, const QByteArray& payload);

    /**
     * @brief 将状态消息编码为 JSON 文本并广播
     * @param message 状态消息
     */
    void broadcastStatus(const OSCMessage& message);

private slots:
    /**
     * @brief 订阅 StatusContainer 的新增状态并广播
     * @param message 新增状态消息
     */
    void onStatusAdded(const OSCMessage& message);

    /**
     * @brief 处理 WebSocket 新连接（当前实现选择广播快照）
     * @param socket 新连接的客户端
     */
    void onNewConnection(QWebSocket* socket);

    /**
     * @brief 处理 WebSocket 收到的消息
     * @param socket 客户端
     * @param payload 原始数据
     */
    void onWebSocketMessage(QWebSocket* socket, const QByteArray& payload);

private:
    static ExternalGateway* s_inst;
    WebSocketServer* _wsServer;
};