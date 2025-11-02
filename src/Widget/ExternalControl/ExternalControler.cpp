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
#include <QToolButton>

#include "Common/GUI/Elements/FaderWidget/FaderWidget.h"
#include "ConstantDefines.h"
ExternalControler::ExternalControler():
    OSC_Receiver(new OSCReceiver(AppConstants::EXTRA_CONTROL_PORT))
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
    // 函数说明：处理接收到的 OSC 消息，根据地址找到对应控件，并按类型安全设置其状态或触发动作
    auto args = message.address.split("/");
    //如果地址层级小于或大于5，则判定格式不对，不处理
    if (args.size()!=5) {
        qDebug() << "Invalid OSC address format:" << message.address;
        return;
    }
   
    QWidget* widget = nullptr;
    switch(getAddressType(args[1]))
    {
    case AddressType::Dataflow:
        {
            if (!m_dataflowmodels) {
                qDebug() << "Dataflow models map is nullptr for address:" << message.address;
                break;
            }
            const QString model_key = args[2];
            NodeId nodeId =args[3].toInt();
            // 查找键而非直接 operator[]，避免插入空值
            auto itModel = m_dataflowmodels->find(model_key);
            if (itModel == m_dataflowmodels->end() || !itModel->second.get()) {
                qDebug() << "Model not found or already deleted for key:" << model_key
                         << "address:" << message.address;
                break;
            }
            CustomDataFlowGraphModel* model = itModel->second.get();
            auto widgets = model->nodeData(nodeId,NodeRole::OSCAddress).value<std::unordered_map<QString, QWidget*>>();
            auto it = widgets.find("/"+args[4]);
            if (it != widgets.end()) {
                widget = it->second;
            }
            break;
        }
    case AddressType::Timeline:
        {
            if (!m_timelinemodel) {
                qDebug() << "Timeline model is nullptr for address:" << message.address;
                break;
            }
            if (args[2]!="default") {
                qDebug() << "Invalid timeline address format:" << message.address;
                break;
            }
            if (args[3]=="toolbar")
            {

                auto it = m_TimelineToolbarMapping->find("/"+args[4]);
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
                ClipId clipId =args[3].toInt();
                auto widgets= m_timelinemodel->clipData(clipId,TimelineRoles::ClipOscWidgetsRole).value<std::unordered_map<QString, QWidget*>>();
                auto it = widgets.find("/"+args[4]);
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
        fader->setValue(message.value.toFloat());
    }
    else if (auto* pushButton = qobject_cast<QPushButton*>(widget)) {
        if (message.value.toBool() != pushButton->isChecked()) {
            pushButton->click();
        }
    }
    // 新增分支：支持 QToolButton（QToolBar 上由 QAction 生成的控件）
    else if (auto* toolButton = qobject_cast<QToolButton*>(widget)) {
        // 如果是可切换按钮，直接同步勾选状态；否则在收到 true 时点击触发
        if (toolButton->isCheckable()) {
            toolButton->setChecked(message.value.toBool());
        } else {
            if (message.value.toBool()) {
                toolButton->click();
            }
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

// void ExternalControler::setDataFlowModel(CustomDataFlowGraphModel *model)
// {
//     this->m_dataflowmodel = model;
// }

void ExternalControler::setDataflowModels(std::map<QString, std::unique_ptr<CustomDataFlowGraphModel>> *models)
{
    this->m_dataflowmodels = models;
}



void ExternalControler::setTimelineModel(TimeLineModel *model)
{
    this->m_timelinemodel = model;
}

void ExternalControler::setTimelineToolBarMap(std::shared_ptr<std::unordered_map<QString, QAction*>> oscMapping)
{
    m_TimelineToolbarMapping=std::move(oscMapping); // 转移所有权
}