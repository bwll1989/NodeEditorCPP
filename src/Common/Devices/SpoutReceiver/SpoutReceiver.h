#pragma once

// 防止 Windows 宏定义冲突
#ifndef NOMINMAX
#define NOMINMAX
#endif

// 防止 Windows.h 定义 min/max 宏
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <QObject>
#include <QStringList>
#include <opencv2/opencv.hpp>
#include "SpoutReceiverWorker.h"
#ifdef SPOUTRECEIVER_LIBRARY
#define SPOUTRECEIVER_EXPORT Q_DECL_EXPORT
#else
#define SPOUTRECEIVER_EXPORT Q_DECL_IMPORT
#endif
/**
 * @brief Spout 接收器封装类
 * 
 * 提供简化的接口来管理 Spout 接收线程。
 * 可以在主线程直接使用此类。
 */
class SPOUTRECEIVER_EXPORT SpoutReceiver : public QObject
{
    Q_OBJECT
public:
    explicit SpoutReceiver(QObject *parent = nullptr);
    ~SpoutReceiver();

    /**
     * @brief 开始接收
     * @param senderName 发送器名称（可选）
     */
    void start(const QString& senderName = "");

    /**
     * @brief 停止接收
     */
    void stop();

    /**
     * @brief 切换发送器
     * @param senderName 新的发送器名称
     */
    void setSenderName(const QString& senderName);

    /**
     * @brief 获取当前可用的发送器列表
     * @return 发送器名称列表
     * @note 此方法可以在主线程直接调用
     */
    static QStringList getSenderList();

signals:
    /**
     * @brief 收到新帧
     */
    void frameReceived(cv::Mat frame);

    /**
     * @brief 连接状态改变
     */
    void connectionStatusChanged(bool connected);

private:
    SpoutReceiverWorker* m_worker;
};
