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

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QMutex>
#include <QtCore/QThread>
#include <QtCore/QStringList>
#include <QtWidgets/QComboBox>
#include <QJsonObject>
#include <QJsonValue>
#include <QtGui/QOpenGLContext>
#include <QtGui/QOffscreenSurface>
#include <QtGui/QSurfaceFormat>
#include <QtCore/QMetaType>
#include "Common/BuildInNodes/AbstractDelegateModel.h"
// 使用 SpoutLibrary API 而不是 SpoutReceiver
#include "SpoutLibrary.h"

// 再次确保没有 min/max 宏定义
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

// 现在安全地包含 OpenCV
#include <opencv2/opencv.hpp>

#include "SpoutOutInterface.hpp"
#include <iostream>
#include <vector>
#include <memory>

using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
    /**
     * @brief Spout接收线程类
     * 
     * 在独立线程中处理Spout图像接收，避免阻塞主线程
     * 使用SpoutLibrary API提供更稳定的接收功能
     */
    class SpoutSendThread : public QThread
    {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数
         * @param parent 父对象
         */
        explicit SpoutSendThread(QObject* parent = nullptr)
            : QThread(parent)
            , m_running(false)
            , m_spout(nullptr)
            , m_imageBuffer(nullptr)
            , m_width(0)
            , m_height(0)
            , m_forceUpdateSenders(false)
            , m_context(nullptr)
            , m_surface(nullptr)
        {
            // 在主线程中创建 OffscreenSurface
            // 使用兼容模式，以防 Spout 需要旧版 OpenGL 功能
            QSurfaceFormat format = QSurfaceFormat::defaultFormat();
            format.setProfile(QSurfaceFormat::CompatibilityProfile);
            
            m_surface = new QOffscreenSurface();
            m_surface->setFormat(format);
            m_surface->create();
        }

        /**
         * @brief 析构函数
         */
        ~SpoutSendThread() override {
            stopReceiving();
            wait();
            // m_surface 在主线程创建，应在主线程销毁
            if (m_surface) {
                m_surface->destroy();
                delete m_surface;
                m_surface = nullptr;
            }
            cleanupSpout();
            
            // 线程结束前清理 OpenGL 上下文
            if (m_context) {
                m_context->doneCurrent();
                delete m_context;
                m_context = nullptr;
            }
        }

        /**
         * @brief 开始接收Spout数据
         * @param senderName 发送器名称，为空则连接到活动发送器
         */
        void startReceiving(const QString& senderName = "") {
            QMutexLocker locker(&m_mutex);
            m_senderName = senderName;
            m_running = true;
            if (!isRunning()) {
                start();
            }
        }

        /**
         * @brief 停止接收Spout数据
         */
        void stopReceiving() {
            QMutexLocker locker(&m_mutex);
            m_running = false;
        }

        /**
         * @brief 设置发送器名称
         * @param senderName 发送器名称
         */
        void setSenderName(const QString& senderName) {
            QMutexLocker locker(&m_mutex);
            m_senderName = senderName;
            qDebug() << "SpoutSendThread: setSenderName:" << m_senderName;
        }

        /**
         * @brief 手动触发发送器列表更新
         */
        void requestSenderListUpdate() {
            QMutexLocker locker(&m_mutex);
            m_forceUpdateSenders = true;
        }

    signals:
        void frameReceived(const cv::Mat& frame);
        void connectionStatusChanged(bool connected);
        void senderListUpdated(const QStringList& senders);

    protected:
        /**
         * @brief 线程主循环
         */
        void run() override {
            // 创建并设置 OpenGL 上下文 (在工作线程中)
            m_context = new QOpenGLContext();
            m_context->setFormat(m_surface->format());
            if (!m_context->create()) {
                qDebug() << "SpoutSendThread: Failed to create OpenGL context";
                delete m_context;
                m_context = nullptr;
                return;
            }

            // 使用主线程创建的 surface
            if (!m_context->makeCurrent(m_surface)) {
                qDebug() << "SpoutSendThread: Failed to make OpenGL context current";
                delete m_context;
                m_context = nullptr;
                return;
            }

            qDebug() << "SpoutSendThread: OpenGL context created and made current";

            initializeSpout();
            
            // 立即扫描一次发送器列表
            updateSenderList();
            
            while (m_running) {
                bool hasFrame = false;
                
                // 如果有指定发送器，尝试接收帧
                if (!m_senderName.isEmpty()) {
                    hasFrame = receiveFrame();
                    emit connectionStatusChanged(hasFrame);
                }
                
                // 定期更新发送器列表或响应手动请求
                static int updateCounter = 0;
                if (m_forceUpdateSenders || (++updateCounter % 60 == 0)) { // 每60帧或手动请求时更新
                    updateSenderList();
                    m_forceUpdateSenders = false;
                }
                
                msleep(hasFrame ? 16 : 100); // 有帧时60fps，无帧时10fps
            }
            
            cleanupSpout();
        }

    private:
        bool m_running;
        bool m_forceUpdateSenders;
        QString m_senderName;
        QMutex m_mutex;
        
        // SpoutLibrary 接口
        SPOUTHANDLE m_spout;
        
        // OpenGL 上下文
        QOpenGLContext* m_context;
        QOffscreenSurface* m_surface;

        // 图像缓冲区
        unsigned char* m_imageBuffer;
        unsigned int m_width;
        unsigned int m_height;
        
        /**
         * @brief 初始化Spout接收器
         */
        void initializeSpout() {
            // 创建 SpoutLibrary 实例
            m_spout = GetSpout();
            if (!m_spout) {
                qDebug() << "SpoutSendThread: Failed to create Spout instance";
                return;
            }
            
            qDebug() << "SpoutSendThread: Spout instance created successfully";
            
            // 设置接收器名称（如果指定）
            if (!m_senderName.isEmpty()) {
                m_spout->SetReceiverName(m_senderName.toLocal8Bit().data());
                qDebug() << "SpoutSendThread: Set receiver name to:" << m_senderName;
            }
            
            // 检查 Spout 状态
            qDebug() << "SpoutSendThread: SDK Version:" << QString::fromStdString(m_spout->GetSDKversion());
            qDebug() << "SpoutSendThread: GL/DX ready:" << m_spout->IsGLDXready();
            qDebug() << "SpoutSendThread: Auto share mode:" << m_spout->GetAutoShare();
            
            // 获取发送器数量
            int senderCount = m_spout->GetSenderCount();
            qDebug() << "SpoutSendThread: Available sender count:" << senderCount;
            
            // 列出所有发送器
            for (int i = 0; i < senderCount; i++) {
                char senderName[256];
                if (m_spout->GetSender(i, senderName, 256)) {
                    qDebug() << "SpoutSendThread: Sender" << i << ":" << senderName;
                }
            }
        }
        
        /**
         * @brief 接收Spout帧数据
         * @return 是否成功接收到帧
         */
        bool receiveFrame() {

            if (!m_spout) {
                qDebug() << "SpoutSendThread: Spout not initialized";
                return false;
            }
            
            // 获取发送器信息
            unsigned int senderWidth = 0, senderHeight = 0;
            HANDLE dxShareHandle = nullptr;
            DWORD dwFormat = 0;
            
            std::string senderNameStd = m_senderName.toStdString();
            bool infoResult = m_spout->GetSenderInfo(senderNameStd.c_str(), senderWidth, senderHeight, dxShareHandle, dwFormat);
            
            if (!infoResult || senderWidth == 0 || senderHeight == 0) {
                // 尝试连接到活动发送器
                char activeSender[256];
                if (m_spout->GetActiveSender(activeSender)) {
                    QString activeSenderName = QString::fromLocal8Bit(activeSender);
                    if (activeSenderName != m_senderName) {
                        qDebug() << "SpoutSendThread: Trying active sender:" << activeSenderName;
                        std::string activeSenderStd = activeSenderName.toStdString();
                        infoResult = m_spout->GetSenderInfo(activeSenderStd.c_str(), senderWidth, senderHeight, dxShareHandle, dwFormat);
                        if (infoResult && senderWidth > 0 && senderHeight > 0) {
                            m_senderName = activeSenderName;
                            m_spout->SetReceiverName(activeSenderStd.c_str());
                        }
                    }
                }
                
                if (!infoResult || senderWidth == 0 || senderHeight == 0) {
                    return false;
                }
            }
            
            // 检查是否需要重新分配缓冲区
            if (m_width != senderWidth || m_height != senderHeight) {
                qDebug() << "SpoutSendThread: Reallocating buffer from" << m_width << "x" << m_height
                         << "to" << senderWidth << "x" << senderHeight;
                
                if (m_imageBuffer) {
                    delete[] m_imageBuffer;
                    m_imageBuffer = nullptr;
                }
                
                m_width = senderWidth;
                m_height = senderHeight;
                m_imageBuffer = new unsigned char[m_width * m_height * 4]; // BGRA/RGBA
                
                qDebug() << "SpoutSendThread: Buffer allocated for" << m_width << "x" << m_height;
            }
            
            // 尝试接收图像数据 - 使用BGRA格式，这是Spout的默认格式
            bool receiveResult = m_spout->ReceiveImage(m_imageBuffer, GL_BGRA, false);
            
            if (!receiveResult) {
                // qDebug() << "SpoutSendThread: ReceiveImage failed"; // 减少日志刷屏
                return false;
            }

            // 调试：检查中心像素值，确认是否有数据
            static int logCounter = 0;
            if (++logCounter % 60 == 0) { // 每60帧打印一次
                int centerX = m_width / 2;
                int centerY = m_height / 2;
                int pixelIndex = (centerY * m_width + centerX) * 4;
                qDebug() << "SpoutSendThread: Frame" << logCounter
                         << "Size:" << m_width << "x" << m_height
                         << "Center Pixel (BGRA):" 
                         << (int)m_imageBuffer[pixelIndex] << (int)m_imageBuffer[pixelIndex+1] 
                         << (int)m_imageBuffer[pixelIndex+2] << (int)m_imageBuffer[pixelIndex+3];
            }
            
            // 检查是否有新帧
            if (!m_spout->IsFrameNew()) {
                return true; // 连接正常，但没有新帧
            }
            
            // 转换为OpenCV Mat - 使用BGRA格式
            cv::Mat bgraFrame(m_height, m_width, CV_8UC4, m_imageBuffer);
            cv::Mat rgbFrame;
            
            // 从BGRA转换为RGB
            cv::cvtColor(bgraFrame, rgbFrame, cv::COLOR_BGRA2RGB);
            
            // 发送帧数据
            emit frameReceived(rgbFrame.clone());
            
            return true;
        }
        
        /**
         * @brief 更新发送器列表
         */
        void updateSenderList() {
            if (!m_spout) {
                return;
            }
            
            QStringList senders;
            int senderCount = m_spout->GetSenderCount();
            
            for (int i = 0; i < senderCount; i++) {
                char senderName[256];
                if (m_spout->GetSender(i, senderName, 256)) {
                    senders.append(QString::fromLocal8Bit(senderName));
                }
            }
            
            emit senderListUpdated(senders);
        }
        
        /**
         * @brief 清理Spout资源
         */
        void cleanupSpout() {
            if (m_imageBuffer) {
                delete[] m_imageBuffer;
                m_imageBuffer = nullptr;
            }
            
            if (m_spout) {
                m_spout->ReleaseReceiver();
                m_spout->Release();
                m_spout = nullptr;
            }
            
            m_width = 0;
            m_height = 0;
            
            qDebug() << "SpoutSendThread: Spout resources cleaned up";
        }
    };

    /**
     * @brief Spout输入节点数据模型
     * 
     * 提供Spout图像接收功能，支持多发送器选择和实时图像传输
     */
    class SpoutOutDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数
         */
        SpoutOutDataModel()
            : m_widget(nullptr)
            , m_receiveThread(nullptr)
            , m_isReceiving(false)
        {
            InPortCount = 0;  // Spout输入不需要输入端口
            OutPortCount = 1;
            CaptionVisible = true;
            Caption = "Spout In";
            WidgetEmbeddable = true;
            Resizable = false;
            PortEditable = false;
            
            initializeReceiver();
        }

        /**
         * @brief 析构函数
         */
        ~SpoutOutDataModel() override {
            stopReceiving();
            if (m_receiveThread) {
                m_receiveThread->wait();
                delete m_receiveThread;
            }
            if (m_widget) {
                m_widget->deleteLater();
            }
        }

        /**
         * @brief 获取端口数据类型
         * @param portType 端口类型
         * @param portIndex 端口索引
         * @return 数据类型
         */
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override {
            Q_UNUSED(portIndex);
            switch (portType) {
            case PortType::Out:
                return ImageData().type();
            default:
                return ImageData().type();
            }
        }

        /**
         * @brief 获取输出数据
         * @param port 端口索引
         * @return 输出数据
         */
        std::shared_ptr<NodeData> outData(PortIndex const port) override {
            Q_UNUSED(port);
            return m_outputImageData ? m_outputImageData : std::make_shared<ImageData>();
        }

        /**
         * @brief 设置输入数据
         * @param data 输入数据
         * @param portIndex 端口索引
         */
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override {
            Q_UNUSED(data);
            Q_UNUSED(portIndex);
            // Spout输入节点不接受输入数据
        }

        /**
         * @brief 保存节点状态
         * @return JSON对象
         */
        QJsonObject save() const override {
            QJsonObject modelJson;
            modelJson["currentSender"] = m_currentSender;
            modelJson["isReceiving"] = m_isReceiving;
            return modelJson;
        }

        /**
         * @brief 加载节点状态
         * @param jsonObject JSON对象
         */
        void load(QJsonObject const &jsonObject) override {
            if (jsonObject.contains("currentSender")) {
                m_currentSender = jsonObject["currentSender"].toString();
            }
            if (jsonObject.contains("isReceiving") && jsonObject["isReceiving"].toBool()) {
                // 延迟启动，确保界面已初始化
                QTimer::singleShot(1000, this, &SpoutOutDataModel::startReceiving);
            }
        }
        
        /**
         * @brief 获取嵌入式控件
         * @return 控件指针
         */
        QWidget *embeddedWidget() override {
            if (!m_widget) {
                m_widget = new SpoutOutInterface();
                
                // 连接信号
                connect(m_widget, &SpoutOutInterface::startReceiving, this, &SpoutOutDataModel::startReceiving);
                connect(m_widget, &SpoutOutInterface::stopReceiving, this, &SpoutOutDataModel::stopReceiving);
                connect(m_widget, &SpoutOutInterface::senderSelected, this, &SpoutOutDataModel::selectSender);
                connect(m_widget, &SpoutOutInterface::refreshRequested, this, &SpoutOutDataModel::refreshSenders);
                
                // 初始刷新发送器列表
                QTimer::singleShot(500, this, &SpoutOutDataModel::refreshSenders);
            }
            return m_widget;
        }

    public slots:
        /**
         * @brief 处理接收到的帧数据
         * @param frame 接收到的图像帧
         */
        void onFrameReceived(const cv::Mat& frame) {
            if (!frame.empty()) {
                m_outputImageData = std::make_shared<ImageData>(frame);
                emit dataUpdated(0);
            }
        }
        
        /**
         * @brief 处理连接状态变化
         * @param connected 连接状态
         */
        void onConnectionStatusChanged(bool connected) {
            if (m_widget) {
                m_widget->updateConnectionStatus(connected);
            }
        }
        
        /**
         * @brief 处理发送器列表更新
         * @param senders 发送器列表
         */
        void onSenderListUpdated(const QStringList& senders) {
            if (m_widget) {
                m_widget->updateSenderList(senders);
            }
        }
        
        /**
         * @brief 开始接收Spout数据
         */
        void startReceiving() {
            if (!m_isReceiving && m_receiveThread) {
                m_receiveThread->startReceiving(m_currentSender);
                m_isReceiving = true;
                qDebug() << "SpoutOutDataModel: Started receiving from:" << m_currentSender;
            }
        }
        
        /**
         * @brief 停止接收Spout数据
         */
        void stopReceiving() {
            if (m_isReceiving && m_receiveThread) {
                m_receiveThread->stopReceiving();
                m_isReceiving = false;
                qDebug() << "SpoutOutDataModel: Stopped receiving";
            }
        }
        
        /**
         * @brief 选择发送器
         * @param senderName 发送器名称
         */
        void selectSender(const QString& senderName) {
            m_currentSender = senderName;
            if (m_receiveThread) {
                m_receiveThread->setSenderName(senderName);
            }
            qDebug() << "SpoutOutDataModel: Selected sender:" << senderName;
        }
        
        /**
         * @brief 刷新发送器列表
         */
        void refreshSenders() {
            if (m_receiveThread) {
                // 触发发送器列表更新
                m_receiveThread->requestSenderListUpdate();
                qDebug() << "SpoutOutDataModel: Requested sender list update";
            }
        }

    private:
        // 界面组件
        SpoutOutInterface *m_widget;
        
        // Spout接收线程
        SpoutSendThread *m_receiveThread;
        
        // 输出数据
        std::shared_ptr<ImageData> m_outputImageData;
        
        // 状态变量
        QString m_currentSender;
        bool m_isReceiving;
        
        /**
         * @brief 初始化接收器
         */
        void initializeReceiver() {
            qRegisterMetaType<cv::Mat>("cv::Mat");
            m_receiveThread = new SpoutSendThread(this);
            
            // 连接信号
            connect(m_receiveThread, &SpoutSendThread::frameReceived,
                    this, &SpoutOutDataModel::onFrameReceived, Qt::QueuedConnection);
            connect(m_receiveThread, &SpoutSendThread::connectionStatusChanged,
                    this, &SpoutOutDataModel::onConnectionStatusChanged, Qt::QueuedConnection);
            connect(m_receiveThread, &SpoutSendThread::senderListUpdated,
                    this, &SpoutOutDataModel::onSenderListUpdated, Qt::QueuedConnection);
                    
            qDebug() << "SpoutOutDataModel: Receiver initialized";
        }
    };
}
