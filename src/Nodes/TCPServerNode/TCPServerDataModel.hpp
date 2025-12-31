#pragma once


#include <QtCore/QObject>
#include "DataTypes/NodeDataList.hpp"
#include "QThread"
#include <QtNodes/NodeDelegateModel>
#include "TCPServerInterface.hpp"
#include <iostream>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include "QGridLayout"
#include <QtCore/qglobal.h>
#include <QThread>
#include <QtNetwork/QTcpServer>
#include <QtNetwork/QTcpSocket>

#include "ConstantDefines.h"

#include "QMutex"
#include "Common/Devices/TcpServer/TcpServer.h"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;
using namespace NodeDataTypes;
using namespace QtNodes;
namespace Nodes
{
    class TCPServerDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:

        TCPServerDataModel(){
            InPortCount =4;
            OutPortCount=4;
            CaptionVisible=true;
            PortEditable=false;
            Caption="TCP Server";
            WidgetEmbeddable= false;
            Resizable=false;
            m_inData=std::make_shared<VariableData>();
            m_outData=std::make_shared<VariableData>();
            server=new TcpServer();
            //        server->moveToThread(serverThread);

            connect(server, &TcpServer::recMsg, this, &TCPServerDataModel::recMsg, Qt::QueuedConnection);
            //        connect(server, &TcpServer::serverMessage, this, &TCPServerDataModel::recMsg, Qt::QueuedConnection);
            connect(widget, &TCPServerInterface::hostChanged, server, &TcpServer::setHost, Qt::QueuedConnection);
            AbstractDelegateModel::registerOSCControl("/send",widget->sendButton);
            AbstractDelegateModel::registerOSCControl("/value",widget->valueEdit);
            connect(widget->sendButton, &QPushButton::clicked, this,[this]()
            {
                server->sendMessage(widget->valueEdit->text(),widget->format->currentIndex());
            },Qt::QueuedConnection);
            //        connect(this, &TCPServerDataModel::stopTCPServer, server, &TcpServer::stopServer, Qt::QueuedConnection);
            //
            //        connect(server, &TcpServer::clientInserted, this, &TCPServerDataModel::insertClient, Qt::QueuedConnection);
            //        connect(server, &TcpServer::clientRemoved, this, &TCPServerDataModel::removeClient, Qt::QueuedConnection);
            ////        connect(widget->send, &QPushButton::clicked, this, &TCPServerDataModel::sendMessage, Qt::QueuedConnection);
            //        connect(this, &TCPServerDataModel::sendTCPMessage, server, &TcpServer::sendMessage, Qt::QueuedConnection);

            //        serverThread->start();

        }
        ~TCPServerDataModel(){
            server->cleanup();
        }
    public:
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                switch (portIndex) {
                    case 0:
                        return "HOST";
                    case 1:
                        return "PORT";
                    case 2:
                        return "VALUE";
                    case 3:
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
                    widget->hostLineEdit->setText(m_inData->value().toString());
                    break;
                case 1:
                    widget->portSpinBox->setValue(m_inData->value().toInt());
                    break;
                case 2:
                    widget->valueEdit->setText(m_inData->value().toString());
                    sendMessage();
                    break;
                case 3:
                    m_inData = std::make_shared<VariableData>(widget->valueEdit->text());
                    sendMessage();
                    break;
                default:
                    break;
            }

            // if (data == nullptr) {
            //     message = QVariant();
            // } else {
            //     auto textData = std::dynamic_pointer_cast<VariableData>(data);
            //     if (textData->value().canConvert<QString>()) {
            //         message = textData->value().toString();
            //     }
            // }
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
            modelJson1["Host"] = widget->hostLineEdit->text();
            modelJson1["Value"] = widget->valueEdit->text();
            modelJson1["Format"] = widget->format->currentIndex();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;

        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                widget->portSpinBox->setValue(v["Port"].toInt());
                widget->hostLineEdit->setText(v["Host"].toString());
                widget->valueEdit->setText(v["Value"].toString());
                widget->format->setCurrentIndex(v["Format"].toInt());
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
            server->sendMessage(m_inData->value().toString(),widget->format->currentIndex());
        }

    signals:
        //    关闭信号
        void stopTCPServer();
        // void sendTCPMessage(const QString &client,const QString &message,const int &format);
    private:
        TCPServerInterface *widget=new TCPServerInterface();
        TcpServer *server;
        //    QThread *serverThread=new QThread();
        std::shared_ptr<VariableData> m_inData;
        std::shared_ptr<VariableData> m_outData;
    };
}