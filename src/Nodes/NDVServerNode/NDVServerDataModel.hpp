#pragma once

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <iostream>
#include <QPushButton>
#include <QtCore/qglobal.h>
#include "NDVServerInterface.hpp"
#include "Common/Devices/TcpServer/TcpServer.h"
#include <QVariantMap>
#include <QMap>
#include <QDateTime>
#include <QTimer>
#include "QThread"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

const QByteArray COMMAND_PREFIX = QByteArray::fromHex("4A480E0010000000000000004A481000484A0100");
const QByteArray COMMAND_SUFFIX = QByteArray::fromHex("000000000000");
const QByteArray COMMAND_STOP =   QByteArray::fromHex("02000000");
const QByteArray COMMAND_NEXT =   QByteArray::fromHex("04000000");
const QByteArray COMMAND_PREV =   QByteArray::fromHex("03000000");
const QByteArray COMMAND_CLOSE =  QByteArray::fromHex("09000000");
const QByteArray COMMAND_PLAY =   QByteArray::fromHex("1600");
const QByteArray COMMAND_HANDSHAKE =   QByteArray::fromHex("00000000");
const QByteArray COMMAND_LOOP =  QByteArray::fromHex("10000000");

namespace Nodes
{
    /**
     * @brief NDV客户端信息结构体
     */
    struct NDVClientInfo {
        QString ipAddress;          // IP地址
        int deviceId;              // 设备ID
        QDateTime lastSeen;        // 最后一次握手时间
        QString status;            // 连接状态
        QDateTime lastHandshake;   // 最后一次发送握手的时间
        
        NDVClientInfo() : deviceId(-1), status("Unknown") {}
        NDVClientInfo(const QString& ip, int id) 
            : ipAddress(ip), deviceId(id), lastSeen(QDateTime::currentDateTime()), 
              status("Connected"), lastHandshake(QDateTime::currentDateTime()) {}
    };

    /**
     * @brief NDV控制节点 - 专门负责发送数据到指定ID的NDV设备
     * 这个节点负责网络通信、设备管理，并定时向所有客户端发送握手信号
     */
    class NDVServerDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        NDVServerDataModel()
        {
            InPortCount = 1;  // 输入端口：指令数据
            OutPortCount = 1; // 输出客户端状态信息
            CaptionVisible = true;
            Caption = "NDV Server";
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = true;
            inData = std::make_shared<VariableData>();

            // 初始化 TcpServer
            mSender = new TcpServer("0.0.0.0", widget->Port->value());

            // 初始化握手定时器
            handshakeTimer = new QTimer(this);
            handshakeTimer->setInterval(5000); // 5秒间隔
            connect(handshakeTimer, &QTimer::timeout, this, &NDVServerDataModel::sendHandshakeToAllClients);
            handshakeTimer->start(); // 启动定时器

            // 连接界面信号到 TcpServer
            connect(widget->IP, &QLineEdit::textChanged, this, [this](const QString &text) {
                mSender->setHost(text, widget->Port->value());
            });

            connect(widget->Port, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
                mSender->setHost(widget->IP->text(), value);
            });

