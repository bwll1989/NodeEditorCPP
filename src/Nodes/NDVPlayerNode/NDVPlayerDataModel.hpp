#pragma once

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <iostream>
#include <QPushButton>
#include <QtCore/qglobal.h>
#include "NDVPlayerInterface.hpp"
#include <QVariantMap>
#include "Common/BuildInNodes/AbstractDelegateModel.h"
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
     * @brief NDV播放器节点 - 抽象化NDV播放器功能
     * 这个节点负责播放器逻辑，将指令传递给NDVControlNode
     */
    class NDVPlayerDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(int fileIndex READ getFileIndex WRITE setFileIndex NOTIFY fileIndexChanged)
        Q_PROPERTY(int playerId READ getPlayerId WRITE setPlayerId NOTIFY playerIdChanged)

    public:
        NDVPlayerDataModel()
        {
            InPortCount = 4;  // 文件索引、播放、停止、循环播放
            OutPortCount = 2; // 指令输出、状态输出
            CaptionVisible = true;
            Caption = "NDV Player";
            WidgetEmbeddable = false;
            Resizable = false;
            
            commandData = std::make_shared<VariableData>();
            statusData = std::make_shared<VariableData>();
            AbstractDelegateModel::registerExternalControl("/play",widget->Play);
            AbstractDelegateModel::registerExternalControl("/stop",widget->Stop);
            AbstractDelegateModel::registerExternalControl("/loop",widget->LoopPlay);
            AbstractDelegateModel::registerExternalControl("/next",widget->Next);
            AbstractDelegateModel::registerExternalControl("/prev",widget->Prev);
            AbstractDelegateModel::registerExternalControl("/index",widget->FileIndex);
            AbstractDelegateModel::registerExternalControl("/playerID",widget->PlayerID);
            
            // 初始化内部状态
            currentFileIndex = widget->FileIndex->value();
            currentPlayerId = widget->PlayerID->value();

            // 连接界面按钮
            connect(widget->Play, &QPushButton::clicked, this, [this]() {
                sendPlayCommand();
            });

            connect(widget->Stop, &QPushButton::clicked, this, [this]() {
                sendStopCommand();
            });

            connect(widget->LoopPlay, &QPushButton::clicked, this, [this]() {
                sendLoopCommand();
            });

            connect(widget->Next, &QPushButton::clicked, this, [this]() {
                sendNextCommand();
            });

            connect(widget->Prev, &QPushButton::clicked, this, [this]() {
                sendPrevCommand();
            });

            // 文件索引变化时更新
            connect(widget->FileIndex, &IntDragValueWidget::valueChanged, this, &NDVPlayerDataModel::setFileIndex);

            // 播放器ID变化时更新
            connect(widget->PlayerID, &IntDragValueWidget::valueChanged, this, &NDVPlayerDataModel::setPlayerId);

            // 初始化状态
            updateStatus();
        }

        ~NDVPlayerDataModel() override {
            delete widget;
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            if (portType == PortType::In) {
                switch (portIndex) {
                case 0:
                    return "INDEX";    // 文件索引输入
                case 1:
                    return "PLAY";          // 播放触发
                case 2:
                    return "STOP";          // 停止触发
                case 3:
                    return "LOOP";          // 循环播放触发
                default:
                    return "";
                }
            } else {
                switch (portIndex) {
                case 0:
                    return "COMMAND";   // 指令输出到NDVControlNode
                case 1:
                    return "STATUS";    // 播放器状态输出
                default:
                    return "";
                }
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
            switch (port) {
            case 0:
                return commandData;  // 指令数据
            case 1:
                return statusData;   // 状态数据
            default:
                return nullptr;
            }
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

            switch (portIndex) {
                case 0: // 文件索引
                {
                    int fileIndex = variableData->value().toInt();
                    setFileIndex(fileIndex);
                    break;
                }
                case 1: // 播放触发
                {
                    if (variableData->value().toBool()) {
                        sendPlayCommand();
                    }else{
                        sendStopCommand();
                    }
                    break;
                }
                case 2: // 停止触发
                {
                    if (variableData->value().toBool()) {
                        sendStopCommand();
                    }
                    break;
                }
                case 3: // 循环播放触发
                {
                    if (variableData->value().toBool()) {
                        sendLoopCommand();
                    }
                    break;
                }
            }
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["FileIndex"] = currentFileIndex;
            modelJson1["PlayerID"] = currentPlayerId;
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["PlayerSettings"] = modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["PlayerSettings"];
            if (!v.isUndefined() && v.isObject()) {
                if (v.toObject().contains("FileIndex"))
                    setFileIndex(v.toObject()["FileIndex"].toInt());
                if (v.toObject().contains("PlayerID"))
                    setPlayerId(v.toObject()["PlayerID"].toInt());
                
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

        void afterModelReady() override {
            AbstractDelegateModel::afterModelReady();
            auto bus = GlobalEventBus::instance();
            bus->subscribe(makeFullOscAddress("/index"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/playerID"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/play"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/stop"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/loop"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/next"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/prev"), this, SLOT(onGlobalEvent(GlobalEvent)));
            
            AbstractDelegateModel::stateFeedBack("/index", currentFileIndex);
            AbstractDelegateModel::stateFeedBack("/playerID", currentPlayerId);
        }

    public:
        int getFileIndex() const { return currentFileIndex; }
        void setFileIndex(int value) {
            if (currentFileIndex == value) return;
            currentFileIndex = value;
            if (widget && widget->FileIndex->value() != value) {
                QSignalBlocker blocker(widget->FileIndex);
                widget->FileIndex->setValue(value);
            }
            emit fileIndexChanged(value);
            AbstractDelegateModel::stateFeedBack("/index", value);
            updateStatus();
        }

        int getPlayerId() const { return currentPlayerId; }
        void setPlayerId(int value) {
            if (currentPlayerId == value) return;
            currentPlayerId = value;
            if (widget && widget->PlayerID->value() != value) {
                QSignalBlocker blocker(widget->PlayerID);
                widget->PlayerID->setValue(value);
            }
            emit playerIdChanged(value);
            AbstractDelegateModel::stateFeedBack("/playerID", value);
            updateStatus();
        }

    Q_SIGNALS:
        void fileIndexChanged(int value);
        void playerIdChanged(int value);

    private Q_SLOTS:
        void onGlobalEvent(const GlobalEvent& ev) {
            if (ev.kind != GlobalEventKind::Command) return;
            QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
            
            if (localPath == "index") setFileIndex(ev.payload.toInt());
            else if (localPath == "playerID") setPlayerId(ev.payload.toInt());
            else if (localPath == "play") sendPlayCommand();
            else if (localPath == "stop") sendStopCommand();
            else if (localPath == "loop") sendLoopCommand();
            else if (localPath == "next") sendNextCommand();
            else if (localPath == "prev") sendPrevCommand();
        }

    private:
        /**
         * @brief 发送播放指令
         */
        void sendPlayCommand() {
            sendCommand("play", currentFileIndex);
            currentState = "Playing";
            AbstractDelegateModel::stateFeedBack("/play", true);
            updateStatus();
        }

        /**
         * @brief 发送停止指令
         */
        void sendStopCommand() {
            sendCommand("stop", 0);
            AbstractDelegateModel::stateFeedBack("/stop", true);
            currentState = "Stopped";
            updateStatus();
        }

        /**
         * @brief 发送循环播放指令
         */
        void sendLoopCommand() {
            sendCommand("loop", currentFileIndex);
            currentState = "Loop Playing";
            AbstractDelegateModel::stateFeedBack("/loop", true);
            updateStatus();
        }

        /**
         * @brief 发送下一个文件指令
         */
        void sendNextCommand() {
            sendCommand("next", 0);
            AbstractDelegateModel::stateFeedBack("/next", true);
        }

        /**
         * @brief 发送上一个文件指令
         */
        void sendPrevCommand() {
            sendCommand("prev", 0);
            AbstractDelegateModel::stateFeedBack("/prev", true);
        }

        /**
         * @brief 发送指令到NDVControlNode
         * @param commandType 指令类型
         * @param fileIndex 文件索引
         */
        void sendCommand(const QString& commandType, int fileIndex) {
            commandData = std::make_shared<VariableData>();
            
            commandData->insert("type", commandType);
            commandData->insert("fileIndex", fileIndex);
            commandData->insert("targetId", currentPlayerId);
            commandData->insert("playerId", currentPlayerId);
            commandData->insert("timestamp", QDateTime::currentDateTime().toString());
            
            // 发出数据更新信号
            Q_EMIT dataUpdated(0);
        }

        /**
         * @brief 更新播放器状态
         */
        void updateStatus() {
            statusData = std::make_shared<VariableData>();
            
            QVariantMap statusMap;
            statusMap["playerId"] = currentPlayerId;
            statusMap["currentFile"] = currentFileIndex;
            statusMap["state"] = currentState;
            statusMap["lastUpdate"] = QDateTime::currentDateTime().toString();
            
            statusData->insert("status", statusMap);
            statusData->insert("playerId", currentPlayerId);
            statusData->insert("state", currentState);
            statusData->insert("currentFile", currentFileIndex);
            
            // 发出状态更新信号
            Q_EMIT dataUpdated(1);
        }

    private:
        std::shared_ptr<VariableData> commandData;  // 指令输出数据
        std::shared_ptr<VariableData> statusData;   // 状态输出数据
        
        int currentFileIndex = 1;
        int currentPlayerId = 1;
        QString currentState = "Stopped";
        
        NDVPlayerInterface * widget = new NDVPlayerInterface();
    };
}
