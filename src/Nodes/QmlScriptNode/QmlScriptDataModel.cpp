#pragma once

#include <QtCore/QObject>
#include <QtQml/QJSEngine>
#include <QtQml/QJSValue>
#include <QtQml/QJSValueList>
#include <QtQml/QJSValueIterator> 
#include "DataTypes/NodeDataList.hpp"
#include "QDir"
#include <QtNodes/NodeDelegateModel>
#include "QThread"
#include <iostream>
#include "QmlScriptInterface.hpp"
#include <QtCore/qglobal.h>
#include "QPushButton"
#include <QDebug>
#include <QLabel>
#include <QLineEdit>
#include "QCheckBox"
#include <QSignalBlocker>
#include <QAbstractSlider>
#include "QmlScriptDataModel.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace Nodes;




unsigned int QmlScriptDataModel::getInputCount() {
    return InPortCount;
}


unsigned int QmlScriptDataModel::getOutputCount() {
    return OutPortCount;
}

unsigned int QmlScriptDataModel::inputIndex() {
    return inputPortIndex;
}

// 函数级注释：供 QML 触发脚本运行（等价于点击原“运行”按钮）。
void QmlScriptDataModel::runScript()
{
    onRunButtonClicked();
}

// 函数级注释：供 QML 打开脚本编辑器窗口。
void QmlScriptDataModel::openEditor()
{
    if (widget) {
        widget->toggleEditorMode();
    }
}

// 函数级注释：供 QML 写入单个持久化配置项，并通知界面刷新。
void QmlScriptDataModel::setSettingValue(const QString &key, const QVariant &value)
{
    {
        QMutexLocker locker(&m_dataMutex);
        m_settings.insert(key, value);
    }
    emit settingsChanged();

    if (m_jsEngine) {
        QJSValue func = m_jsEngine->globalObject().property("onSettingChanged");
        if (func.isCallable()) {
            QJSValueList args;
            args << m_jsEngine->toScriptValue(key);
            args << m_jsEngine->toScriptValue(value);
            func.call(args);
        }
    }
}

// 函数级注释：供 QML 删除持久化配置项，并通知界面刷新。
void QmlScriptDataModel::removeSetting(const QString &key)
{
    {
        QMutexLocker locker(&m_dataMutex);
        m_settings.remove(key);
    }
    emit settingsChanged();

    if (m_jsEngine) {
        QJSValue func = m_jsEngine->globalObject().property("onSettingRemoved");
        if (func.isCallable()) {
            QJSValueList args;
            args << m_jsEngine->toScriptValue(key);
            func.call(args);
        }
    }
}

// 函数级注释：供脚本将动态控件声明（UI Schema）下发到 QML。
void QmlScriptDataModel::setUiSchema(const QJSValue &schema)
{
    if (!schema.isArray() && !schema.isObject()) {
        return;
    }

    QVariantList list;
    const QVariant v = schema.toVariant();
    if (v.typeId() == QMetaType::QVariantList) {
        list = v.toList();
    } else if (v.typeId() == QMetaType::QVariantMap) {
        list.append(v.toMap());
    }

    {
        QMutexLocker locker(&m_dataMutex);
        m_uiSchema = list;
    }
    emit uiSchemaChanged();
}

Q_INVOKABLE void QmlScriptDataModel::clearLayout() {
    // 确保在主线程中执行
    if (QThread::currentThread() != QApplication::instance()->thread()) {
        QMetaObject::invokeMethod(this, "clearLayout", Qt::BlockingQueuedConnection);
        return;
    }
    
    // 删除所有控件
    for (QWidget* widget : m_widgets.values()) {
        // unregisterOSCControl(QString("/%1").arg(m_widgets.key(widget)));
        // 立即删除而不是使用deleteLater
        delete widget;
    }
    m_widgets.clear();
    m_widgetCounter = 0;
}