            // 连接 TcpServer 的信号到数据更新
            connect(mSender, &TcpServer::recMsg, this, &NDVServerDataModel::handleIncomingData);

        }

        ~NDVServerDataModel() override {
            if (handshakeTimer) {
                handshakeTimer->stop();
                delete handshakeTimer;
            }
            delete mSender;
            delete widget;
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            if (portType == PortType::In) {

                return "PLAYER "+QString::number(portIndex);  // 接收来自NDVPlayer的指令数据

            } else {
                return "CLIENT_STATUS";     // 输出客户端状态信息
            }
        }

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            Q_UNUSED(portType)
            return VariableData().type();
        }

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            Q_UNUSED(port);
            return inData;
        }

        /**
         * @brief 处理输入数据
         * @param data 输入数据
         * @param portIndex 端口索引
         */
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (data == nullptr) {
                return;
            }
            
            auto variableData = std::dynamic_pointer_cast<VariableData>(data);
            if (!variableData) {
                return;
            }
            currentCommandData = variableData;
            executeCommand();
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["IP"] = widget->IP->text();
            modelJson1["Port"] = widget->Port->value();
            modelJson1["HandshakeEnabled"] = handshakeTimer->isActive();
            
            // 保存客户端映射关系
            QJsonObject clientsJson;
            for (auto it = ndvClients.begin(); it != ndvClients.end(); ++it) {
                QJsonObject clientInfo;
                clientInfo["ip"] = it.value().ipAddress;
                clientInfo["id"] = it.value().deviceId;
                clientInfo["lastSeen"] = it.value().lastSeen.toString();
                clientInfo["lastHandshake"] = it.value().lastHandshake.toString();
                clientsJson[QString::number(it.key())] = clientInfo;
            }
            modelJson1["clients"] = clientsJson;
            
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["Address"] = modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["Address"];
            if (!v.isUndefined() && v.isObject()) {
                widget->IP->setText(v.toObject()["IP"].toString());
                widget->Port->setValue(v.toObject()["Port"].toInt());
                
                // 恢复握手定时器状态
                bool handshakeEnabled = v.toObject()["HandshakeEnabled"].toBool();
                if (handshakeEnabled && !handshakeTimer->isActive()) {
                    handshakeTimer->start();
                } else if (!handshakeEnabled && handshakeTimer->isActive()) {
                    handshakeTimer->stop();
                }
                
                // 加载客户端映射关系
                QJsonValue clientsValue = v.toObject()["clients"];
                if (clientsValue.isObject()) {
                    QJsonObject clientsObj = clientsValue.toObject();
                    for (auto it = clientsObj.begin(); it != clientsObj.end(); ++it) {
                        int id = it.key().toInt();
                        QJsonObject clientInfo = it.value().toObject();
                        NDVClientInfo info;
                        info.ipAddress = clientInfo["ip"].toString();
                        info.deviceId = clientInfo["id"].toInt();
                        info.lastSeen = QDateTime::fromString(clientInfo["lastSeen"].toString());
                        info.lastHandshake = QDateTime::fromString(clientInfo["lastHandshake"].toString());
                        ndvClients[id] = info;
                    }
                }
                
                NodeDelegateModel::load(p);
            }
        }

        QWidget *embeddedWidget() override {
            return widget;
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
    private:
        /**
         * @brief 向所有客户端发送握手信号
         * 每5秒自动调用一次
         */
        void sendHandshakeToAllClients() {
            if (ndvClients.isEmpty()) {
                qDebug() << "没有已连接的客户端，跳过握手发送";
                return;
            }

            QDateTime currentTime = QDateTime::currentDateTime();
            for (auto it = ndvClients.begin(); it != ndvClients.end(); ++it) {
                int clientId = it.key();
                NDVClientInfo& clientInfo = it.value();
                
                // 构建握手指令
                QByteArray handshakeCommand;
                handshakeCommand.append(COMMAND_PREFIX);
                handshakeCommand.append(COMMAND_HANDSHAKE);
                handshakeCommand.append(COMMAND_SUFFIX);
                
                // 发送握手指令
                mSender->sendBytesArrayToHost(handshakeCommand, clientInfo.ipAddress);
                // 更新最后握手时间
                clientInfo.lastHandshake = currentTime;
            }
            
            // 更新客户端列表
            updateClientList();
        }

        /**
         * @brief 执行当前指令
         */
        void executeCommand() {
            if (!currentCommandData || currentCommandData->isEmpty()) {
                return;
            }

            QVariantMap commandMap = currentCommandData->getMap();
            
            // 获取指令类型和参数
            QString commandType = commandMap.value("type", "").toString();
            int fileIndex = commandMap.value("fileIndex", 0).toInt();
            int targetId = commandMap.value("targetId", currentTargetId).toInt();
            QString targetIP = commandMap.value("targetIP", "").toString();

            // 发送指令
            if (!targetIP.isEmpty()) {
                sendCommandToIP(commandType, fileIndex, targetIP);
            } else {
                sendCommandToTargetId(commandType, fileIndex, targetId);
            }
        }

        /**
         * @brief 向指定IP发送指令
         * @param commandType 指令类型
         * @param fileIndex 文件索引
         * @param targetIP 目标IP地址
         */
        void sendCommandToIP(const QString& commandType, int fileIndex, const QString& targetIP) {
            QByteArray command = getCommandByType(commandType);
            if (command.isEmpty()) {
                qDebug() << QString("未知的指令类型: %1").arg(commandType);
                return;
            }
            
            QByteArray fullCommand = buildCommand(command, fileIndex, 1);
            mSender->sendBytesArrayToHost(fullCommand, targetIP);
            
            qDebug() << QString("向IP [%1] 发送 %2 指令，文件索引: %3")
                        .arg(targetIP).arg(commandType).arg(fileIndex);
        }

        /**
         * @brief 向指定ID发送指令
         * @param commandType 指令类型
         * @param fileIndex 文件索引
         * @param targetId 目标ID
         */
        void sendCommandToTargetId(const QString& commandType, int fileIndex, int targetId) {
            QByteArray command = getCommandByType(commandType);
            if (command.isEmpty()) {
                qDebug() << QString("未知的指令类型: %1").arg(commandType);
                return;
            }
            
            if (targetId == 0) {
                // 广播到所有客户端
                for (auto it = ndvClients.begin(); it != ndvClients.end(); ++it) {
                    QByteArray fullCommand = buildCommand(command, fileIndex, it.key());
                    mSender->sendBytesArrayToHost(fullCommand, it.value().ipAddress);
                }
                return;
            }
            
            if (!ndvClients.contains(targetId)) {
                qDebug() << QString("目标客户端ID %1 不存在").arg(targetId);
                return;
            }
            
            QByteArray fullCommand = buildCommand(command, fileIndex, targetId);
            QString targetIP = ndvClients[targetId].ipAddress;
            mSender->sendBytesArrayToHost(fullCommand, targetIP);
        }

        /**
         * @brief 根据指令类型获取对应的命令字节数组
         * @param commandType 指令类型字符串
         * @return 对应的命令字节数组
         */
        QByteArray getCommandByType(const QString& commandType) {
            QString type = commandType.toLower();
            
            if (type == "play") return COMMAND_PLAY;
            else if (type == "stop") return COMMAND_STOP;
            else if (type == "loop") return COMMAND_LOOP;
            else if (type == "close") return COMMAND_CLOSE;
            else if (type == "next") return COMMAND_NEXT;
            else if (type == "prev") return COMMAND_PREV;
            else if (type == "handshake") return COMMAND_HANDSHAKE;
            
            return QByteArray();
        }

        /**
         * @brief 构建完整的NDV指令
         * @param command 命令类型
         * @param fileIndex 文件索引
         * @param targetId 目标设备ID
         * @return 完整的指令字节数组
         */
        QByteArray buildCommand(const QByteArray& command, int fileIndex, int targetId) {
            QByteArray fullCommand;
            fullCommand.append(COMMAND_PREFIX);
            fullCommand.append(command);
            
            if (command == COMMAND_PLAY || command == COMMAND_LOOP) {
                fullCommand.append(static_cast<char>(fileIndex));
                fullCommand.append(static_cast<char>(0x00));
            } else if (command == COMMAND_HANDSHAKE) {
                fullCommand.append(static_cast<char>(targetId));
                fullCommand.append(static_cast<char>(0x00));
            }
            
            fullCommand.append(COMMAND_SUFFIX);
            return fullCommand;
        }

        /**
         * @brief 更新客户端列表到输出数据
         */
        void updateClientList() {
            inData = std::make_shared<VariableData>();
            
            QVariantMap clientsMap;
            for (auto it = ndvClients.begin(); it != ndvClients.end(); ++it) {
                QVariantMap clientInfo;
                clientInfo["ip"] = it.value().ipAddress;
                clientInfo["id"] = it.value().deviceId;
                clientInfo["status"] = it.value().status;
                clientInfo["lastSeen"] = it.value().lastSeen.toString();
                clientInfo["lastHandshake"] = it.value().lastHandshake.toString();
                clientsMap[QString::number(it.key())] = clientInfo;
            }
            
            inData->insert("ConnectedClients", clientsMap);
            inData->insert("ClientCount", ndvClients.size());
            inData->insert("HandshakeActive", handshakeTimer->isActive());
            inData->insert("LastHandshakeTime", QDateTime::currentDateTime().toString());
            
            Q_EMIT dataUpdated(0);
        }

    private Q_SLOTS:
        /**
         * @brief 处理接收到的NDV数据，解析握手请求并维护客户端映射关系
         * @param data 包含16进制数据的QVariantMap
         */
        void handleIncomingData(const QVariantMap &data) {
            QString hexData = data["hex"].toString();
            QString clientIP = data["host"].toString();
            
            if (hexData.length() < 46) return;
            
            QString keyword = hexData.mid(40, 2);
            if (keyword.toUpper() != "0A") return;
            
            QString idHex = hexData.mid(44, 2);
            bool ok;
            int ndvId = idHex.toInt(&ok, 16);
            
            if (ok) {
                NDVClientInfo clientInfo(clientIP, ndvId);
                clientInfo.status = "Connected";
                ndvClients[ndvId] = clientInfo;
                updateClientList();
            }
        }

    private:
        std::shared_ptr<VariableData> inData;
        std::shared_ptr<VariableData> currentCommandData;
        int currentTargetId = 0;
        QTimer* handshakeTimer;  // 握手定时器
        NDVServerInterface * widget = new NDVServerInterface();
        TcpServer * mSender = nullptr;
        QMap<int, NDVClientInfo> ndvClients;
    };
}