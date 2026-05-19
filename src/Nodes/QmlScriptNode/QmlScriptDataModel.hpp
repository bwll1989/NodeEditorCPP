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
#include <QCheckBox>
#include "JSEngineDefines/SupportWidgets.hpp"
#include "JSEngineDefines/JSEngineDefines.hpp"
#include <QtConcurrent/QtConcurrentRun>
#include <QFutureWatcher>
#include <QMutex>
#include <QMutexLocker>
#include <QMetaObject>
#include <QMetaType>
#include <QVariantMap>
#include "Common/BuildInNodes/AbstractDelegateModel.h"
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


class QmlScriptDataModel : public AbstractDelegateModel
{
    Q_OBJECT

public:
    Q_PROPERTY(QVariantMap settings READ settings NOTIFY settingsChanged)
    Q_PROPERTY(bool executing READ executing NOTIFY executingChanged)
    Q_PROPERTY(QVariantList uiSchema READ uiSchema NOTIFY uiSchemaChanged)

    /**
     * @brief 构造函数，初始化QmlScriptDataModel
     */
    QmlScriptDataModel():widget(new QmlScriptInterface())
    {
        InPortCount = 4;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = "JS Script";
        WidgetEmbeddable = false;
        Resizable = true;
        PortEditable = true;
        inputPortIndex = 0;

        widget->setBackend(this);

        // 初始化QmlScript引擎
        initJSEngine();
        connect(widget->codeWidget->importJS,SIGNAL(clicked(bool)), this, SLOT(onRunButtonClicked()));
        connect(widget->codeWidget->updateUI, &QPushButton::clicked, this, [this]() {
            clearLayout();
            initInterface();
        });
        // 连接Future完成信号到处理槽
        connect(&m_jsWatcher, &QFutureWatcher<QJSValue>::finished, this, &QmlScriptDataModel::handleJsExecutionFinished);
    }

    // 函数级注释：供 QML 读取的 settings（会随工程保存）。
    QVariantMap settings() const
    {
        QMutexLocker locker(&m_dataMutex);
        return m_settings;
    }

    // 函数级注释：供 QML 判断当前是否正在执行脚本。
    bool executing() const
    {
        return m_jsExecuting;
    }

    // 函数级注释：QML 触发脚本运行（等价于点击原“运行”按钮）。
    Q_INVOKABLE void runScript();

    // 函数级注释：QML 打开脚本编辑器窗口。
    Q_INVOKABLE void openEditor();

    // 函数级注释：QML 写入持久化配置（key/value），用于面板表单编辑。
    Q_INVOKABLE void setSettingValue(const QString& key, const QVariant& value);

    // 函数级注释：QML 删除持久化配置项。
    Q_INVOKABLE void removeSetting(const QString& key);

    // 函数级注释：从脚本设置 UI Schema（数组：[{type,key,label,...}]），用于 QML 动态生成控件。
    Q_INVOKABLE void setUiSchema(const QJSValue& schema);

    // 函数级注释：供 QML 读取当前 UI Schema。
    QVariantList uiSchema() const
    {
        QMutexLocker locker(&m_dataMutex);
        return m_uiSchema;
    }

    // 函数级注释：供 QML 调用脚本中的全局函数（用于控件事件回调）。
    Q_INVOKABLE QVariant callJsFunction(const QString& functionName, const QVariantList& args = QVariantList());
    
    /**
     * @brief 析构函数
     */
    ~QmlScriptDataModel()
    {

    }
    
