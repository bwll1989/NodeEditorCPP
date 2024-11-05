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
#include "QMutex"
#include "Common/Devices/TcpServer/TcpServer.h"
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;



class TCPServerDataModel : public NodeDelegateModel
{
    Q_OBJECT

public:

    TCPServerDataModel(){
        InPortCount =1;
        OutPortCount=1;
        CaptionVisible=true;
        PortEditable=true;
        Caption="TCP Server";
        WidgetEmbeddable= false;
        Resizable=false;
        server=new TcpServer();
        server->moveToThread(serverThread);

        connect(server, &TcpServer::serverError, this, &TCPServerDataModel::recMsg, Qt::QueuedConnection);
        connect(server, &TcpServer::serverMessage, this, &TCPServerDataModel::recMsg, Qt::QueuedConnection);
        connect(widget->Port, &QSpinBox::valueChanged, server, &TcpServer::setPort, Qt::QueuedConnection);
        connect(this, &TCPServerDataModel::stopTCPServer, server, &TcpServer::stopServer, Qt::QueuedConnection);

        connect(server, &TcpServer::clientInserted, this, &TCPServerDataModel::insertClient, Qt::QueuedConnection);
        connect(server, &TcpServer::clientRemoved, this, &TCPServerDataModel::removeClient, Qt::QueuedConnection);
        connect(widget->send, &QPushButton::clicked, this, &TCPServerDataModel::sendMessage, Qt::QueuedConnection);
        connect(this, &TCPServerDataModel::sendTCPMessage, server, &TcpServer::sendMessage, Qt::QueuedConnection);

        serverThread->start();

    }
    ~TCPServerDataModel(){
            emit stopTCPServer();
            if(serverThread->isRunning()){
                serverThread->quit();
                serverThread->wait();
            }
            delete server;
            delete serverThread;

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
        return widget;
    }


    QJsonObject save() const override
    {
        QJsonObject modelJson1;
        modelJson1["Port"] = widget->Port->value();
        QJsonObject modelJson  = NodeDelegateModel::save();
        modelJson["values"]=modelJson1;
        return modelJson;
    }

    void load(const QJsonObject &p) override
    {
        QJsonValue v = p["values"];
        if (!v.isUndefined()&&v.isObject()) {
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
//    新增连接
    void insertClient(const QString &message)
    {
        widget->clientList->addItem(message);
    }
//    连接关闭
    void removeClient(const QString &message){
        int index = widget->clientList->findText(message);
        if (index != -1) {
            widget->clientList->removeItem(index);
        }
    }
    void sendMessage(){

        QString msg=widget->sendBox->text();
        QString client=widget->clientList->currentText();
        emit sendTCPMessage(client,msg);
    }

signals:
//    关闭信号
    void stopTCPServer();
    void sendTCPMessage(const QString &client,const QString &message);
private:
    TCPServerInterface *widget=new TCPServerInterface();
    TcpServer *server;
    QThread *serverThread=new QThread();
    QVariant message;
};
