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
#include "Common/Devices/StatusContainer/StatusContainer.h"
#include <QCoreApplication>
#include "Common/Devices/OSCSender/OSCSender.h"

class ExternalControler:public QObject {
        Q_OBJECT
public:
    explicit ExternalControler();
    ~ExternalControler();

private:
    //OSC接收器
    OSCReceiver *OSC_Receiver;
    OSCSender *OSC_Feedback;
    StatusContainer *StatusContainer;
};
