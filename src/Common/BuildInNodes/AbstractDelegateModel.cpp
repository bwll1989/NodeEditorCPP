//
// Created by WuBin on 2025/12/29.
//

#include "AbstractDelegateModel.h"

#include "Common/GUI/PropertyTreeWidget/PropertyTreeWidget.h"

#include <QCoreApplication>
#include <QCheckBox>
#include <QClipboard>
#include <QComboBox>
#include <QDoubleSpinBox>
#include <QAbstractButton>
#include <QAbstractSlider>
#include <QSpinBox>
#include <QLineEdit>

#include "Common/AppConfig/ConfigManager.h"
#include "../Devices/StatusContainer/StatusContainer.h"
#include "../Devices/StatusContainer/GlobalEventBus.hpp"
#include "OSCSender/OSCSender.h"

class QCheckBox;



AbstractDelegateModel::AbstractDelegateModel()
    : QtNodes::NodeDelegateModel() {}

AbstractDelegateModel::~AbstractDelegateModel()
{
    /**
     * 函数级注释：析构阶段避免同步销毁大量 QWidget 导致退出变慢
     * - _autoPropertyWidget 使用 QPointer，若已被父控件销毁会自动置空
     * - 若仍存活：先解绑对象，再尝试 deleteLater；若应用已进入 closingDown 则不强制销毁
     */
    if (_autoPropertyWidget) {
        _autoPropertyWidget->setObject(nullptr);
        if (QCoreApplication::instance() && !QCoreApplication::closingDown()) {
            _autoPropertyWidget->deleteLater();
        }
        _autoPropertyWidget = nullptr;
    }
}

QWidget* AbstractDelegateModel::embeddedWidget()
{
    // 函数级注释：默认返回基于 Q_PROPERTY 的属性树，避免为简单节点重复手写界面映射
    if (!_autoPropertyWidget) {
        auto* w = new PropertyTreeWidget();
        w->setObject(this);
        _autoPropertyWidget = w;
    }
    return _autoPropertyWidget;
}

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

void AbstractDelegateModel::registerExternalBinding(const QString& oscAddress,
                                                   QObject* target,
                                                   QtNodes::NodeDelegateModel::ExternalBinding binding) {
    // 函数级注释：调用父类完成绑定；根据初始化状态决定是否立即注册到 StatusContainer（仅对控件绑定有效）
    QtNodes::NodeDelegateModel::registerExternalBinding(oscAddress, target, binding);

    // if (_ready) {
    //     QWidget* control = binding.control.data();
    //     const QString full = makeFullOscAddress(oscAddress);
    //     if (control) {
    //         StatusContainer::instance()->registerWidget(control, full);
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
    // // 函数级注释：激活初始化完成状态，并批量注册控件与推送初始值
    if (_ready) {
        return;
    }
    _ready = true;
    const auto mapping = getExternalControlAddressMapping();
    for (const auto& kv : mapping) {
        const QString& rel = kv.first;
        QWidget* control = kv.second.control.data();
        const QString full = makeFullOscAddress(rel);
        StatusContainer::instance()->registerWidget(control, full);

        const QString member = kv.second.member;
        if (member.isEmpty()) {
            continue;
        }

        const QVariant v = this->property(member.toUtf8().constData());
        if (!v.isValid()) {
            continue;
        }
        this->stateFeedBack(rel, v);
    }
    afterModelReady();
}

void AbstractDelegateModel::afterModelReady() {
    // 函数级注释：默认什么也不做；派生类可重写以在模型就绪后执行额外初始化
}

