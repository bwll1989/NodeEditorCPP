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

#include "NDIInInterface.hpp"
#include <iostream>
#include <vector>
#include <memory>
#include "ConstantDefines.h"
#include "OSCSender/OSCSender.h"
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
        /**
         * @brief 构造函数
         * @param parent 父对象
         */
        // 在NDIReceiveThread构造函数中添加
        explicit NDIReceiveThread(QObject* parent = nullptr)
        : QThread(parent)
        , m_running(false)
        , m_imageBuffer(nullptr)
        , m_width(0)
        , m_height(0)
        , m_forceUpdateSenders(false)
        , m_ndi_find(nullptr)  // 新增
        , m_ndi_recv(nullptr)  // 新增
        , m_p_sources(nullptr) // 新增
        , m_no_sources(0)      // 新增
        , m_ndi_initialized(false) // 新增
        {
            initializeNDI();

        }

        /**
         * @brief 析构函数
         */
        ~NDIReceiveThread() override {
            stopReceiving();
            wait();
            cleanupNDIReceiver();
        }

        /**
         * @brief 开始接收NDI数据
         * @param senderName 发送器名称，为空则连接到活动发送器
         */
        void startReceiving(const QString& senderName = "") {
            QMutexLocker locker(&m_mutex);
            m_senderName = senderName;
            m_running = true;
            if (!isRunning()) {
                start();
                emit connectionStatusChanged(m_running);
            }
        }

        /**
         * @brief 停止接收NDI数据
         */
        void stopReceiving() {
            QMutexLocker locker(&m_mutex);
            m_running = false;
            emit connectionStatusChanged(m_running);
        }

        /**
         * @brief 设置发送器名称
         * @param senderName 发送器名称
         */
        void setSenderName(const QString& senderName) {
            QMutexLocker locker(&m_mutex);
            m_senderName = senderName;
            
            // 连接到新的发送器
            if (!senderName.isEmpty()) {
                locker.unlock();
                connectToSender(senderName);
            }
        }

        /**
         * @brief 手动触发发送器列表更新
         */
        void requestSenderListUpdate() {
            QMutexLocker locker(&m_mutex);
            m_forceUpdateSenders = true;
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

        // 创建NDI查找实例
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

        qDebug() << "NDI初始化成功";
    }

    /**
     * @brief 接收NDI帧数据
     * @return 是否成功接收到帧
     */
    bool receiveFrame() {
        if (!m_ndi_recv) {
            return false;
        }

        // 创建视频帧结构
        NDIlib_video_frame_v2_t video_frame;

        // 尝试接收视频帧（超时100ms）
        switch (NDIlib_recv_capture_v2(m_ndi_recv, &video_frame, nullptr, nullptr, 100)) {
            case NDIlib_frame_type_video:
            {
                // 成功接收到视频帧
                if (video_frame.p_data && video_frame.xres > 0 && video_frame.yres > 0) {
                    // 转换NDI帧到OpenCV Mat
                    cv::Mat frame = convertNDIFrameToMat(video_frame);

                    if (!frame.empty()) {
                        emit frameReceived(frame);
                    }

                    // 释放视频帧
                    NDIlib_recv_free_video_v2(m_ndi_recv, &video_frame);
                    return true;
                }

                // 释放视频帧
                NDIlib_recv_free_video_v2(m_ndi_recv, &video_frame);
                break;
            }
            case NDIlib_frame_type_none:
                // 没有帧可用，这是正常的
                break;
            default:
                // 其他类型的帧或错误
                break;
        }

        return false;
    }

    /**
     * @brief 更新NDI发送器列表
     */
    void updateSenderList() {
        if (!m_ndi_find) {
            return;
        }

        // 等待发送器列表更新
        NDIlib_find_wait_for_sources(m_ndi_find, 1000);

        // 获取当前可用的发送器
        m_p_sources = NDIlib_find_get_current_sources(m_ndi_find, &m_no_sources);

        QStringList senderNames;
        for (uint32_t i = 0; i < m_no_sources; i++) {
            QString senderName = QString::fromUtf8(m_p_sources[i].p_ndi_name);
            senderNames.append(senderName);
        }

        emit senderListUpdated(senderNames);
        qDebug() << "发现" << m_no_sources << "个NDI发送器";
    }

    /**
     * @brief 清理NDI接收器
     */
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

        qDebug() << "NDI资源已清理";
    }

    /**
     * @brief 将NDI帧转换为OpenCV Mat
     * @param video_frame NDI视频帧
     * @return OpenCV Mat对象
     */
    cv::Mat convertNDIFrameToMat(const NDIlib_video_frame_v2_t& video_frame) {
        cv::Mat frame;

        // 根据NDI帧格式进行转换
        switch (video_frame.FourCC) {
            case NDIlib_FourCC_type_BGRA:
            {
                // BGRA格式，直接创建Mat
                frame = cv::Mat(video_frame.yres, video_frame.xres, CV_8UC4,
                               (void*)video_frame.p_data, video_frame.line_stride_in_bytes);
                // 转换为BGR格式（去除Alpha通道）
                cv::cvtColor(frame, frame, cv::COLOR_BGRA2BGR);
                break;
            }
            case NDIlib_FourCC_type_BGRX:
            {
                // BGRX格式
                frame = cv::Mat(video_frame.yres, video_frame.xres, CV_8UC4,
                               (void*)video_frame.p_data, video_frame.line_stride_in_bytes);
                // 转换为BGR格式
                cv::cvtColor(frame, frame, cv::COLOR_BGRA2BGR);
                break;
            }
            case NDIlib_FourCC_type_RGBA:
            {
                // RGBA格式
                frame = cv::Mat(video_frame.yres, video_frame.xres, CV_8UC4,
                               (void*)video_frame.p_data, video_frame.line_stride_in_bytes);
                // 转换为BGR格式
                cv::cvtColor(frame, frame, cv::COLOR_RGBA2BGR);
                break;
            }
            case NDIlib_FourCC_type_RGBX:
            {
                // RGBX格式
                frame = cv::Mat(video_frame.yres, video_frame.xres, CV_8UC4,
                               (void*)video_frame.p_data, video_frame.line_stride_in_bytes);
                // 转换为BGR格式
                cv::cvtColor(frame, frame, cv::COLOR_RGBA2BGR);
                break;
            }
            case NDIlib_FourCC_type_UYVY:
            {
                // UYVY格式（YUV422）
                frame = cv::Mat(video_frame.yres, video_frame.xres, CV_8UC2,
                               (void*)video_frame.p_data, video_frame.line_stride_in_bytes);
                // 转换为BGR格式
                cv::cvtColor(frame, frame, cv::COLOR_YUV2BGR_UYVY);
                break;
            }
            default:
                qDebug() << "不支持的NDI帧格式:" << video_frame.FourCC;
                break;
        }

        // 克隆数据以避免内存问题
        if (!frame.empty()) {
            frame = frame.clone();
        }

        return frame;
    }

    /**
     * @brief 连接到指定的NDI发送器
     * @param senderName 发送器名称
     * @return 是否连接成功
     */
    bool connectToSender(const QString& senderName) {
        if (!m_ndi_find || senderName.isEmpty()) {
            return false;
        }

        // 查找指定的发送器
        const NDIlib_source_t* target_source = nullptr;
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

        // 清理之前的接收器
        if (m_ndi_recv) {
            NDIlib_recv_destroy(m_ndi_recv);
            m_ndi_recv = nullptr;
        }

        // 创建新的接收器
        NDIlib_recv_create_v3_t recv_create;
        recv_create.source_to_connect_to = *target_source;
        recv_create.color_format = NDIlib_recv_color_format_BGRX_BGRA;
        recv_create.bandwidth = NDIlib_recv_bandwidth_highest;
        recv_create.allow_video_fields = false;
        recv_create.p_ndi_recv_name = "NodeEditor NDI Receiver";

        m_ndi_recv = NDIlib_recv_create_v3(&recv_create);

        if (m_ndi_recv) {
            qDebug() << "成功连接到NDI发送器:" << senderName;
            return true;
        } else {
            qDebug() << "连接NDI发送器失败:" << senderName;
            return false;
        }
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
            while (m_running) {
                QMutexLocker locker(&m_mutex);
                
                // 检查是否需要更新发送器列表
                if (m_forceUpdateSenders) {
                    updateSenderList();
                    m_forceUpdateSenders = false;
                }
                
                // 如果有接收器，尝试接收帧
                if (m_ndi_recv) {
                    locker.unlock();
                    receiveFrame();
                } else {
                    locker.unlock();
                    // 没有连接时短暂休眠
                    msleep(50);
                }
            }
        }

    // 在NDIReceiveThread类中完善以下方法实现
    
    private:
    // NDI相关变量
    NDIlib_find_instance_t m_ndi_find;
    NDIlib_recv_instance_t m_ndi_recv;
    const NDIlib_source_t* m_p_sources;
    uint32_t m_no_sources;
    bool m_ndi_initialized;
    bool m_running;
    bool m_forceUpdateSenders;
    QString m_senderName;
    QMutex m_mutex;
    // 图像缓冲区
    unsigned char* m_imageBuffer;
    unsigned int m_width;
    unsigned int m_height;
    };

    /**
     * @brief NDI输入节点数据模型
     * 
     * 提供NDI图像接收功能，支持多发送器选择和实时图像传输
     */
    class NDIInDataModel : public NodeDelegateModel
    {
        Q_OBJECT

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
            NodeDelegateModel::registerOSCControl("/enable",m_widget->m_startStopButton);
            NodeDelegateModel::registerOSCControl("/refresh",m_widget->m_refreshButton);
            NodeDelegateModel::registerOSCControl("/source",m_widget->m_senderComboBox);
            initializeReceiver();
            // 连接信号
            connect(m_widget, &NDIInInterface::startReceiving, this, &NDIInDataModel::startReceiving);
            connect(m_widget, &NDIInInterface::stopReceiving, this, &NDIInDataModel::stopReceiving);
            connect(m_widget, &NDIInInterface::senderSelected, this, &NDIInDataModel::selectSender);
            connect(m_widget, &NDIInInterface::refreshRequested, this, &NDIInDataModel::refreshSenders);
        }

        /**
         * @brief 析构函数
         */
        ~NDIInDataModel() override {
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
                return VariableData().type();
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
            switch (portIndex) {
                case 0: {
                    auto Data = std::dynamic_pointer_cast<VariableData>(data);
                    if (Data && m_receiveThread) {
                        // 发送图像数据
                        m_widget->m_senderComboBox->setCurrentText(Data->value().toString());
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

            // 然后恢复接收状态
            if (jsonObject.contains("isReceiving") && jsonObject["isReceiving"].toBool()) {

                m_widget->m_startStopButton->setChecked(true);

            }
            if (jsonObject.contains("currentSender")) {
                QString senderName = jsonObject["currentSender"].toString();
                m_currentSender = senderName;
                // 确保界面已创建
                if (m_widget) {
                    // 直接设置文本，无论发送器是否在列表中

                    m_widget->m_senderComboBox->setCurrentText(senderName);
                     // 延迟1秒执行refreshSenders函数
                    QTimer::singleShot(1000, this, [this]() {
                        refreshSenders();
                    });

                }
            }
        }
        
        /**
         * @brief 获取嵌入式控件
         * @return 控件指针
         */
        QWidget *embeddedWidget() override {

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
         * @brief 开始接收NDI数据
         */
        void startReceiving() {
            if (!m_isReceiving && m_receiveThread) {

                refreshSenders();
                m_receiveThread->startReceiving(m_currentSender);
                m_isReceiving = true;


            }
        }
        
        /**
         * @brief 停止接收NDI数据
         */
        void stopReceiving() {
            if (m_isReceiving && m_receiveThread) {
                m_receiveThread->stopReceiving();
                m_isReceiving = false;
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

        }
        
        /**
         * @brief 刷新发送器列表
         */
        void refreshSenders() {
            if (m_receiveThread) {
                // 触发发送器列表更新
                m_receiveThread->requestSenderListUpdate();
            }
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
                    
            qDebug() << "NDIInDataModel: Receiver initialized";
        }
    };
}
