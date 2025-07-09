#pragma once


#include <QtCore/QObject>

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include "TCPClientInterface.hpp"
#include <iostream>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include "QGridLayout"
#include <QtCore/qglobal.h>
#include <QThread>
#include "Common/Devices/TcpClient/TcpClient.h"
#include "QMutex"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;

using namespace NodeDataTypes;
namespace Nodes
{
    class TCPClientDataModel : public NodeDelegateModel
    {
        Q_OBJECT

    public:

        TCPClientDataModel(){
            // 在新线程中启动服务器
            InPortCount =4;
            OutPortCount=4;
            CaptionVisible=true;
            PortEditable=false;
            Caption=PLUGIN_NAME;
            WidgetEmbeddable= false;
            Resizable=false;
            m_inData=std::make_shared<VariableData>();
            m_outData=std::make_shared<VariableData>();
            NodeDelegateModel::registerOSCControl("/host",widget->hostEdit);
            NodeDelegateModel::registerOSCControl("/port",widget->portSpinBox);
            NodeDelegateModel::registerOSCControl("/value",widget->valueEdit);
            NodeDelegateModel::registerOSCControl("/send",widget->send);
            client->moveToThread(clientThread);

            connect(this,&TCPClientDataModel::sendTCPMessage, client, &TcpClient::sendMessage, Qt::QueuedConnection);
            connect(widget->hostEdit,&QLineEdit::editingFinished,this,&TCPClientDataModel::hostChange,Qt::QueuedConnection);
            connect(widget->portSpinBox,&QSpinBox::valueChanged,this,&TCPClientDataModel::hostChange,Qt::QueuedConnection);
            connect(this,&TCPClientDataModel::connectTCPServer,client,&TcpClient::connectToServer,Qt::QueuedConnection);
            connect(client,&TcpClient::isReady,widget->send,&QPushButton::setEnabled,Qt::QueuedConnection);
            connect(client,&TcpClient::recMsg,this,&TCPClientDataModel::recMsg,Qt::QueuedConnection);
            connect(widget->send, &QPushButton::clicked, this,[this]()
            {
                 emit sendTCPMessage(widget->valueEdit->text());
            },Qt::QueuedConnection);
            clientThread->start();
        }
        ~TCPClientDataModel(){
            emit stopTCPClient();
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

            Q_UNUSED(portIndex)
            Q_UNUSED(portType)
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

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (data == nullptr) {
                return;
            }
            m_inData = std::dynamic_pointer_cast<VariableData>(data);
            switch (portIndex) {
            case 0:
                widget->hostEdit->setText(m_inData->value().toString());
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

        }

        QWidget *embeddedWidget() override
        {
            return widget;
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["Port"] = widget->portSpinBox->value();
            modelJson1["IP"] = widget->hostEdit->text();
            modelJson1["Value"] = widget->valueEdit->text();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                widget->hostEdit->setText(v["IP"].toString());
                widget->portSpinBox->setValue(v["Port"].toInt());
                widget->valueEdit->setText(v["Value"].toString());

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
            emit sendTCPMessage(m_inData->value().toString());
        }

        void hostChange()
        {
            emit connectTCPServer(widget->hostEdit->text(),widget->portSpinBox->value());
        }
    signals:
        //    关闭信号
        void stopTCPClient();
        void sendTCPMessage(const QString &message);
        void connectTCPServer(const QString &host,int port);
    private:
        TCPClientInterface *widget=new TCPClientInterface();
        TcpClient *client=new TcpClient();
        QThread *clientThread=new QThread();
        std::shared_ptr<VariableData> m_inData;
        std::shared_ptr<VariableData> m_outData;
    };
}