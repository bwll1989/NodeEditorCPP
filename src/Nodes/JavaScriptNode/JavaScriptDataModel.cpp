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
#include "JavaScriptInterface.hpp"
#include <QtCore/qglobal.h>
#include "QPushButton"
#include <QDebug>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include "JavaScriptDataModel.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace Nodes;




unsigned int JavaScriptDataModel::getInputCount() {
    return InPortCount;
}


unsigned int JavaScriptDataModel::getOutputCount() {
    return OutPortCount;
}

unsigned int JavaScriptDataModel::inputIndex() {
    return inputPortIndex;
}

Q_INVOKABLE void JavaScriptDataModel::clearLayout() {
    // 确保在主线程中执行
    if (QThread::currentThread() != QApplication::instance()->thread()) {
        QMetaObject::invokeMethod(this, "clearLayout", Qt::BlockingQueuedConnection);
        return;
    }
    
    // 删除所有控件
    for (QWidget* widget : m_widgets.values()) {
        unregisterOSCControl(QString("/%1").arg(m_widgets.key(widget)));
        // 立即删除而不是使用deleteLater
        delete widget;
    }
    m_widgets.clear();
    m_widgetCounter = 0;
}

Q_INVOKABLE int JavaScriptDataModel::addToLayout(QObject* widgetObj, int x, int y,int rowSpan, int columnSpan) {
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
    registerOSCControl(QString("/%1").arg(id),widget_obj);
    return id;
}

void JavaScriptDataModel::loadScripts(QString code) {
    if (code.isEmpty()) {
        return;
    }

    // 如果已经有一个脚本在执行，则不启动新的执行
    if (m_jsExecuting) {
        return;
    }

    try {
        // 标记JavaScript正在执行
        m_jsExecuting = true;
        
        // 使用QtConcurrent在后台线程中执行JavaScript代码
        QFuture<QJSValue> future = QtConcurrent::run(
            [this, code]() -> QJSValue {
                try {
                    // 在后台线程中执行JavaScript代码
                    return m_jsEngine->evaluate(code);
                } catch (const std::exception& e) {
                    qWarning() << "JavaScript执行异常:" << e.what();
                    // 创建一个错误对象返回
                    QJSValue errorObj = m_jsEngine->newObject();
                    errorObj.setProperty("isError", true);
                    errorObj.setProperty("message", QString(e.what()));
                    errorObj.setProperty("lineNumber", -1);
                    return errorObj;
                }
            }
        );
        
        // 设置watcher监视执行结果
        m_jsWatcher.setFuture(future);
    } catch (const std::exception& e) {
        m_jsExecuting = false;
        qWarning() << "JavaScript执行异常:" << e.what();
    }
}

void JavaScriptDataModel::handleJsExecutionFinished() {
    // 重置执行状态
    m_jsExecuting = false;
    
    QJSValue result = m_jsWatcher.result();
    
    if (result.isError()) {
        qWarning() << "JavaScript执行错误:"
                  << result.property("lineNumber").toInt()
                  << result.toString();
    }
    
}

void JavaScriptDataModel::initJSEngine() {
    // 创建JavaScript引擎
    m_jsEngine = new QJSEngine(this);
    m_jsEngine->installExtensions(QJSEngine::AllExtensions);
    // 创建Node全局对象
    QJSValue jsObject = m_jsEngine->newQObject(this);
    m_jsEngine->globalObject().setProperty("Node", jsObject);
    m_jsEngine->globalObject().setProperty("SpinBox",  m_jsEngine->newQMetaObject<SpinBox>());
    m_jsEngine->globalObject().setProperty("VSlider",  m_jsEngine->newQMetaObject<VSlider>());
    m_jsEngine->globalObject().setProperty("HSlider",  m_jsEngine->newQMetaObject<HSlider>());
    m_jsEngine->globalObject().setProperty("CheckBox",  m_jsEngine->newQMetaObject<CheckBox>());
    m_jsEngine->globalObject().setProperty("LineEdit",  m_jsEngine->newQMetaObject<LineEdit>());
    m_jsEngine->globalObject().setProperty("ComboBox",  m_jsEngine->newQMetaObject<ComboBox>());
    m_jsEngine->globalObject().setProperty("Label",  m_jsEngine->newQMetaObject<Label>());
    m_jsEngine->globalObject().setProperty("Button",  m_jsEngine->newQMetaObject<Button>());
    m_jsEngine->globalObject().setProperty("DoubleSpinBox",  m_jsEngine->newQMetaObject<DoubleSpinBox>());
    
    // 先执行脚本，但不直接调用initInterface
    if (!script.isEmpty()) {
        QJSValue result = m_jsEngine->evaluate(script);
        if (result.isError()) {
            qWarning() << "JavaScript初始化错误:"
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
 * @brief 在主线程中执行JavaScript的initInterface函数
 * 解决跨线程UI操作问题
 */
Q_INVOKABLE void JavaScriptDataModel::initInterface() {

    // 确保在主线程中执行
    if (QThread::currentThread() != QApplication::instance()->thread()) {
        // 如果不在主线程，使用QMetaObject::invokeMethod切换到主线程
        QMetaObject::invokeMethod(this, "initInterface", Qt::QueuedConnection);
        return;
    }
    
    if (!m_jsEngine) {
        qWarning() << "JavaScript引擎未初始化";
        return;
    }

    // 检查并执行JavaScript中的initInterface函数
    QJSValue initInterfaceFunc = m_jsEngine->globalObject().property("initInterface");
    if (initInterfaceFunc.isCallable()) {
        QJSValue initResult = initInterfaceFunc.call();
        if (initResult.isError()) {
            qWarning() << "initInterface执行错误:"
                      << initResult.property("lineNumber").toInt()
                      << initResult.toString();
        }
    } else {
        qDebug() << "JavaScript中未找到initInterface函数";
    }
}

/**
 * @brief 处理输入端口事件，并将参数传递给JavaScript函数
 * @param portIndex 触发事件的端口索引
 */
Q_INVOKABLE void JavaScriptDataModel::inputEventHandler(int portIndex)
{
    if (!m_jsEngine) {
        qWarning() << "JavaScript引擎未初始化";
        return;
    }

    // 检查并执行JavaScript中的inputEventHandler函数
    QJSValue inputEventHandlerFunc = m_jsEngine->globalObject().property("inputEventHandler");
    if (inputEventHandlerFunc.isCallable()) {
        // 创建参数列表并传递portIndex
        QJSValueList args;
        args << portIndex;
        
        // 调用JavaScript函数并传递参数
        QJSValue result = inputEventHandlerFunc.call(args);
        if (result.isError()) {
            qWarning() << "inputEventHandler执行错误:"
                      << result.property("lineNumber").toInt()
                      << result.toString();
        }
    } else {
        qDebug() << "JavaScript中未找到inputEventHandler函数";
    }
}