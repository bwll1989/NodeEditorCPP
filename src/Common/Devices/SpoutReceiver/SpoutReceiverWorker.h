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

#include <QThread>
#include <QMutex>
#include <QOpenGLContext>
#include <QOffscreenSurface>
#include <QSize>
#include <QDebug>

#include "SpoutLibrary.h"

// 再次确保没有 min/max 宏定义
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

#include <opencv2/opencv.hpp>

/**
 * @brief Spout接收器工作线程
 * 
 * 负责在独立线程中管理 OpenGL 上下文和 Spout 接收操作。
 * 实现了线程安全的发送器切换和图像接收。
 */
class SpoutReceiverWorker : public QThread
{
    Q_OBJECT

public:
    explicit SpoutReceiverWorker(QObject* parent = nullptr);
    ~SpoutReceiverWorker() override;

    /**
     * @brief 设置目标发送器名称
     * @param name 发送器名称
     */
    void setSenderName(const QString& name);

    /**
     * @brief 停止接收并退出线程
     */
    void stop();

signals:
    /**
     * @brief 收到新帧信号
     * @param frame OpenCV 格式的图像 (BGR)
     */
    void frameReceived(cv::Mat frame);

    /**
     * @brief 连接状态改变信号
     * @param connected 是否已连接到发送器
     */
    void connectionStatusChanged(bool connected);

protected:
    /**
     * @brief 线程主循环
     */
    void run() override;

private:
    /**
     * @brief 初始化 Spout 实例
     */
    void initializeSpout();

    /**
     * @brief 接收单帧图像
     * @return 是否成功接收
     */
    bool receiveFrame();

    /**
     * @brief 清理资源
     */
    void cleanup();

    // 线程同步
    QMutex m_mutex;
    bool m_running;
    
    // 发送器名称管理
    QString m_senderName;         // 当前工作线程使用的名称
    QString m_pendingSenderName;  // 待切换的名称
    bool m_switchRequested;       // 切换请求标志

    // 状态
    bool m_isConnected;

    // OpenGL 资源
    QOpenGLContext* m_context;
    QOffscreenSurface* m_surface;

    // Spout 资源
    SPOUTHANDLE m_spout;
    unsigned char* m_imageBuffer;
    unsigned int m_width;
    unsigned int m_height;
};
