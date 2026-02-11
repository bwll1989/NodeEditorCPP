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
    class UDPSocketDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(QString targetHost READ getTargetHost WRITE setTargetHost NOTIFY targetHostChanged)
        Q_PROPERTY(int targetPort READ getTargetPort WRITE setTargetPort NOTIFY targetPortChanged)
        Q_PROPERTY(QString value READ getValue WRITE setValue NOTIFY valueChanged)
        Q_PROPERTY(QString listeningHost READ getListeningHost WRITE setListeningHost NOTIFY listeningHostChanged)
        Q_PROPERTY(int listeningPort READ getListeningPort WRITE setListeningPort NOTIFY listeningPortChanged)

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
            
            connect(this, &UDPSocketDataModel::sendUDPMessage, client, &UdpSocket::sendMessage, Qt::QueuedConnection);
            connect(widget,&UDPSocketInterface::hostChanged,client, &UdpSocket::setHost,Qt::QueuedConnection);
            connect(client, &UdpSocket::recMsg, this, &UDPSocketDataModel::recMsg, Qt::QueuedConnection);
            
            // UI -> Setters
            connect(widget->targetHostEdit, &QLineEdit::editingFinished, this, [this](){
                setTargetHost(widget->targetHostEdit->text());
            });
            connect(widget->targetPortSpinBox, &IntDragValueWidget::valueChanged, this, &UDPSocketDataModel::setTargetPort);
            connect(widget->valueEdit, &QLineEdit::editingFinished, this, [this](){
                setValue(widget->valueEdit->text());
            });
            connect(widget->listeningHostEdit, &QLineEdit::editingFinished, this, [this](){
                setListeningHost(widget->listeningHostEdit->text());
            });
            connect(widget->listeningPortSpinBox, &QSpinBox::valueChanged, this, &UDPSocketDataModel::setListeningPort);
            
            connect(widget->sendButton, &QPushButton::clicked, this, [this]() {
                sendMessage();
            });
             AbstractDelegateModel::registerExternalControl("/send",widget->sendButton);
            AbstractDelegateModel::registerExternalControl("/targetHost",widget->targetHostEdit);
            AbstractDelegateModel::registerExternalControl("/targetPort",widget->targetPortSpinBox);
            AbstractDelegateModel::registerExternalControl("/value",widget->valueEdit);
            AbstractDelegateModel::registerExternalControl("/format",widget->format);
            // Initialize from widget defaults
            m_targetHost = widget->targetHostEdit->text();
            m_targetPort = widget->targetPortSpinBox->value();
            m_value = widget->valueEdit->text();
            m_listeningHost = widget->listeningHostEdit->text();
            m_listeningPort = widget->listeningPortSpinBox->value();
        }
        ~UDPSocketDataModel(){

            client->cleanup();
            delete client;
            //        delete clientThread;
            widget->deleteLater();
        }
        
        void afterModelReady() override {
            auto *bus = GlobalEventBus::instance();
            bus->subscribe(makeFullOscAddress("/targetHost"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/targetPort"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/value"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/listeningHost"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/listeningPort"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/send"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

    public:
        // Getters
        QString getTargetHost() const { return m_targetHost; }
        int getTargetPort() const { return m_targetPort; }
        QString getValue() const { return m_value; }
        QString getListeningHost() const { return m_listeningHost; }
        int getListeningPort() const { return m_listeningPort; }

        // Setters
        void setTargetHost(const QString &host) {
            if (m_targetHost == host) return;
            m_targetHost = host;
            if (widget && widget->targetHostEdit) {
                QSignalBlocker blocker(widget->targetHostEdit);
                widget->targetHostEdit->setText(host);
            }
            emit targetHostChanged(m_targetHost);
            AbstractDelegateModel::stateFeedBack("/targetHost", m_targetHost);
        }

        void setTargetPort(int port) {
            if (m_targetPort == port) return;
            m_targetPort = port;
            if (widget && widget->targetPortSpinBox) {
                QSignalBlocker blocker(widget->targetPortSpinBox);
                widget->targetPortSpinBox->setValue(port);
            }
            emit targetPortChanged(m_targetPort);
            AbstractDelegateModel::stateFeedBack("/targetPort", m_targetPort);
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
        
        void setListeningHost(const QString &host) {
            if (m_listeningHost == host) return;
            m_listeningHost = host;
            if (widget && widget->listeningHostEdit) {
                QSignalBlocker blocker(widget->listeningHostEdit);
                widget->listeningHostEdit->setText(host);
            }
            updateListening();
            emit listeningHostChanged(m_listeningHost);
            AbstractDelegateModel::stateFeedBack("/listeningHost", m_listeningHost);
        }

        void setListeningPort(int port) {
            if (m_listeningPort == port) return;
            m_listeningPort = port;
            if (widget && widget->listeningPortSpinBox) {
                QSignalBlocker blocker(widget->listeningPortSpinBox);
                widget->listeningPortSpinBox->setValue(port);
            }
            updateListening();
            emit listeningPortChanged(m_listeningPort);
            AbstractDelegateModel::stateFeedBack("/listeningPort", m_listeningPort);
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
                setTargetHost(m_inData->value().toString());
                break;
            case 1:
                setTargetPort(m_inData->value().toInt());
                break;
            case 2:
                setValue(m_inData->value().toString());
                sendMessage();
                break;
            case 3:
                // Trigger send with current value
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
            modelJson1["Listening host"] = m_listeningHost;
            modelJson1["Listening port"] = m_listeningPort;
            modelJson1["Target host"] = m_targetHost;
            modelJson1["Target port"] = m_targetPort;
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
                setListeningHost(v["Listening host"].toString());
                setListeningPort(v["Listening port"].toInt());
                setTargetHost(v["Target host"].toString());
                setTargetPort(v["Target port"].toInt());
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
            if (localPath == "targetHost") setTargetHost(ev.payload.toString());
            else if (localPath == "targetPort") setTargetPort(ev.payload.toInt());
            else if (localPath == "value") setValue(ev.payload.toString());
            else if (localPath == "listeningHost") setListeningHost(ev.payload.toString());
            else if (localPath == "listeningPort") setListeningPort(ev.payload.toInt());
            else if (localPath == "send") sendMessage();
        }
        
        void updateListening() {
             // For UDPSocket, hostChanged signal on widget triggered setHost on client.
             // We can now call it directly via invokeMethod or emit a signal.
             // The original code used widget->hostChanged signal connected to client->setHost.
             // We can just emit that signal if we want to reuse the connection, 
             // BUT we should control it ourselves.
             // Let's invoke setHost directly on client.
             QMetaObject::invokeMethod(client, "setHost", Qt::QueuedConnection, Q_ARG(QString, m_listeningHost), Q_ARG(int, m_listeningPort));
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
        
        void sendMessage() {
            emit sendUDPMessage(m_targetHost, m_targetPort, m_value, widget->format->currentIndex());
            AbstractDelegateModel::stateFeedBack("/send", true);
        }

    signals:
        void sendUDPMessage(const QString &host,const int &port,const QString &message,const int &format=0);
        void startUDPSocket(const QString &host,int port);
        
        void targetHostChanged(const QString &host);
        void targetPortChanged(int port);
        void valueChanged(const QString &value);
        void listeningHostChanged(const QString &host);
        void listeningPortChanged(int port);

    private:
        UDPSocketInterface *widget=new UDPSocketInterface();
        UdpSocket *client=new UdpSocket();
        //    QVariant message;
        std::shared_ptr<VariableData> m_inData;
        std::shared_ptr<VariableData> m_outData;
        
        QString m_targetHost = "127.0.0.1";
        int m_targetPort = 8080;
        QString m_value;
        QString m_listeningHost = "127.0.0.1";
        int m_listeningPort = 9090;
    };
}