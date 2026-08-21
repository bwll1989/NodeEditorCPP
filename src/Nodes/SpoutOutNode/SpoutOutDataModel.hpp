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

#include "NodeDataList.hpp"
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
#include <QDebug>
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/ImageGpuUpload.h"
#include "Common/DataTypes/ImageReadback.h"
#include "TimestampGenerator/TimestampGenerator.hpp"
// 使用 SpoutLibrary API
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
#include <QPointer>
#include <atomic>
#include <iostream>
#include <vector>
#include <memory>

using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace
{
/** 从 ImageData ring buffer 取最新 Mat（须在 GUI 线程） */
inline cv::Mat readLatestMatForSend(const std::shared_ptr<ImageData>& input)
{
    ImageFrame frame;
    if (!input || !getLatestImageFrame(input, frame) || frame.empty()) {
        return {};
    }
    return ImageReadback::matFromFrame(frame);
}
} // namespace

namespace Nodes
{
namespace
{
/** SetSenderName 需要字节数组在调用期间保持有效 */
void setSpoutSenderName(SPOUTHANDLE spout, const QString& senderName)
{
    if (!spout) {
        return;
    }
    const QByteArray nameBytes = senderName.isEmpty()
                                     ? QByteArray("NodeEditor Spout")
                                     : senderName.toLocal8Bit();
    spout->SetSenderName(nameBytes.constData());
}

void destroyGlContextInWorkerThread(QOpenGLContext*& context)
{
    if (!context) {
        return;
    }
    context->doneCurrent();
    delete context;
    context = nullptr;
}
} // namespace

    /**
     * @brief Spout发送线程类
     * 
     * 在独立线程中处理Spout图像发送，避免阻塞主线程
     * 维护独立的 OpenGL 上下文以供 Spout 使用
     */
    class SpoutSenderThread : public QThread
    {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数
         * @param parent 父对象
         */
        explicit SpoutSenderThread(QObject* parent = nullptr)
            : QThread(parent)
            , m_spout(nullptr)
            , m_context(nullptr)
            , m_surface(nullptr)
            , m_nameChanged(false)
            , m_newFrameAvailable(false)
        {
            // 在主线程中创建 OffscreenSurface
            QSurfaceFormat format = QSurfaceFormat::defaultFormat();
            format.setProfile(QSurfaceFormat::CompatibilityProfile);
            
            m_surface = new QOffscreenSurface();
            m_surface->setFormat(format);
            m_surface->create();
        }

        /**
         * @brief 析构函数
         */
        ~SpoutSenderThread() override {
            stopSending();
            if (isRunning()) {
                wait(3000);
            }
            // m_surface 在主线程创建/销毁；m_context 仅在工作线程 run() 内创建/销毁
            if (m_surface) {
                m_surface->destroy();
                delete m_surface;
                m_surface = nullptr;
            }
        }

        /**
         * @brief 开始发送Spout数据
         */
        void startSending() {
            QMutexLocker locker(&m_mutex);
            m_running.store(true, std::memory_order_release);
            if (!isRunning()) {
                start();
            }
        }

        /**
         * @brief 停止发送Spout数据
         */
        void stopSending() {
            m_running.store(false, std::memory_order_release);
        }

        /**
         * @brief 设置发送器名称
         * @param senderName 发送器名称
         */
        void setSenderName(const QString& senderName) {
            QMutexLocker locker(&m_mutex);
            if (m_senderName != senderName) {
                m_senderName = senderName;
                m_nameChanged = true;
            }
        }

        /**
         * @brief 更新要发送的帧
         * @param frame OpenCV图像帧
         */
        void updateFrame(const cv::Mat& frame) {
            if (frame.empty()) return;
            
            QMutexLocker locker(&m_mutex);
            cv::Mat converted;
            if (frame.channels() == 3) {
                cv::cvtColor(frame, converted, cv::COLOR_BGR2BGRA);
            } else if (frame.channels() == 4) {
                if (frame.type() == CV_8UC4) {
                    converted = frame;
                } else {
                    frame.convertTo(converted, CV_8UC4);
                }
            } else if (frame.channels() == 1) {
                cv::cvtColor(frame, converted, cv::COLOR_GRAY2BGRA);
            } else {
                return;
            }

            m_nextFrame = converted.clone();
            m_newFrameAvailable = true;
        }

    signals:
        void connectionStatusChanged(bool sending);

