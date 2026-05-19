//
// Created by bwll1 on 2024/9/1.
//

#include "ExternalControler.hpp"
#include <QJsonDocument>
#include <QJsonObject>
#include <QByteArray>
#include <QHostAddress>
#include <QJsonParseError>
#include "tinyosc.h"
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QPushButton>
#include <QComboBox>
#include <QTextEdit>
#include <QLineEdit>
#include "OSCMessage.h"
#include <QToolButton>

#include "Common/GUI/Elements/FaderWidget/FaderWidget.h"
#include "Common/AppConfig/ConfigManager.h"
#include "Common/Devices/MqttClient/MqttClient.h"

ExternalControler::ExternalControler():
    StatusContainer(StatusContainer::instance())
{
    const auto& config = ConfigManager::instance();

    if (config.isOscEnabled()) {
        OSC_Receiver = new OSCReceiver(config.getExtraControlPort());
        OSC_Feedback = new OSCSender(config.getExtraFeedbackHost(), config.getExtraFeedbackPort());

        connect(OSC_Receiver, &OSCReceiver::receiveOSCMessage, StatusContainer::instance(), &StatusContainer::parseOSC);
        connect(StatusContainer::instance(), &StatusContainer::statusUpdated, this, [this](const StatusItem& item) {
            OSCMessage message = item.toOSCMessage();
            message.host = ConfigManager::instance().getExtraFeedbackHost();
            message.port = ConfigManager::instance().getExtraFeedbackPort();
            if (OSC_Feedback) {
                OSC_Feedback->sendOSCMessageWithQueue(message);
            }
        });
    }

    if (config.isMqttEnabled()) {
        setupMqtt();
    }
}

void ExternalControler::setupMqtt()
{
    const auto& config = ConfigManager::instance();

    Mqtt_Client = new MqttClient(this);

    connect(Mqtt_Client, &MqttClient::messageReceived, this, &ExternalControler::onMqttMessage);
    connect(Mqtt_Client, &MqttClient::errorOccurred, this, [](const QString& err) {
        qWarning() << "MQTT external control error:" << err;
    });
    connect(Mqtt_Client, &MqttClient::isConnectedChanged, this, [this](bool connected) {
        if (!Mqtt_Client) {
            return;
        }
        if (!connected) {
            qWarning() << "MQTT external control disconnected from broker";
            return;
        }
        const QString controlTopic = ConfigManager::instance().getMqttControlTopic();
        qInfo() << "MQTT external control connected, subscribing to" << controlTopic;
        Mqtt_Client->subscribe(controlTopic);
    });
    connect(StatusContainer::instance(), &StatusContainer::statusUpdated, this, [this](const StatusItem& item) {
        if (!Mqtt_Client) {
            return;
        }
        const QString payload = QString::fromUtf8(
            QJsonDocument(item.toJsonObject()).toJson(QJsonDocument::Compact));
        Mqtt_Client->publish(ConfigManager::instance().getMqttFeedbackTopic(), payload);
    });

    Mqtt_Client->connectToHost(
        config.getMqttHost(),
        config.getMqttPort(),
        config.getMqttUsername(),
        config.getMqttPassword());
}

void ExternalControler::onMqttMessage(const QString& topic, const QByteArray& payload)
{
    Q_UNUSED(topic)

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(payload, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) {
        return;
    }

    const QJsonObject obj = doc.object();
    QString addr = obj.value("address").toString();
    if (addr.isEmpty()) {
        addr = obj.value("addr").toString();
    }
    if (addr.isEmpty()) {
        return;
    }

    OSCMessage msg;
    msg.host = QStringLiteral("127.0.0.1");
    msg.port = 0;
    msg.address = addr;
    msg.value = obj.value("value").toVariant();
    StatusContainer::instance()->parseOSC(msg);
}

ExternalControler::~ExternalControler()
{
    if (Mqtt_Client) {
        Mqtt_Client->disconnectFromHost();
    }
    delete OSC_Receiver;
    delete OSC_Feedback;
}
