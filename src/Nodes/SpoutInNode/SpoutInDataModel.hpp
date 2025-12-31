#pragma once


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
#include "SpoutReceiver/SpoutReceiver.h"
// 使用 SpoutLibrary API 而不是 SpoutReceiver
#include "SpoutLibrary.h"

// 现在安全地包含 OpenCV
#include <opencv2/opencv.hpp>

#include "SpoutInInterface.hpp"
#include <iostream>
#include <vector>
#include <memory>
#include "Common/BuildInNodes/AbstractDelegateModel.h"
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
    /**
     * @brief Spout输入节点数据模型
     * 
     * 提供Spout图像接收功能，支持多发送器选择和实时图像传输
     */
    class SpoutInDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        /**
         * @brief 构造函数
         */
        SpoutInDataModel()
            : m_receiveThread(nullptr)
            , m_isReceiving(false)
        {
            InPortCount = 2;  // Spout输入不需要输入端口
            OutPortCount = 1;
            CaptionVisible = true;
            Caption = "Spout In";
            WidgetEmbeddable = true;
            Resizable = false;
            PortEditable = false;
            initializeReceiver();
            AbstractDelegateModel::registerOSCControl("/source",m_widget->m_senderComboBox);
            AbstractDelegateModel::registerOSCControl("/enable",m_widget->m_startStopButton);
        }

        /**
         * @brief 析构函数
         */
        ~SpoutInDataModel() override {
            stopReceiving();
            if (m_receiveThread) {
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
                m_widget->m_senderComboBox->setCurrentText(m_currentSender);
            }
            if (jsonObject.contains("isReceiving") && jsonObject["isReceiving"].toBool()) {
                // 延迟启动，确保界面已初始化
                QTimer::singleShot(1000, [this]() {
                    startReceiving();
                });
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
         * @brief 开始接收Spout数据
         */
        void startReceiving() {

            if (m_receiveThread && !m_isReceiving) {
              
                m_receiveThread->start(m_currentSender);
                m_isReceiving = true;

                emit onConnectionStatusChanged(true); // 通知 UI
            }
           
        }
        
        /**
         * @brief 停止接收Spout数据
         */
        void stopReceiving() {
            if (m_receiveThread && m_isReceiving) {
                m_receiveThread->stop();
                m_isReceiving = false;
                emit onConnectionStatusChanged(false); // 通知 UI
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
                if (!m_isReceiving) {
                    startReceiving();
                }
            }
        }
        
        /**
         * @brief 刷新发送器列表
         */
        void refreshSenders() {
            // 使用 SpoutReceiver 提供的静态方法直接在主线程查询
            QStringList senders = SpoutReceiver::getSenderList();
            m_widget->updateSenderList(senders);

        }
        /**
                 * @brief 初始化接收器
                 */
        void initializeReceiver() {
            qRegisterMetaType<cv::Mat>("cv::Mat");
            m_receiveThread = new SpoutReceiver(this);
            // 连接信号
            connect(m_receiveThread, &SpoutReceiver::frameReceived,
                    this, &SpoutInDataModel::onFrameReceived, Qt::QueuedConnection);
            connect(m_receiveThread, &SpoutReceiver::connectionStatusChanged,
                    this, &SpoutInDataModel::onConnectionStatusChanged, Qt::QueuedConnection);
            connect(m_widget, &SpoutInInterface::startReceiving, this, &SpoutInDataModel::startReceiving);
            connect(m_widget, &SpoutInInterface::stopReceiving, this, &SpoutInDataModel::stopReceiving);
            connect(m_widget, &SpoutInInterface::senderSelected, this, &SpoutInDataModel::selectSender);
            connect(m_widget, &SpoutInInterface::refreshRequested, this, &SpoutInDataModel::refreshSenders);

        }
    private:
        // 界面组件
        SpoutInInterface *m_widget=new SpoutInInterface();
        
        // Spout接收线程
        SpoutReceiver *m_receiveThread;
        
        // 输出数据
        std::shared_ptr<ImageData> m_outputImageData;
        
        // 状态变量
        QString m_currentSender;
        bool m_isReceiving;



    };
}
