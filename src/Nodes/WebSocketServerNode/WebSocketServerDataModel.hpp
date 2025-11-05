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

#include "ConstantDefines.h"
#include "OSCMessage.h"
#include "Common/Devices/WebSocketServer/WebSocketServer.h"
#include "OSCSender/OSCSender.h"
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;
using namespace NodeDataTypes;
namespace Nodes
{
    class WebSocketServerDataModel : public NodeDelegateModel
    {
        Q_OBJECT

    public:

        WebSocketServerDataModel(){
            InPortCount =3;
            OutPortCount=4;
            CaptionVisible=true;
            PortEditable=false;
            Caption="WebSocket Server";
            WidgetEmbeddable= false;
            Resizable=false;
            m_inData=std::make_shared<VariableData>();
            m_outData=std::make_shared<VariableData>();
            NodeDelegateModel::registerOSCControl("/port", widget->portSpinBox);
            NodeDelegateModel::registerOSCControl("/value", widget->valueEdit);
            NodeDelegateModel::registerOSCControl("/send", widget->sendButton);
            server = new WebSocketServer();
            connect(server, &WebSocketServer::messageReceived, this, [this](QWebSocket *socket, const QByteArray &message) {
                QVariantMap msgMap;
                msgMap.insert("hex", QString(message.toHex())); // 转换为QString类型
                msgMap.insert("host", socket->peerAddress().toString());
                msgMap.insert("utf-8", QString::fromUtf8(message)); // 修复UTF-8解码方式
                msgMap.insert("ascii", QString::fromLatin1(message)); // 修复ANSI解码方式
                msgMap.insert("default", message);
                recMsg(msgMap);
            }, Qt::QueuedConnection);
            connect(widget, &WebSocketServerInterface::hostChanged, this, [this](int port) {
                server->start(static_cast<quint16>(port));
            }, Qt::QueuedConnection);
            connect(widget->sendButton, &QPushButton::clicked, this, [this]() {
                sendMessage();
            }, Qt::QueuedConnection);
        }
        ~WebSocketServerDataModel(){
            if(server) server->stop();
            delete server;
        }
    public:
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                switch (portIndex) {
                    case 0:
                        return "PORT";
                    case 1:
                        return "VALUE";
                    case 2:
                        return "TRIGGER";
                    default:
                        break;
                }
            case PortType::Out:
                switch (portIndex)
                {
                    case 0:
                        return "RESULT";
                    case 1:
                        return "HOST";
                    case 2:
                        return "VALUE";
                    case 3:
                        return "HEX";
                    default:
                        break;
                }
            default:
                break;
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
                case 0:
                    return m_outData;
                case 1:
                    return std::make_shared<VariableData>(m_outData->value("host").toString());
                case 2:
                    return std::make_shared<VariableData>(m_outData->value());
                case 3:
                    return std::make_shared<VariableData>(m_outData->value("hex"));
                default:
                    return nullptr;
            }
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override {
            if (data == nullptr) {
                return;
            }
            m_inData = std::dynamic_pointer_cast<VariableData>(data);
            switch (portIndex) {
                case 0:
                    widget->portSpinBox->setValue(m_inData->value().toInt());
                    server->setPort(static_cast<quint16>(m_inData->value().toInt()));
                    break;
                case 1:
                    widget->valueEdit->setText(m_inData->value().toString());
                    sendMessage();
                    break;
                case 2:
                    m_inData = std::make_shared<VariableData>(widget->valueEdit->text());
                    sendMessage();
                    break;
                default:
                    break;
            }
        }

        QWidget *embeddedWidget() override
        {
            return widget;
            //        return nullptr;
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["Port"] = widget->portSpinBox->value();
            modelJson1["Value"] = widget->valueEdit->text();
            modelJson1["Format"] = widget->format->currentIndex();
            modelJson1["MessageType"] = widget->messageType->currentIndex();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;

        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                widget->portSpinBox->setValue(v["Port"].toInt());
                widget->valueEdit->setText(v["Value"].toString());
                widget->format->setCurrentIndex(v["Format"].toInt());
                widget->messageType->setCurrentIndex(v["MessageType"].toInt());
            }
        }

    public slots:
    //    收到信息时
        void recMsg(const QVariantMap &msg)
        {
            m_outData=std::make_shared<VariableData>(msg);
            Q_EMIT dataUpdated(0);
            Q_EMIT dataUpdated(1);
            Q_EMIT dataUpdated(2);
            Q_EMIT dataUpdated(3);
        }

        void sendMessage(){
            if(!m_inData){
                return;
            }
            QByteArray data;
            switch (widget->format->currentIndex()) {
            case 0: // HEX格式
                data = QByteArray::fromHex(widget->valueEdit->text().toUtf8());
                break;
            case 1: // UTF-8格式
                data = widget->valueEdit->text().toUtf8();
                break;
            case 2: // ASCII格式
                data = widget->valueEdit->text().toLatin1(); // 使用Latin-1编码（ASCII扩展）
                break;
            default:
                data = widget->valueEdit->text().toUtf8();
                break;
            }

            // 这里假设WebSocketServer有broadcastMessage(QByteArray)方法
            if(server) server->broadcastMessage(data, widget->messageType->currentIndex());
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
        WebSocketServerInterface *widget=new WebSocketServerInterface();
        WebSocketServer *server = nullptr;
        std::shared_ptr<VariableData> m_inData;
        std::shared_ptr<VariableData> m_outData;
    };
}