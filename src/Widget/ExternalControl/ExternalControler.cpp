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

ExternalControler::ExternalControler():
    OSC_Receiver(nullptr)
    ,OSC_Feedback(nullptr)
    ,StatusContainer(StatusContainer::instance())
{
    if (ConfigManager::instance().isOscEnabled()) {
        OSC_Receiver = new OSCReceiver(ConfigManager::instance().getExtraControlPort());
        OSC_Feedback = new OSCSender(ConfigManager::instance().getExtraFeedbackHost(), ConfigManager::instance().getExtraFeedbackPort());

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
}

ExternalControler::~ExternalControler()
{
    delete OSC_Receiver;
    delete OSC_Feedback;
}


