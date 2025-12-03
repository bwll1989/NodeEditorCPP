//
// Created by bwll1 on 2024/9/1.
//

#pragma once

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QQueue>
#include <QtNetwork/QUdpSocket>
#include <QTimer>
#include <QtCore/qglobal.h>
#include "ArtnetFrame.h"
#include "ConstantDefines.h"
#if defined(ARTNETTRANSMITTER_LIBRARY)
#define ARTNETTRANSMITTER_EXPORT Q_DECL_EXPORT
#else
#define ARTNETTRANSMITTER_EXPORT Q_DECL_IMPORT
#endif

/**
 * @brief Art-Net传输器单例类
 * 
 * 专注于Art-Net数据包的发送功能，通过队列机制依次发送ArtnetFrame数据
 */
class ARTNETTRANSMITTER_EXPORT ArtnetTransmitter : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 获取单例实例
     * @return ArtnetTransmitter* 单例实例指针
     */
    static ArtnetTransmitter* getInstance();

private:
    /**
     * @brief 私有构造函数，确保单例模式
     * 自动启动UDP套接字服务
     * @param parent 父对象
     */
    explicit ArtnetTransmitter(QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~ArtnetTransmitter();

    // 禁用拷贝构造和赋值操作
    ArtnetTransmitter(const ArtnetTransmitter&) = delete;
    ArtnetTransmitter& operator=(const ArtnetTransmitter&) = delete;

    // 单例实例
    static ArtnetTransmitter* artnetInstance;

public slots:
    /**
     * @brief 添加Art-Net数据帧到发送队列
     * @param frame Art-Net数据帧
     */
    void enqueueFrame(const ArtnetFrame& frame);

    /**
     * @brief 批量添加Art-Net数据帧到发送队列
     * @param frames Art-Net数据帧列表
     */
    void enqueueFrames(const QList<ArtnetFrame>& frames);

    /**
     * @brief 清空发送队列
     */
    void clearQueue();

    /**
     * @brief 获取队列中待发送的帧数量
     * @return int 队列长度
     */
    int getQueueSize() const;

signals:
    /**
     * @brief 数据帧发送成功信号
     * @param frame 已发送的数据帧
     * @param bytesWritten 实际发送的字节数
     */
    void frameSent(const ArtnetFrame& frame, qint64 bytesWritten);

    /**
     * @brief 数据帧发送失败信号
     * @param frame 发送失败的数据帧
     * @param errorString 错误信息
     */
    void frameSendFailed(const ArtnetFrame& frame, const QString& errorString);

    /**
     * @brief 队列状态变化信号
     * @param queueSize 当前队列大小
     */
    void queueSizeChanged(int queueSize);

private slots:
    /**
     * @brief 初始化UDP套接字
     */
    void initializeSocket();

    /**
     * @brief 清理资源
     */
    void cleanup();

    /**
     * @brief 处理发送队列
     *
     * 每25ms触发一次，若队列不为空，则一次性取出现有队列中的所有帧并逐一发送。
     * 发送完成后更新队列大小信号。
     */
    Q_INVOKABLE void processQueue();

    /**
     * @brief 定时处理队列
     */
    void onQueueTimer();

private:
    // 工作线程
    QThread* mThread;
    
    // UDP套接字
    QUdpSocket* mSocket;
    
    // 发送队列
    QQueue<ArtnetFrame> mSendQueue;
    
    // 互斥锁
    mutable QMutex mMutex;
    
    // 队列处理定时器
    QTimer* mQueueTimer;
    
    // 发送间隔（毫秒）
    int mSendInterval=1000/AppConstants::ARTNET_OUTPUT_FPS; // 发送间隔（毫秒），默认40fps

    /**
     * @brief 创建Art-Net数据包
     * @param frame Art-Net数据帧
     * @return QByteArray 构建的Art-Net数据包
     */
    QByteArray createArtnetPacket(const ArtnetFrame& frame);

    /**
     * @brief 实际发送数据包的内部函数
     * @param frame Art-Net数据帧
     * @return bool 发送是否成功
     */
    bool sendFrameInternal(const ArtnetFrame& frame);
};

/**
 * @brief 导出获取Art-Net传输器单例实例的函数
 * @return ArtnetTransmitter* 单例实例指针
 */
ARTNETTRANSMITTER_EXPORT ArtnetTransmitter* getArtnetTransmitterInstance();


