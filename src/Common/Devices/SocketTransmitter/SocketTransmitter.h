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

#if defined(CLIENTCONTROLLER_LIBRARY)
#define DLL_EXPORT Q_DECL_EXPORT
#else
#define DLL_EXPORT Q_DECL_IMPORT
#endif

class DLL_EXPORT SocketTransmitter : public QObject
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
DLL_EXPORT SocketTransmitter* getSharedInstance();

#endif // SOCKETTRANSMITTER_H
