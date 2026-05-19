#pragma once

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include "QTimer"
#include <iostream>
#include <QtCore/qglobal.h>
#include "MqttInterface.hpp"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
#include "Common/Devices/MqttClient/MqttClient.h"
#include <QtWidgets/QSpinBox>
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include <QSignalBlocker>

using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
using namespace QtNodes;
namespace Nodes
{
    class MqttDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(QString host READ getHost WRITE setHost NOTIFY hostChanged)
        Q_PROPERTY(int port READ getPort WRITE setPort NOTIFY portChanged)
        Q_PROPERTY(QString username READ getUsername WRITE setUsername NOTIFY usernameChanged)
        Q_PROPERTY(QString password READ getPassword WRITE setPassword NOTIFY passwordChanged)
        Q_PROPERTY(QString topic READ getTopic WRITE setTopic NOTIFY topicChangedProp)
        Q_PROPERTY(QString payload READ getPayload WRITE setPayload NOTIFY payloadChanged)
        
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

            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "host";
                b.control = widget->hostEdit;
                AbstractDelegateModel::registerExternalBinding("/host", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "port";
                b.control = widget->portSpinBox;
                AbstractDelegateModel::registerExternalBinding("/port", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "topic";
                b.control = widget->topicEdit;
                AbstractDelegateModel::registerExternalBinding("/topic", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "payload";
                b.control = widget->payloadEdit;
                AbstractDelegateModel::registerExternalBinding("/payload", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "publish";
                b.control = widget->publishButton;
                AbstractDelegateModel::registerExternalBinding("/publish", this, b);
            }
            // AbstractDelegateModel::registerExternalControl("/publish", widget->publishButton);
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "connect";
                b.control = widget->statusButton;
                AbstractDelegateModel::registerExternalBinding("/connect", this, b);
            }

            // AbstractDelegateModel::registerExternalControl("/connect", widget->statusButton);

            connect(widget->hostEdit, &QLineEdit::editingFinished, this, [this](){ setHost(widget->hostEdit->text()); });
            connect(widget->portSpinBox, &IntDragValueWidget::valueChanged, this, &MqttDataModel::setPort);
            connect(widget->usernameEdit, &QLineEdit::editingFinished, this, [this](){ setUsername(widget->usernameEdit->text()); });
            connect(widget->passwordEdit, &QLineEdit::editingFinished, this, [this](){ setPassword(widget->passwordEdit->text()); });
            connect(widget->topicEdit, &QLineEdit::editingFinished, this, [this](){ setTopic(widget->topicEdit->text()); });
            connect(widget->payloadEdit, &QLineEdit::editingFinished, this, [this](){ setPayload(widget->payloadEdit->text()); });

            connect(widget->publishButton, &QPushButton::clicked, this, &MqttDataModel::publishMessage, Qt::QueuedConnection);

            m_connectDebounce = new QTimer(this);
            m_connectDebounce->setSingleShot(true);
            m_connectDebounce->setInterval(400);
            connect(m_connectDebounce, &QTimer::timeout, this, &MqttDataModel::connectChange);

            client = new MqttClient();
            connect(client, &MqttClient::isConnectedChanged, this, [this](bool ready){
                widget->statusButton->setEnabled(true);
                widget->statusButton->setText(ready ? "Connected" : "Disconnect");
                widget->statusButton->setChecked(ready);
                widget->statusButton->setStyleSheet(ready ? "color: green; font-weight: bold;" : "color: red; font-weight: bold;");
                if (m_reportedConnectState != ready) {
                    m_reportedConnectState = ready;
                    stateFeedBack("/connect", ready);
                }
                m_brokerConnected = ready;
                if (ready) {
                    m_activeConnectionKey = makeConnectionKey();
                    subscribeCurrentTopic();
                }
            }, Qt::QueuedConnection);
            connect(client, &MqttClient::messageReceived, this, &MqttDataModel::onMessage, Qt::QueuedConnection);
            // connect(client, &MqttClient::errorOccurred, this, &MqttDataModel::onError, Qt::QueuedConnection);
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
                setHost(v->value().toString());
                break;
            case 1:
                setPort(v->value().toInt());
                break;
            case 2:
                setTopic(v->value().toString());
                break;
            case 3:
                setPayload(v->value().toString());
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
            QJsonObject modelJson = NodeDelegateModel::save();
            QJsonObject values;
            values["host"] = m_host;
            values["port"] = m_port;
            values["username"] = m_username;
            values["password"] = m_password;
            values["topic"] = m_topic;
            values["payload"] = m_payload;
            values["qos"] = widget->qosCombo->currentIndex();
            modelJson["values"] = values;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined() && v.isObject()) {
                QJsonObject values = v.toObject();
                if (values.contains("host")) setHost(values["host"].toString());
                if (values.contains("port")) setPort(values["port"].toInt());
                if (values.contains("username")) setUsername(values["username"].toString());
                if (values.contains("password")) setPassword(values["password"].toString());
                if (values.contains("topic")) setTopic(values["topic"].toString());
                if (values.contains("payload")) setPayload(values["payload"].toString());
                if (values.contains("qos")) widget->qosCombo->setCurrentIndex(values["qos"].toInt());
                
                scheduleConnect();
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

        void afterModelReady() override
        {
            AbstractDelegateModel::afterModelReady();
            auto bus = GlobalEventBus::instance();
            
            bus->subscribe(makeFullOscAddress("/host"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/port"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/topic"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/payload"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/publish"), this, SLOT(onGlobalEvent(GlobalEvent)));
            
            // Initial feedback
            // AbstractDelegateModel::stateFeedBack("/host", m_host);
            // AbstractDelegateModel::stateFeedBack("/port", m_port);
            // AbstractDelegateModel::stateFeedBack("/username", m_username);
            // AbstractDelegateModel::stateFeedBack("/password", m_password);
            // AbstractDelegateModel::stateFeedBack("/topic", m_topic);
            // AbstractDelegateModel::stateFeedBack("/payload", m_payload);
        }

    public slots:
        void scheduleConnect()
        {
            if (!connectionTargetChanged()) {
                return;
            }
            if (!m_connectDebounce) {
                connectChange();
                return;
            }
            m_connectDebounce->start();
        }

        void connectChange()
        {
            if (!client || m_host.trimmed().isEmpty()) {
                return;
            }
            const QString key = makeConnectionKey();
            if (key == m_activeConnectionKey && m_brokerConnected) {
                return;
            }
            client->connectToHost(m_host, m_port, m_username, m_password);
        }

        void publishMessage()
        {
            if (!client) return;
            client->publish(m_topic, m_payload, widget->qosCombo->currentData().toInt(), false);
            stateFeedBack("/publish",true);
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

        // void onError(const QString &err)
        // {
        //     stateFeedBack("/error", err);
        // }

    public:
        // Property Getters and Setters
        QString getHost() const { return m_host; }
        void setHost(const QString& value) {
            if (m_host == value) return;
            m_activeConnectionKey.clear();
            m_host = value;
            if (widget && widget->hostEdit->text() != value) {
                QSignalBlocker blocker(widget->hostEdit);
                widget->hostEdit->setText(value);
            }
            emit hostChanged(value);
            AbstractDelegateModel::stateFeedBack("/host", value);
            scheduleConnect();
        }

        int getPort() const { return m_port; }
        void setPort(int value) {
            if (m_port == value) return;
            m_activeConnectionKey.clear();
            m_port = value;
            if (widget && widget->portSpinBox->value() != value) {
                QSignalBlocker blocker(widget->portSpinBox);
                widget->portSpinBox->setValue(value);
            }
            emit portChanged(value);
            AbstractDelegateModel::stateFeedBack("/port", value);
            scheduleConnect();
        }

        QString getUsername() const { return m_username; }
        void setUsername(const QString& value) {
            if (m_username == value) return;
            m_activeConnectionKey.clear();
            m_username = value;
            if (widget && widget->usernameEdit->text() != value) {
                QSignalBlocker blocker(widget->usernameEdit);
                widget->usernameEdit->setText(value);
            }
            emit usernameChanged(value);
            scheduleConnect();
        }

        QString getPassword() const { return m_password; }
        void setPassword(const QString& value) {
            if (m_password == value) return;
            m_activeConnectionKey.clear();
            m_password = value;
            if (widget && widget->passwordEdit->text() != value) {
                QSignalBlocker blocker(widget->passwordEdit);
                widget->passwordEdit->setText(value);
            }
            emit passwordChanged(value);
            scheduleConnect();
        }

        QString getTopic() const { return m_topic; }
        void setTopic(const QString& value) {
            if (m_topic == value) return;
            m_topic = value;
            if (widget && widget->topicEdit->text() != value) {
                QSignalBlocker blocker(widget->topicEdit);
                widget->topicEdit->setText(value);
            }
            emit topicChangedProp(value);
            subscribeCurrentTopic();
        }

        QString getPayload() const { return m_payload; }
        void setPayload(const QString& value) {
            if (m_payload == value) return;
            m_payload = value;
            if (widget && widget->payloadEdit->text() != value) {
                QSignalBlocker blocker(widget->payloadEdit);
                widget->payloadEdit->setText(value);
            }
            emit payloadChanged(value);
        }

    Q_SIGNALS:
        void hostChanged(QString value);
        void portChanged(int value);
        void usernameChanged(QString value);
        void passwordChanged(QString value);
        void topicChangedProp(QString value);
        void payloadChanged(QString value);
        void qosChanged(int value);

    private Q_SLOTS:
        void onGlobalEvent(const GlobalEvent& ev)
        {
            QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
            if (localPath == "host") setHost(ev.payload.toString());
            else if (localPath == "port") setPort(ev.payload.toInt());
            else if (localPath == "topic") setTopic(ev.payload.toString());
            else if (localPath == "payload") setPayload(ev.payload.toString());
            else if (localPath == "publish") publishMessage();
        }

    private:
        static QString normalizeHostForKey(const QString& host)
        {
            QString h = host.trimmed();
            static const QStringList prefixes = {
                QStringLiteral("mqtts://"), QStringLiteral("mqtt://"),
                QStringLiteral("ssl://"), QStringLiteral("tcp://"),
            };
            for (const QString& prefix : prefixes) {
                if (h.startsWith(prefix, Qt::CaseInsensitive)) {
                    h = h.mid(prefix.size());
                    break;
                }
            }
            const int slash = h.indexOf(QLatin1Char('/'));
            if (slash > 0) {
                h = h.left(slash);
            }
            return h.trimmed();
        }

        QString makeConnectionKey() const
        {
            return QStringLiteral("%1|%2|%3|%4")
                .arg(normalizeHostForKey(m_host))
                .arg(m_port)
                .arg(m_username.trimmed())
                .arg(m_password);
        }

        bool connectionTargetChanged() const
        {
            return makeConnectionKey() != m_activeConnectionKey;
        }

        void subscribeCurrentTopic()
        {
            if (!client || !m_brokerConnected) {
                return;
            }
            const QString topic = m_topic.trimmed();
            if (topic.isEmpty()) {
                return;
            }
            client->subscribe(topic, widget->qosCombo->currentData().toInt());
        }

        MqttInterface *widget = new MqttInterface();
        MqttClient *client = nullptr;
        QTimer *m_connectDebounce = nullptr;
        std::shared_ptr<VariableData> m_outData;

        QString m_activeConnectionKey;
        bool m_brokerConnected = false;
        bool m_reportedConnectState = false;

        QString m_host;
        int m_port = 1883;
        QString m_username;
        QString m_password;
        QString m_topic;
        QString m_payload;
    };
}
