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

#include "ConstantDefines.h"
#include "Common/Devices/TcpClient/TcpClient.h"
#include "QMutex"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;

using namespace NodeDataTypes;
using namespace QtNodes;
namespace Nodes
{
    class TCPClientDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(QString host READ getHost WRITE setHost NOTIFY hostChanged)
        Q_PROPERTY(int port READ getPort WRITE setPort NOTIFY portChanged)
        Q_PROPERTY(QString value READ getValue WRITE setValue NOTIFY valueChanged)
        Q_PROPERTY(bool connected READ getConnected WRITE setConnected NOTIFY connectedChanged)

    public:
        TCPClientDataModel()
        {
            // 在新线程中启动服务器
            InPortCount = 4;
            OutPortCount = 4;
            CaptionVisible = true;
            PortEditable = false;
            Caption = PLUGIN_NAME;
            WidgetEmbeddable = false;
            Resizable = false;
            m_inData = std::make_shared<VariableData>();
            m_outData = std::make_shared<VariableData>();
             AbstractDelegateModel::registerExternalControl("/host", widget->hostEdit);
            AbstractDelegateModel::registerExternalControl("/port", widget->portSpinBox);
            AbstractDelegateModel::registerExternalControl("/value", widget->valueEdit);
            AbstractDelegateModel::registerExternalControl("/send", widget->send);
            AbstractDelegateModel::registerExternalControl("/connect", widget->statusButton);

            // UI Connections
            connect(widget->hostEdit, &QLineEdit::editingFinished, this, [this]() {
                setHost(widget->hostEdit->text());
            });

            connect(widget->portSpinBox, &QSpinBox::valueChanged, this, [this](int val) {
                setPort(val);
            });

            connect(widget->valueEdit, &QLineEdit::editingFinished, this, [this]() {
                setValue(widget->valueEdit->text());
            });

            connect(widget->send, &QPushButton::clicked, this, [this]() {
                sendMessage();
            });

            // Client Connections
            connect(this, &TCPClientDataModel::sendTCPMessage, client, &TcpClient::sendMessage, Qt::QueuedConnection);
            connect(this, &TCPClientDataModel::connectTCPServer, client, &TcpClient::connectToServer, Qt::QueuedConnection);
            connect(this, &TCPClientDataModel::disconnectTCPServer, client, &TcpClient::disconnectFromServer, Qt::QueuedConnection);
            connect(this, &TCPClientDataModel::stopTCPClient, client, &TcpClient::stopTimer, Qt::QueuedConnection);

            connect(client, &TcpClient::isReady, this, [this](bool isReady) {
                if (m_connected != isReady) {
                    m_connected = isReady;
                    emit connectedChanged(m_connected);
                }
                
                // Update UI (Display Status Only)
                QSignalBlocker blocker(widget->statusButton);
                widget->statusButton->setText(isReady ? "Connected" : "Disconnect");
                widget->statusButton->setChecked(isReady);
                widget->statusButton->setStyleSheet(isReady ? "color: green; font-weight: bold;" : "color: red; font-weight: bold;");
                widget->send->setEnabled(isReady);
                AbstractDelegateModel::stateFeedBack("/connected", m_connected);
            }, Qt::QueuedConnection);

            connect(client, &TcpClient::recMsg, this, &TCPClientDataModel::recMsg, Qt::QueuedConnection);
            
            // widget->statusButton is display-only, no click handler to toggle connection.
        }

        ~TCPClientDataModel()
        {
            if (client) {
                client->disconnectFromServer();
                delete client;
                client = nullptr;
            }
        }

        QString getHost() const { return m_host; }
        void setHost(const QString& host) {
            if (m_host == host) return;
            m_host = host;
            {
            QSignalBlocker blocker(widget->hostEdit);
            widget->hostEdit->setText(m_host);
            }
            emit connectTCPServer(m_host,m_port);
            emit hostChanged(m_host);
            AbstractDelegateModel::stateFeedBack("/host", m_host);

        }

        int getPort() const { return m_port; }
        void setPort(int port) {
            if (m_port == port) return;
            m_port = port;
            {
                QSignalBlocker blocker(widget->portSpinBox);
                widget->portSpinBox->setValue(m_port);
            }
            emit connectTCPServer(m_host,m_port);
            emit portChanged(m_port);
            AbstractDelegateModel::stateFeedBack("/port", m_port);

        }

