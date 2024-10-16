#pragma once


#include <QtCore/QObject>
#include <QHostAddress>
#include "Nodes/NodeDataList.hpp"
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
        // 在新线程中启动服务器
        InPortCount =1;
        OutPortCount=1;
        PortEditable=true;
        CaptionVisible=true;
        Caption=PLUGIN_NAME;
        WidgetEmbeddable= false;
        Resizable=false;
        client->moveToThread(clientThread);
        connect(widget->send, &QPushButton::clicked, this,&UDPSocketDataModel::sendMessage,Qt::QueuedConnection);
        connect(this, &UDPSocketDataModel::sendUDPMessage, client, &UdpSocket::sendMessage, Qt::QueuedConnection);
        connect(widget->IP,&QLineEdit::editingFinished,this,&UDPSocketDataModel::hostChange,Qt::QueuedConnection);
        connect(widget->Port,&QSpinBox::valueChanged,this,&UDPSocketDataModel::hostChange,Qt::QueuedConnection);
        connect(client, &UdpSocket::recMsg, this, &UDPSocketDataModel::recMsg, Qt::QueuedConnection);
        connect(client, &UdpSocket::isReady, widget->send, &QPushButton::setEnabled, Qt::QueuedConnection);
        connect(this, &UDPSocketDataModel::startUDPSocket, client, &UdpSocket::startSocket, Qt::QueuedConnection);
        clientThread->start();
    }
    ~UDPSocketDataModel(){
        emit stopUDPSocket();
        if(clientThread->isRunning()){
            clientThread->quit();
            clientThread->wait();
        }

        delete client;
        delete clientThread;

        widget->deleteLater();
    }
public:

    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        QString in = "➩";
        QString out = "➩";
        switch (portType) {
            case PortType::In:
                return in;
            case PortType::Out:
                return out;
            default:
                break;
        }
        return "";
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
                        return VariantData().type();
                }
                break;
            case PortType::Out:
                return VariantData().type();
                break;

            case PortType::None:
                break;
        }
        return VariantData().type();
    }

    std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
    {
        Q_UNUSED(portIndex)
        return std::make_shared<VariantData>(message);
    }

    void setInData(std::shared_ptr<NodeData>, PortIndex const) override {}

    QWidget *embeddedWidget() override
    {
        return widget;}

    QJsonObject save() const override
    {
        QJsonObject modelJson1;
        modelJson1["Port"] = widget->Port->value();
        modelJson1["IP"] = widget->IP->text();
        QJsonObject modelJson  = NodeDelegateModel::save();
        modelJson["values"]=modelJson1;
        return modelJson;
    }

    void load(const QJsonObject &p) override
    {
        QJsonValue v = p["values"];
        if (!v.isUndefined()&&v.isObject()) {
            widget->IP->setText(v["IP"].toString());
            widget->Port->setValue(v["Port"].toInt());

        }
    }

public slots:
//    收到信息时
    void recMsg(const QString &msg)
    {
        widget->receiveBox->append(msg);
        widget->receiveBox->update();
        message.setValue(msg);
        Q_EMIT dataUpdated(0);
    }

    void sendMessage(){

        QString msg=widget->sendBox->text();

        emit sendUDPMessage(msg);
    }

    void hostChange()
    {
        emit startUDPSocket(widget->IP->text(),widget->Port->value());
    }
signals:
//    关闭信号
    void stopUDPSocket();
    void sendUDPMessage(const QString &message);
    void startUDPSocket(const QString &host,int port);
private:
    UDPSocketInterface *widget=new UDPSocketInterface();
    UdpSocket *client=new UdpSocket();
    QThread *clientThread=new QThread();
    QVariant message;
};
