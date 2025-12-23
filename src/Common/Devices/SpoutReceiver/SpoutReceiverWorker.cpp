//
// Created by WuBin on 2025/12/19.
//

#include "SpoutReceiverWorker.h"

#include <QtGui/QOffscreenSurface>
#include <QtGui/QSurfaceFormat>

SpoutReceiverWorker::SpoutReceiverWorker(QObject* parent)
    : QThread(parent)
    , m_running(false)
    , m_switchRequested(false)
    , m_isConnected(false)
    , m_context(nullptr)
    , m_surface(nullptr)
    , m_spout(nullptr)
    , m_imageBuffer(nullptr)
    , m_width(0)
    , m_height(0)
{
    // 在创建线程（主线程）中初始化 Surface
    QSurfaceFormat format = QSurfaceFormat::defaultFormat();
    format.setProfile(QSurfaceFormat::CompatibilityProfile); // Spout 通常需要兼容模式
    
    m_surface = new QOffscreenSurface();
    m_surface->setFormat(format);
    m_surface->create();
}

SpoutReceiverWorker::~SpoutReceiverWorker()
{
    stop();
    wait();

    // Surface 必须在创建它的线程（通常是主线程）销毁
    if (m_surface) {
        m_surface->destroy();
        delete m_surface;
        m_surface = nullptr;
    }
}

void SpoutReceiverWorker::setSenderName(const QString& name)
{
    QMutexLocker locker(&m_mutex);
    if (m_senderName != name || m_pendingSenderName != name) {
        m_pendingSenderName = name;
        m_switchRequested = true;
    }
}

void SpoutReceiverWorker::stop()
{
    QMutexLocker locker(&m_mutex);
    m_running = false;
    m_switchRequested = true;
}

void SpoutReceiverWorker::run()
{
    // 1. 创建 OpenGL 上下文 (在工作线程中)
    m_context = new QOpenGLContext();
    m_context->setFormat(m_surface->format());
    if (!m_context->create()) {
        qWarning() << "SpoutReceiverWorker: Failed to create OpenGL context";
        return;
    }

    if (!m_context->makeCurrent(m_surface)) {
        qWarning() << "SpoutReceiverWorker: Failed to make OpenGL context current";
        delete m_context;
        m_context = nullptr;
        return;
    }

    // 2. 初始化 Spout
    initializeSpout();

    m_running = true;
    m_isConnected = false;
    // 3. 主循环
    while (true) {
        // 检查退出条件
        {
            QMutexLocker locker(&m_mutex);
            if (!m_running) {
                break;
            }
        }
        // 处理发送器切换
        bool switchNeeded = false;
        QString newSenderName;
        {
            QMutexLocker locker(&m_mutex);
            if (m_switchRequested) {
                switchNeeded = true;
                newSenderName = m_pendingSenderName;
                m_switchRequested = false;
            }
        }

        if (switchNeeded) {
            // 释放旧资源
            if (m_spout) {
                m_spout->ReleaseReceiver();
            }
            if (m_imageBuffer) {
                delete[] m_imageBuffer;
                m_imageBuffer = nullptr;
            }
            m_width = 0;
            m_height = 0;
            // 更新名称
            m_senderName = newSenderName;
            // 如果新名称不为空，尝试设置接收器名称
            if (!m_senderName.isEmpty() && m_spout) {
                m_spout->SetReceiverName(m_senderName.toLocal8Bit().data());
            }
            
            // 重置连接状态
            m_isConnected = false;
            emit connectionStatusChanged(false);
        }
        // 接收帧
        bool hasFrame = false;
        if (!m_senderName.isEmpty()) {
            hasFrame = receiveFrame();
        }

        // 更新连接状态
        if (hasFrame != m_isConnected) {
            m_isConnected = hasFrame;
            emit connectionStatusChanged(m_isConnected);
        }

        // 如果未连接或无帧，适当休眠以降低 CPU 占用
        if (!hasFrame) {
            msleep(33); // 约 30fps 的检查频率
        } else {
            // 有帧时也稍微休眠一下，或者依赖 Spout 的同步
            // Spout 的 ReceiveImage 可能会等待垂直同步，取决于设置
            // 这里加一点点延时防止死循环占用过高，但对于实时性要求高的应用可以去掉
             msleep(10);
        }
    }
    // 4. 清理
    cleanup();

    if (m_context) {
        m_context->doneCurrent();
        delete m_context;
        m_context = nullptr;
    }
}

void SpoutReceiverWorker::initializeSpout()
{
    m_spout = GetSpout();
    if (!m_spout) {
        qWarning() << "SpoutReceiverWorker: Failed to create Spout instance";
    }
}

bool SpoutReceiverWorker::receiveFrame()
{
    if (!m_spout) return false;

    unsigned int senderWidth = 0;
    unsigned int senderHeight = 0;
    HANDLE dxShareHandle = nullptr;
    DWORD dwFormat = 0;

    // 1. 获取发送器信息
    // 只有当尺寸未知或需要定期检查时才调用 GetSenderInfo ?
    // 为了稳健性，我们可以先尝试 ReceiveImage，如果失败再检查信息。
    // 但标准流程通常是先确保连接。
    
    // 简化逻辑：直接调用 ReceiveImage，如果缓冲区不够大会失败吗？
    // Spout SDK 文档建议先 CheckReceiver 或 GetSenderInfo
    
    // 这里采用 SpoutInDataModel 中的稳健逻辑：
    // 周期性检查 SenderInfo 以应对分辨率变化
    static int infoCounter = 0;
    if (m_width == 0 || m_height == 0 || (++infoCounter % 60 == 0)) {
        bool infoResult = m_spout->GetSenderInfo(m_senderName.toStdString().c_str(), senderWidth, senderHeight, dxShareHandle, dwFormat);
        
        // 如果指定名称找不到，尝试寻找活动发送器？
        // 只有当 m_senderName 为空时才找活动发送器，但这里 m_senderName 不为空
        // 所以如果找不到，就是真的断开了
        
        if (infoResult && senderWidth > 0 && senderHeight > 0) {
            if (m_width != senderWidth || m_height != senderHeight) {
                // 分辨率变化，重新分配缓冲区
                if (m_imageBuffer) {
                    delete[] m_imageBuffer;
                    m_imageBuffer = nullptr;
                }
                m_width = senderWidth;
                m_height = senderHeight;
                m_imageBuffer = new unsigned char[m_width * m_height * 4]; // BGRA
            }
        } else {
            return false;
        }
    }

    if (m_width == 0 || m_height == 0 || !m_imageBuffer) return false;

    // 2. 接收图像
    bool result = m_spout->ReceiveImage(m_imageBuffer, GL_BGRA, false); // false = 不翻转
    
    if (result) {
        // 检查是否有新帧
        if (m_spout->IsFrameNew()) {
            // 转换为 OpenCV Mat (BGRA -> BGR)
            cv::Mat bgraFrame(m_height, m_width, CV_8UC4, m_imageBuffer);
            cv::Mat bgrFrame;
            cv::cvtColor(bgraFrame, bgrFrame, cv::COLOR_BGRA2BGR);
            
            // 发送深拷贝
            emit frameReceived(bgrFrame.clone());
        }
        return true;
    }
    
    return false;
}

void SpoutReceiverWorker::cleanup()
{
    if (m_imageBuffer) {
        delete[] m_imageBuffer;
        m_imageBuffer = nullptr;
    }

    if (m_spout) {
        m_spout->ReleaseReceiver();
        m_spout->Release();
        m_spout = nullptr;
    }
}
