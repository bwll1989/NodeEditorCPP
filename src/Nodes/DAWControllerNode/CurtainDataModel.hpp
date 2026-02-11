#pragma once

#include <QtCore/QObject>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QByteArray>
#include <QtCore/QDataStream>
#include <QtCore/QDateTime>
#include <QtCore/QTimer>

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include "CurtainInterface.hpp"
#include <iostream>
#include <qcryptographichash.h>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include "QGridLayout"
#include <QtCore/qglobal.h>
#include <QThread>
#include "Common/Devices/TcpClient/TcpClient.h"
#include "QMutex"
#include "PluginDefinition.hpp"
#include  "Common/BuildInNodes/AbstractDelegateModel.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;

using namespace NodeDataTypes;
using namespace QtNodes;
namespace Nodes
{
    /**
     * @brief 电动幕帘节点数据模型
     * 
     * 实现幕帘协议：
     * - 接收Open/Close/Reset指令
     * - 构造C5 xx CMD 5C格式指令
     * - 解析反馈状态
     * - 每3秒发送JSON心跳包 "HeartBeat"
     */
    class CurtainDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(QString host READ getHost WRITE setHost NOTIFY hostChanged)
        Q_PROPERTY(int port READ getPort WRITE setPort NOTIFY portChanged)

    public:
        /**
         * @brief 构造函数，初始化幕帘节点
         */
        CurtainDataModel(){
            // 设置端口数量和基本属性
            InPortCount = 3;  // Open, Close, Reset
            OutPortCount = 1; // 输出端口：接收到的信号（解析后）
            CaptionVisible = true;
            PortEditable = false;
            Caption = "Curtain Controller";
            WidgetEmbeddable = false;
            Resizable = false;
            
            // 初始化数据
            m_inData = std::make_shared<VariableData>();
            m_connectionStatus = std::make_shared<VariableData>();
            
            // 初始化心跳定时器
            m_heartbeatTimer = new QTimer(this);
            m_heartbeatTimer->setInterval(3000); // 3秒间隔
            connect(m_heartbeatTimer, &QTimer::timeout, this, &CurtainDataModel::sendHeartbeat);

            // 注册外部控制
            AbstractDelegateModel::registerExternalControl("/host", widget->hostEdit);
            AbstractDelegateModel::registerExternalControl("/port", widget->portSpinBox);
            AbstractDelegateModel::registerExternalControl("/open", widget->openButton);
            AbstractDelegateModel::registerExternalControl("/close", widget->closeButton);
            AbstractDelegateModel::registerExternalControl("/reset", widget->resetButton);
            AbstractDelegateModel::registerExternalControl("/status", widget->connectionStatusLabel);

            // 同步UI初始值
            m_host = widget->hostEdit->text();
            m_port = widget->portSpinBox->value();

            if (m_host.isEmpty()) {
                m_host = "192.168.0.10";
                widget->hostEdit->setText(m_host);
            }

            // 连接信号和槽
            connect(this, &CurtainDataModel::connectTCPServer, client, &TcpClient::connectToServer, Qt::QueuedConnection);
            connect(client, &TcpClient::isReady, this, &CurtainDataModel::onConnectionStatusChanged, Qt::QueuedConnection);
            connect(client, &TcpClient::recMsg, this, &CurtainDataModel::recMsg, Qt::QueuedConnection);
            
            // UI -> Property connections
            connect(widget->hostEdit, &QLineEdit::editingFinished, this, [this](){
                setHost(widget->hostEdit->text());
            });
            connect(widget->portSpinBox, &IntDragValueWidget::valueChanged, this, &CurtainDataModel::setPort);


            connect(this, &CurtainDataModel::stopTCPClient, client, &TcpClient::stopTimer, Qt::QueuedConnection);
            
            connect(widget->openButton, &QPushButton::clicked, this, [this]() {
                sendCurtainCommand("01");
            });
            connect(widget->closeButton, &QPushButton::clicked, this, [this]() {
                sendCurtainCommand("02");
            });
            connect(widget->resetButton, &QPushButton::clicked, this, [this]() {
                sendCurtainCommand("FF");
            });

            // 自动连接
            hostChange();
        }
        
        /**
         * @brief 析构函数，清理资源
         */
        ~CurtainDataModel(){
            // 停止心跳定时器
            if (m_heartbeatTimer) {
                m_heartbeatTimer->stop();
            }
            
            client->disconnectFromServer();
            delete client;
            client=nullptr;
        }