        QString getValue() const { return m_value; }
        void setValue(const QString& value) {
            if (m_value == value) return;
            m_value = value;
            QSignalBlocker blocker(widget->valueEdit);
            widget->valueEdit->setText(m_value);
            emit valueChanged(m_value);
            AbstractDelegateModel::stateFeedBack("/value", m_value);
        }

        bool getConnected() const { return m_connected; }
        void setConnected(bool connected) {
            // // Even if m_connected is already same, we might want to enforce connection state
            // // if it was disconnected externally or something. But for now, standard check.
            // if (m_connected == connected && client->isConnected() == connected) return;
            //
            // if (connected) {
            //     emit connectTCPServer(m_host, m_port);
            // } else {
            //     emit disconnectTCPServer();
            // }
        }



        void afterModelReady() override {
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/host"), this,SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/port"), this,SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/value"), this,SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/connected"), this,SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/send"), this,SLOT(onGlobalEvent(GlobalEvent)));
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                switch (portIndex) {
                case 0: return "HOST";
                case 1: return "PORT";
                case 2: return "VALUE";
                case 3: return "TRIGGER";
                default: break;
                }
            case PortType::Out:
                switch (portIndex) {
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
            Q_UNUSED(portIndex)
            Q_UNUSED(portType)
            return VariableData().type();
        }

        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            switch (portIndex) {
            case 0: return m_outData;
            case 1: return std::make_shared<VariableData>(m_host);
            case 2: return std::make_shared<VariableData>(m_value);
            case 3: return std::make_shared<VariableData>(m_outData->value("hex"));
            default: return nullptr;
            }
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (data == nullptr) return;
            m_inData = std::dynamic_pointer_cast<VariableData>(data);
            switch (portIndex) {
            case 0: setHost(m_inData->value().toString()); break;
            case 1: setPort(m_inData->value().toInt()); break;
            case 2: 
                setValue(m_inData->value().toString()); 
                sendMessage(); 
                break;
            case 3:
                if (m_inData->value().toBool()) {
                    setValue(widget->valueEdit->text());
                    sendMessage();
                }
                break;
            default: break;
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
            modelJson1["IP"] = m_host;
            modelJson1["Value"] = m_value;
            modelJson1["Format"] = widget->format->currentIndex();
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["values"] = modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined() && v.isObject()) {
                setHost(v["IP"].toString());
                setPort(v["Port"].toInt());
                setValue(v["Value"].toString());
                widget->format->setCurrentIndex(v["Format"].toInt());
            }
        }

        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex index) const override {
            auto result = ConnectionPolicy::One;
            switch (portType) {
                case PortType::In: result = ConnectionPolicy::Many; break;
                case PortType::Out: result = ConnectionPolicy::Many; break;
                case PortType::None: break;
            }
            return result;
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

        void sendMessage() {
            emit sendTCPMessage(m_value, widget->format->currentIndex());
            AbstractDelegateModel::stateFeedBack("/send", true);
        }
        void onGlobalEvent(const GlobalEvent& ev) {
            if (ev.kind == GlobalEventKind::Command) {
                QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
                if (localPath == "host") setHost(ev.payload.toString());
                else if (localPath == "port") setPort(ev.payload.toInt());
                else if (localPath == "value") setValue(ev.payload.toString());
                else if (localPath == "connected") setConnected(ev.payload.toBool());
                else if (localPath == "send") sendMessage();
            }
        }
    signals:
        void hostChanged(QString host);
        void portChanged(int port);
        void valueChanged(QString value);
        void connectedChanged(bool connected);
        
        void stopTCPClient();
        void sendTCPMessage(const QString &message, const int &format = 0);
        void connectTCPServer(const QString &host, int port);
        void disconnectTCPServer();

    private:
        TCPClientInterface *widget = new TCPClientInterface();
        TcpClient *client = new TcpClient();
        std::shared_ptr<VariableData> m_inData;
        std::shared_ptr<VariableData> m_outData;
        
        QString m_host = "127.0.0.1";
        int m_port = 8080;
        QString m_value;
        bool m_connected = false;
    };
}
