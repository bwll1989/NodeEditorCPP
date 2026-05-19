#include "StatusContainer.h"
#include <QCheckBox>
#include <QSpinBox>
#include <QDoubleSpinBox>
#include <QSlider>
#include <QPushButton>
#include <QComboBox>
#include <QTextEdit>
#include <QLineEdit>
#include <QToolButton>
#include <QTimer>
#include "Elements/FaderWidget/FaderWidget.h"

StatusContainer* StatusContainer::instance() {
    static StatusContainer inst;
    return &inst;
}

StatusContainer::StatusContainer(QObject* parent) : QObject(parent)
{
    // 函数级注释：订阅全局事件总线，监听状态反馈事件并转发到本地状态更新
    connect(GlobalEventBus::instance(), &GlobalEventBus::eventPublished,
            this, &StatusContainer::onGlobalEvent);
}

void StatusContainer::registerWidget(QWidget* p, const QString& a) {

    // 函数级注释：注册控件指针到指定地址。若地址已存在，仅更新其指针；否则新建条目，值为空
    if (a.isEmpty()) return;
    QWriteLocker g(&_lock);
    auto it = _latest.find(a);
    if (it != _latest.end()) {
        StatusItem item = it.value();
        item.ptr = p;  // 支持 nullptr 以解除绑定
        it.value() = item;
    } else {
        _latest.insert(a, StatusItem(p, a, QVariant()));
    }
}


bool StatusContainer::updateState(const OSCMessage& message) {

    if (message.address.isEmpty()) return false;

    // 暂无已注册控件指针，上层可在注册阶段补齐
    StatusItem item(nullptr, message.address, message.value);

    {
        QWriteLocker g(&_lock);
        StatusItem finalItem;
        auto it = _latest.find(item.address);
        if (it != _latest.end()) {
            StatusItem existing = it.value();
            existing.value = item.value; // 仅更新值，保留已注册的控件指针
            it.value() = existing;
            finalItem = existing;
        } else {
            _latest.insert(item.address, item);
            finalItem = item;
        }
        _queue.enqueue(finalItem);
    }
    emit statusUpdated(_queue.back());
    return true;
}

bool StatusContainer::updateState(const QString& address, const QVariant& value, const QString& type) {
    if (address.isEmpty()) return false;
    OSCMessage msg;
    msg.address = address;
    msg.value = value;
    msg.type = type;
    msg.port = 0;
    return updateState(msg);
}

bool StatusContainer::contains(const QString& address) const {
    QReadLocker g(&_lock);
    return _latest.contains(address);
}

StatusItem StatusContainer::last(const QString& address) const {
    QReadLocker g(&_lock);
    return _latest.value(address, StatusItem());
}

QVector<StatusItem> StatusContainer::queryByPrefix(const QString& prefix) const {
    QVector<StatusItem> out;
    QReadLocker g(&_lock);
    for (auto it = _latest.constBegin(); it != _latest.constEnd(); ++it) {
        if (it.key().startsWith(prefix)) {
            out.push_back(it.value());
        }
    }
    return out;
}

QVector<StatusItem> StatusContainer::drain() {
    QVector<StatusItem> out;
    QWriteLocker g(&_lock);
    out.reserve(_queue.size());
    while (!_queue.isEmpty()) {
        out.push_back(_queue.dequeue());
    }
    return out;
}

void StatusContainer::clearAll() {
    QWriteLocker g(&_lock);
    _latest.clear();
    _queue.clear();
}

void StatusContainer::onGlobalEvent(const GlobalEvent& ev) {
    // 函数级注释：收到全局事件总线的状态反馈事件时，将其转换为本地状态更新
    if (ev.kind != GlobalEventKind::Feedback) {
        return;
    }
    if (ev.address.isEmpty()) {
        return;
    }
    updateState(ev.address, ev.payload);
}

QWidget* StatusContainer::getWidget(const QString& address) {
    // 函数级注释：直接通过 StatusContainer 使用完整地址查找控件，并按类型安全设置其状态或触发动作
    if (!StatusContainer::instance()->contains(address)) {
        qDebug() << "Address not registered in StatusContainer:" << address;
        return nullptr;
    }
    return StatusContainer::instance()->last(address).ptr;
}



void StatusContainer::parseOSC(const OSCMessage &message) {
    // 函数级注释：将外部 OSC 消息转换为命令事件，交由全局事件总线驱动业务逻辑
    if (message.address.isEmpty()) {
        return;
    }
    GlobalEventBus::instance()->publishCommand(message.address, message.value);
}

void StatusContainer::parseStatus(const StatusItem& message) {
    // 函数级注释：将外部状态消息转换为状态反馈事件，写入全局事件总线
    if (message.address.isEmpty()) {
        return;
    }
    GlobalEventBus::instance()->publishState(message.address, message.value);
}

