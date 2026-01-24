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
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
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
        Q_PROPERTY(QString host READ getHost WRITE setHost NOTIFY hostChanged)
        Q_PROPERTY(int port READ getPort WRITE setPort NOTIFY portChanged)
        Q_PROPERTY(QString value READ getValue WRITE setValue NOTIFY valueChanged)

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
            
            // Connect UI to Setters
            connect(widget->hostLineEdit, &QLineEdit::editingFinished, this, [this](){
                setHost(widget->hostLineEdit->text());
            });
            connect(widget->portSpinBox, &QSpinBox::valueChanged, this, &TCPServerDataModel::setPort);
            connect(widget->valueEdit, &QLineEdit::textChanged, this, &TCPServerDataModel::setValue); // Use textChanged for value or editingFinished? textChanged is more immediate but high freq. editingFinished is safer. 
            // Previous code for TCPClient used editingFinished for host, but value?
            // TCPClient registered external control for valueEdit.
            // Let's use editingFinished for value to be consistent with input fields usually.
            disconnect(widget->valueEdit, &QLineEdit::textChanged, this, &TCPServerDataModel::setValue); // Just in case
            connect(widget->valueEdit, &QLineEdit::editingFinished, this, [this](){
                setValue(widget->valueEdit->text());
            });

            connect(widget->sendButton, &QPushButton::clicked, this, &TCPServerDataModel::sendMessage, Qt::QueuedConnection);
            
            // Initial sync
            m_host = widget->hostLineEdit->text();
            m_port = widget->portSpinBox->value();
            m_value = widget->valueEdit->text();
        }
        ~TCPServerDataModel(){
            server->cleanup();
        }
        
        void afterModelReady() override {
            auto *bus = GlobalEventBus::instance();
            bus->subscribe(makeFullOscAddress("/host"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/port"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/value"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/send"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

    public:
        // Getters
        QString getHost() const { return m_host; }
        int getPort() const { return m_port; }
        QString getValue() const { return m_value; }

        // Setters
        void setHost(const QString &host) {
            if (m_host == host) return;
            m_host = host;
            
            if (widget && widget->hostLineEdit) {
                QSignalBlocker blocker(widget->hostLineEdit);
                widget->hostLineEdit->setText(host);
            }
            
            updateServer();
            emit hostChanged(m_host);
            AbstractDelegateModel::stateFeedBack("/host", m_host);
        }

        void setPort(int port) {
            if (m_port == port) return;
            m_port = port;
            
            if (widget && widget->portSpinBox) {
                QSignalBlocker blocker(widget->portSpinBox);
                widget->portSpinBox->setValue(port);
            }
            
            updateServer();
            emit portChanged(m_port);
            AbstractDelegateModel::stateFeedBack("/port", m_port);
        }

        void setValue(const QString &val) {
            if (m_value == val) return;
            m_value = val;
            
            if (widget && widget->valueEdit) {
                QSignalBlocker blocker(widget->valueEdit);
                widget->valueEdit->setText(val);
            }
            
            emit valueChanged(m_value);
            AbstractDelegateModel::stateFeedBack("/value", m_value);
        }

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
                    setHost(m_inData->value().toString());
                    break;
                case 1:
                    setPort(m_inData->value().toInt());
                    break;
                case 2:
                    setValue(m_inData->value().toString());
                    sendMessage();
                    break;
                case 3:
                    if (m_inData->value().toBool()) {
                        setValue(m_inData->value().toString());
                        sendMessage();
                    }
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
            modelJson1["Host"] = m_host;
            modelJson1["Value"] = m_value;
            modelJson1["Format"] = widget->format->currentIndex();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                setPort(v["Port"].toInt());
                setHost(v["Host"].toString());
                setValue(v["Value"].toString());
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

    private Q_SLOTS:
        void onGlobalEvent(const GlobalEvent& ev) {
            if (ev.kind != GlobalEventKind::Command) return;
            QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
            if (localPath == "host") setHost(ev.payload.toString());
            else if (localPath == "port") setPort(ev.payload.toInt());
            else if (localPath == "value") setValue(ev.payload.toString());
            else if (localPath == "send") sendMessage();
        }
        
        void updateServer() {
             QMetaObject::invokeMethod(server, "setHost", Qt::QueuedConnection, Q_ARG(QString, m_host), Q_ARG(int, m_port));
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
            server->sendMessage(m_value, widget->format->currentIndex());
            AbstractDelegateModel::stateFeedBack("/send", true);
        }

    signals:
        //    关闭信号
        void stopTCPServer();
        void hostChanged(const QString &host);
        void portChanged(int port);
        void valueChanged(const QString &value);
        
    private:
        TCPServerInterface *widget=new TCPServerInterface();
        TcpServer *server;
        std::shared_ptr<VariableData> m_inData;
        std::shared_ptr<VariableData> m_outData;
        
        QString m_host;
        int m_port = 8080;
        QString m_value;
    };
}