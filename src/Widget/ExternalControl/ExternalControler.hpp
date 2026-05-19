//
// Created by bwll1 on 2024/9/1.
//
#pragma once
#include <QObject>
#include <QColor>
#include <QJsonObject>
#include <QString>
#include <memory>
#include "Common/Devices/OSCReceiver/OSCReceiver.h"
#include "OSCMessage.h"
#include <QWidget>
#include "Common/Devices/StatusContainer/StatusContainer.h"
#include <QCoreApplication>
#include "Common/Devices/OSCSender/OSCSender.h"

class MqttClient;

class ExternalControler:public QObject {
        Q_OBJECT
public:
    explicit ExternalControler();
    ~ExternalControler();

private:
    void setupMqtt();
    void onMqttMessage(const QString& topic, const QByteArray& payload);

    //OSC接收器
    OSCReceiver *OSC_Receiver = nullptr;
    OSCSender *OSC_Feedback = nullptr;
    MqttClient *Mqtt_Client = nullptr;
    StatusContainer *StatusContainer;
};
