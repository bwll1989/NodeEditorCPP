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

#include <Processing.NDI.Lib.h>

// 再次确保没有 min/max 宏定义
#ifdef min
#undef min
#endif
#ifdef max
#undef max
#endif

// 现在安全地包含 OpenCV
#include <opencv2/opencv.hpp>
#include "ConstantDefines.h"
#include "OSCSender/OSCSender.h"
#include "NDIOutInterface.hpp"
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
     * @brief NDI发送线程类
     * 
     * 在独立线程中处理NDI图像发送，避免阻塞主线程
     * 使用NDILibrary API提供稳定的发送功能
     */
    class NDISendThread : public QThread
    {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数
         * @param parent 父对象
         */
        explicit NDISendThread(QObject* parent = nullptr)
            : QThread(parent)
            , m_running(false)
            , m_ndi_send(nullptr)
            , m_ndi_initialized(false)
            , m_senderName("NodeEditor NDI Output")
            , m_frameRate(30.0)
        {
            initializeNDI();
        }

        /**
         * @brief 析构函数
         */
        ~NDISendThread() override {
            stopSending();
            wait();
            cleanupNDISender();
        }

         /**
         * @brief 开始发送NDI数据
         * @param senderName 发送器名称
         * @param width 视频宽度
         * @param height 视频高度
         * @param frameRate 帧率
         */
        void startSending(const QString& senderName = "", int width = 1920, int height = 1080, double frameRate = 30.0) {
            QMutexLocker locker(&m_mutex);
            
            // 如果NDI未初始化，重新初始化
            if (!m_ndi_initialized) {
                initializeNDI();
            }
            
            if (!m_running && m_ndi_initialized) {
                if (!senderName.isEmpty()) {
                    m_senderName = senderName;
                }
                if (createNDISender()) {
                    m_running = true;
                    start();
                    emit sendingStatusChanged(true);
                    qDebug() << "NDI发送器启动成功:" << m_senderName;
                } else {
                    emit errorOccurred("创建NDI发送器失败");
                }
            }
        }

        /**
         * @brief 停止发送NDI数据
         */
        void stopSending() {
            QMutexLocker locker(&m_mutex);
            if (m_running) {
            
                m_running = false;
                locker.unlock();
                
                // 等待线程结束
                if (isRunning()) {
                    wait(3000); // 最多等待3秒
                    if (isRunning()) {
                        terminate(); // 强制终止
                        wait(1000);
                    }
                }
                
                // 清理NDI发送器
                cleanupNDISender();
                
                // 发送状态变化信号
                emit sendingStatusChanged(false);
                qDebug() << "NDI发送器已停止";
            }
        }

        /**
         * @brief 设置发送器名称
         * @param senderName 发送器名称
         */
        void setSenderName(const QString& senderName) {
            QMutexLocker locker(&m_mutex);
            
            if (m_senderName == senderName) {
                return; // 名称相同，无需更改
            }
            
            bool wasRunning = m_running;
            QString oldName = m_senderName;
            m_senderName = senderName;
            
            // 如果正在运行，需要重新创建发送器
            if (wasRunning) {
                locker.unlock();

                
                // 停止当前发送
                stopSending();
                // 重新开始发送
                QMutexLocker newLocker(&m_mutex);
                if (createNDISender()) {
                    m_running = true;
                    newLocker.unlock();
                    start();
                    emit sendingStatusChanged(true);
                } else {
                    emit errorOccurred("重新创建NDI发送器失败");
                }
            }
        }

        /**
         * @brief 发送图像帧
         * @param frame OpenCV图像帧
         */
        void sendFrame(const cv::Mat& frame) {
            if (!frame.empty() && m_ndi_send && m_running) {
                QMutexLocker locker(&m_frameMutex);
                m_currentFrame = frame.clone();
                m_hasNewFrame = true;
            }
        }

    signals:
        void sendingStatusChanged(bool sending);
        void errorOccurred(const QString& error);

    protected:
        /**
         * @brief 线程运行函数
         */
        void run() override {
            while (m_running) {
                QMutexLocker frameLocker(&m_frameMutex);
                
                if (m_hasNewFrame && !m_currentFrame.empty()) {
                    cv::Mat frameToSend = m_currentFrame.clone();
                    m_hasNewFrame = false;
                    frameLocker.unlock();
                    
                    // 发送帧
                    sendNDIFrame(frameToSend);
                } else {
                    frameLocker.unlock();
                    // 没有新帧时短暂休眠
                    msleep(16); // 约60fps
                }
            }
        }
/**
         * @brief 初始化NDI库
         */
        void initializeNDI() {
            // 初始化NDI库
            if (!NDIlib_initialize()) {
                qDebug() << "NDI初始化失败";
                return;
            }

            m_ndi_initialized = true;
            qDebug() << "NDI初始化成功";
        }

        /**
         * @brief 创建NDI发送器
         * @return 是否创建成功
         */
        bool createNDISender() {
            if (m_ndi_send) {
                NDIlib_send_destroy(m_ndi_send);
                m_ndi_send = nullptr;
            }

            // 创建NDI发送器
            NDIlib_send_create_t send_create;
            send_create.p_ndi_name = m_senderName.toUtf8().constData();
            send_create.p_groups = nullptr;
            send_create.clock_video = true;
            send_create.clock_audio = false;

            m_ndi_send = NDIlib_send_create(&send_create);

            if (m_ndi_send) {
                qDebug() << "NDI发送器创建成功:" << m_senderName;
                return true;
            } else {
                qDebug() << "NDI发送器创建失败:" << m_senderName;
                return false;
            }
        }

        /**
         * @brief 发送NDI帧
         * @param frame OpenCV图像帧
         */
        void sendNDIFrame(const cv::Mat& frame) {
            if (!m_ndi_send || frame.empty()) {
                return;
            }

            // 转换OpenCV Mat到NDI格式
            cv::Mat ndiFrame;
            convertMatToNDIFrame(frame, ndiFrame);

            if (ndiFrame.empty()) {
                return;
            }

            // 创建NDI视频帧
            NDIlib_video_frame_v2_t video_frame;
            video_frame.xres = ndiFrame.cols;
            video_frame.yres = ndiFrame.rows;
            video_frame.FourCC = NDIlib_FourCC_type_BGRX;
            video_frame.frame_rate_N = static_cast<int>(m_frameRate * 1000);
            video_frame.frame_rate_D = 1000;
            video_frame.picture_aspect_ratio = static_cast<float>(ndiFrame.cols) / static_cast<float>(ndiFrame.rows);
            video_frame.frame_format_type = NDIlib_frame_format_type_progressive;
            video_frame.timecode = NDIlib_send_timecode_synthesize;
            video_frame.p_data = ndiFrame.data;
            video_frame.line_stride_in_bytes = ndiFrame.step[0];
            video_frame.p_metadata = nullptr;

            // 发送视频帧
            NDIlib_send_send_video_v2(m_ndi_send, &video_frame);
        }

        /**
         * @brief 将OpenCV Mat转换为NDI格式
         * @param src 源图像
         * @param dst 目标图像
         */
        void convertMatToNDIFrame(const cv::Mat& src, cv::Mat& dst) {
            if (src.empty()) {
                return;
            }

            // 根据源图像格式进行转换
            switch (src.channels()) {
                case 1:
                    // 灰度图转BGRX
                    cv::cvtColor(src, dst, cv::COLOR_GRAY2BGRA);
                    break;
                case 3:
                    // BGR转BGRX
                    cv::cvtColor(src, dst, cv::COLOR_BGR2BGRA);
                    break;
                case 4:
                    // 已经是4通道，检查是否需要重排列
                    if (src.type() == CV_8UC4) {
                        dst = src.clone();
                    } else {
                        src.convertTo(dst, CV_8UC4);
                    }
                    break;
                default:
                    qDebug() << "不支持的图像格式，通道数:" << src.channels();
                    return;
            }

            // 确保数据连续
            if (!dst.isContinuous()) {
                dst = dst.clone();
            }
        }

        /**
         * @brief 清理NDI发送器
         */
        void cleanupNDISender() {
            if (m_ndi_send) {
                NDIlib_send_destroy(m_ndi_send);
                m_ndi_send = nullptr;
            }

            if (m_ndi_initialized) {
                NDIlib_destroy();
                m_ndi_initialized = false;
            }

            qDebug() << "NDI发送器资源已清理";
        }
    private:
        // NDI相关变量
        NDIlib_send_instance_t m_ndi_send;
        bool m_ndi_initialized;
        bool m_running;
        QString m_senderName;
        double m_frameRate;
        // 线程同步
        QMutex m_mutex;
        QMutex m_frameMutex;
        
        // 帧数据
        cv::Mat m_currentFrame;
        bool m_hasNewFrame = false;
    };

    /**
     * @brief NDI输出节点数据模型类
     * 
     * 接收ImageData输入，通过NDI协议发送到网络
     */
    class NDIOutDataModel : public NodeDelegateModel
    {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数
         */
        NDIOutDataModel()
            : m_widget(new NDIOutInterface())
            , m_sendThread(nullptr)
            , m_isSending(false)
        {
            InPortCount = 2;
            OutPortCount = 0;  // NDI输出不需要输出端口
            CaptionVisible = true;
            Caption = "NDI Out";
            WidgetEmbeddable = true;
            Resizable = false;
            PortEditable = false;

            initializeSender();
            NodeDelegateModel::registerOSCControl("/enable",m_widget->m_startStopButton);

        }

        /**
         * @brief 析构函数
         */
        ~NDIOutDataModel() override {
            if (m_sendThread) {
                m_sendThread->stopSending();
                m_sendThread->wait();
                delete m_sendThread;
            }
            if (m_widget) {
                delete m_widget;
            }
        }

        /**
         * @brief 获取端口数据类型
         * @param portType 端口类型
         * @param portIndex 端口索引
         * @return 数据类型
         */
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override {
            if (portType == PortType::In && portIndex == 0) {
                return ImageData().type();
            }
            return VariableData().type();
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
         * @brief 获取输出数据（NDI输出节点无输出）
         * @param port 端口索引
         * @return 空指针
         */
        std::shared_ptr<NodeData> outData(PortIndex const port) override {
            return nullptr;
        }

        /**
         * @brief 设置输入数据
         * @param data 输入数据
         * @param portIndex 端口索引
         */
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override {
            switch (portIndex) {
                case 0: {
                    auto imageData = std::dynamic_pointer_cast<ImageData>(data);
                    if (imageData && m_sendThread) {
                        // 发送图像数据
                        m_sendThread->sendFrame(imageData->imgMat());
                        }
                    }
                    break;
                case 1: {
                    auto Data = std::dynamic_pointer_cast<VariableData>(data);
                    if (Data)
                        m_widget->m_startStopButton->setChecked(Data->value().toBool());

                }
                    break;
                    default: {break;}
            }
        }

        /**
         * @brief 保存节点状态
         * @return JSON对象
         */
        QJsonObject save() const override {
            QJsonObject obj;
            obj["senderName"] = m_currentSenderName;
            obj["isSending"] = m_isSending;
            return obj;
        }

        /**
         * @brief 加载节点状态
         * @param jsonObject JSON对象
         */
        void load(QJsonObject const &jsonObject) override {
            if (jsonObject.contains("senderName")) {
                m_currentSenderName = jsonObject["senderName"].toString();
                m_widget->m_senderNameEdit->setText(m_currentSenderName);
                if (m_sendThread) {
                    m_sendThread->setSenderName(m_currentSenderName);
                }

            }
            if (jsonObject.contains("isSending")) {
                m_widget->m_startStopButton->setChecked(jsonObject["isSending"].toBool());
            }

        }
        
        /**
         * @brief 获取嵌入式控件
         * @return 控件指针
         */
        QWidget *embeddedWidget() override {

            // 连接信号
            connect(m_widget, &NDIOutInterface::startSending, this, &NDIOutDataModel::startSending);
            connect(m_widget, &NDIOutInterface::stopSending, this, &NDIOutDataModel::stopSending);
            connect(m_widget, &NDIOutInterface::senderNameChanged, this, &NDIOutDataModel::setSenderName);

            return m_widget;
        }

    public slots:
        /**
         * @brief 处理发送状态变化
         * @param sending 发送状态
         */
        void onSendingStatusChanged(bool sending) {
            m_isSending = sending;
            if (m_widget) {
                m_widget->updateSendingStatus(sending);
            }
        }
        
        /**
         * @brief 处理错误
         * @param error 错误信息
         */
        void onErrorOccurred(const QString& error) {
            if (m_widget) {
                m_widget->showError(error);
                m_isSending = false;
                m_widget->m_startStopButton->setChecked(false);

            }
        }
        
        /**
         * @brief 开始发送NDI数据
         */
        void startSending() {
            if (!m_isSending && m_sendThread) {
                m_sendThread->startSending(m_currentSenderName);
                m_isSending = true;
                qDebug() << "NDI输出开始发送:" << m_currentSenderName;
            }
        }
        
        /**
         * @brief 停止发送NDI数据
         */
        void stopSending() {
            if (m_isSending && m_sendThread) {
                m_sendThread->stopSending();
                // 等待线程完全停止
                if (m_sendThread->isRunning()) {
                    m_sendThread->wait(3000);
                }
                m_isSending = false;
                
                // 更新界面状态
                if (m_widget) {
                    m_widget->updateSendingStatus(false);
                }
                
                qDebug() << "NDI输出停止发送";
            }
        }
        
        /**
         * @brief 设置发送器名称
         * @param senderName 发送器名称
         */
        void setSenderName(const QString& senderName) {
            if (m_currentSenderName == senderName) {
                return; // 名称相同，无需更改
            }
            
            m_currentSenderName = senderName;
            
            if (m_sendThread) {
                // 如果正在发送，线程会自动处理重新创建
                m_sendThread->setSenderName(senderName);
            }
        }
        /**
         * @brief 初始化发送器
         */
        void initializeSender() {
            m_sendThread = new NDISendThread(this);
            m_currentSenderName = "NodeEditor NDI Output";

            // 连接信号
            connect(m_sendThread, &NDISendThread::sendingStatusChanged,
                    this, &NDIOutDataModel::onSendingStatusChanged, Qt::QueuedConnection);
            connect(m_sendThread, &NDISendThread::errorOccurred,
                    this, &NDIOutDataModel::onErrorOccurred, Qt::QueuedConnection);

        }

        void stateFeedBack(const QString& oscAddress,QVariant value) override {

            OSCMessage message;
            message.host = AppConstants::EXTRA_FEEDBACK_HOST;
            message.port = AppConstants::EXTRA_FEEDBACK_PORT;
            message.address = "/dataflow/" + getParentAlias() + "/" + QString::number(getNodeID()) + oscAddress;
            message.value = value;
            OSCSender::instance()->sendOSCMessageWithQueue(message);
        }
    private:
        // 界面组件
        NDIOutInterface *m_widget;
        
        // NDI发送线程
        NDISendThread *m_sendThread;
        
        // 状态变量
        QString m_currentSenderName;
        bool m_isSending;
        

    };
}