    /**
     * @brief 获取端口标题
     * @param portType 端口类型（输入/输出）
     * @param portIndex 端口索引
     * @return 端口标题字符串
     */
    Q_INVOKABLE QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {

        switch (portType) {
            case PortType::In:
                return "OUT "+QString::number(portIndex);
            case PortType::Out:
                return "IN "+QString::number(portIndex);
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
    Q_INVOKABLE unsigned int nPorts(PortType portType) const override
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
        {
            QMutexLocker locker(&m_dataMutex);
            modelJson1["settings"] = QJsonObject::fromVariantMap(m_settings);
        }
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
            const QJsonObject valuesObj = v.toObject();
            script = valuesObj["scripts"].toString();
            {
                QMutexLocker locker(&m_dataMutex);
                m_settings = valuesObj["settings"].toObject().toVariantMap();
            }
            emit settingsChanged();
            clearLayout();
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
    }
    
    /**
     * @brief 处理QmlScript异步执行完成
     */
    void handleJsExecutionFinished();


private:
    /**
     * @brief 初始化QmlScript引擎
     */
    void initJSEngine();
    
    /**
     * @brief 执行QmlScript脚本
     * @param code QmlScript代码
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
     * @brief 在主线程中执行QmlScript的initInterface函数
     * 解决跨线程UI操作问题
     */
    Q_INVOKABLE void initInterface();

    Q_INVOKABLE void inputEventHandler(int portIndex);
public slots:
    /**
     * @brief 获取输入值（供QmlScript调用）
     * @param portIndex 输入端口索引
     * @return 输入值的QmlScript对象
     */
   Q_INVOKABLE QJSValue getInputValue(int portIndex) {
        if (portIndex >= 0 && portIndex < static_cast<int>(InPortCount) && in_data.contains(portIndex)) {
            return JSEngineDefines::variantMapToJSValue(m_jsEngine,in_data[portIndex]);
        }
        return m_jsEngine->newObject();
    }
    /**
     * @brief 获取输出值（供QmlScript调用）
     * @param portIndex 输出端口索引
     * @return 输出值的QmlScript对象
     */
    Q_INVOKABLE QJSValue getOutputValue(int portIndex) {
        if (portIndex >= 0 && portIndex < static_cast<int>(OutPortCount) && out_data.contains(portIndex)) {
            return JSEngineDefines::variantMapToJSValue(m_jsEngine,out_data[portIndex]);
        }
        return m_jsEngine->newObject();
    }
    
    /**
     * @brief 设置输出值（供QmlScript调用）
     * @param portIndex 输出端口索引
     * @param value QmlScript值
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

    /**
     * @brief 保存单个键值到节点持久化设置（会随工程文件保存）
     * @param key 键
     * @param value 值（任意JS值，内部转为QVariant保存）
     */
    Q_INVOKABLE void setSetting(const QString& key, const QJSValue& value)
    {
        {
            QMutexLocker locker(&m_dataMutex);
            m_settings.insert(key, value.toVariant());
        }
        emit settingsChanged();
    }

    /**
     * @brief 批量设置持久化配置（对象的每个属性都会写入settings）
     * @param obj JS对象
     */
    Q_INVOKABLE void setSettings(const QJSValue& obj)
    {
        if (!obj.isObject()) {
            return;
        }
        {
            QMutexLocker locker(&m_dataMutex);
            QJSValueIterator it(obj);
            while (it.hasNext()) {
                it.next();
                m_settings.insert(it.name(), it.value().toVariant());
            }
        }
        emit settingsChanged();
    }

    /**
     * @brief 读取单个持久化配置
     * @param key 键
     * @return 对应的JS值；若不存在则为undefined
     */
    Q_INVOKABLE QJSValue getSetting(const QString& key)
    {
        if (!m_jsEngine || !m_settings.contains(key)) {
            return QJSValue();
        }
        return m_jsEngine->toScriptValue(m_settings.value(key));
    }

    /**
     * @brief 判断持久化配置是否存在
     * @param key 键
     * @return true存在，false不存在
     */
    Q_INVOKABLE bool hasSetting(const QString& key) const
    {
        return m_settings.contains(key);
    }

    /**
     * @brief 获取全部持久化配置
     * @return JS对象（包含所有settings键值）；若引擎未初始化则为undefined
     */
    Q_INVOKABLE QJSValue getSettings()
    {
        if (!m_jsEngine) {
            return QJSValue();
        }
        QJSValue obj = m_jsEngine->newObject();
        for (auto it = m_settings.constBegin(); it != m_settings.constEnd(); ++it) {
            obj.setProperty(it.key(), m_jsEngine->toScriptValue(it.value()));
        }
        return obj;
    }

signals:
    // 函数级注释：settings 发生变化时通知 QML 刷新。
    void settingsChanged();

    // 函数级注释：脚本执行状态变化时通知 QML 刷新。
    void executingChanged();

    // 函数级注释：UI Schema 发生变化时通知 QML 重建动态控件。
    void uiSchemaChanged();

private:
    /**
     * @brief 为指定控件生成用于持久化的Key（不依赖脚本手动设置）
     * @param widget 控件指针
     * @param id addToLayout返回的控件ID
     * @param x 网格行（-1表示未指定）
     * @param y 网格列（-1表示未指定）
     * @param field 字段名（如value/text/checked/index）
     * @return settings key
     */
    QString makeAutoSaveKey(QWidget* widget, int id, int x, int y, const QString& field) const;

    /**
     * @brief 自动为控件绑定“变更即保存”逻辑（无需脚本改动）
     * @param widget 控件指针
     * @param id 控件ID
     * @param x 网格行
     * @param y 网格列
     */
    void bindAutoSave(QWidget* widget, int id, int x, int y);

    /**
     * @brief 自动恢复控件值（在UI创建完成后调用）
     * @param widget 控件指针
     * @param id 控件ID
     * @param x 网格行
     * @param y 网格列
     */
    void restoreAutoSavedValue(QWidget* widget, int id, int x, int y);

    QMap<unsigned int, QVariantMap> in_data;  // 输入数据存储
    QMap<unsigned int, QVariantMap> out_data; // 输出数据存储
    QmlScriptInterface *widget;             // 界面控件
    unsigned inputPortIndex;                 // 当前输入端口索引
    QJSEngine *m_jsEngine;                    // QmlScript引擎
    QMap<int, QWidget*> m_widgets;           // 存储创建的控件
    int m_widgetCounter = 0;                 // 控件ID计数器
    QFutureWatcher<QJSValue> m_jsWatcher;    // QmlScript执行Future监视器
    bool m_jsExecuting = false;              // QmlScript是否正在执行
    QString script=R"(
function initInterface() {
    Node.setUiSchema([
        VSlider({ key: "gain", label: "Gain", min: 0, max: 1, step: 0.01, default: 0.5, onChanged: "onGain" }),
        SpinBox({ key: "count", label: "Count", min: 0, max: 100, step: 1, default: 10, onChanged: "onCount" }),
        CheckBox({ key: "enabled", label: "Enabled", default: true, onChanged: "onEnabled" }),
        Button({ text: "手动运行", onClicked: "run" })
    ]);
}

function onSettingChanged(key, value) {
    console.log("setting changed", key, value);
}

function onGain(key, value) {
    console.log("gain", value);
}

function onCount(key, value) {
    console.log("count", value);
}

function onEnabled(key, value) {
    console.log("enabled", value);
}

function run() {
    console.log("run clicked");
}

function inputEventHandler(index){
    console.log(index)
    console.log(Node.getInputValue(index)["default"])
}
)";
    QVariantMap m_settings;                  // 自动持久化的控件配置
    QVariantList m_uiSchema;                 // 动态控件声明（供QML组件化渲染）
    mutable QMutex m_dataMutex;  // 添加互斥锁保护数据访问
};
}

