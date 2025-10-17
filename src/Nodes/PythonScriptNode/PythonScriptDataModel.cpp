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
#include "PythonScriptInterface.hpp"
#include <QtCore/qglobal.h>
#include "QPushButton"
#include <QDebug>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include "PythonScriptDataModel.hpp"
#include "PythonWorkerThread.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace Nodes;




unsigned int PythonScriptDataModel::getInputCount() {
    return InPortCount;
}


unsigned int PythonScriptDataModel::getOutputCount() {
    return OutPortCount;
}

unsigned int PythonScriptDataModel::inputIndex() {
    return inputPortIndex;
}

Q_INVOKABLE void PythonScriptDataModel::clearLayout() {
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

Q_INVOKABLE int PythonScriptDataModel::addToLayout(QObject* widgetObj, int x, int y,int rowSpan, int columnSpan) {
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

/**
 * @brief 初始化Python工作线程
 */
void PythonScriptDataModel::initPythonWorkerThread() {
    // 创建Python工作线程
    m_pythonWorker = new PythonWorkerThread(this);
    
    // 设置端口数量
    m_pythonWorker->setPortCounts(InPortCount, OutPortCount);
    
    // 连接信号槽
    connect(m_pythonWorker, &PythonWorkerThread::executionFinished,
            this, &PythonScriptDataModel::handlePythonExecutionFinished);
    
    connect(m_pythonWorker, &PythonWorkerThread::outputDataChanged,
            this, &PythonScriptDataModel::handleOutputDataChanged);
    
    connect(m_pythonWorker, &PythonWorkerThread::messageReceived,
            this, &PythonScriptDataModel::handlePythonMessage);

}

/**
 * @brief 加载Python脚本到工作线程
 * @param code Python代码字符串
 */
void PythonScriptDataModel::loadScripts(QString code) {
    if (code.isEmpty()) {
        return;
    }
    
    if (!m_pythonWorker) {
        qWarning() << "Python工作线程未初始化";
        return;
    }
    
    // 更新端口数量
    m_pythonWorker->setPortCounts(InPortCount, OutPortCount);
    
    // 设置当前输入数据到工作线程
    {
        QMutexLocker locker(&m_dataMutex);
        for (auto it = in_data.begin(); it != in_data.end(); ++it) {
            m_pythonWorker->setInputData(it.key(), it.value());
        }
    }
    
    // 设置脚本并启动执行
    m_pythonWorker->setScript(code);
    m_pythonWorker->startExecution();
}

/**
 * @brief 处理Python执行完成
 * @param success 执行是否成功
 * @param errorMessage 错误消息
 */
void PythonScriptDataModel::handlePythonExecutionFinished(bool success, const QString& errorMessage) {
    if (success) {
        qDebug() << "Python脚本执行成功";
        // 可以在这里调用初始化界面等后续操作

    } else {
        qDebug() << "Python脚本执行失败:" << errorMessage;
    }
}

/**
 * @brief 处理Python输出数据变化
 * @param portIndex 端口索引
 * @param data 输出数据
 */
void PythonScriptDataModel::handleOutputDataChanged(int portIndex, const QVariantMap& data) {
    {
        QMutexLocker locker(&m_dataMutex);
        out_data[portIndex] = data;
    }
    
    // 发射数据更新信号
    emit dataUpdated(portIndex);
    
    qDebug() << "输出端口" << portIndex << "数据已更新";
}

/**
 * @brief 处理Python消息
 * @param message 消息内容
 */
void PythonScriptDataModel::handlePythonMessage(const QString& message) {
    qDebug() << "Python消息:" << message;
}

