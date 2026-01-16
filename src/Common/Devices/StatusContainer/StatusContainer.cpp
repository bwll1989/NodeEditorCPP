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
    
    // 使用 QTimer::singleShot(0, ...) 确保所有 UI 更新都在主线程执行
    // 并且通过 Lambda 捕获值，避免 worker 线程访问 UI 或状态不一致问题

    if (auto* spinBox = qobject_cast<QSpinBox*>(widget)) {
        int v = message.value.toInt();
        QTimer::singleShot(0, spinBox, [spinBox, v](){ spinBox->setValue(v); });
    }
    else if (auto* doubleSpinBox = qobject_cast<QDoubleSpinBox*>(widget)) {
        double v = message.value.toDouble();
        QTimer::singleShot(0, doubleSpinBox, [doubleSpinBox, v](){ doubleSpinBox->setValue(v); });
    }
    else if (auto* slider = qobject_cast<QSlider*>(widget)) {
        int v = message.value.toInt();
        QTimer::singleShot(0, slider, [slider, v](){ slider->setValue(v); });
    }
    else if (auto* checkBox = qobject_cast<QCheckBox*>(widget)) {
        bool v = message.value.toBool();
        QTimer::singleShot(0, checkBox, [checkBox, v](){ checkBox->setChecked(v); });
    }
    else if (auto* fader = qobject_cast<FaderWidget*>(widget)) {
        float v = message.value.toFloat();
        QTimer::singleShot(0, fader, [fader, v](){ fader->setValue(v); });
    }
    else if (auto* pushButton = qobject_cast<QPushButton*>(widget)) {
        bool v = message.value.toBool();
        QTimer::singleShot(0, pushButton, [pushButton, v](){
            // 对于 checkable 按钮，根据当前状态决定是否 click 以触发 clicked 信号并切换状态
            // 避免直接 setChecked 导致不触发 clicked 信号，或 invokeMethod("click") 导致的逻辑错误
            if (pushButton->isCheckable()) {
                if (pushButton->isChecked() != v) {
                    pushButton->click();
                }
            } else {
                // 普通按钮仅在 true 时触发点击
                if (v) {
                    pushButton->click();
                }
            }
        });
    }
    else if (auto* toolButton = qobject_cast<QToolButton*>(widget)) {
        bool v = message.value.toBool();
        QTimer::singleShot(0, toolButton, [toolButton, v](){
            if (toolButton->isCheckable()) {
                if (toolButton->isChecked() != v) {
                    toolButton->click();
                }
            } else {
                if (v) {
                    toolButton->click();
                }
            }
        });
    }
    else if (auto* comboBox = qobject_cast<QComboBox*>(widget)) {
        int v = message.value.toInt();
        QTimer::singleShot(0, comboBox, [comboBox, v](){ comboBox->setCurrentIndex(v); });
    }
    else if (auto* lineEdit = qobject_cast<QLineEdit*>(widget)) {
        QString v = message.value.toString();
        QTimer::singleShot(0, lineEdit, [lineEdit, v](){ lineEdit->setText(v); });
    }
    else if (auto* textEdit = qobject_cast<QTextEdit*>(widget)) {
        QString v = message.value.toString();
        QTimer::singleShot(0, textEdit, [textEdit, v](){ textEdit->setText(v); });
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
     
    // 使用 QTimer::singleShot(0, ...) 确保所有 UI 更新都在主线程执行
    // 并且通过 Lambda 捕获值，避免 worker 线程访问 UI 或状态不一致问题

    if (auto* spinBox = qobject_cast<QSpinBox*>(widget)) {
        int v = message.value.toInt();
        QTimer::singleShot(0, spinBox, [spinBox, v](){ spinBox->setValue(v); });
    }
    else if (auto* doubleSpinBox = qobject_cast<QDoubleSpinBox*>(widget)) {
        double v = message.value.toDouble();
        QTimer::singleShot(0, doubleSpinBox, [doubleSpinBox, v](){ doubleSpinBox->setValue(v); });
    }
    else if (auto* slider = qobject_cast<QSlider*>(widget)) {
        int v = message.value.toInt();
        QTimer::singleShot(0, slider, [slider, v](){ slider->setValue(v); });
    }
    else if (auto* checkBox = qobject_cast<QCheckBox*>(widget)) {
        bool v = message.value.toBool();
        QTimer::singleShot(0, checkBox, [checkBox, v](){ checkBox->setChecked(v); });
    }
    else if (auto* fader = qobject_cast<FaderWidget*>(widget)) {
        float v = message.value.toFloat();
        QTimer::singleShot(0, fader, [fader, v](){ fader->setValue(v); });
    }
    else if (auto* pushButton = qobject_cast<QPushButton*>(widget)) {
        bool v = message.value.toBool();
        QTimer::singleShot(0, pushButton, [pushButton, v](){
            // 对于 checkable 按钮，根据当前状态决定是否 click 以触发 clicked 信号并切换状态
            // 避免直接 setChecked 导致不触发 clicked 信号，或 invokeMethod("click") 导致的逻辑错误
            if (pushButton->isCheckable()) {
                if (pushButton->isChecked() != v) {
                    pushButton->click();
                }
            } else {
                // 普通按钮仅在 true 时触发点击
                if (v) {
                    pushButton->click();
                }
            }
        });
    }
    else if (auto* toolButton = qobject_cast<QToolButton*>(widget)) {
        bool v = message.value.toBool();
        QTimer::singleShot(0, toolButton, [toolButton, v](){
            if (toolButton->isCheckable()) {
                if (toolButton->isChecked() != v) {
                    toolButton->click();
                }
            } else {
                if (v) {
                    toolButton->click();
                }
            }
        });
    }
    else if (auto* comboBox = qobject_cast<QComboBox*>(widget)) {
        int v = message.value.toInt();
        QTimer::singleShot(0, comboBox, [comboBox, v](){ comboBox->setCurrentIndex(v); });
    }
    else if (auto* lineEdit = qobject_cast<QLineEdit*>(widget)) {
        QString v = message.value.toString();
        QTimer::singleShot(0, lineEdit, [lineEdit, v](){ lineEdit->setText(v); });
    }
    else if (auto* textEdit = qobject_cast<QTextEdit*>(widget)) {
        QString v = message.value.toString();
        QTimer::singleShot(0, textEdit, [textEdit, v](){ textEdit->setText(v); });
    }
    else {
        qDebug() << "Unsupported widget type for address:" << message.address;
    }
}

