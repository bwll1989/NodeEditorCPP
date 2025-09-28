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
#include <unordered_map>
#include <QAction>

#include "Common/GUI/Elements/FaderWidget/FaderWidget.h"

ExternalControler::ExternalControler():
    OSC_Receiver(new OSCReceiver(8991))
 {
    // 创建 UDP 套接字并绑定到指定端口
    connect(OSC_Receiver, &OSCReceiver::receiveOSCMessage, this, &ExternalControler::hasOSC);

}
ExternalControler::~ExternalControler()
{
    // 清理映射中的所有控件
    delete OSC_Receiver;
}
void ExternalControler::hasOSC(const OSCMessage &message) {
    
    auto args = message.address.split("/");
    
    //如果地址层级小于或大于3，则判定格式不对，不处理
    if (args.size()!=4) {
        qDebug() << "Invalid OSC address format:" << message.address;
        return;
    }
   
    QWidget* widget = nullptr;
    switch(getAddressType(args[1]))
    {
    case AddressType::Dataflow:
        {
            NodeId nodeId =args[2].toInt();
            auto widgets = m_dataflowmodel->nodeData(nodeId,NodeRole::OSCAddress).value<std::unordered_map<QString, QWidget*>>();
            auto it = widgets.find("/"+args[3]);
            if (it != widgets.end()) {
                widget = it->second;
            }
            break;
        }
    case AddressType::Timeline:
        {
            if (args[2]=="toolbar")
            {

                auto it = m_TimelineToolbarMapping->find("/"+args[3]);
                if (it != m_TimelineToolbarMapping->end()) {
                    QAction* action = it->second;
                    // 触发QAction的点击信号
                    if (message.value.toInt()==1)
                    {
                        action->trigger();
                    }
                    return;
                }
                break;
            }else
            {
                ClipId clipId =args[2].toInt();
                auto widgets= m_timelinemodel->clipData(clipId,TimelineRoles::ClipOscWidgetsRole).value<std::unordered_map<QString, QWidget*>>();
                auto it = widgets.find("/"+args[3]);
                if (it != widgets.end()) {
                    widget = it->second;
                }
                break;
            }

        }
    default:
       //其他情况
        break;
    }

     if (!widget) 
    {
        qDebug() << "Invalid widget pointer for address:" << message.address;
        return;
    }
    // 使用 qobject_cast 进行安全的类型检查
    if (auto* spinBox = qobject_cast<QSpinBox*>(widget)) {

        spinBox->setValue(message.value.toInt());
    }
    else if (auto* doubleSpinBox = qobject_cast<QDoubleSpinBox*>(widget)) {
        doubleSpinBox->setValue(message.value.toDouble());
    }
    else if (auto* slider = qobject_cast<QSlider*>(widget)) {
        slider->setValue(message.value.toInt());
    }
    else if (auto* checkBox = qobject_cast<QCheckBox*>(widget)) {
        checkBox->setChecked(message.value.toBool());
    }
    else if (auto* fader = qobject_cast<FaderWidget*>(widget)) {
        qDebug() << "Successfully cast to FaderWidget!";
        fader->setValue(message.value.toFloat());
    }
    else if (auto* pushButton = qobject_cast<QPushButton*>(widget)) {
        if (message.value.toBool()!=pushButton->isChecked())
        {
            pushButton->click();
        }
    }
    else if (auto* comboBox = qobject_cast<QComboBox*>(widget)) {
        comboBox->setCurrentIndex(message.value.toInt());
    }
    else if (auto* slider= qobject_cast<QSlider*>(widget)) {
        slider->setValue(message.value.toInt());
    }
    else if (auto* lineEdit = qobject_cast<QLineEdit*>(widget)) {
        lineEdit->setText(message.value.toString());
    }
    else if (auto* textEdit = qobject_cast<QTextEdit*>(widget)) {
        textEdit->setText(message.value.toString());
    }

    else {
        qDebug() << "Unsupported widget type for address:" << message.address;
    }
}

void ExternalControler::setDataFlowModel(CustomDataFlowGraphModel *model)
{
    this->m_dataflowmodel = model;
}


void ExternalControler::setTimelineModel(TimeLineModel *model)
{
    this->m_timelinemodel = model;
}

void ExternalControler::setTimelineToolBarMap(std::shared_ptr<std::unordered_map<QString, QAction*>> oscMapping)
{
    m_TimelineToolbarMapping=std::move(oscMapping); // 转移所有权
}