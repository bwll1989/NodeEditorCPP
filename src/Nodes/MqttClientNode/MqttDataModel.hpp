#pragma once

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include "QTimer"
#include <iostream>
#include <QtCore/qglobal.h>
#include "MqttInterface.hpp"
#include "ConstantDefines.h"
#include "OSCSender/OSCSender.h"
#include "Common/Devices/MqttClient/MqttClient.h"
#include "OSCMessage.h"
#include <QtWidgets/QSpinBox>
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
using namespace QtNodes;
namespace Nodes
{
    class MqttDataModel : public NodeDelegateModel
    {
        Q_OBJECT

    public:
        MqttDataModel()
        {
            InPortCount = 5;
            OutPortCount = 3;
            CaptionVisible = true;
            Caption = "Mqtt Client";
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = false;
            m_outData = std::make_shared<VariableData>();

            NodeDelegateModel::registerOSCControl("/host", widget->hostEdit);
            NodeDelegateModel::registerOSCControl("/port", widget->portSpinBox);
            NodeDelegateModel::registerOSCControl("/username", widget->usernameEdit);
            NodeDelegateModel::registerOSCControl("/password", widget->passwordEdit);
            NodeDelegateModel::registerOSCControl("/topic", widget->topicEdit);
            NodeDelegateModel::registerOSCControl("/payload", widget->payloadEdit);
            NodeDelegateModel::registerOSCControl("/publish", widget->publishButton);
            NodeDelegateModel::registerOSCControl("/connect", widget->statusButton);

            connect(widget->hostEdit, &QLineEdit::editingFinished, this, &MqttDataModel::connectChange, Qt::QueuedConnection);
            connect(widget->portSpinBox, qOverload<int>(&QSpinBox::valueChanged), this, &MqttDataModel::connectChange, Qt::QueuedConnection);
            connect(widget->usernameEdit, &QLineEdit::editingFinished, this, &MqttDataModel::connectChange, Qt::QueuedConnection);
            connect(widget->passwordEdit, &QLineEdit::editingFinished, this, &MqttDataModel::connectChange, Qt::QueuedConnection);
            connect(widget->topicEdit, &QLineEdit::editingFinished, this, &MqttDataModel::topicChanged, Qt::QueuedConnection);

            connect(widget->publishButton, &QPushButton::clicked, this, &MqttDataModel::publishMessage, Qt::QueuedConnection);

            client = new MqttClient();
            connect(client, &MqttClient::isConnectedChanged, this, [this](bool ready){
                widget->statusButton->setEnabled(true);
                widget->statusButton->setText(ready ? "Connected" : "Disconnect");
                widget->statusButton->setChecked(ready);
                widget->statusButton->setStyleSheet(ready ? "color: green; font-weight: bold;" : "color: red; font-weight: bold;");
                if (ready) {
                    // 自动订阅当前主题
                    client->subscribe(widget->topicEdit->text(), widget->qosCombo->currentData().toInt());
                }
            }, Qt::QueuedConnection);
            connect(client, &MqttClient::messageReceived, this, &MqttDataModel::onMessage, Qt::QueuedConnection);
            connect(client, &MqttClient::errorOccurred, this, &MqttDataModel::onError, Qt::QueuedConnection);
        }

        ~MqttDataModel() override
        {
            if (client) {
                client->disconnectFromHost();
                delete client;
                client = nullptr;
            }
        }

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            Q_UNUSED(portType)
            return VariableData().type();
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch(portType)
            {
            case PortType::In:
                switch(portIndex)
                {
                case 0:
                    return "HOST";
                case 1:
                    return "PORT";
                case 2:
                    return "TOPIC";
                case 3:
                    return "VALUE";
                case 4:
                    return "TRIGGER";
                default:
                    return "";
                }
            case PortType::Out:
                switch (portIndex) {
                case 0: return "RESULT";
                case 1: return "TOPIC";
                case 2: return "VALUE";
                default: return "";
                }
            default:
                return "";
            }
        }

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            switch (port) {
            case 0: return m_outData;
            case 1: return std::make_shared<VariableData>(m_outData->value("topic").toString());
            case 2: return std::make_shared<VariableData>(m_outData->value());
            default: return nullptr;
            }
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (data == nullptr) {
                return;
            }

            auto v = std::dynamic_pointer_cast<VariableData>(data);
            if (!v) return;
            switch (portIndex) {
            case 0:
                widget->hostEdit->setText(v->value().toString());
                connectChange();
                break;
            case 1:
                widget->portSpinBox->setValue(v->value().toInt());
                connectChange();
                break;
            case 2:
                widget->topicEdit->setText(v->value().toString());
                topicChanged();
                break;
            case 3:
                widget->payloadEdit->setText(v->value().toString());
                publishMessage();
                break;
            case 4:
                publishMessage();
                break;
            default: break;
            }
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["host"] = widget->hostEdit->text();
            modelJson1["port"] = widget->portSpinBox->value();
            modelJson1["username"] = widget->usernameEdit->text();
            modelJson1["password"] = widget->passwordEdit->text();
            modelJson1["topic"] = widget->topicEdit->text();
            modelJson1["payload"] = widget->payloadEdit->text();
            modelJson1["qos"] = widget->qosCombo->currentIndex();
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["values"] = modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined() && v.isObject()) {
                widget->hostEdit->setText(v["host"].toString());
                widget->portSpinBox->setValue(v["port"].toInt());
                widget->usernameEdit->setText(v["username"].toString());
                widget->passwordEdit->setText(v["password"].toString());
                widget->topicEdit->setText(v["topic"].toString());
                widget->payloadEdit->setText(v["payload"].toString());
                widget->qosCombo->setCurrentIndex(v["qos"].toInt());
                connectChange();
            }
        }

        QWidget *embeddedWidget() override
        {
            return widget;
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

        void stateFeedBack(const QString& oscAddress,QVariant value) override {

            OSCMessage message;
            message.host = AppConstants::EXTRA_FEEDBACK_HOST;
            message.port = AppConstants::EXTRA_FEEDBACK_PORT;
            message.address = "/dataflow/" + getParentAlias() + "/" + QString::number(getNodeID()) + oscAddress;
            message.value = value;
            OSCSender::instance()->sendOSCMessageWithQueue(message);
        }
    public slots:
        void connectChange()
        {
            if (!client) return;
            client->connectToHost(widget->hostEdit->text(),
                                  widget->portSpinBox->value(),
                                  widget->usernameEdit->text(),
                                  widget->passwordEdit->text());
        }

        void topicChanged()
        {
            if (!client) return;
            client->subscribe(widget->topicEdit->text(), widget->qosCombo->currentData().toInt());
        }

        void publishMessage()
        {
            if (!client) return;
            client->publish(widget->topicEdit->text(),
                            widget->payloadEdit->text(),
                            widget->qosCombo->currentData().toInt(),
                            false);
        }

        void onMessage(const QString &topic, const QByteArray &payload)
        {
            QVariantMap m;
            m.insert("topic", topic);
            m.insert("default", QString::fromUtf8(payload));
            // hex
            QByteArray hex = payload.toHex(' ');
            m.insert("hex", QString::fromLatin1(hex));
            m_outData = std::make_shared<VariableData>(m);
            Q_EMIT dataUpdated(0);
            Q_EMIT dataUpdated(1);
            Q_EMIT dataUpdated(2);
        }

        void onError(const QString &err)
        {
            stateFeedBack("/error", err);
        }

    private:
        MqttInterface *widget = new MqttInterface();
        MqttClient *client = nullptr;
        std::shared_ptr<VariableData> m_outData;
    };
}
