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
#include <QDebug>
#include "Common/BuildInNodes/AbstractDelegateModel.h"
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
            , m_running(false)
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
            wait();
            // m_surface 在主线程创建，应在主线程销毁
            if (m_surface) {
                m_surface->destroy();
                delete m_surface;
                m_surface = nullptr;
            }
            
            // 线程结束前清理 OpenGL 上下文
            if (m_context) {
                // 注意：这里可能需要在 run() 结束前清理，或者确保 context 没有被 current
                delete m_context;
                m_context = nullptr;
            }
        }

        /**
         * @brief 开始发送Spout数据
         */
        void startSending() {
            QMutexLocker locker(&m_mutex);
            m_running = true;
            if (!isRunning()) {
                start();
            }
        }

        /**
         * @brief 停止发送Spout数据
         */
        void stopSending() {
            QMutexLocker locker(&m_mutex);
            m_running = false;
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
            // 创建并设置 OpenGL 上下文 (在工作线程中)
            m_context = new QOpenGLContext();
            m_context->setFormat(m_surface->format());
            if (!m_context->create()) {
                qDebug() << "SpoutSenderThread: Failed to create OpenGL context";
                return;
            }

            // 使用主线程创建的 surface
            if (!m_context->makeCurrent(m_surface)) {
                qDebug() << "SpoutSenderThread: Failed to make OpenGL context current";
                return;
            }

            // 初始化 Spout
            m_spout = GetSpout();
            if (!m_spout) {
                qDebug() << "SpoutSenderThread: Failed to create Spout instance";
                return;
            }

            // 初始设置发送器名称
            {
                QMutexLocker locker(&m_mutex);
                if (!m_senderName.isEmpty()) {
                    m_spout->SetSenderName(m_senderName.toLocal8Bit().data());
                } else {
                    m_spout->SetSenderName("NodeEditor Spout");
                }
            }
            
            emit connectionStatusChanged(true);

            cv::Mat currentFrame;
            
            while (m_running) {
                // 检查名称变更
                {
                    QMutexLocker locker(&m_mutex);
                    if (m_nameChanged) {
                        m_spout->ReleaseSender();
                        m_spout->SetSenderName(m_senderName.toLocal8Bit().data());
                        m_nameChanged = false;
                        qDebug() << "SpoutSenderThread: Sender name changed to" << m_senderName;
                    }
                    
                    if (m_newFrameAvailable) {
                        m_nextFrame.copyTo(currentFrame);
                        m_newFrameAvailable = false;
                    }
                }

                if (!currentFrame.empty()) {
                    // 发送图像
                    // SendImage(pixels, width, height, glFormat, bInvert)
                    // 使用 GL_BGRA 因为我们已经转换过了
                    bool success = m_spout->SendImage(
                        currentFrame.data,
                        currentFrame.cols,
                        currentFrame.rows,
                        GL_BGRA,
                        false // bInvert (OpenCV 通常是 top-down, OpenGL 纹理通常是 bottom-up, Spout 可能处理这个? 通常不需要反转如果 Spout 内部处理了)
                        // Spout documentation says: bInvert - Flip the image vertically.
                        // OpenCV is Top-Left origin. OpenGL is Bottom-Left origin.
                        // If we send as is, it might be upside down in Spout Receiver.
                        // Let's try false first (default), change to true if needed.
                        // Usually Spout SendImage handles texture upload which flips it effectively for GL.
                    );
                    
                    if (!success) {
                        // qDebug() << "SpoutSenderThread: SendImage failed";
                    }
                }

                // 保持 ~60fps
                msleep(16);
            }
            
            // 清理
            if (m_spout) {
                m_spout->ReleaseSender();
                m_spout->Release();
                m_spout = nullptr;
            }
            
            emit connectionStatusChanged(false);
            
            if (m_context) {
                m_context->doneCurrent();
            }
        }

    private:
        bool m_running;
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
            if (m_sendThread) {
                m_sendThread->stopSending();
                m_sendThread->wait();
                delete m_sendThread;
            }
            if (m_widget) {
                m_widget->deleteLater();
            }
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
                if (!m_isSending || !m_sendThread) return;
                auto imageData = std::dynamic_pointer_cast<ImageData>(data);
                if (imageData && !imageData->isEmpty()) {
                    m_sendThread->updateFrame(imageData->imgMat());
                }
                return;
            }
            case 1: {
                bool enabled = false;
                if (data) {
                    auto variableData = std::dynamic_pointer_cast<VariableData>(data);
                    if (variableData) {
                        enabled = variableData->value().toBool();
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
                
                // 连接信号
                connect(m_widget, &SpoutOutInterface::startSending,
                        this, &SpoutOutDataModel::onStartSending);
                connect(m_widget, &SpoutOutInterface::stopSending,
                        this, &SpoutOutDataModel::onStopSending);
                connect(m_widget, &SpoutOutInterface::senderNameChanged,
                        this, &SpoutOutDataModel::onSenderNameChanged);
                
                // 初始状态
                if (m_sendThread) {
                     // 如果需要同步状态
                }
            }
            return m_widget;
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
        SpoutOutInterface* m_widget;
        SpoutSenderThread* m_sendThread;
        bool m_isSending;
        QString m_senderName;

        /**
         * @brief 设置发送启停状态（UI/端口统一入口）
         * @param enabled true 开始发送，false 停止发送
         */
        void setSendingEnabled(bool enabled) {
            if (m_isSending == enabled) {
                if (m_widget) {
                    m_widget->updateConnectionStatus(m_isSending);
                }
                return;
            }

            m_isSending = enabled;
            if (m_sendThread) {
                if (enabled) {
                    m_sendThread->startSending();
                } else {
                    m_sendThread->stopSending();
                }
            }
            if (m_widget) {
                m_widget->updateConnectionStatus(enabled);
            }
        }

        void initializeSender() {
            m_sendThread = new SpoutSenderThread(this);
            connect(m_sendThread, &SpoutSenderThread::connectionStatusChanged,
                    this, &SpoutOutDataModel::onThreadStatusChanged);
            
            // 默认名称
            m_senderName = "NodeEditor Spout";
            m_sendThread->setSenderName(m_senderName);
        }
    };
}
