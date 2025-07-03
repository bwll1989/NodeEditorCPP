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
    // 删除所有控件
    for (QWidget* widget : m_widgets.values()) {
        unregisterOSCControl(QString("/%1").arg(m_widgets.key(widget)));
        widget->deleteLater();
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

    try {
        // 更新Node对象的属性

        // 执行用户脚本
        QJSValue result = m_jsEngine->evaluate(code);

        if (result.isError()) {
            qWarning() << "JavaScript执行错误:"
                      << result.property("lineNumber").toInt()
                      << result.toString();
        }
    } catch (const std::exception& e) {
        qWarning() << "JavaScript执行异常:" << e.what();
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
        // 先执行脚本
        if (!script.isEmpty()) {
            QJSValue result = m_jsEngine->evaluate(script);

            if (result.isError()) {
                qWarning() << "JavaScript初始化错误:"
                          << result.property("lineNumber").toInt()
                          << result.toString();
                return;
            }

            // 检查并执行initInterface函数（如果存在）
            QJSValue initInterfaceFunc = m_jsEngine->globalObject().property("initInterface");
            if (initInterfaceFunc.isCallable()) {
                QJSValue initResult = initInterfaceFunc.call();
                if (initResult.isError()) {
                    qWarning() << "initInterface执行错误:"
                              << initResult.property("lineNumber").toInt()
                              << initResult.toString();
                }
            }
        }
    }