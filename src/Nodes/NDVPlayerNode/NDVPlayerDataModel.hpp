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
#include "ConstantDefines.h"


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
            AbstractDelegateModel::registerOSCControl("/play",widget->Play);
            AbstractDelegateModel::registerOSCControl("/stop",widget->Stop);
            AbstractDelegateModel::registerOSCControl("/loop",widget->LoopPlay);
            AbstractDelegateModel::registerOSCControl("/next",widget->Next);
            AbstractDelegateModel::registerOSCControl("/prev",widget->Prev);
            AbstractDelegateModel::registerOSCControl("/index",widget->FileIndex);
            AbstractDelegateModel::registerOSCControl("/playerID",widget->PlayerID);

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
            connect(widget->FileIndex, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
                currentFileIndex = value;
                updateStatus();
            });

            // 播放器ID变化时更新
            connect(widget->PlayerID, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int value) {
                currentPlayerId = value;
                updateStatus();
            });

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
                    widget->FileIndex->setValue(fileIndex);
                    currentFileIndex = fileIndex;
                    updateStatus();
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
            modelJson1["FileIndex"] = widget->FileIndex->value();
            modelJson1["PlayerID"] = widget->PlayerID->value();
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["PlayerSettings"] = modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["PlayerSettings"];
            if (!v.isUndefined() && v.isObject()) {
                widget->FileIndex->setValue(v.toObject()["FileIndex"].toInt());
                widget->PlayerID->setValue(v.toObject()["PlayerID"].toInt());
                currentFileIndex = widget->FileIndex->value();
                currentPlayerId = widget->PlayerID->value();
                
                updateStatus();
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
         * @brief 发送播放指令
         */
        void sendPlayCommand() {
            sendCommand("play", currentFileIndex);
            currentState = "Playing";
            updateStatus();
        }

        /**
         * @brief 发送停止指令
         */
        void sendStopCommand() {
            sendCommand("stop", 0);
            currentState = "Stopped";
            updateStatus();
        }

        /**
         * @brief 发送循环播放指令
         */
        void sendLoopCommand() {
            sendCommand("loop", currentFileIndex);
            currentState = "Loop Playing";
            updateStatus();
        }

        /**
         * @brief 发送下一个文件指令
         */
        void sendNextCommand() {
            sendCommand("next", 0);
        }

        /**
         * @brief 发送上一个文件指令
         */
        void sendPrevCommand() {
            sendCommand("prev", 0);
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