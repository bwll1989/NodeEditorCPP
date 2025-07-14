#pragma once


#include <QtCore/QObject>
#include <QHostAddress>
#include "DataTypes/NodeDataList.hpp"
#include "QThread"
#include <QtNodes/NodeDelegateModel>
#include "UDPSocketInterface.hpp"
#include <iostream>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include "QGridLayout"
#include <QtCore/qglobal.h>
#include <QThread>
#include "Common/Devices/UdpSocket/UdpSocket.h"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;
using namespace NodeDataTypes;
namespace Nodes
{
    class UDPSocketDataModel : public NodeDelegateModel
    {
        Q_OBJECT
    public:

        UDPSocketDataModel(){
            InPortCount =4;
            OutPortCount=4;
            PortEditable=true;
            CaptionVisible=true;
            Caption=PLUGIN_NAME;
            WidgetEmbeddable= true;
            Resizable=true;
            m_inData=std::make_shared<VariableData>();
            m_outData=std::make_shared<VariableData>();
            //        connect(widget->send, &QPushButton::clicked, this,&UDPSocketDataModel::sendMessage,Qt::QueuedConnection);
            connect(this, &UDPSocketDataModel::sendUDPMessage, client, &UdpSocket::sendMessage, Qt::QueuedConnection);
            ////        connect(widget->IP,&QLineEdit::editingFinished,this,&UDPSocketDataModel::hostChange,Qt::QueuedConnection);
            ////        connect(widget->Port,&QSpinBox::valueChanged,this,&UDPSocketDataModel::hostChange,Qt::QueuedConnection);
            connect(widget,&UDPSocketInterface::hostChanged,client, &UdpSocket::setHost,Qt::QueuedConnection);
            connect(client, &UdpSocket::recMsg, this, &UDPSocketDataModel::recMsg, Qt::QueuedConnection);
            // connect(widget->sendButton, &QPushButton::clicked, this,, Qt::QueuedConnection);
            //        connect(client, &UdpSocket::isReady, widget->send, &QPushButton::setEnabled, Qt::QueuedConnection);
            ////        connect(this, &UDPSocketDataModel::startUDPSocket, client, &UdpSocket::setHost, Qt::QueuedConnection);
            connect(widget->sendButton, &QPushButton::clicked, this, [this]() {
                emit sendUDPMessage(widget->targetHostEdit->text(), widget->targetPortSpinBox->value(), widget->valueEdit->text(), widget->format->currentIndex());
            });
        }
        ~UDPSocketDataModel(){

            client->cleanup();
            delete client;
            //        delete clientThread;
            widget->deleteLater();
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                switch (portIndex) {
            case 0:
                    return "TARGET HOST";
            case 1:
                    return "TARGET PORT";
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
                widget->targetHostEdit->setText(m_inData->value().toString());
                break;
            case 1:
                widget->targetPortSpinBox->setValue(m_inData->value().toInt());
                break;
            case 2:
                widget->valueEdit->setText(m_inData->value().toString());
                emit sendUDPMessage(widget->targetHostEdit->text(),widget->targetPortSpinBox->value(),m_inData->value().toString(),widget->format->currentIndex());
                break;
            case 3:
                m_inData = std::make_shared<VariableData>(widget->valueEdit->text());
                emit sendUDPMessage(widget->targetHostEdit->text(),widget->targetPortSpinBox->value(),m_inData->value().toString(),widget->format->currentIndex());
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
            modelJson1["Listening host"] = widget->listeningHostEdit->text();
            modelJson1["Listening port"] = widget->listeningPortSpinBox->value();
            modelJson1["Target host"] = widget->targetHostEdit->text();
            modelJson1["Target port"] = widget->targetPortSpinBox->value();
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
                widget->listeningHostEdit->setText(v["Listening host"].toString());
                widget->listeningPortSpinBox->setValue(v["Listening port"].toInt());
                widget->targetHostEdit->setText(v["Target host"].toString());
                widget->targetPortSpinBox->setValue(v["Target port"].toInt());
                widget->valueEdit->setText(v["Value"].toString());
                widget->format->setCurrentIndex(v["Format"].toInt());
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

    signals:
        void sendUDPMessage(const QString &host,const int &port,const QString &message,const int &format=0);
        void startUDPSocket(const QString &host,int port);
    private:
        UDPSocketInterface *widget=new UDPSocketInterface();
        UdpSocket *client=new UdpSocket();
        //    QVariant message;
        std::shared_ptr<VariableData> m_inData;
        std::shared_ptr<VariableData> m_outData;

    };
}