        void afterModelReady() override
        {
            AbstractDelegateModel::afterModelReady();
            auto bus = GlobalEventBus::instance();
            
            // Properties
            bus->subscribe(makeFullOscAddress("/host"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/port"), this, SLOT(onGlobalEvent(GlobalEvent)));
            
            // Commands
            bus->subscribe(makeFullOscAddress("/open"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/close"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/reset"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

    public:
        /**
         * @brief 获取端口标题
         * @param portType 端口类型
         * @param portIndex 端口索引
         * @return 端口标题
         */
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                switch (portIndex) {
                case 0:
                    return "OPEN";
                case 1:
                    return "CLOSE";
                case 2:
                    return "RESET";
                default:
                    break;
                }
            case PortType::Out:
                switch (portIndex)
                {
                case 0:
                    return "RUNNING";
                default:
                    break;
                }
            default:
                break;
            }
            return "";
        }

        /**
         * @brief 获取端口数据类型
         */
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            return VariableData().type();
        }

        /**
         * @brief 获取输出数据
         */
        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {

            return std::make_shared<VariableData>(m_isRunning);
        }

        /**
         * @brief 设置输入数据
         */
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (!data) return;
            auto varData = std::dynamic_pointer_cast<VariableData>(data);
            if (varData && varData->value().toBool()) {
                switch(portIndex) {
                    case 0: // Open
                        sendCurtainCommand("01");
                        break;
                    case 1: // Close
                        sendCurtainCommand("02");
                        break;
                    case 2: // Reset
                        sendCurtainCommand("FF");
                        break;
                }
            }
        }

        QWidget *embeddedWidget() override { return widget; }

        QJsonObject save() const override
        {
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["host"] = m_host;
            modelJson["port"] = m_port;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            if (p.contains("host")) {
                m_host = p["host"].toString();
                widget->hostEdit->setText(m_host);
            }
            if (p.contains("port")) {
                m_port = p["port"].toInt();
                widget->portSpinBox->setValue(m_port);
            }
            hostChange();
        }

        // 属性访问器
        QString getHost() const { return m_host; }
        void setHost(const QString &host) {
            if (m_host == host) return;
            m_host = host;
            widget->hostEdit->setText(m_host);
            emit hostChanged();
            hostChange();
        }

        int getPort() const { return m_port; }
        void setPort(int port) {
            if (m_port == port) return;
            m_port = port;
            widget->portSpinBox->setValue(m_port);
            emit portChanged();
            hostChange();
        }

    signals:
        void hostChanged();
        void portChanged();
        void connectTCPServer(QString host, int port);
        void stopTCPClient();

    public slots:
        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind == GlobalEventKind::Command) {
                QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
                if (localPath == "open") sendCurtainCommand("01");
                else if (localPath == "close") sendCurtainCommand("02");
                else if (localPath == "reset") sendCurtainCommand("FF");
                else if (localPath == "host") setHost(ev.payload.toString());
                else if (localPath == "port") setPort(ev.payload.toInt());
            }
        }

        void onConnectionStatusChanged(const bool &isReady)
        {
            m_connectionStatus = std::make_shared<VariableData>(isReady);
            widget->updateConnectionStatus(isReady);
            stateFeedBack("/status", isReady);
            
            if (isReady) {
                m_heartbeatTimer->start();
            } else {
                m_heartbeatTimer->stop();
            }
        }

        void recMsg(const QVariantMap &Msg)
        {
            QByteArray data = Msg.value("default").toByteArray();
            // 解析返回的数据
            // 协议格式：C5 [Address] [Mode] [ExecStatus] [Fault] ... 5C
            if (data.size() >= 5 && (unsigned char)data.at(0) == 0xC6) {
                // Byte 4 (Index 3): ExecStatus (11: Running/Triggered)
                unsigned char execStatus = (unsigned char)data.at(3);
                if (execStatus == 0x11) {
                    m_isRunning = true;
                    Q_EMIT dataUpdated(0);
                }

                // Debug log
                // QString hex = data.toHex().toUpper();
                // std::cout << "Curtain Recv: " << hex.toStdString() << " Running: " << running << std::endl;
            }
        }

        void sendHeartbeat()
        {
            if (client) {
                // 发送JSON格式的心跳包，使用格式1 (UTF-8) 发送纯文本
                QByteArray heartbeat = "HeartBeat";
                client->sendMessage(heartbeat, 1);
            }
        }

        void hostChange() {
            emit stopTCPClient();
            QThread::msleep(20);
            if (!m_host.isEmpty() && m_port > 0) {
                emit connectTCPServer(m_host, m_port);
            }
        }

        /**
         * @brief 发送幕帘控制指令
         * @param cmd 控制码字符串 (e.g., "01", "02", "FF")
         */
        void sendCurtainCommand(QString cmd)
        {
            if (!client) return;

            // 获取IP地址最后一段
            QStringList ipParts = m_host.split('.');
            if (ipParts.size() != 4) return;
            
            bool ok;
            int lastOctet = ipParts.last().toInt(&ok);
            if (!ok) return;

            // 构造指令：C5 + 地址(Hex) + 控制码 + 5C
            QString addressHex = QString("%1").arg(lastOctet, 2, 16, QChar('0')).toUpper();
            QString command = "C5" + addressHex + cmd + "5C";
            
            // 发送HEX字符串
            // 例如 IP=...6, cmd="01" -> "C506015C"
            client->sendMessage(command, 0);
        }

    private:
        CurtainInterface *widget = new CurtainInterface();
        QString m_host;
        int m_port = 10001;
        std::shared_ptr<VariableData> m_inData;
        std::shared_ptr<VariableData> m_connectionStatus;
        TcpClient *client = new TcpClient();
        QTimer *m_heartbeatTimer;
        bool m_isRunning = false;
        qint64 m_lastCommandTime = 0;
    };
}
