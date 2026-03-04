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
#include "Common/BuildInNodes/AbstractDelegateModel.h"
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

#include "NDIInInterface.hpp"
#include <iostream>
#include <vector>
#include <memory>
#include "OSCSender/OSCSender.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include <QSignalBlocker>

using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
    /**
     * @brief NDI接收线程类
     * 
     * 在独立线程中处理NDI图像接收，避免阻塞主线程
     * 使用NDILibrary API提供更稳定的接收功能
     */
    class NDIReceiveThread : public QThread
    {
        Q_OBJECT

    public:
        explicit NDIReceiveThread(QObject* parent = nullptr)
        : QThread(parent)
        , m_isCapturing(false)
        , m_updatePending(false)
        , m_connectionPending(false)
        , m_ndi_find(nullptr)
        , m_ndi_recv(nullptr)
        , m_p_sources(nullptr)
        , m_no_sources(0)
        , m_ndi_initialized(false)
        {
        }

        ~NDIReceiveThread() override {
            stopThread();
            // Allow thread to exit gracefully
            if (!wait(200)) {
                 terminate();
                 wait();
            }
        }

        void stopThread() {
            requestInterruption();
        }

        void startCapturing(const QString& senderName = "") {
            QMutexLocker locker(&m_mutex);
            if (!senderName.isEmpty()) {
                m_pendingSenderName = senderName;
                m_connectionPending = true;
            }
            m_isCapturing = true;
            emit connectionStatusChanged(true);
        }

        void stopCapturing() {
            QMutexLocker locker(&m_mutex);
            m_isCapturing = false;
            emit connectionStatusChanged(false);
        }

        void setSenderName(const QString& senderName) {
            QMutexLocker locker(&m_mutex);
            m_pendingSenderName = senderName;
            m_connectionPending = true;
        }

        void requestSenderListUpdate() {
            QMutexLocker locker(&m_mutex);
            m_updatePending = true;
        }

        void initializeNDI() {
            if (!NDIlib_initialize()) {
                qDebug() << "NDI初始化失败";
                return;
            }

            NDIlib_find_create_t find_create;
            find_create.show_local_sources = true;
            find_create.p_groups = nullptr;
            find_create.p_extra_ips = nullptr;

            m_ndi_find = NDIlib_find_create_v2(&find_create);
            if (!m_ndi_find) {
                qDebug() << "NDI查找实例创建失败";
                NDIlib_destroy();
                return;
            }

            m_ndi_recv = nullptr;
            m_p_sources = nullptr;
            m_no_sources = 0;
            m_ndi_initialized = true;
        }

        bool receiveFrame() {
            if (!m_ndi_recv) {
                return false;
            }

            NDIlib_video_frame_v2_t video_frame;
            switch (NDIlib_recv_capture_v2(m_ndi_recv, &video_frame, nullptr, nullptr, 50)) {
                case NDIlib_frame_type_video:
                {
                    if (video_frame.p_data && video_frame.xres > 0 && video_frame.yres > 0) {
                        cv::Mat frame = convertNDIFrameToMat(video_frame);
                        if (!frame.empty()) {
                            emit frameReceived(frame);
                        }
                        NDIlib_recv_free_video_v2(m_ndi_recv, &video_frame);
                        return true;
                    }
                    NDIlib_recv_free_video_v2(m_ndi_recv, &video_frame);
                    break;
                }
                default:
                    break;
            }
            return false;
        }

        void updateSenderList() {
            if (!m_ndi_find) return;

            // Wait up to 100ms, but this runs in worker thread without lock
            // Reduce wait time to prevent long blocking during destruction
            NDIlib_find_wait_for_sources(m_ndi_find, 100);

            m_p_sources = NDIlib_find_get_current_sources(m_ndi_find, &m_no_sources);

            QStringList senderNames;
            for (uint32_t i = 0; i < m_no_sources; i++) {
                QString senderName = QString::fromUtf8(m_p_sources[i].p_ndi_name);
                senderNames.append(senderName);
            }

            emit senderListUpdated(senderNames);
        }

        void cleanupNDIReceiver() {
            if (m_ndi_recv) {
                NDIlib_recv_destroy(m_ndi_recv);
                m_ndi_recv = nullptr;
            }
            if (m_ndi_find) {
                NDIlib_find_destroy(m_ndi_find);
                m_ndi_find = nullptr;
            }
            if (m_ndi_initialized) {
                NDIlib_destroy();
                m_ndi_initialized = false;
            }
        }

        cv::Mat convertNDIFrameToMat(const NDIlib_video_frame_v2_t& video_frame) {
            cv::Mat frame;
            switch (video_frame.FourCC) {
                case NDIlib_FourCC_type_BGRA:
                    frame = cv::Mat(video_frame.yres, video_frame.xres, CV_8UC4, (void*)video_frame.p_data, video_frame.line_stride_in_bytes);
                    // cv::cvtColor(frame, frame, cv::COLOR_BGRA2BGR);
                    break;
                case NDIlib_FourCC_type_BGRX:
                    frame = cv::Mat(video_frame.yres, video_frame.xres, CV_8UC4, (void*)video_frame.p_data, video_frame.line_stride_in_bytes);
                    // cv::cvtColor(frame, frame, cv::COLOR_BGRA2BGR);
                    break;
                case NDIlib_FourCC_type_RGBA:
                    frame = cv::Mat(video_frame.yres, video_frame.xres, CV_8UC4, (void*)video_frame.p_data, video_frame.line_stride_in_bytes);
                    cv::cvtColor(frame, frame, cv::COLOR_RGBA2BGRA);
                    break;
                case NDIlib_FourCC_type_RGBX:
                    frame = cv::Mat(video_frame.yres, video_frame.xres, CV_8UC4, (void*)video_frame.p_data, video_frame.line_stride_in_bytes);
                    cv::cvtColor(frame, frame, cv::COLOR_RGBA2BGRA);
                    break;
                case NDIlib_FourCC_type_UYVY:
                    frame = cv::Mat(video_frame.yres, video_frame.xres, CV_8UC2, (void*)video_frame.p_data, video_frame.line_stride_in_bytes);
                    cv::cvtColor(frame, frame, cv::COLOR_YUV2BGRA_UYVY);
                    break;
                default:
                    break;
            }
            // Deep copy is REQUIRED because NDI owns the buffer and we free it immediately after this.
            if (!frame.empty()) frame = frame.clone();
            return frame;
        }

        bool connectToSender(const QString& senderName) {
            if (!m_ndi_find || senderName.isEmpty()) return false;

            const NDIlib_source_t* target_source = nullptr;
            // First try to find in current sources
            for (uint32_t i = 0; i < m_no_sources; i++) {
                QString currentName = QString::fromUtf8(m_p_sources[i].p_ndi_name);
                if (currentName == senderName) {
                    target_source = &m_p_sources[i];
                    break;
                }
            }
            
            if (!target_source) {
                qDebug() << "未找到指定的NDI发送器:" << senderName;
                return false;
            }

            if (m_ndi_recv) {
                NDIlib_recv_destroy(m_ndi_recv);
                m_ndi_recv = nullptr;
            }

            NDIlib_recv_create_v3_t recv_create;
            recv_create.source_to_connect_to = *target_source;
            recv_create.color_format = NDIlib_recv_color_format_BGRX_BGRA;
            recv_create.bandwidth = NDIlib_recv_bandwidth_highest;
            recv_create.allow_video_fields = false;
            recv_create.p_ndi_recv_name = "NodeEditor NDI Receiver";

            m_ndi_recv = NDIlib_recv_create_v3(&recv_create);

            bool success = (m_ndi_recv != nullptr);
            if (success) {
                 qDebug() << "成功连接到NDI发送器:" << senderName;
            } else {
                 qDebug() << "连接NDI发送器失败:" << senderName;
            }
            return success;
        }

    signals:
        void frameReceived(const cv::Mat& frame);
        void connectionStatusChanged(bool connected);
        void senderListUpdated(const QStringList& senders);

    protected:
        void run() override {
            initializeNDI();

            while (!isInterruptionRequested()) {
                bool doUpdate = false;
                bool doConnect = false;
                bool doCapture = false;
                QString newSender;

                {
                    QMutexLocker locker(&m_mutex);
                    if (m_updatePending) {
                        doUpdate = true;
                        m_updatePending = false;
                    }
                    if (m_connectionPending) {
                        doConnect = true;
                        m_connectionPending = false;
                        newSender = m_pendingSenderName;
                    }
                    doCapture = m_isCapturing;
                }

                if (doUpdate) {
                    updateSenderList();
                }

                if (doConnect) {
                    connectToSender(newSender);
                }

                if (doCapture) {
                    receiveFrame();
                } else {
                    msleep(10);
                }
                
                if (!doUpdate && !doConnect && !doCapture) {
                    msleep(50);
                }
            }
            
            cleanupNDIReceiver();
        }
    
    private:
        NDIlib_find_instance_t m_ndi_find;
        NDIlib_recv_instance_t m_ndi_recv;
        const NDIlib_source_t* m_p_sources;
        uint32_t m_no_sources;
        bool m_ndi_initialized;
        
        std::atomic<bool> m_isCapturing;
        bool m_updatePending;
        bool m_connectionPending;
        QString m_pendingSenderName;
        
        QMutex m_mutex;
    };

    /**
     * @brief NDI输入节点数据模型
     * 
     * 提供NDI图像接收功能，支持多发送器选择和实时图像传输
     */
    class NDIInDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(QString sourceName READ getSourceName WRITE setSourceName NOTIFY sourceNameChanged)
        Q_PROPERTY(bool enable READ getEnable WRITE setEnable NOTIFY enableChanged)

    public:
        /**
         * @brief 构造函数
         */
        NDIInDataModel()
            : m_widget(new NDIInInterface())
            , m_receiveThread(nullptr)
            , m_isReceiving(false)
        {
            InPortCount = 2;
            OutPortCount = 1;
            CaptionVisible = true;
            Caption = "NDI In";
            WidgetEmbeddable = true;
            Resizable = false;
            PortEditable = false;
            
            AbstractDelegateModel::registerExternalControl("/enable", m_widget->m_startStopButton);
            AbstractDelegateModel::registerExternalControl("/refresh", m_widget->m_refreshButton);
            AbstractDelegateModel::registerExternalControl("/source", m_widget->m_senderComboBox);
            
            initializeReceiver();
            
            // 连接信号到属性Setter
            connect(m_widget, &NDIInInterface::startReceiving, this, [this](){ setEnable(true); });
            connect(m_widget, &NDIInInterface::stopReceiving, this, [this](){ setEnable(false); });
            connect(m_widget, &NDIInInterface::senderSelected, this, &NDIInDataModel::setSourceName);
            connect(m_widget, &NDIInInterface::refreshRequested, this, &NDIInDataModel::refreshSenders);
        }

        /**
         * @brief 析构函数
         */
        ~NDIInDataModel() override {
            setEnable(false); // 停止接收
            if (m_receiveThread) {
                m_receiveThread->stopThread();
                if (!m_receiveThread->wait(2000)) {
                    m_receiveThread->terminate();
                    m_receiveThread->wait();
                }
                delete m_receiveThread;
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
            case PortType::Out:
                return ImageData().type();
            default:
                return VariableData().type();
            }
        }

        /**
         * @brief 获取输出数据
         */
        std::shared_ptr<NodeData> outData(PortIndex const port) override {
            Q_UNUSED(port);
            return m_outputImageData ? m_outputImageData : std::make_shared<ImageData>();
        }

        /**
         * @brief 设置输入数据
         */
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override {
            switch (portIndex) {
                case 0: {
                    auto Data = std::dynamic_pointer_cast<VariableData>(data);
                    if (Data && m_receiveThread) {
                        setSourceName(Data->value().toString());
                    }
                }
                    break;
                case 1: {
                    auto Data = std::dynamic_pointer_cast<VariableData>(data);
                    if (Data)
                        setEnable(Data->value().toBool());
                }
                    break;
                default: {break;}
            }
        }

        /**
         * @brief 保存节点状态
         */
        QJsonObject save() const override {
            QJsonObject modelJson = NodeDelegateModel::save();
            QJsonObject values;
            values["currentSender"] = m_currentSender;
            values["isReceiving"] = m_isReceiving;
            modelJson["values"] = values;
            return modelJson;
        }

        /**
         * @brief 加载节点状态
         */
        void load(QJsonObject const &jsonObject) override {
            QJsonValue v = jsonObject["values"];
            if (!v.isUndefined() && v.isObject()) {
                QJsonObject values = v.toObject();
                if (values.contains("currentSender")) {
                    QString senderName = values["currentSender"].toString();
                    setSourceName(senderName);
                }
                if (values.contains("isReceiving")) {
                    setEnable(values["isReceiving"].toBool());
                }
            } else {
                // 兼容旧的保存格式
                if (jsonObject.contains("currentSender")) {
                    setSourceName(jsonObject["currentSender"].toString());
                }
                if (jsonObject.contains("isReceiving")) {
                    setEnable(jsonObject["isReceiving"].toBool());
                }
            }
        }
        
        /**
         * @brief 获取嵌入式控件
         */
        QWidget *embeddedWidget() override {
            return m_widget;
        }

        void afterModelReady() override
        {
            AbstractDelegateModel::afterModelReady();
            auto bus = GlobalEventBus::instance();
            
            bus->subscribe(makeFullOscAddress("/source"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/enable"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/refresh"), this, SLOT(onGlobalEvent(GlobalEvent)));

        }

    public slots:
        void onGlobalEvent(const GlobalEvent& ev)
        {
            QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
            if (localPath == "source") {
                setSourceName(ev.payload.toString());
            } else if (localPath == "enable") {
                setEnable(ev.payload.toBool());
            } else if (localPath == "refresh") {
                refreshSenders();
            }
        }

        /**
         * @brief 处理接收到的帧数据
         */
        void onFrameReceived(const cv::Mat& frame) {
            if (!frame.empty()) {
                m_outputImageData = std::make_shared<ImageData>(frame);
                emit dataUpdated(0);
            }
        }
        
        /**
         * @brief 处理连接状态变化
         */
        void onConnectionStatusChanged(bool connected) {
            if (m_widget) {
                m_widget->updateConnectionStatus(connected);
            }
        }
        
        /**
         * @brief 处理发送器列表更新
         */
        void onSenderListUpdated(const QStringList& senders) {
            if (m_widget) {
                m_widget->updateSenderList(senders);
            }
        }
        
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case QtNodes::PortType::In:
                switch (portIndex)
                {
                    case 0:
                            return "SOURCE";
                    case 1:
                            return "ENABLE";
                }
            case QtNodes::PortType::Out:
                return "IMAGE";
            default:
                break;
            }
            return "";
        }
        
        /**
         * @brief 刷新发送器列表
         */
        void refreshSenders() {
            if (m_receiveThread) {
                // 触发发送器列表更新
                m_receiveThread->requestSenderListUpdate();
                AbstractDelegateModel::stateFeedBack("/refresh", true);
            }
        }

    public:
        // Property Accessors
        QString getSourceName() const { return m_currentSender; }
        void setSourceName(const QString& value) {
            if (m_currentSender == value) return;
            m_currentSender = value;
            
            if (m_widget && m_widget->m_senderComboBox->currentText() != value) {
                QSignalBlocker blocker(m_widget->m_senderComboBox);
                m_widget->m_senderComboBox->setCurrentText(value);
            }
            
            // Logic to select sender
            if (m_receiveThread) {
                m_receiveThread->setSenderName(value);
            }
            
            emit sourceNameChanged(value);
            AbstractDelegateModel::stateFeedBack("/source", value);
        }

        bool getEnable() const { return m_isReceiving; }
        void setEnable(bool value) {
            if (m_isReceiving == value) return;
            
            if (value) {
                // Start receiving logic
                if (m_receiveThread) {
                    m_receiveThread->startCapturing(m_currentSender);
                }
            } else {
                // Stop receiving logic
                if (m_receiveThread) {
                    m_receiveThread->stopCapturing();
                }
            }
            
            m_isReceiving = value;
            
            if (m_widget && m_widget->m_startStopButton->isChecked() != value) {
                QSignalBlocker blocker(m_widget->m_startStopButton);
                m_widget->m_startStopButton->setChecked(value);
            }

            emit enableChanged(value);
            AbstractDelegateModel::stateFeedBack("/enable", value);
        }

    signals:
        void sourceNameChanged(QString value);
        void enableChanged(bool value);

    private:
        // 界面组件
        NDIInInterface *m_widget;
        
        // NDI接收线程
        NDIReceiveThread *m_receiveThread;
        
        // 输出数据
        std::shared_ptr<ImageData> m_outputImageData;
        
        // 状态变量
        QString m_currentSender;
        bool m_isReceiving;
        
        /**
         * @brief 初始化接收器
         */
        void initializeReceiver() {
            m_receiveThread = new NDIReceiveThread(this);

            // 连接信号
            connect(m_receiveThread, &NDIReceiveThread::frameReceived,
                    this, &NDIInDataModel::onFrameReceived, Qt::QueuedConnection);
            connect(m_receiveThread, &NDIReceiveThread::connectionStatusChanged,
                    this, &NDIInDataModel::onConnectionStatusChanged, Qt::QueuedConnection);
            connect(m_receiveThread, &NDIReceiveThread::senderListUpdated,
                    this, &NDIInDataModel::onSenderListUpdated, Qt::QueuedConnection);

            // Use HighestPriority instead of TimeCriticalPriority to prevent UI starvation
            m_receiveThread->start(QThread::HighestPriority); 
            qDebug() << "NDIInDataModel: Receiver initialized and thread started with HighestPriority";
        }
    };
}