Q_INVOKABLE int QmlScriptDataModel::addToLayout(QObject* widgetObj, int x, int y,int rowSpan, int columnSpan) {
    QWidget* widget_obj = qobject_cast<QWidget*>(widgetObj);
    if (!widget_obj) {
        return -1; // 转换失败
    }
    // 统一设置控件尺寸策略
    widget_obj->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    if (x != -1 && y != -1) {

        widget->top_layout->addWidget(widget_obj, x, y, rowSpan, columnSpan);
    } else {
        widget->top_layout->addWidget(widget_obj);
    }
    for (int c = 0; c < widget->top_layout->columnCount(); c++) {
        if (widget->top_layout->columnStretch(c) == 0) {
            widget->top_layout->setColumnStretch(c, 1); // 新列设置拉伸因子
            widget->top_layout->setColumnMinimumWidth(c, 1);
        }
    }
    for (int r = 0; r < widget->top_layout->rowCount(); r++) {
        if (widget->top_layout->rowStretch(r) == 0) {
            widget->top_layout->setRowStretch(r, 1); // 新列设置拉伸因子
            widget->top_layout->setRowMinimumHeight(r, 1);
        }
    }
    int id = m_widgetCounter++;
    m_widgets[id] = widget_obj;

    bindAutoSave(widget_obj, id, x, y);
    QMetaObject::invokeMethod(this, [this, widget_obj, id, x, y]() {
        restoreAutoSavedValue(widget_obj, id, x, y);
    }, Qt::QueuedConnection);

    // registerOSCControl(QString("/%1").arg(id),widget_obj);
    return id;
}

void QmlScriptDataModel::loadScripts(QString code) {
    if (code.isEmpty()) {
        return;
    }

    if (QThread::currentThread() != QApplication::instance()->thread()) {
        QMetaObject::invokeMethod(this, [this, code]() {
            loadScripts(code);
        }, Qt::QueuedConnection);
        return;
    }

    // 如果已经有一个脚本在执行，则不启动新的执行
    if (m_jsExecuting) {
        return;
    }

    if (!m_jsEngine) {
        return;
    }

    m_jsExecuting = true;
    emit executingChanged();

    QJSValue result = m_jsEngine->evaluate(code);
    if (result.isError()) {
        qWarning() << "QmlScript执行错误:"
                   << result.property("lineNumber").toInt()
                   << result.toString();
    }

    m_jsExecuting = false;
    emit executingChanged();
}

void QmlScriptDataModel::handleJsExecutionFinished() {
    // 重置执行状态
    m_jsExecuting = false;
    
    QJSValue result = m_jsWatcher.result();
    
    if (result.isError()) {
        qWarning() << "QmlScript执行错误:"
                  << result.property("lineNumber").toInt()
                  << result.toString();
    }
    
}

void QmlScriptDataModel::initJSEngine() {
    // 创建QmlScript引擎
    m_jsEngine = new QJSEngine(this);
    m_jsEngine->installExtensions(QJSEngine::AllExtensions);
    // 创建Node全局对象
    QJSValue jsObject = m_jsEngine->newQObject(this);
    m_jsEngine->globalObject().setProperty("Node", jsObject);

    auto defineBuilder = [this](const QString& name, const QString& type) {
        const QString code = QString("(function(spec){ spec = spec || {}; spec.type = '%1'; return spec; })").arg(type);
        m_jsEngine->globalObject().setProperty(name, m_jsEngine->evaluate(code));
    };

    defineBuilder("SpinBox", "spinbox");
    defineBuilder("VSlider", "slider");
    defineBuilder("HSlider", "slider");
    defineBuilder("CheckBox", "checkbox");
    defineBuilder("LineEdit", "lineedit");
    defineBuilder("ComboBox", "combobox");
    defineBuilder("Label", "label");
    defineBuilder("Button", "button");

    // 先执行脚本，但不直接调用initInterface
    if (!script.isEmpty()) {
        QJSValue result = m_jsEngine->evaluate(script);
        if (result.isError()) {
            qWarning() << "QmlScript初始化错误:"
                      << result.property("lineNumber").toInt()
                      << result.toString();
            return;
        }
        
        // 脚本加载完成后，通过C++方法调用initInterface
        // 这样确保UI操作在主线程中进行
        initInterface();
    }
}

/**
 * @brief 在主线程中执行QmlScript的initInterface函数
 * 解决跨线程UI操作问题
 */
