#ifndef SOCKETTRANSMITTER_H
#define SOCKETTRANSMITTER_H
#include <QObject>
#include <QWebSocketServer>
#include <QWebSocket>
#include <QThread>
#include <QMutex>
#include <QQueue>
#include <QJsonDocument>
#include <QtCore/qglobal.h>

// 【2026-11-13】专用导出宏：原 DLL_EXPORT 是各节点插件 PluginDefinition.hpp
// 的通用宏名——插件的 #ifndef DLL_EXPORT 抢注会让本头在插件 TU 内错误展开为
// dllexport（TimeLineNode 首编 LNK2019 staticMetaObject 即此因），改专有名隔离。
#ifndef CLIENTCONTROLLER_EXPORT
#if defined(CLIENTCONTROLLER_LIBRARY)
#define CLIENTCONTROLLER_EXPORT Q_DECL_EXPORT
#else
#define CLIENTCONTROLLER_EXPORT Q_DECL_IMPORT
#endif
#endif

class CLIENTCONTROLLER_EXPORT SocketTransmitter : public QObject
{
    Q_OBJECT
public:
    // 获取单例实例
    static SocketTransmitter* getInstance();

    // 启动服务器
    bool startServer(quint16 port = 34455);

    // 停止服务器
    void stopServer();
    // 私有构造函数
    explicit SocketTransmitter(QObject* parent = nullptr);

    // SocketTransmitter(const SocketTransmitter&) = delete;
    //
    // SocketTransmitter& operator=(const SocketTransmitter&) = delete;
    // 单例实例
    static SocketTransmitter* socketInstance;
public slots:
    // 添加JSON数据到发送队列
    void enqueueJson(const QJsonDocument& json);

signals:
    // 新连接信号
    void newConnection(QWebSocket* socket);

private:
    // WebSocket服务器
    QWebSocketServer* m_server;

    // 客户端连接列表
    QList<QWebSocket*> m_clients;

    // 发送队列
    QQueue<QJsonDocument> m_sendQueue;

    // 互斥锁
    QMutex m_mutex;

public slots:
    // 处理新连接
    void onNewConnection();

    // 处理客户端断开连接
    void onClientDisconnected();

    // 处理发送队列
    Q_INVOKABLE void processQueue();
};

// 导出获取单例实例的函数
CLIENTCONTROLLER_EXPORT SocketTransmitter* getClientControlInstance();

#endif // SOCKETTRANSMITTER_H