    protected:
        /**
         * @brief 线程主循环
         */
        void run() override {
            m_context = new QOpenGLContext();
            m_context->setFormat(m_surface->format());
            if (!m_context->create()) {
                qDebug() << "SpoutSenderThread: Failed to create OpenGL context";
                delete m_context;
                m_context = nullptr;
                return;
            }

            if (!m_context->makeCurrent(m_surface)) {
                qDebug() << "SpoutSenderThread: Failed to make OpenGL context current";
                destroyGlContextInWorkerThread(m_context);
                return;
            }

            m_spout = GetSpout();
            if (!m_spout) {
                qDebug() << "SpoutSenderThread: Failed to create Spout instance";
                destroyGlContextInWorkerThread(m_context);
                return;
            }

            {
                QMutexLocker locker(&m_mutex);
                setSpoutSenderName(m_spout, m_senderName);
            }

            emit connectionStatusChanged(true);

            cv::Mat currentFrame;

            while (m_running.load(std::memory_order_acquire)) {
                {
                    QMutexLocker locker(&m_mutex);
                    if (m_nameChanged) {
                        m_spout->ReleaseSender();
                        setSpoutSenderName(m_spout, m_senderName);
                        m_nameChanged = false;
                        qDebug() << "SpoutSenderThread: Sender name changed to" << m_senderName;
                    }

                    if (m_newFrameAvailable) {
                        m_nextFrame.copyTo(currentFrame);
                        m_newFrameAvailable = false;
                    }
                }

                if (!currentFrame.empty()) {
                    m_spout->SendImage(
                        currentFrame.data,
                        currentFrame.cols,
                        currentFrame.rows,
                        GL_BGRA,
                        false);
                }

                msleep(16);
            }

            if (m_spout) {
                m_spout->ReleaseSender();
                m_spout->Release();
                m_spout = nullptr;
            }

            emit connectionStatusChanged(false);
            destroyGlContextInWorkerThread(m_context);
        }

    private:
        std::atomic<bool> m_running{false};
        QMutex m_mutex;
        
        QString m_senderName;
        bool m_nameChanged;
        
        cv::Mat m_nextFrame;
        bool m_newFrameAvailable;
        
        // SpoutLibrary 接口
        SPOUTHANDLE m_spout;
        
        // OpenGL 上下文
        QOpenGLContext* m_context;
        QOffscreenSurface* m_surface;
    };

    /**
     * @brief Spout输出节点数据模型
     * 
     * 将输入的图像数据通过 Spout 发送给其他应用程序
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
            , m_sendThread(nullptr)
            , m_isSending(false)
        {
            InPortCount = 2;
            OutPortCount = 0;
            CaptionVisible = true;
            Caption = "Spout Out";
            Resizable = false;
            PortEditable = false;
            
            initializeSender();
        }

        /**
         * @brief 析构函数
         */
        ~SpoutOutDataModel() override {
            m_shuttingDown.store(true);

            disconnect(TimestampGenerator::getInstance(), nullptr, this, nullptr);

            if (m_isSending) {
                m_isSending = false;
                if (m_sendThread) {
                    m_sendThread->stopSending();
                }
            }

            if (m_sendThread) {
                disconnect(m_sendThread, nullptr, this, nullptr);
                if (m_sendThread->isRunning()) {
                    m_sendThread->wait(3000);
                }
                delete m_sendThread;
                m_sendThread = nullptr;
            }

            m_inImage0.reset();
        }

        void afterModelReady() override {
            AbstractDelegateModel::afterModelReady();
            ImageGpuUpload::instance().warmup();

            connect(TimestampGenerator::getInstance(),
                    &TimestampGenerator::frameCountUpdated,
                    this,
                    [this](qint64) { trySendLatestFrame(); },
                    Qt::QueuedConnection);
        }

        /**
         * @brief 获取端口数据类型
         */
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override {
            Q_UNUSED(portIndex);
            switch (portType) {
            case PortType::In:
                if (portIndex==0)
                    return ImageData().type();
                else
                    return VariableData().type();
            default:
                return VariableData().type();
            }
        }