Q_INVOKABLE void QmlScriptDataModel::initInterface() {

    // 确保在主线程中执行
    if (QThread::currentThread() != QApplication::instance()->thread()) {
        // 如果不在主线程，使用QMetaObject::invokeMethod切换到主线程
        QMetaObject::invokeMethod(this, "initInterface", Qt::QueuedConnection);
        return;
    }
    
    if (!m_jsEngine) {
        qWarning() << "QmlScript引擎未初始化";
        return;
    }

    // 检查并执行QmlScript中的initInterface函数
    QJSValue initInterfaceFunc = m_jsEngine->globalObject().property("initInterface");
    if (initInterfaceFunc.isCallable()) {
        QJSValue initResult = initInterfaceFunc.call();
        if (initResult.isError()) {
            qWarning() << "initInterface执行错误:"
                      << initResult.property("lineNumber").toInt()
                      << initResult.toString();
        }
    } else {
        qDebug() << "QmlScript中未找到initInterface函数";
    }
}

/**
 * @brief 处理输入端口事件，并将参数传递给QmlScript函数
 * @param portIndex 触发事件的端口索引
 */
Q_INVOKABLE void QmlScriptDataModel::inputEventHandler(int portIndex)
{
    if (!m_jsEngine) {
        qWarning() << "QmlScript引擎未初始化";
        return;
    }

    // 检查并执行QmlScript中的inputEventHandler函数
    QJSValue inputEventHandlerFunc = m_jsEngine->globalObject().property("inputEventHandler");
    if (inputEventHandlerFunc.isCallable()) {
        // 创建参数列表并传递portIndex
        QJSValueList args;
        args << portIndex;

        // 调用QmlScript函数并传递参数
        QJSValue result = inputEventHandlerFunc.call(args);
        if (result.isError()) {
            qWarning() << "inputEventHandler执行错误:"
                      << result.property("lineNumber").toInt()
                      << result.toString();
        }
    } else {
        qDebug() << "QmlScript中未找到inputEventHandler函数";
    }
}

// 函数级注释：供 QML 调用脚本中的全局函数（用于控件事件回调）。
QVariant QmlScriptDataModel::callJsFunction(const QString &functionName, const QVariantList &args)
{
    if (!m_jsEngine) {
        return QVariant();
    }

    if (QThread::currentThread() != QApplication::instance()->thread()) {
        QVariant result;
        QMetaObject::invokeMethod(this, [this, &result, functionName, args]() {
            result = callJsFunction(functionName, args);
        }, Qt::BlockingQueuedConnection);
        return result;
    }

    QJSValue func = m_jsEngine->globalObject().property(functionName);
    if (!func.isCallable()) {
        return QVariant();
    }

    QJSValueList jsArgs;
    jsArgs.reserve(args.size());
    for (const QVariant &v : args) {
        jsArgs << m_jsEngine->toScriptValue(v);
    }

    const QJSValue ret = func.call(jsArgs);
    if (ret.isError()) {
        qWarning() << "callJsFunction执行错误:"
                   << ret.property("lineNumber").toInt()
                   << ret.toString();
        return QVariant();
    }
    return ret.toVariant();
}

/**
 * @brief 为指定控件生成用于持久化的Key（不依赖脚本手动设置）
 */
QString QmlScriptDataModel::makeAutoSaveKey(QWidget* widget, int id, int x, int y, const QString& field) const
{
    const QString typeName = widget ? QString::fromLatin1(widget->metaObject()->className()) : QString("Widget");
    const QString loc = (x >= 0 && y >= 0) ? QString("r%1c%2").arg(x).arg(y) : QString("id%1").arg(id);
    return QString("%1/%2/%3").arg(typeName, loc, field);
}

/**
 * @brief 自动为控件绑定“变更即保存”逻辑（无需脚本改动）
 */
