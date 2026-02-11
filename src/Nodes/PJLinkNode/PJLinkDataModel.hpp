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
#include "PJLinkInterface.hpp"
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
     * @brief PJLink触发事件收集系统节点数据模型
     * 
     * 实现PJLink协议解析，支持：
     * - 接收PJLink消息帧
     * - 解析消息头和JSON数据
     * - 提取SignalID和相关数据
     * - 以VariableData形式输出解析结果
     * - 每5秒发送心跳包维持连接
     */
    class PJLinkDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(QString host READ getHost WRITE setHost NOTIFY hostChanged)
        Q_PROPERTY(int port READ getPort WRITE setPort NOTIFY portChanged)
        Q_PROPERTY(QString password READ getPassword WRITE setPassword NOTIFY passwordChanged)
        Q_PROPERTY(QString customCommand READ getCustomCommand WRITE setCustomCommand NOTIFY customCommandChanged)

    public:
        /**
         * @brief 构造函数，初始化PJLink节点
         */
        PJLinkDataModel(){
            // 设置端口数量和基本属性
            InPortCount = 3;  //
            OutPortCount = 1; // 输出端口：SIGNAL_ID
            CaptionVisible = true;
            PortEditable = false;
            Caption = PLUGIN_NAME;
            WidgetEmbeddable = false;
            Resizable = false;
            
            // 初始化数据
            m_inData = std::make_shared<VariableData>();
            m_connectionStatus = std::make_shared<VariableData>();
            
            // 初始化心跳定时器
            m_heartbeatTimer = new QTimer(this);
            m_heartbeatTimer->setInterval(5000); // 5秒间隔
            connect(m_heartbeatTimer, &QTimer::timeout, this, &PJLinkDataModel::sendHeartbeat);
            NodeDelegateModel::registerExternalControl("/host", widget->hostEdit);
            NodeDelegateModel::registerExternalControl("/port", widget->portSpinBox);
            NodeDelegateModel::registerExternalControl("/password", widget->passwordLineEdit);
            NodeDelegateModel::registerExternalControl("/powerOn", widget->powerOnButton);
            NodeDelegateModel::registerExternalControl("/powerOff", widget->powerOffButton);
            NodeDelegateModel::registerExternalControl("/muteOn", widget->muteOnButton);
            NodeDelegateModel::registerExternalControl("/muteOff", widget->muteOffButton);
            NodeDelegateModel::registerExternalControl("/customCommand", widget->customCommandButton);
            NodeDelegateModel::registerExternalControl("/custom", widget->customCommandLineEdit);
            NodeDelegateModel::registerExternalControl("/status", widget->connectionStatusLabel);
            // 同步UI初始值
            m_host = widget->hostEdit->text();
            m_port = widget->portSpinBox->value();
            m_password = widget->passwordLineEdit->text();
            m_customPJLinkCommand = widget->customCommandLineEdit->text();
            if (m_host.isEmpty()) {
                m_host = "192.168.0.10";
                widget->hostEdit->setText(m_host);
            }

            // 连接信号和槽
            connect(this, &PJLinkDataModel::connectTCPServer, client, &TcpClient::connectToServer, Qt::QueuedConnection);
            connect(client, &TcpClient::isReady, this, &PJLinkDataModel::onConnectionStatusChanged, Qt::QueuedConnection);
            connect(client, &TcpClient::recMsg, this, &PJLinkDataModel::recMsg, Qt::QueuedConnection);
            
            // UI -> Property connections
            connect(widget->hostEdit, &QLineEdit::editingFinished, this, [this](){
                setHost(widget->hostEdit->text());
            });
            connect(widget->portSpinBox, &IntDragValueWidget::valueChanged, this, &PJLinkDataModel::setPort);
            connect(widget->passwordLineEdit, &QLineEdit::editingFinished, this, [this](){
                setPassword(widget->passwordLineEdit->text());
            });
            connect(widget->customCommandLineEdit, &QLineEdit::editingFinished, this, [this](){
                setCustomCommand(widget->customCommandLineEdit->text());
            });

            connect(this, &PJLinkDataModel::stopTCPClient, client, &TcpClient::stopTimer, Qt::QueuedConnection);
            
            connect(widget->powerOnButton, &QPushButton::clicked, this, [this]() {
                sendPJLinkCommand("POWR", "1");
            });
            connect(widget->powerOffButton, &QPushButton::clicked, this, [this]() {
                sendPJLinkCommand("POWR", "0");
            });
            connect(widget->muteOnButton, &QPushButton::clicked, this, [this]() {
                sendPJLinkCommand("AVMT", "31");
            });
            connect(widget->muteOffButton, &QPushButton::clicked, this, [this]() {
                sendPJLinkCommand("AVMT", "30");
            });
            connect(widget->customCommandButton, &QPushButton::clicked, this, [this]() {
                sendCustomPJLinkCommand(widget->customCommandLineEdit->text());
            });
            // 自动连接
            hostChange();
            // 初始化认证状态
            m_isAuthenticated = false;
            m_randomKey = "";
        }
        
        /**
         * @brief 析构函数，清理资源
         */
        ~PJLinkDataModel(){
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
            bus->subscribe(makeFullOscAddress("/password"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/customCommand"), this, SLOT(onGlobalEvent(GlobalEvent)));
            
            // Commands
            bus->subscribe(makeFullOscAddress("/powerOn"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/powerOff"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/muteOn"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/muteOff"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/custom"), this, SLOT(onGlobalEvent(GlobalEvent)));
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
                    return "POWER";
                case 1:
                    return "MUTE";
                case 2:
                        return "CUSTOM";
                default:
                    break;
                }
            case PortType::Out:
                switch (portIndex)
                {
                case 0:
                    return "STATUS";
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
         * @param portType 端口类型
         * @param portIndex 端口索引
         * @return 数据类型
         */
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            Q_UNUSED(portType)
            return VariableData().type();
        }

       /**
         * @brief 获取输出数据
         * @param portIndex 端口索引
         * @return 输出数据
         */
        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            switch (portIndex)
            {
            case 0:
                return m_connectionStatus;  // 连接状态
            default:
                return nullptr;
            }
        }

        /**
         * @brief 设置输入数据
         * @param data 输入数据
         * @param portIndex 端口索引
         */
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (data == nullptr) {
                return;
            }
            m_inData = std::dynamic_pointer_cast<VariableData>(data);
            switch (portIndex) {
            case 0: {
                if (m_inData&&m_inData->value().toBool())
                    sendPJLinkCommand("POWR", "1");
                else
                    sendPJLinkCommand("POWR", "0");
            }
                break;
            case 1:{
                if (m_inData&&m_inData->value().toBool())
                    sendPJLinkCommand("AVMT", "31");
                else
                    sendPJLinkCommand("AVMT", "30");
                }
            case 2: {
                if (m_inData&&m_inData->value().toBool())
                    sendCustomPJLinkCommand(widget->customCommandLineEdit->text());
            }
                break;
            default:
                break;
            }
        }

        /**
         * @brief 获取嵌入式控件
         * @return 控件指针
         */
        QWidget *embeddedWidget() override
        {
            return widget;
        }

        /**
     * @brief 保存节点配置
     * @return JSON对象
     */
    QJsonObject save() const override
    {
        QJsonObject modelJson1;
        modelJson1["IP"] = m_host;
        modelJson1["Port"] = m_port;
        modelJson1["Password"] = m_password;
        modelJson1["CustomCommand"] = m_customPJLinkCommand;
        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson["values"] = modelJson1;
        return modelJson;
    }
    
    /**
     * @brief 加载节点配置
     * @param p JSON对象
     */
    void load(const QJsonObject &p) override
    {
        QJsonValue v = p["values"];
        if (!v.isUndefined() && v.isObject()) {
            if (v["IP"].isString()) setHost(v["IP"].toString());
            if (v["Port"].isDouble()) setPort(v["Port"].toInt());
            if (v["Password"].isString()) setPassword(v["Password"].toString());
            if (v["CustomCommand"].isString()) setCustomCommand(v["CustomCommand"].toString());
        }
    }

    ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex index) const override {
        auto result = ConnectionPolicy::One;
        switch (portType) {
            case PortType::In:
                result = ConnectionPolicy::Many;
                break;
            case PortType::Out:
                result = ConnectionPolicy::Many;
                break;
            case PortType::None:
                break;
        }

        return result;
    }
    
    public: // Getters and Setters
        QString getHost() const { return m_host; }
        void setHost(const QString &value) {
            if (m_host == value) return;
            m_host = value;
            if (widget && widget->hostEdit->text() != value) {
                QSignalBlocker blocker(widget->hostEdit);
                widget->hostEdit->setText(value);
            }
            emit hostChanged(value);
            AbstractDelegateModel::stateFeedBack("/host", value);
            hostChange();
        }

        int getPort() const { return m_port; }
        void setPort(int value) {
            if (m_port == value) return;
            m_port = value;
            if (widget && widget->portSpinBox->value() != value) {
                QSignalBlocker blocker(widget->portSpinBox);
                widget->portSpinBox->setValue(value);
            }
            emit portChanged(value);
            AbstractDelegateModel::stateFeedBack("/port", value);
            hostChange();
        }

        QString getPassword() const { return m_password; }
        void setPassword(const QString &value) {
            if (m_password == value) return;
            m_password = value;
            if (widget && widget->passwordLineEdit->text() != value) {
                QSignalBlocker blocker(widget->passwordLineEdit);
                widget->passwordLineEdit->setText(value);
            }
            emit passwordChanged(value);
            AbstractDelegateModel::stateFeedBack("/password", value);
            hostChange();
        }

        QString getCustomCommand() const { return m_customPJLinkCommand; }
        void setCustomCommand(const QString &value) {
            if (m_customPJLinkCommand == value) return;
            m_customPJLinkCommand = value;
            if (widget && widget->customCommandLineEdit->text() != value) {
                QSignalBlocker blocker(widget->customCommandLineEdit);
                widget->customCommandLineEdit->setText(value);
            }
            emit customCommandChanged(value);
            AbstractDelegateModel::stateFeedBack("/customCommand", value);
        }

    public slots:
        /**
     * @brief 接收TCP消息并解析PJLink协议
     * @param msg 接收到的消息数据
     */
    void recMsg(const QVariantMap &msg)
    {
        if (msg.contains("default")) {
            QByteArray rawData = msg["default"].toByteArray();
            QString response = QString::fromUtf8(rawData).trimmed();
            parsePJLinkMessage(rawData);
            // 解析PJLink响应
            if (response.startsWith("PJLINK ")) {
                handlePJLinkResponse(response);
            }
        }
    }

        /**
         * @brief 连接状态改变处理
         * @param isReady 连接状态
         */
        void onConnectionStatusChanged(const bool &isReady)
        {
            m_connectionStatus = std::make_shared<VariableData>(isReady);
            widget->updateConnectionStatus(isReady);
            
            // 根据连接状态控制心跳定时器
            if (isReady) {
                m_heartbeatTimer->start();
                QtNodes::NodeValidationState state;
                setValidatonState(state);
            } else {
                QtNodes::NodeValidationState state;
                state._state = QtNodes::NodeValidationState::State::Error;
                state._stateMessage = QStringLiteral("连接错误");
                setValidatonState(state);
                m_heartbeatTimer->stop();
            }
            AbstractDelegateModel::stateFeedBack("/connect", isReady);
            Q_EMIT dataUpdated(0);
        }

        /**
         * @brief 主机或端口改变时重新连接
         */
        void hostChange()
        {
            m_isAuthenticated = false;
            m_randomKey.clear();
            m_authHash.clear();

            // 重新连接TCP服务器
            emit connectTCPServer(m_host, m_port);

        }

    private Q_SLOTS:
        void onGlobalEvent(const GlobalEvent& ev) {
            if (ev.kind != GlobalEventKind::Command) return;
            QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
            
            if (localPath == "host") {
                setHost(ev.payload.toString());
            } else if (localPath == "port") {
                setPort(ev.payload.toInt());
            } else if (localPath == "password") {
                setPassword(ev.payload.toString());
            } else if (localPath == "customCommand") {
                setCustomCommand(ev.payload.toString());
            } else if (localPath == "powerOn") {
                sendPJLinkCommand("POWR", "1");
            } else if (localPath == "powerOff") {
                sendPJLinkCommand("POWR", "0");
            } else if (localPath == "muteOn") {
                sendPJLinkCommand("AVMT", "31");
            } else if (localPath == "muteOff") {
                sendPJLinkCommand("AVMT", "30");
            } else if (localPath == "custom") {
                sendCustomPJLinkCommand(ev.payload.toString());
            }
        }

    signals:
        /**
         * @brief 停止TCP客户端信号
         */
        void stopTCPClient();
        
        /**
         * @brief 连接TCP服务器信号
         * @param host 主机地址
         * @param port 端口号
         */
        void connectTCPServer(const QString &host, int port);

        void hostChanged(const QString &host);
        void portChanged(int port);
        void passwordChanged(const QString &password);
        void customCommandChanged(const QString &value);

    private:
        
        /**
         * @brief 创建PJLink协议数据包
         * @param jsonData JSON数据
         * @return 完整的PJLink数据包
         */
        QByteArray createPJLinkPacket(const QByteArray &jsonData)
        {
            QByteArray packet;
            QDataStream stream(&packet, QIODevice::WriteOnly);
            stream.setByteOrder(QDataStream::LittleEndian);
            
            return packet;
        }

        /**
     * @brief 发送心跳包
     */
    void sendHeartbeat()
    {
        // 发送电源状态查询作为心跳
        if (m_isAuthenticated) {
            sendPJLinkCommand("POWR", "?");
        }
    }
