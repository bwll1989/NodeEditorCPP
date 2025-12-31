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
#include "ConstantDefines.h"

ExternalControler::ExternalControler():
    OSC_Receiver(new OSCReceiver(AppConstants::EXTRA_CONTROL_PORT))
    ,OSC_Feedback(new OSCSender(AppConstants::EXTRA_FEEDBACK_HOST,AppConstants::EXTRA_FEEDBACK_PORT))
    ,StatusContainer(StatusContainer::instance())
{
    // 创建 UDP 套接字并绑定到指定端口
    connect(OSC_Receiver, &OSCReceiver::receiveOSCMessage,StatusContainer::instance(),&StatusContainer::parseOSC);
    
    connect(StatusContainer::instance(), &StatusContainer::statusUpdated, this, [this](const StatusItem& item) {
        // 发送 OSC 反馈
        OSCMessage message = item.toOSCMessage();
        message.host = AppConstants::EXTRA_FEEDBACK_HOST;
        message.port = AppConstants::EXTRA_FEEDBACK_PORT;
        OSC_Feedback->sendOSCMessageWithQueue(message);
    });
}

ExternalControler::~ExternalControler()
{
    // 清理映射中的所有控件
    delete OSC_Receiver;
}


