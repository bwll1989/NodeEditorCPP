#pragma once

#include <QObject>
#include <QThread>
#include <QUdpSocket>
#include <QMutex>
#include <QQueue>
#include <QTimer>
#include <QVariant>
#include "OSCMessage.h"

#ifdef OSCTRANSMITTER_LIBRARY
#define OSCTRANSMITTER_EXPORT Q_DECL_EXPORT
#else
#define OSCTRANSMITTER_EXPORT Q_DECL_IMPORT
#endif

/**
 * @brief OSC传输器类 - 多线程全局单例实现
 * 提供线程安全的OSC消息发送功能，支持队列和直接发送两种模式
 */
class OSCTRANSMITTER_EXPORT OSCSender : public QObject
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数，自动启动传输器
     * @param dstHost 默认目标主机地址
     * @param port 默认目标端口
     * @param parent 父对象
     */
    explicit OSCSender(QString dstHost="127.0.0.1", quint16 port = 6001, QObject *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~OSCSender();
    
    /**
     * @brief 获取全局单例实例
     * @return OSCSender单例指针
     */
    static OSCSender* instance() {
        static OSCSender* sender = nullptr;
        if (!sender) {
            sender = new OSCSender();
        }
        return sender;
    }
    
public slots:
    /**
     * @brief 处理消息队列中的OSC消息
     */
    void processQueue();
    
    /**
     * @brief 将OSC消息加入队列发送（线程安全）
     * @param message OSC消息对象
     * @return 是否成功加入队列
     */
    bool sendOSCMessageWithQueue(const OSCMessage &message);
    
    /**
     * @brief 直接发送OSC消息（线程安全）
     * @param message OSC消息对象
     * @return 是否发送成功
     */
    bool sendOSCMessageDirectly(const OSCMessage &message);
    
    /**
     * @brief 设置目标主机和端口
     * @param address 主机地址
     * @param port 端口号
     */
    void setHost(QString address, int port);
    
    /**
     * @brief 初始化UDP套接字
     */
    void initializeSocket();
    
    /**
     * @brief 清理资源
     */
    void cleanup();
    
private:
    QTimer *m_timer;                    ///< 消息处理定时器
    quint16 mPort;                      ///< 目标端口
    QString mHost;                      ///< 目标主机
    QThread *mThread;                   ///< 工作线程
    QUdpSocket *mSocket;                ///< UDP套接字
    QMutex m_mutex;                     ///< 线程同步互斥锁
    QQueue<OSCMessage> m_messageQueue;  ///< OSC消息队列
    
    /// 消息处理间隔时间（毫秒）
    static const int PROCESS_INTERVAL = 16;
};