        /**
         * @brief 获取输出数据 (无输出)
         */
        std::shared_ptr<NodeData> outData(PortIndex const port) override {
            Q_UNUSED(port);
            return nullptr;
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {

            switch (portType) {
                case QtNodes::PortType::In:
                    switch (portIndex)
                    {
                    case 0:
                            return "IMAGE";
                    case 1:
                            return "ENABLE";
                    }
                case QtNodes::PortType::Out:
                    return "";
                default:
                    break;
            }
            return "";
        }
        /**
         * @brief 设置输入数据
         */
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override {
            switch (portIndex) {
            case 0: {
                m_inImage0 = std::dynamic_pointer_cast<ImageData>(data);
                m_lastSeenInputTimestamp = -1;
                return;
            }
            case 1: {
                bool enabled = false;
                if (data) {
                    auto variableData = std::dynamic_pointer_cast<VariableData>(data);
                    if (variableData) {
                        enabled = variableData->asBool();
                    }
                }
                setSendingEnabled(enabled);
                return;
            }
            default:
                return;
            }
        }

        /**
         * @brief 创建嵌入式控件
         */
        QWidget* embeddedWidget() override {
            if (!m_widget) {
                m_widget = new SpoutOutInterface();

                connect(m_widget, &SpoutOutInterface::startSending,
                        this, &SpoutOutDataModel::onStartSending);
                connect(m_widget, &SpoutOutInterface::stopSending,
                        this, &SpoutOutDataModel::onStopSending);
                connect(m_widget, &SpoutOutInterface::senderNameChanged,
                        this, &SpoutOutDataModel::onSenderNameChanged);
            }
            return m_widget.data();
        }

        /**
         * @brief 保存节点状态
         */
        QJsonObject save() const override {
            QJsonObject modelJson;
            modelJson["senderName"] = m_senderName;
            modelJson["isSending"] = m_isSending;
            return modelJson;
        }

        /**
         * @brief 加载节点状态
         */
        void load(QJsonObject const& p) override {
            QJsonValue v = p["senderName"];
            if (!v.isUndefined()) {
                m_senderName = v.toString();
                if (m_sendThread) {
                    m_sendThread->setSenderName(m_senderName);
                }
            }
            v = p["isSending"];
            if (!v.isUndefined()) {
                setSendingEnabled(v.toBool());
            }
        }

    private slots:
        void onStartSending() {
            setSendingEnabled(true);
        }

        void onStopSending() {
            setSendingEnabled(false);
        }

        void onSenderNameChanged(const QString& name) {
            m_senderName = name;
            if (m_sendThread) {
                m_sendThread->setSenderName(name);
            }
        }
        
        void onThreadStatusChanged(bool sending) {
             // 可以用来同步实际状态到UI
        }

    private:
        /**
         * @brief 设置发送启停状态（UI/端口统一入口）
         * @param enabled true 开始发送，false 停止发送
         */
        void setSendingEnabled(bool enabled) {
            if (m_shuttingDown.load()) {
                return;
            }

            if (m_isSending == enabled) {
                if (SpoutOutInterface* widget = m_widget.data()) {
                    widget->updateConnectionStatus(m_isSending);
                }
                return;
            }

            m_isSending = enabled;
            if (m_sendThread) {
                if (enabled) {
                    m_lastSeenInputTimestamp = -1;
                    m_sendThread->startSending();
                    trySendLatestFrame();
                } else {
                    m_sendThread->stopSending();
                }
            }
            if (SpoutOutInterface* widget = m_widget.data()) {
                widget->updateConnectionStatus(enabled);
            }
        }

        /** 系统 tick：有新输入帧且正在发送时，读 Mat 并交给 Spout 发送线程 */
        void trySendLatestFrame()
        {
            if (m_shuttingDown.load() || !m_isSending || !m_sendThread || !m_inImage0
                || imageDataIsEmpty(m_inImage0)) {
                return;
            }

            ImageFrame peek;
            if (!getLatestImageFrame(m_inImage0, peek) || peek.empty()) {
                return;
            }
            if (peek.timestamp >= 0 && peek.timestamp <= m_lastSeenInputTimestamp) {
                return;
            }

            cv::Mat mat = readLatestMatForSend(m_inImage0);
            if (mat.empty()) {
                return;
            }

            m_lastSeenInputTimestamp = peek.timestamp;
            m_sendThread->updateFrame(mat);
        }

        void initializeSender() {
            m_sendThread = new SpoutSenderThread(this);
            connect(m_sendThread, &SpoutSenderThread::connectionStatusChanged,
                    this, &SpoutOutDataModel::onThreadStatusChanged, Qt::QueuedConnection);

            m_senderName = "NodeEditor Spout";
            m_sendThread->setSenderName(m_senderName);
        }

        QPointer<SpoutOutInterface> m_widget;
        SpoutSenderThread* m_sendThread = nullptr;

        std::shared_ptr<ImageData> m_inImage0;
        qint64 m_lastSeenInputTimestamp = -1;

        bool m_isSending = false;
        QString m_senderName;
        std::atomic<bool> m_shuttingDown{false};
    };
}
