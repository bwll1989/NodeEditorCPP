#pragma once
#include <QtCore/QObject>
#include "DataTypes/NodeDataList.hpp"
#include "QThread"
#include <QtNodes/NodeDelegateModel>
#include "WebSocketServerInterface.hpp"
#include <iostream>
#include <QtWebSockets/QWebSocket>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include <QSignalBlocker>

#include "ConstantDefines.h"
#include "Common/Devices/WebSocketServer/WebSocketServer.h"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;
using namespace NodeDataTypes;
namespace Nodes
{
    class WebSocketServerDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(int port READ getPort WRITE setPort NOTIFY portChanged)
        Q_PROPERTY(QString value READ getValue WRITE setValue NOTIFY valueChanged)

    public:

        WebSocketServerDataModel(){
            InPortCount = 3;
            OutPortCount = 4;
            CaptionVisible = true;
            PortEditable = false;
            Caption = "WebSocket Server";
            WidgetEmbeddable = false;
            Resizable = false;
            m_inData = std::make_shared<VariableData>();
            m_outData = std::make_shared<VariableData>();
            
            server = new WebSocketServer();
            AbstractDelegateModel::registerExternalControl("/port", widget->portSpinBox);
            AbstractDelegateModel::registerExternalControl("/value", widget->valueEdit);
            AbstractDelegateModel::registerExternalControl("/send", widget->sendButton);
            // UI Connections
            connect(widget->portSpinBox, &QSpinBox::valueChanged, this, [this](int val) {
                setPort(val);
            });

            connect(widget->valueEdit, &QLineEdit::editingFinished, this, [this]() {
                setValue(widget->valueEdit->text());
            });

            connect(widget->sendButton, &QPushButton::clicked, this, [this]() {
                sendMessage();
            }, Qt::QueuedConnection);

            // Server Connections
            connect(server, &WebSocketServer::messageReceived, this, [this](QWebSocket *socket, const QByteArray &message) {
                QVariantMap msgMap;
                msgMap.insert("hex", QString(message.toHex())); // 转换为QString类型
                msgMap.insert("host", socket->peerAddress().toString());
                msgMap.insert("utf-8", QString::fromUtf8(message)); // 修复UTF-8解码方式
                msgMap.insert("ascii", QString::fromLatin1(message)); // 修复ANSI解码方式
                msgMap.insert("default", message);
                recMsg(msgMap);
            }, Qt::QueuedConnection);
        }

        ~WebSocketServerDataModel(){
            if(server) server->stop();
            delete server;
        }

        int getPort() const { return m_port; }
        void setPort(int port) {
            if (m_port == port) return;
            m_port = port;
            {
                QSignalBlocker blocker(widget->portSpinBox);
                widget->portSpinBox->setValue(m_port);
            }
            // Restart server on port change
            if (server) {
                server->start(static_cast<quint16>(m_port));
            }
            emit portChanged(m_port);
            AbstractDelegateModel::stateFeedBack("/port", m_port);
        }

        QString getValue() const { return m_value; }
        void setValue(const QString& value) {
            if (m_value == value) return;
            m_value = value;
            {
                QSignalBlocker blocker(widget->valueEdit);
                widget->valueEdit->setText(m_value);
            }
            emit valueChanged(m_value);
            AbstractDelegateModel::stateFeedBack("/value", m_value);
        }

        void afterModelReady() override {
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/port"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/value"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/send"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                switch (portIndex) {
                    case 0: return "PORT";
                    case 1: return "VALUE";
                    case 2: return "TRIGGER";
                    default: break;
                }
            case PortType::Out:
                switch (portIndex)
                {
                    case 0: return "RESULT";
                    case 1: return "HOST";
                    case 2: return "VALUE";
                    case 3: return "HEX";
                    default: break;
                }
            default: break;
            }
            return "";
        }

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex);
            Q_UNUSED(portType);
            return VariableData().type();
        }

        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            switch (portIndex)
            {
                case 0: return m_outData;
                case 1: return std::make_shared<VariableData>(m_outData->value("host").toString());
                case 2: return std::make_shared<VariableData>(m_outData->value());
                case 3: return std::make_shared<VariableData>(m_outData->value("hex"));
                default: return nullptr;
            }
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override {
            if (data == nullptr) return;
            m_inData = std::dynamic_pointer_cast<VariableData>(data);
            switch (portIndex) {
                case 0:
                    setPort(m_inData->value().toInt());
                    break;
                case 1:
                    setValue(m_inData->value().toString());
                    sendMessage();
                    break;
                case 2:
                    sendMessage(); // Just trigger
                    break;
                default:
                    break;
            }
        }

        QWidget *embeddedWidget() override
        {
            return widget;
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["Port"] = m_port;
            modelJson1["Value"] = m_value;
            modelJson1["Format"] = widget->format->currentIndex();
            modelJson1["MessageType"] = widget->messageType->currentIndex();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"] = modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined() && v.isObject()) {
                setPort(v["Port"].toInt());
                setValue(v["Value"].toString());
                widget->format->setCurrentIndex(v["Format"].toInt());
                widget->messageType->setCurrentIndex(v["MessageType"].toInt());
            }
        }

    public slots:
        void recMsg(const QVariantMap &msg)
        {
            m_outData = std::make_shared<VariableData>(msg);
            Q_EMIT dataUpdated(0);
            Q_EMIT dataUpdated(1);
            Q_EMIT dataUpdated(2);
            Q_EMIT dataUpdated(3);
        }

        void sendMessage(){
            QByteArray data;
            switch (widget->format->currentIndex()) {
            case 0: // HEX格式
                data = QByteArray::fromHex(m_value.toUtf8());
                break;
            case 1: // UTF-8格式
                data = m_value.toUtf8();
                break;
            case 2: // ASCII格式
                data = m_value.toLatin1(); // 使用Latin-1编码（ASCII扩展）
                break;
            default:
                data = m_value.toUtf8();
                break;
            }

            if(server) server->broadcastMessage(data, widget->messageType->currentIndex());
            AbstractDelegateModel::stateFeedBack("/send", true);
        }

        void onGlobalEvent(const GlobalEvent& ev) {
            if (ev.kind == GlobalEventKind::Command) {
                QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
                if (localPath == "port") setPort(ev.payload.toInt());
                else if (localPath == "value") setValue(ev.payload.toString());
                else if (localPath == "send") sendMessage();
            }
        }

    signals:
        void portChanged(int port);
        void valueChanged(QString value);

    private:
        WebSocketServerInterface *widget = new WebSocketServerInterface();
        WebSocketServer *server = nullptr;
        std::shared_ptr<VariableData> m_inData;
        std::shared_ptr<VariableData> m_outData;

        int m_port = 8888;
        QString m_value;
    };
}