void QmlScriptDataModel::bindAutoSave(QWidget* widget, int id, int x, int y)
{
    if (!widget) {
        return;
    }

    if (auto spin = qobject_cast<QSpinBox*>(widget)) {
        const QString key = makeAutoSaveKey(widget, id, x, y, "value");
        connect(spin, qOverload<int>(&QSpinBox::valueChanged), this, [this, key](int v) {
            QMutexLocker locker(&m_dataMutex);
            m_settings.insert(key, v);
        });
        return;
    }

    if (auto dspin = qobject_cast<QDoubleSpinBox*>(widget)) {
        const QString key = makeAutoSaveKey(widget, id, x, y, "value");
        connect(dspin, qOverload<double>(&QDoubleSpinBox::valueChanged), this, [this, key](double v) {
            QMutexLocker locker(&m_dataMutex);
            m_settings.insert(key, v);
        });
        return;
    }

    if (auto slider = qobject_cast<QAbstractSlider*>(widget)) {
        const QString key = makeAutoSaveKey(widget, id, x, y, "value");
        connect(slider, &QAbstractSlider::valueChanged, this, [this, key](int v) {
            QMutexLocker locker(&m_dataMutex);
            m_settings.insert(key, v);
        });
        return;
    }

    if (auto edit = qobject_cast<QLineEdit*>(widget)) {
        const QString key = makeAutoSaveKey(widget, id, x, y, "text");
        connect(edit, &QLineEdit::textChanged, this, [this, key](const QString& t) {
            QMutexLocker locker(&m_dataMutex);
            m_settings.insert(key, t);
        });
        return;
    }

    if (auto cb = qobject_cast<QCheckBox*>(widget)) {
        const QString key = makeAutoSaveKey(widget, id, x, y, "checked");
        connect(cb, &QCheckBox::toggled, this, [this, key](bool on) {
            QMutexLocker locker(&m_dataMutex);
            m_settings.insert(key, on);
        });
        return;
    }

    if (auto combo = qobject_cast<QComboBox*>(widget)) {
        const QString keyIndex = makeAutoSaveKey(widget, id, x, y, "index");
        const QString keyText = makeAutoSaveKey(widget, id, x, y, "text");
        connect(combo, qOverload<int>(&QComboBox::currentIndexChanged), this, [this, keyIndex, keyText, combo](int idx) {
            QMutexLocker locker(&m_dataMutex);
            m_settings.insert(keyIndex, idx);
            m_settings.insert(keyText, combo->currentText());
        });
        return;
    }
}

/**
 * @brief 自动恢复控件值（在UI创建完成后调用）
 */
void QmlScriptDataModel::restoreAutoSavedValue(QWidget* widget, int id, int x, int y)
{
    if (!widget) {
        return;
    }

    if (auto spin = qobject_cast<QSpinBox*>(widget)) {
        const QString key = makeAutoSaveKey(widget, id, x, y, "value");
        QVariant v;
        {
            QMutexLocker locker(&m_dataMutex);
            v = m_settings.value(key);
        }
        if (v.isValid()) {
            QSignalBlocker b(spin);
            spin->setValue(v.toInt());
        }
        return;
    }

    if (auto dspin = qobject_cast<QDoubleSpinBox*>(widget)) {
        const QString key = makeAutoSaveKey(widget, id, x, y, "value");
        QVariant v;
        {
            QMutexLocker locker(&m_dataMutex);
            v = m_settings.value(key);
        }
        if (v.isValid()) {
            QSignalBlocker b(dspin);
            dspin->setValue(v.toDouble());
        }
        return;
    }

    if (auto slider = qobject_cast<QAbstractSlider*>(widget)) {
        const QString key = makeAutoSaveKey(widget, id, x, y, "value");
        QVariant v;
        {
            QMutexLocker locker(&m_dataMutex);
            v = m_settings.value(key);
        }
        if (v.isValid()) {
            QSignalBlocker b(slider);
            slider->setValue(v.toInt());
        }
        return;
    }

    if (auto edit = qobject_cast<QLineEdit*>(widget)) {
        const QString key = makeAutoSaveKey(widget, id, x, y, "text");
        QVariant v;
        {
            QMutexLocker locker(&m_dataMutex);
            v = m_settings.value(key);
        }
        if (v.isValid()) {
            QSignalBlocker b(edit);
            edit->setText(v.toString());
        }
        return;
    }

    if (auto cb = qobject_cast<QCheckBox*>(widget)) {
        const QString key = makeAutoSaveKey(widget, id, x, y, "checked");
        QVariant v;
        {
            QMutexLocker locker(&m_dataMutex);
            v = m_settings.value(key);
        }
        if (v.isValid()) {
            QSignalBlocker b(cb);
            cb->setChecked(v.toBool());
        }
        return;
    }

    if (auto combo = qobject_cast<QComboBox*>(widget)) {
        const QString keyIndex = makeAutoSaveKey(widget, id, x, y, "index");
        QVariant v;
        {
            QMutexLocker locker(&m_dataMutex);
            v = m_settings.value(keyIndex);
        }
        if (v.isValid()) {
            const int idx = v.toInt();
            if (idx >= 0 && idx < combo->count()) {
                QSignalBlocker b(combo);
                combo->setCurrentIndex(idx);
            }
        }
        return;
    }
}