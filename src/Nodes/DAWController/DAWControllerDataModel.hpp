#pragma once

#include <QtCore/QObject>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonArray>
#include <QtCore/QByteArray>
#include <QtCore/QDataStream>
#include <QtCore/QDateTime>
#include <QtCore/QTimer>

#include "Common/DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include "DAWControllerInterface.hpp"
#include <iostream>
#include <qcryptographichash.h>
#include <QtWidgets/QLineEdit>
#include "QGridLayout"
#include <QtCore/qglobal.h>
#include <QThread>
#include "Common/Devices/FTDAWController/FTDAWController.h"
#include "QMutex"
#include "PluginDefinition.hpp"
#include  "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;

using namespace NodeDataTypes;
using namespace QtNodes;
namespace Nodes
{
    class DAWControllerNode : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
        Q_PROPERTY(QString command READ command WRITE setCommand NOTIFY commandChanged)
        Q_PROPERTY(bool send READ send WRITE setSend NOTIFY sendChanged)
        Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)

    public:
        /**
         * @brief 构造函数，初始化PJLink节点
         */
        DAWControllerNode(){
            // 设置端口数量和基本属性
            InPortCount = 2;  //
            OutPortCount = 1; // 输出端口：STATUS (包含连接状态、最后消息、时间戳)
            CaptionVisible = true;
            PortEditable = false;
            Caption = PLUGIN_NAME;
            WidgetEmbeddable = false;
            Resizable = false;
            
            // 初始化数据
            m_inData = std::make_shared<VariableData>();
            m_outData = std::make_shared<VariableData>();

            m_controller = FTDAWController::acquire();

            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "send";
                b.control = widget->customCommandButton;
                AbstractDelegateModel::registerExternalBinding("/send", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "host";
                b.control = widget->hostEdit;
                AbstractDelegateModel::registerExternalBinding("/host", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "command";
                b.control = widget->customCommandLineEdit;
                AbstractDelegateModel::registerExternalBinding("/command", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "connected";
                AbstractDelegateModel::registerExternalBinding("/connected", this, b);
            }

            connect(widget->hostEdit, &QLineEdit::editingFinished, this, [=](){
                setHost(widget->hostEdit->text());
            });
            connect(widget->customCommandLineEdit, &QLineEdit::editingFinished, this, [=]() {
                setCommand(widget->customCommandLineEdit->text());
            });

            connect(widget->customCommandButton, &QPushButton::clicked, this, [=](){
                setSend(true);
            });


            if (m_controller) {
                connect(m_controller, &FTDAWController::isReady, this, [=](bool ready){
                    setConnected(ready);
                });
                
                connect(this, &DAWControllerNode::hostChanged, this, [=](const QString &host){
                    if (widget && widget->hostEdit) {
                        const QSignalBlocker blocker(widget->hostEdit);
                        widget->hostEdit->setText(host);
                    }
                });
                connect(this, &DAWControllerNode::commandChanged, this, [=](const QString &command){
                    if (widget && widget->customCommandLineEdit) {
                        const QSignalBlocker blocker(widget->customCommandLineEdit);
                        widget->customCommandLineEdit->setText(command);
                    }
                });
            }
        }
        
        /**
         * @brief 析构函数，清理资源
         */
        ~DAWControllerNode(){
            if (m_controller) {
                FTDAWController::release();
                m_controller = nullptr;
            }
        }

    public:
        QString host() const { return m_host; }
        QString command() const { return m_command; }
        /**
         * 函数级注释：获取发送触发属性（瞬时触发，外部使用 true 作为脉冲）
         */
        bool send() const { return m_send; }
        bool connected() const { return m_connected; }

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
                            return "Command";
                                break;
                        case 1:
                            return "Trigger";
                                break;
                    }
                break;
            case PortType::Out:
                return "STATUS";
                break;
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
                return m_outData;  // 包含所有状态信息的Map
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
            auto varData = std::dynamic_pointer_cast<VariableData>(data);
            if (!varData) {
                return;
            }

            if (portIndex == 0) {
                setCommand(varData->value().toString());
            }
            else if (portIndex == 1) {
                if (varData->value().toBool() && !command().isEmpty()) {
                   setSend(true);
                }
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
        modelJson1["IP"] = host();
        modelJson1["Command"] = command();
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
            const QString ip = v["IP"].toString();
            const QString cmd = v["Command"].toString();
            setHost(ip);
            setCommand(cmd);
        }
    }

    public slots:
        /**
         * 函数级注释：设置发送触发属性；当为 true 时执行发送并进行状态反馈
         */
        void setSend(bool value)
        {
            if (!value) {
                return;
            }
            if (m_send) {
                return;
            }
            m_send = true;
            Q_EMIT sendChanged(true);

            const QString cmd = command();
            if (m_controller && !cmd.isEmpty()) {
                m_controller->sendMessage(cmd);
                const QString timestamp = QDateTime::currentDateTime().toString("yyyy-MM-dd HH:mm:ss.zzz");
                m_outData->insert("last_message", cmd);
                m_outData->insert("timestamp", timestamp);
                Q_EMIT dataUpdated(0);
            }

            m_send = false;
            Q_EMIT sendChanged(false);
        }

        void setHost(const QString& host)
        {
            if (m_host == host) {
                return;
            }
            m_host = host;
            Q_EMIT hostChanged(host);
            if (m_controller) {
                m_controller->connectToServer(host);
            }
        }

        void setCommand(const QString& cmd)
        {
            if (m_command == cmd) {
                return;
            }
            m_command = cmd;
            Q_EMIT commandChanged(cmd);
        }

        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }

            const QString addrSend = makeFullOscAddress("/send");
            const QString addrHost = makeFullOscAddress("/host");
            const QString addrCmd = makeFullOscAddress("/command");

            if (ev.address == addrSend) {
                setSend(ev.payload.toBool());
            } else if (ev.address == addrHost) {
                setHost(ev.payload.toString());
            } else if (ev.address == addrCmd) {
                setCommand(ev.payload.toString());
            }
        }
    Q_SIGNALS:
        void hostChanged(const QString& host);
        void commandChanged(const QString& cmd);
        void sendChanged(bool value);
        void connectedChanged(bool connected);

     protected:
        /**
         * 函数级注释：模型就绪后订阅全局事件总线，使用包含正确节点ID的完整地址
         */
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/send"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/host"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/command"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
        }



        void setConnected(bool ready)
        {
            if (m_connected == ready) {
                return;
            }
            m_connected = ready;
            if (widget) {
                widget->updateConnectionStatus(ready);
            }
            m_outData->insert("connected", ready);
            Q_EMIT dataUpdated(0);
            Q_EMIT connectedChanged(ready);
        }

        DAWControllerInterface *widget = new DAWControllerInterface();
        std::shared_ptr<VariableData> m_inData;
        std::shared_ptr<VariableData> m_outData;           // 输出数据 (Map类型)
        FTDAWController* m_controller = nullptr;
        QString m_host;
        QString m_command;
        bool m_send = false;
        bool m_connected = false;

    };

}
