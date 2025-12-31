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
#include "Elements/FaderWidget/FaderWidget.h"

StatusContainer* StatusContainer::instance() {
    static StatusContainer inst;
    return &inst;
}

StatusContainer::StatusContainer(QObject* parent) : QObject(parent) {}

void StatusContainer::registerWidget(QWidget* p, const QString& a) {

    // 函数级注释：注册控件指针到指定地址。若地址已存在，仅更新其指针；否则新建条目，值为空
    if (!p || a.isEmpty()) return;
    QWriteLocker g(&_lock);
    auto it = _latest.find(a);
    if (it != _latest.end()) {
        StatusItem item = it.value();
        item.ptr = p;
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

QWidget* StatusContainer::getWidget(const QString& address) {
    // 函数级注释：直接通过 StatusContainer 使用完整地址查找控件，并按类型安全设置其状态或触发动作
    if (!StatusContainer::instance()->contains(address)) {
        qDebug() << "Address not registered in StatusContainer:" << address;
        return nullptr;
    }
    return StatusContainer::instance()->last(address).ptr;
}



void StatusContainer::parseOSC(const OSCMessage &message) {
    QWidget* widget = getWidget(message.address);
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

void StatusContainer::parseStatus(const StatusItem& message) {
    QWidget* widget = getWidget(message.address);
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

