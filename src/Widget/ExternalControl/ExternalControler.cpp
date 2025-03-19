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

ExternalControler::ExternalControler():
    OSC_Receiver(new OSCReceiver(8991))
 {
    // 创建 UDP 套接字并绑定到指定端口
    connect(OSC_Receiver, &OSCReceiver::receiveOSCMessage, this, &ExternalControler::hasOSC);
}
ExternalControler::~ExternalControler()
{
    // 清理映射中的所有控件
    controlMap.clear();
    delete OSC_Receiver;
}
void ExternalControler::hasOSC(const OSCMessage &message) {
    auto it = controlMap.find(message.address);
    if(it == controlMap.end()){
        qDebug() << "No control found for address:" << message.address;
        return;
    }

    QWidget* widget = it->second;
    if (!widget) {
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
    else if (auto* pushButton = qobject_cast<QPushButton*>(widget)) {
        qDebug() << "Setting QPushButton value:" << message.value.toBool();
        pushButton->click();
    }
    else if (auto* comboBox = qobject_cast<QComboBox*>(widget)) {
        comboBox->setCurrentText(message.value.toString());
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

void ExternalControler::registerControl(const QString& oscAddress, QWidget* control)
{
    if (!control) return;
    
    // 如果已存在相同地址的映射，先移除旧的
    auto it = controlMap.find(oscAddress);
    if (it != controlMap.end()) {
        controlMap.erase(it);
    }
    
    // 添加新的映射
    controlMap[oscAddress] = control;
}

void ExternalControler::unregisterControl(const QString& oscAddress)
{
    auto it = controlMap.find(oscAddress);
    if (it != controlMap.end()) {
        controlMap.erase(it);
    }
}

QWidget* ExternalControler::getControl(const QString& oscAddress) const
{
    auto it = controlMap.find(oscAddress);
    return (it != controlMap.end()) ? it->second : nullptr;
}

QStringList ExternalControler::getRegisteredAddresses() const
{
    QStringList addresses;
    for (const auto& pair : controlMap) {
        addresses << pair.first;
    }
    return addresses;
}