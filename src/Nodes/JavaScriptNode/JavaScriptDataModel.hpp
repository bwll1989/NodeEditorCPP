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
#include <QtConcurrent/QtConcurrentRun>
#include <QFutureWatcher>
#include <QMutex>
#include <QMutexLocker>
#include <QMetaObject>
#include "ConstantDefines.h"
#include "OSCSender/OSCSender.h"
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
using namespace QtNodes;
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
        connect(widget->codeWidget->importJS,SIGNAL(clicked(bool)), this, SLOT(onRunButtonClicked()));
        connect(widget->codeWidget->updateUI, &QPushButton::clicked, this, [this]() {
            clearLayout();
            initInterface();
        });
        // 连接Future完成信号到处理槽
        connect(&m_jsWatcher, &QFutureWatcher<QJSValue>::finished, this, &JavaScriptDataModel::handleJsExecutionFinished);
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
    /**
     * @brief 获取输出数据
     * @param portIndex 输出端口索引
     * @return 输出数据
     */
    std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
    {
        QMutexLocker locker(&m_dataMutex);  // 加锁保护
        if (out_data.contains(portIndex)) {
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
            inputEventHandler(portIndex);
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
    ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex index) const override {
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
private Q_SLOTS:
    /**
     * @brief 手动运行脚本
     */
    void onRunButtonClicked()
    {
        script = widget->codeWidget->saveCode();
        loadScripts(script);
    }
    
    /**
     * @brief 处理JavaScript异步执行完成
     */
    void handleJsExecutionFinished();

    void stateFeedBack(const QString& oscAddress,QVariant value) override {

        OSCMessage message;
        message.host = AppConstants::EXTRA_FEEDBACK_HOST;
        message.port = AppConstants::EXTRA_FEEDBACK_PORT;
        message.address = "/dataflow/" + getParentAlias() + "/" + QString::number(getNodeID()) + oscAddress;
        message.value = value;
        OSCSender::instance()->sendOSCMessageWithQueue(message);
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
    
    /**
     * @brief 在主线程中执行JavaScript的initInterface函数
     * 解决跨线程UI操作问题
     */
    Q_INVOKABLE void initInterface();

    Q_INVOKABLE void inputEventHandler(int portIndex);
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
            // 使用QMetaObject::invokeMethod确保在主线程中执行
            QMetaObject::invokeMethod(this, [this, portIndex, value]() {
                out_data[portIndex] = JSEngineDefines::jsValueToVariantMap(value);
                emit dataUpdated(portIndex);
            }, Qt::QueuedConnection);
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
    QFutureWatcher<QJSValue> m_jsWatcher;    // JavaScript执行Future监视器
    bool m_jsExecuting = false;              // JavaScript是否正在执行
    QString script=R"(
var slider1;
function initInterface() {
    // 在这里编写初始化界面的代码
    // 例如：创建按钮、文本框等
}
function inputEventHandler(index){
    console.log(index)
    console.log(Node.getInputValue(index)["default"])
}
)";
 QMutex m_dataMutex;  // 添加互斥锁保护数据访问
};
}

