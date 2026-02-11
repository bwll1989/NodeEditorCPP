#pragma once

#include <QtCore/QObject>
#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include "WebSocketClientInterface.hpp"
#include <iostream>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include "QGridLayout"
#include <QtCore/qglobal.h>
#include <QUrl>
#include <QSignalBlocker>
#include "Common/Devices/WebSocketClient/WebSocketClient.h"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;

using namespace NodeDataTypes;
namespace Nodes
{
    class WebSocketClientDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(QString url READ getUrl WRITE setUrl NOTIFY urlChanged)
        Q_PROPERTY(QString value READ getValue WRITE setValue NOTIFY valueChanged)
        Q_PROPERTY(bool connected READ getConnected WRITE setConnected NOTIFY connectedChanged)

    public:
        WebSocketClientDataModel() {
            InPortCount = 3;
            OutPortCount = 3;
            CaptionVisible = true;
            PortEditable = false;
            Caption = PLUGIN_NAME;
            WidgetEmbeddable = false;
            Resizable = false;
            m_inData = std::make_shared<VariableData>();
            m_outData = std::make_shared<VariableData>();
             AbstractDelegateModel::registerExternalControl("/host", widget->hostUrlEdit);
            AbstractDelegateModel::registerExternalControl("/value", widget->valueEdit);
            AbstractDelegateModel::registerExternalControl("/send", widget->send);
            AbstractDelegateModel::registerExternalControl("/connect", widget->statusButton);
            
            m_client = new WebSocketClient(this, QUrl(m_url));

            // UI Connections
            connect(widget->send, &QPushButton::clicked, this, [this]() {
                sendMessage();
            });

            connect(widget->hostUrlEdit, &QLineEdit::editingFinished, this, [this]() {
                setUrl(widget->hostUrlEdit->text());
            });
            
            connect(widget->valueEdit, &QLineEdit::editingFinished, this, [this]() {
                setValue(widget->valueEdit->text());
            });

            // Client Connections
            connect(this, &WebSocketClientDataModel::connectToUrl, m_client, &WebSocketClient::connectToServer, Qt::QueuedConnection);
            connect(this, &WebSocketClientDataModel::sendWSMessage, m_client, &WebSocketClient::sendMessage, Qt::QueuedConnection);
            
            connect(m_client, &WebSocketClient::recMsg, this, &WebSocketClientDataModel::recMsg, Qt::QueuedConnection);
            connect(m_client, &WebSocketClient::isReady, this, &WebSocketClientDataModel::onConnected, Qt::QueuedConnection);
        }

        ~WebSocketClientDataModel() {
        }

        QString getUrl() const { return m_url; }
        void setUrl(const QString& url) {
            if (m_url == url) return;
            m_url = url;
            {
                QSignalBlocker blocker(widget->hostUrlEdit);
                widget->hostUrlEdit->setText(m_url);
            }
            emit urlChanged(m_url);
            AbstractDelegateModel::stateFeedBack("/url", m_url);
            emit connectToUrl(QUrl(m_url));
        }

        QString getValue() const { return m_value; }
        void setValue(const QString& value) {
            if (m_value == value) return;
            m_value = value;
            {
                QSignalBlocker blocker(widget->valueEdit);
                widget->valueEdit->setText(m_value);
            }
            emit valueChanged(m_value);
            AbstractDelegateModel::stateFeedBack("/value", m_value);
        }

        bool getConnected() const { return m_connected; }
        void setConnected(bool connected) {
            if (m_connected == connected) return;
            m_connected = connected;
            
            // Update UI status
            QSignalBlocker blocker(widget->statusButton);
            widget->statusButton->setChecked(m_connected);
            widget->statusButton->setText(m_connected ? "Connected" : "Disconnected");
            widget->statusButton->setStyleSheet(m_connected ? "color: green; font-weight: bold;" : "color: red; font-weight: bold;");
            widget->send->setEnabled(m_connected);
            
            emit connectedChanged(m_connected);
            AbstractDelegateModel::stateFeedBack("/connect", m_connected);
        }

        void afterModelReady() override {
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/url"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/host"), this, SLOT(onGlobalEvent(GlobalEvent))); // Alias for url
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/value"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(AbstractDelegateModel::makeFullOscAddress("/send"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                switch (portIndex) {
            case 0: return "URL";
            case 1: return "VALUE";
            case 2: return "TRIGGER";
            default: break;
                }
            case PortType::Out:
                switch (portIndex)
                {
            case 0: return "RESULT";
            case 1: return "URL";
            case 2: return "STRING";
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
            switch (portIndex)
            {
            case 0: return m_outData;
            case 1: return std::make_shared<VariableData>(m_url);
            case 2: return std::make_shared<VariableData>(m_outData->value());
            default: return nullptr;
            }
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (data == nullptr) return;
            m_inData = std::dynamic_pointer_cast<VariableData>(data);
            switch (portIndex) {
            case 0:
                setUrl(m_inData->value().toString());
                break;
            case 1:
                setValue(m_inData->value().toString());
                sendMessage();
                break;
            case 2:
                sendMessage(); // Just trigger
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
            modelJson1["URL"] = m_url;
            modelJson1["Value"] = m_value;
            modelJson1["Format"] = widget->format->currentIndex();
            modelJson1["MessageType"] = widget->messageType->currentIndex();
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["values"] = modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined() && v.isObject()) {
                setUrl(v["URL"].toString());
                setValue(v["Value"].toString());
                widget->format->setCurrentIndex(v["Format"].toInt());
                widget->messageType->setCurrentIndex(v["MessageType"].toInt());
            }
        }

    public slots:
        void sendMessage() {
            emit sendWSMessage(m_value, widget->messageType->currentIndex(), widget->format->currentIndex());
            AbstractDelegateModel::stateFeedBack("/send", true);
        }

        void recMsg(const QVariantMap &msg)
        {
            m_outData = std::make_shared<VariableData>(msg);
            Q_EMIT dataUpdated(0);
            Q_EMIT dataUpdated(1);
            Q_EMIT dataUpdated(2);
        }

        void onConnected(bool isReady) {
            setConnected(isReady);
        }

        void onGlobalEvent(const GlobalEvent& ev) {
            if (ev.kind == GlobalEventKind::Command) {
                QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
                if (localPath == "url" || localPath == "host") setUrl(ev.payload.toString());
                else if (localPath == "value") setValue(ev.payload.toString());
                else if (localPath == "send") sendMessage();
                // /connected is usually read-only for client status, but maybe we want to force disconnect/connect?
                // For now, let's say it reflects status.
            }
        }

    signals:
        void urlChanged(QString url);
        void valueChanged(QString value);
        void connectedChanged(bool connected);
        
        void connectToUrl(const QUrl &url);
        void sendWSMessage(const QString &msg, const int &messageType = 0, const int &format = 0);

    private:
        WebSocketClientInterface *widget = new WebSocketClientInterface();
        WebSocketClient *m_client;
        std::shared_ptr<VariableData> m_inData;
        std::shared_ptr<VariableData> m_outData;

        QString m_url = "ws://echo.websocket.org";
        QString m_value;
        bool m_connected = false;
    };
}