/**
     * @brief 处理PJLink协议响应
     * @param response 服务器响应字符串
     */
    void handlePJLinkResponse(const QString &response)
    {
        QStringList parts = response.split(" ");
        if (parts.size() >= 3 && parts[0] == "PJLINK") {
            QString classVersion = parts[1];
            QString authInfo = parts[2];
            
            if (authInfo == "0") {
                // 无需认证
                m_isAuthenticated = true;
                qDebug() << "PJLink: 无需密码认证";
            } else if (authInfo.length() == 8) {
                // 需要密码认证
                m_randomKey = authInfo;
                performAuthentication();
            } else if (authInfo == "ERRA") {

                m_isAuthenticated = false;
            }
        }
    }
    
    /**
     * @brief 执行密码认证
     */
    void performAuthentication()
    {
        if (m_password.isEmpty()) {
            qDebug() << "PJLink: 密码为空，跳过认证";
            return;
        }
        
        // 根据PJLink规范生成MD5哈希
        QString authString = m_randomKey + m_password;
        QByteArray hash = QCryptographicHash::hash(authString.toUtf8(), QCryptographicHash::Md5);
        QString hashHex = hash.toHex();
        
        m_authHash = hashHex;
        m_isAuthenticated = true;
        
        qDebug() << "PJLink: 认证完成，哈希值:" << hashHex;
    }
    
    /**
     * @brief 发送PJLink指令
     * @param command 指令名称（如POWR、AVMT等）
     * @param parameter 指令参数
     */
    void sendPJLinkCommand(const QString &command, const QString &parameter)
    {
        if (!client || !m_isAuthenticated) {
            qDebug() << "PJLink: 未连接或未认证，无法发送指令";
            return;
        }
        
        QString pjlinkCommand;
        
        // 根据认证状态构建指令
        if (m_authHash.isEmpty()) {
            // 无密码认证：格式为 %1指令 参数
            pjlinkCommand = QString("%1%2 %3\r").arg("%1").arg(command).arg(parameter);
        } else {
            // 有密码认证：格式为 哈希值%1指令 参数
            pjlinkCommand = m_authHash + "%1" + command + " " + parameter + "\r";
        }

        // 发送指令
        QByteArray commandData = pjlinkCommand.toUtf8();
        client->sendMessage(commandData.toHex(), 0); // 0表示HEX格式
        AbstractDelegateModel::stateFeedBack("/send", true);
    }
    /**
     * @brief 发送自定义PJLink指令
     * @param command 完整的指令字符串，格式为"指令 参数"
     * 将指令按空格分割，第一部分作为指令，第二部分作为参数发送
     */
    void sendCustomPJLinkCommand(const QString &command) {
        setCustomCommand(command);
        // 按空格分割指令
        QStringList parts = command.split(' ', Qt::SkipEmptyParts);
        
        if (parts.size() >= 2) {
            // 如果有至少两个部分，第一位作为指令，第二位作为参数
            QString cmd = parts[0];
            QString param = parts[1];
            // widget->customCommandLineEdit->setText(command); // Handled by setCustomCommand
            sendPJLinkCommand(cmd, param);
        } else if (parts.size() == 1) {
            // 如果只有一个部分，作为指令发送，参数为空
            QString cmd = parts[0];
            // widget->customCommandLineEdit->setText(cmd); // Handled by setCustomCommand
            sendPJLinkCommand(cmd, "");
        } else {
            // 如果指令为空或无效
            qDebug() << "无效的PJLink指令:" << command;
        }
    }
    /**
     * @brief 解析PJLink消息协议
     * @param data 原始消息数据
     */
    void parsePJLinkMessage(const QByteArray &data)
    {
        QString message = QString::fromUtf8(data);
        m_connectionStatus= std::make_shared<VariableData>(message);
        Q_EMIT dataUpdated(0);
    }
    private:
        PJLinkInterface *widget = new PJLinkInterface();
        TcpClient *client = new TcpClient();
        QTimer *m_heartbeatTimer;  // 心跳定时器
        bool m_isAuthenticated;        // 认证状态
        QString m_randomKey;           // 服务器返回的随机密钥
        QString m_authHash;            // MD5认证哈希值
        // 数据成员 - 只保留需要的一个输入和一个输出
        std::shared_ptr<VariableData> m_inData;
        std::shared_ptr<VariableData> m_connectionStatus;  // 连接状态输出
        QString m_customPJLinkCommand;
        QString m_host;
        int m_port;
        QString m_password;
    };

}
