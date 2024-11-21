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

class UDPSocketDataModel : public NodeDelegateModel
{
    Q_OBJECT
public:

    UDPSocketDataModel(){
        InPortCount =1;
        OutPortCount=1;
        PortEditable=true;
        CaptionVisible=true;
        Caption=PLUGIN_NAME;
        WidgetEmbeddable= false;
        Resizable=true;
        inData=std::make_shared<VariableData>();
//        connect(widget->send, &QPushButton::clicked, this,&UDPSocketDataModel::sendMessage,Qt::QueuedConnection);
        connect(this, &UDPSocketDataModel::sendUDPMessage, client, &UdpSocket::sendMessage, Qt::QueuedConnection);
////        connect(widget->IP,&QLineEdit::editingFinished,this,&UDPSocketDataModel::hostChange,Qt::QueuedConnection);
////        connect(widget->Port,&QSpinBox::valueChanged,this,&UDPSocketDataModel::hostChange,Qt::QueuedConnection);
        connect(widget,&UDPSocketInterface::hostChanged,client, &UdpSocket::setHost,Qt::QueuedConnection);
        connect(client, &UdpSocket::arrayMsg, this, &UDPSocketDataModel::recMsg, Qt::QueuedConnection);
//        connect(client, &UdpSocket::isReady, widget->send, &QPushButton::setEnabled, Qt::QueuedConnection);
////        connect(this, &UDPSocketDataModel::startUDPSocket, client, &UdpSocket::setHost, Qt::QueuedConnection);
    }
    ~UDPSocketDataModel(){

        client->cleanup();
        delete client;
//        delete clientThread;
        widget->deleteLater();
    }

public:

    unsigned int nPorts(PortType portType) const override
    {
        unsigned int result = 1;

        switch (portType) {
            case PortType::In:
                result = InPortCount;
                break;

            case PortType::Out:
                result = OutPortCount;

            default:
                break;
        }

        return result;
    }

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {

        switch (portType) {
            case PortType::In:
                switch (portIndex) {
                    case 0:
                        return VariableData().type();
                }
                break;
            case PortType::Out:
                return VariableData().type();
                break;

            case PortType::None:
                break;
        }
        return VariableData().type();
    }

    std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
    {
        Q_UNUSED(portIndex)
        return inData;
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override {
        if (data== nullptr){
            return;
        }
        auto Da = std::dynamic_pointer_cast<VariableData>(data);
        emit sendUDPMessage(Da->value().toString());
    }

    QWidget *embeddedWidget() override
    {
        return widget;
    }

    QJsonObject save() const override
    {
        QJsonObject modelJson1;
//        modelJson1["Port"] = widget->Port->value();
//        modelJson1["IP"] = widget->IP->text();
        QJsonObject modelJson  = NodeDelegateModel::save();
        modelJson["values"]=modelJson1;
        return modelJson;
    }

    void load(const QJsonObject &p) override
    {
        QJsonValue v = p["values"];
        if (!v.isUndefined()&&v.isObject()) {
//            widget->IP->setText(v["IP"].toString());
//            widget->Port->setValue(v["Port"].toInt());

        }
    }

public slots:
//    收到信息时
    void recMsg(const QByteArray msg)
    {
        inData->insert("default",msg.toHex());
        Q_EMIT dataUpdated(0);
    }

//    void sendMessage(){
//
//
//        QString msg="";
//        emit sendUDPMessage(msg);
//    }


signals:
    void sendUDPMessage(const QString &message);
    void startUDPSocket(const QString &host,int port);
private:
    UDPSocketInterface *widget=new UDPSocketInterface();
    UdpSocket *client=new UdpSocket();
//    QVariant message;
    std::shared_ptr<VariableData> inData;

};
