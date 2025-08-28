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
#include "CustomScriptInterface.hpp"
#include <QtCore/qglobal.h>
#include "QPushButton"
#include <QDebug>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include "SupportWidgets.hpp"
#include "JSEngineDefines/JSEngineDefines.hpp"
#include <QtConcurrent/QtConcurrentRun>
#include <QFutureWatcher>
#include <QMutex>
#include <QMutexLocker>
#include <QMetaObject>
#include <QTimer>
#include "JSPluginManager.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes {

class CustomScriptDataModel : public NodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数，初始化CustomScriptDataModel
     */
    public:
        /**
         * @brief 默认构造函数（保留兼容性）
         */
        CustomScriptDataModel() : widget(new CustomScriptInterface())
        {
            InPortCount = 4;
            OutPortCount = 1;
            CaptionVisible = true;
            Caption = "Custom Script";
            WidgetEmbeddable = false;
            Resizable = true;
            PortEditable = true;
            // 连接Future完成信号到处理槽
            connect(&m_jsWatcher, &QFutureWatcher<QJSValue>::finished, this, &CustomScriptDataModel::handleJsExecutionFinished);

    }
    
    /**
     * @brief 带插件信息的构造函数（推荐使用�?
     * @param pluginInfo JS插件信息
     */
    CustomScriptDataModel(const JSPluginInfo& pluginInfo) : widget(new CustomScriptInterface())
    {
        // 从插件元数据中获取端口配�?
        QJsonObject metadata = pluginInfo.metadata;
        InPortCount = metadata.value("inputs").toInt(1);    // 默认1个输入端�?
        OutPortCount = metadata.value("outputs").toInt(1);  // 默认1个输出端�?
        
        CaptionVisible = true;
        WidgetEmbeddable = metadata.value("embeddable").toBool(false);  // 从元数据获取是否可嵌�?
        Resizable = metadata.value("resizable").toBool(false);          // 从元数据获取是否可调整大�?默认不可缩放
        PortEditable = metadata.value("portEditable").toBool(false);    // 从元数据获取端口是否可编辑，默认不可编辑
        
        // 立即设置插件信息
        m_pluginInfo = pluginInfo;
        Caption = pluginInfo.name.isEmpty() ? "JS Script" : pluginInfo.name;
        
        // 设置脚本内容
        if (!pluginInfo.code.isEmpty()) {
            script = pluginInfo.code;
        }
        
        // // 重新初始化JS引擎并执行脚�?
        initJSEngine();
        connect(&m_jsWatcher, &QFutureWatcher<QJSValue>::finished, this, &CustomScriptDataModel::handleJsExecutionFinished);
    }
    
    /**
     * @brief 析构函数
     */
    ~CustomScriptDataModel()
    {
        widget->deleteLater();
    }
    
    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {

        switch (portType) {
            case PortType::In:
                return "IN "+QString::number(portIndex);
            case PortType::Out:
                return "OUT "+QString::number(portIndex);
            default:
                break;
        }
        return "";
    }


    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        Q_UNUSED(portType)
        return VariableData().type();
    }

    std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
    {
        QMutexLocker locker(&m_dataMutex);
        if (out_data.contains(portIndex)) {
            return std::make_shared<VariableData>(out_data[portIndex]);
        }
        
        return std::make_shared<VariableData>();
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override {
        if (auto d = std::dynamic_pointer_cast<VariableData>(data))
        {
            inputPortIndex = portIndex;
            in_data[portIndex] = d->getMap();
            inputEventHandler(portIndex);
        }
    }

    QWidget *embeddedWidget() override
    {
        return widget;
    }

    QJsonObject save() const override
    {
        QJsonObject modelJson1;

        return modelJson1;
    }

    void load(const QJsonObject &p) override
    {

    }

    /**
     * @brief 设置插件信息（用于动态插件注册）
     */
    void setPluginInfo(const JSPluginInfo& pluginInfo) {
        m_pluginInfo = pluginInfo;
        
        // 更新节点标题
        Caption = pluginInfo.name.isEmpty() ? "JS Script" : pluginInfo.name;
        
        // 设置脚本内容
        if (!pluginInfo.code.isEmpty()) {
            script = pluginInfo.code;
            
            // 重新初始化JS引擎并执行脚�?
            initJSEngine();
        }
    }
    
    /**
     * @brief 获取当前插件信息
     */
    const JSPluginInfo& getPluginInfo() const {
        return m_pluginInfo;
    }
    
private Q_SLOTS:

    
    void handleJsExecutionFinished();
    
private:
    void initJSEngine();
    void loadScripts(QString code);
    
public:
    Q_INVOKABLE unsigned int getInputCount();
    Q_INVOKABLE unsigned int getOutputCount();
    Q_INVOKABLE unsigned int inputIndex();
    Q_INVOKABLE int addToLayout(QObject* widgetObj, int x=-1, int y=-1, int rowSpan=1, int columnSpan=1);
    Q_INVOKABLE void clearLayout();
    Q_INVOKABLE void initInterface();

    Q_INVOKABLE void inputEventHandler(int portIndex);

public slots:
    Q_INVOKABLE QJSValue getInputValue(int portIndex) {
        if (portIndex >= 0 && portIndex < static_cast<int>(InPortCount) && in_data.contains(portIndex)) {
            return JSEngineDefines::variantMapToJSValue(m_jsEngine, in_data[portIndex]);
        }
        return m_jsEngine->newObject();
    }
    
    Q_INVOKABLE QJSValue getOutputValue(int portIndex) {
        if (portIndex >= 0 && portIndex < static_cast<int>(OutPortCount) && out_data.contains(portIndex)) {
            return JSEngineDefines::variantMapToJSValue(m_jsEngine, out_data[portIndex]);
        }
        return m_jsEngine->newObject();
    }
    
    Q_INVOKABLE void setOutputValue(int portIndex, const QJSValue& value) {
        if (portIndex >= 0 && portIndex < static_cast<int>(OutPortCount)) {
            QMetaObject::invokeMethod(this, [this, portIndex, value]() {
                out_data[portIndex] = JSEngineDefines::jsValueToVariantMap(value);
                emit dataUpdated(portIndex);
            }, Qt::QueuedConnection);
        }
    }

private:
    QMap<unsigned int, QVariantMap> in_data;
    QMap<unsigned int, QVariantMap> out_data;
    CustomScriptInterface *widget;
    unsigned inputPortIndex;
    QJSEngine *m_jsEngine;
    QMap<int, QWidget*> m_widgets;
    int m_widgetCounter = 0;
    QFutureWatcher<QJSValue> m_jsWatcher;
    bool m_jsExecuting = false;
    QString script; // 当前插件的脚本内�?
    QMutex m_dataMutex;
    JSPluginInfo m_pluginInfo; // 保留，用于存储当前插件信�?

};

} // namespace Nodes

