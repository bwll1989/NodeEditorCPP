//
// Created by WuBin on 2025/12/29.
//

#include "AbstractDelegateModel.h"

#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QAbstractButton>
#include <QAbstractSlider>
#include <QSpinBox>
#include <QLineEdit>

#include "ConstantDefines.h"
#include "../Devices/StatusContainer/StatusContainer.h"
#include "../Devices/StatusContainer/GlobalEventBus.hpp"
#include "OSCSender/OSCSender.h"

class QCheckBox;



AbstractDelegateModel::AbstractDelegateModel()
    : QtNodes::NodeDelegateModel() {}

AbstractDelegateModel::~AbstractDelegateModel() = default;

ConnectionPolicy AbstractDelegateModel::portConnectionPolicy(PortType portType, PortIndex index) const {
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

void AbstractDelegateModel::registerExternalControl(const QString& oscAddress, QWidget* control) {
    // 函数级注释：调用父类完成基本映射；根据初始化状态决定是否立即注册到 StatusContainer
    QtNodes::NodeDelegateModel::registerExternalControl(oscAddress, control);
    // if (_ready) {
    //     // 已完成初始化：立即在状态容器中注册该控件
    //     const QString full = makeFullOscAddress(oscAddress);
    //     if (control) {
    //         StatusContainer::instance()->registerWidget(control, full);
    //     }
    //     // 初始化完成后立即推送当前值
    //     if (auto* button = qobject_cast<QAbstractButton*>(control)) {
    //         if (button->isCheckable()){
    //             this->stateFeedBack(oscAddress, QVariant(button->isChecked()));
    //         }
    //     } else if (auto* slider = qobject_cast<QAbstractSlider*>(control)) {
    //         this->stateFeedBack(oscAddress, QVariant(slider->value()));
    //     } else if (auto* spinBox = qobject_cast<QSpinBox*>(control)) {
    //         this->stateFeedBack(oscAddress, QVariant(spinBox->value()));
    //     } else if (auto* doubleSpinBox = qobject_cast<QDoubleSpinBox*>(control)) {
    //         this->stateFeedBack(oscAddress, QVariant(doubleSpinBox->value()));
    //     } else if (auto* lineEdit = qobject_cast<QLineEdit*>(control)) {
    //         this->stateFeedBack(oscAddress, QVariant(lineEdit->text()));
    //     } else if (auto* comboBox = qobject_cast<QComboBox*>(control)) {
    //         this->stateFeedBack(oscAddress, QVariant(comboBox->currentIndex()));
    //     } else if (auto* checkBox = qobject_cast<QCheckBox*>(control)) {
    //         this->stateFeedBack(oscAddress, QVariant(checkBox->isChecked()));
    //     }
    // }
}


void AbstractDelegateModel::stateFeedBack(const QString& oscAddress, QVariant value) {
    // 函数级注释：默认将状态反馈封装为 StateFeedback 事件发送到全局事件总线
    if (!_ready) return;
    const QString full = makeFullOscAddress(oscAddress);
    GlobalEventBus::instance()->publishState(full, value);
}

QString AbstractDelegateModel::makeFullOscAddress(const QString& relative) const {
    // 函数级注释：拼接完整地址，确保以 '/' 开头的相对路径正确连接
    const QString norm = relative.startsWith('/') ? relative : ("/" + relative);
    return "/dataflow/" + getParentAlias() + "/" + QString::number(getNodeID()) + norm;
}

void AbstractDelegateModel::onModelReady() {
    // 函数级注释：激活初始化完成状态，并批量注册控件与推送初始值
    _ready = true;
    const auto mapping = getExternalControlAddressMapping();
    for (const auto& kv : mapping) {
        const QString& rel = kv.first;
        QWidget* control = kv.second;
        if (!control) continue;
        const QString full = makeFullOscAddress(rel);
        StatusContainer::instance()->registerWidget(control, full);
        // 推送当前值
        if (auto* button = qobject_cast<QAbstractButton*>(control)) {
            if (button->isCheckable()){
                this->stateFeedBack(rel, QVariant(button->isChecked()));
            }
        } else if (auto* slider = qobject_cast<QAbstractSlider*>(control)) {
            this->stateFeedBack(rel, QVariant(slider->value()));
        } else if (auto* spinBox = qobject_cast<QSpinBox*>(control)) {
            this->stateFeedBack(rel, QVariant(spinBox->value()));
        } else if (auto* doubleSpinBox = qobject_cast<QDoubleSpinBox*>(control)) {
            this->stateFeedBack(rel, QVariant(doubleSpinBox->value()));
        } else if (auto* lineEdit = qobject_cast<QLineEdit*>(control)) {
            this->stateFeedBack(rel, QVariant(lineEdit->text()));
        } else if (auto* comboBox = qobject_cast<QComboBox*>(control)) {
            this->stateFeedBack(rel, QVariant(comboBox->currentIndex()));
        } else if (auto* checkBox = qobject_cast<QCheckBox*>(control)) {
            this->stateFeedBack(rel, QVariant(checkBox->isChecked()));
        }
    }
    afterModelReady();
}

void AbstractDelegateModel::afterModelReady() {
    // 函数级注释：默认什么也不做；派生类可重写以在模型就绪后执行额外初始化
}

