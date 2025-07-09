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
#include "SupportWidgets.hpp"
#include "JSEngineDefines/JSEngineDefines.hpp"
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
///
namespace Nodes {


class JavaScriptDataModel : public NodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数，初始化JavaScriptDataModel
     */
    JavaScriptDataModel():widget(new JavaScriptInterface())
    {
        InPortCount = 4;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = "JS Script";
        WidgetEmbeddable = false;
        Resizable = true;
        PortEditable = true;
        inputPortIndex = 0;
        
        // 初始化JavaScript引擎
        initJSEngine();

        connect(widget->codeWidget->run, SIGNAL(clicked(bool)), this, SLOT(onRunButtonClicked()));
        connect(widget->run, SIGNAL(clicked(bool)), this, SLOT(onRunButtonClicked()));

    }
    
    /**
     * @brief 析构函数
     */
    ~JavaScriptDataModel()
    {
        widget->deleteLater();
    }
    
    /**
     * @brief 获取端口标题
     * @param portType 端口类型（输入/输出）
     * @param portIndex 端口索引
     * @return 端口标题字符串
     */
    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        QString in = "➩";
        QString out = "➩";
        switch (portType) {
            case PortType::In:
                return in;
            case PortType::Out:
                return out;
            default:
                break;
        }
        return "";
    }

    /**
     * @brief 获取指定类型的端口数量
     * @param portType 端口类型（输入/输出）
     * @return 端口数量
     */
    unsigned int nPorts(PortType portType) const override
    {
        unsigned int result = 1;

        switch (portType) {
            case PortType::In:
                result = InPortCount;
                break;

            case PortType::Out:
                result = OutPortCount;

            default:
                break;
        }

        return result;
    }

    /**
     * @brief 获取端口数据类型
     * @param portType 端口类型（输入/输出）
     * @param portIndex 端口索引
     * @return 数据类型
     */
    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        Q_UNUSED(portType)
        return VariableData().type();
    }

    /**
     * @brief 获取输出数据
     * @param portIndex 输出端口索引
     * @return 输出数据
     */
    std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
    {
        if (portIndex < out_data.size() && out_data.contains(portIndex)) {
            return std::make_shared<VariableData>(out_data[portIndex]);
        }
        return std::make_shared<VariableData>();
    }

    /**
     * @brief 设置输入数据
     * @param data 输入数据
     * @param portIndex 输入端口索引
     */
    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override {
        if (auto d = std::dynamic_pointer_cast<VariableData>(data))
        {
            inputPortIndex = portIndex;
            in_data[portIndex] = d->getMap();
            onRunButtonClicked();
        }
    }

    /**
     * @brief 获取嵌入式控件
     * @return 控件指针
     */
    QWidget *embeddedWidget() override
    {
        widget->codeWidget->loadCodeFromCode(script);
        return widget;
    }

    /**
     * @brief 保存节点状态
     * @return 包含节点状态的JSON对象
     */
    QJsonObject save() const override
    {
        QJsonObject modelJson1;
        modelJson1["scripts"] = widget->codeWidget->saveCode();
        QJsonObject modelJson = NodeDelegateModel::save();
        modelJson["values"] = modelJson1;
        modelJson["InPortCount"] = int(InPortCount);
        modelJson["OutPortCount"] = int(OutPortCount);
        return modelJson;
    }

    /**
     * @brief 加载节点状态
     * @param p 包含节点状态的JSON对象
     */
    void load(const QJsonObject &p) override
    {
        QJsonValue v = p["values"];
        if (!v.isUndefined() && v.isObject()) {
            script = v["scripts"].toString();
            widget->codeWidget->loadCodeFromCode(script);
            initJSEngine();
            InPortCount = p["InPortCount"].toInt();
            OutPortCount = p["OutPortCount"].toInt();
        };
    }
    
private Q_SLOTS:
    /**
     * @brief 手动运行脚本
     */
    void onRunButtonClicked()
    {
        script = widget->codeWidget->saveCode();
        loadScripts(script);
        Q_EMIT dataUpdated(0);
    }

private:
    /**
     * @brief 初始化JavaScript引擎
     */
    void initJSEngine();
    
    /**
     * @brief 执行JavaScript脚本
     * @param code JavaScript代码
     */
    void loadScripts(QString code);
    
public :

    /**
    * @brief 获取输入端口数量
    * @return 输入端口数量
    */
    Q_INVOKABLE unsigned int getInputCount() ;

    /**
     * @brief 获取输出端口数量
     * @return 输出端口数量
     */
    Q_INVOKABLE unsigned int getOutputCount();

    /**
     * @brief 获取当前输入端口索引
     * @return 输入端口索引
     */
    Q_INVOKABLE unsigned int inputIndex() ;

    /**
     * @brief 添加任意控件到top_layout
     * @param widgetObj 控件对象指针
     * @param x 网格布局的行位置，-1表示使用默认添加方式
     * @param y 网格布局的列位置，-1表示使用默认添加方式
     * @return 控件ID，用于后续引用
     */
    Q_INVOKABLE int addToLayout(QObject* widgetObj, int x=-1, int y=-1,int rowSpan=1, int columnSpan=1);
    /**
     * @brief 清除right_layout中的所有控件
     */
    Q_INVOKABLE void clearLayout() ;

public slots:
    /**
     * @brief 获取输入值（供JavaScript调用）
     * @param portIndex 输入端口索引
     * @return 输入值的JavaScript对象
     */
   Q_INVOKABLE QJSValue getInputValue(int portIndex) {
        if (portIndex >= 0 && portIndex < static_cast<int>(InPortCount) && in_data.contains(portIndex)) {
            return JSEngineDefines::variantMapToJSValue(m_jsEngine,in_data[portIndex]);
        }
        return m_jsEngine->newObject();
    }
    /**
     * @brief 获取输出值（供JavaScript调用）
     * @param portIndex 输出端口索引
     * @return 输出值的JavaScript对象
     */
    Q_INVOKABLE QJSValue getOutputValue(int portIndex) {
        if (portIndex >= 0 && portIndex < static_cast<int>(OutPortCount) && out_data.contains(portIndex)) {
            return JSEngineDefines::variantMapToJSValue(m_jsEngine,out_data[portIndex]);
        }
        return m_jsEngine->newObject();
    }
    
    /**
     * @brief 设置输出值（供JavaScript调用）
     * @param portIndex 输出端口索引
     * @param value JavaScript值
     */
   Q_INVOKABLE void setOutputValue(int portIndex, const QJSValue& value) {
        if (portIndex >= 0 && portIndex < static_cast<int>(OutPortCount)) {
            out_data[portIndex] = JSEngineDefines::jsValueToVariantMap(value);
            emit dataUpdated(portIndex);
        }
    }

private:
    QMap<unsigned int, QVariantMap> in_data;  // 输入数据存储
    QMap<unsigned int, QVariantMap> out_data; // 输出数据存储
    JavaScriptInterface *widget;             // 界面控件
    unsigned inputPortIndex;                 // 当前输入端口索引
    QJSEngine *m_jsEngine;                    // JavaScript引擎
    QMap<int, QWidget*> m_widgets;           // 存储创建的控件
    int m_widgetCounter = 0;                 // 控件ID计数器
    QString script=R"(
var slider1;
function initInterface() {
    // 在这里编写初始化界面的代码
    // 例如：创建按钮、文本框等
}
)";
};
}

