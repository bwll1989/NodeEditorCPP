#pragma once

#include <QObject>
#include <QMutex>
#include <QVariantMap>
#include <QTimer>
#include "../TcpClient/TcpClient.h"

#ifdef FTDAWController_LIBRARY
#define FTDAWController_EXPORT Q_DECL_EXPORT
#else
#define FTDAWController_EXPORT Q_DECL_IMPORT
#endif
class FTDAWController_EXPORT FTDAWController : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 获取当前全局 TCP 客户端单例指针（不改变引用计数）
     * @return FTDAWController 单例指针
     */
    static FTDAWController* instance();
    
    /**
     * @brief 获取或创建全局 TCP 客户端单例并增加引用计数
     * @return FTDAWController 单例指针
     */
    static FTDAWController* acquire();
    
    /**
     * @brief 释放一次单例引用，当引用计数为 0 时销毁单例
     */
    static void release();
    
    /**
     * @brief 主动连接到服务器（端口固定为 2003）
     * @param host 目标主机地址
     */
    Q_INVOKABLE void connectToServer(const QString &host = QStringLiteral("127.0.0.1"));
    
    /**
     * @brief 主动断开当前连接
     */
    Q_INVOKABLE void disconnectFromServer();
    
    /**
     * @brief 发送文本消息
     * @param message 文本内容
     * @param format 自定义格式标识，透传给 TcpClient
     */
    Q_INVOKABLE void sendMessage(const QString &message, int format = 2);

signals:
    /**
     * @brief TCP 连接就绪状态变化
     * @param ready 是否已就绪
     */
    void isReady(const bool &ready);
    
    /**
     * @brief 收到服务器消息
     * @param msg 解析后的消息内容
     */
    void recMsg(const QVariantMap &msg);
    /**
     * @brief 主机地址变化信号
     * @param host 新的主机地址
     */
    void onHostChanged(const QString &host);
private slots:
    void sendHeartbeat();
    void onConnectionStateChanged(bool ready);

private:
    /**
     * @brief 私有构造函数，仅供单例工厂使用
     */
    explicit FTDAWController(QObject *parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~FTDAWController();

    FTDAWController(const FTDAWController&) = delete;
    FTDAWController& operator=(const FTDAWController&) = delete;

    QMutex m_mutex;
    TcpClient *m_client;
    QTimer *m_heartbeatTimer;
    QString m_host;
    static const int FIXED_PORT = 2003;
    qint64 m_lastCommandTime = 0;
    
    static FTDAWController* s_instance;
    static int s_refCount;
    static QMutex s_refMutex;
};

