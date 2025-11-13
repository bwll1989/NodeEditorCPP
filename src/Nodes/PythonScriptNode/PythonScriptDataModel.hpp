#pragma once

#include <QtCore/QObject>
#include <QtCore/QProcess>
#include <QtCore/QTemporaryFile>
#include "DataTypes/NodeDataList.hpp"
#include "QDir"
#include <QtNodes/NodeDelegateModel>
#include "QThread"
#include <iostream>
#include "PythonScriptInterface.hpp"  // 保持这个引用
#include <QtCore/qglobal.h>
#include "QPushButton"
#include <QDebug>
#include <QLabel>
#include <QLineEdit>
#include <QCheckBox>
#include "SupportWidgets.hpp"
#include <QtConcurrent/QtConcurrentRun>
#include <QFutureWatcher>
#include <QMutex>
#include <QMutexLocker>
#include <QMetaObject>

#include "ConstantDefines.h"
#include "OSCMessage.h"
#include "pybind11/pybind11.h"
#include "pybind11/embed.h"
#include "pybind11/eval.h"
#include "PythonEngineDefines.h"
#include "PythonWorkerThread.hpp"  // 添加PythonWorkerThread头文件
#include "OSCSender/OSCSender.h"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
namespace py = pybind11;

namespace Nodes {

class PythonScriptDataModel : public NodeDelegateModel
{
    Q_OBJECT

public:
    /**
     * @brief 构造函数，初始化PythonScriptDataModel
     */
    PythonScriptDataModel():widget(new PythonScriptInterface())
    {
        InPortCount = 4;
        OutPortCount = 1;
        CaptionVisible = true;
        Caption = "Python Script";
        WidgetEmbeddable = false;
        Resizable = true;
        PortEditable = true;
        inputPortIndex = 0;

        // 初始化PythonWorkerThread而不是直接初始化Python引擎
        initPythonWorkerThread();
        
        connect(widget->codeWidget->importJS,SIGNAL(clicked(bool)), this, SLOT(onRunButtonClicked()));
        connect(widget->codeWidget->updateUI, &QPushButton::clicked, this, [this]() {
            clearLayout();
        });
        

    }



private:
    /**
     * @brief 初始化Python工作线程
     */
    void initPythonWorkerThread();
    
    /**
     * @brief 加载Python脚本到工作线程
     * @param code Python代码字符串
     */
    void loadScripts(QString code);
    
    /**
     * @brief 处理Python执行完成
     * @param success 执行是否成功
     * @param errorMessage 错误消息
     */
    void handlePythonExecutionFinished(bool success, const QString& errorMessage);
    
    /**
     * @brief 处理Python输出数据变化
     * @param portIndex 端口索引
     * @param data 输出数据
     */
    void handleOutputDataChanged(int portIndex, const QVariantMap& data);
    
    /**
     * @brief 处理Python消息
     * @param message 消息内容
     */
    void handlePythonMessage(const QString& message);

public:
    /**
     * @brief 析构函数
     */
    ~PythonScriptDataModel()
    {
        // 停止Python工作线程
        if (m_pythonWorker) {
            m_pythonWorker->stopExecution();
            m_pythonWorker->deleteLater();
        }
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
            InPortCount = p["InPortCount"].toInt();
            OutPortCount = p["OutPortCount"].toInt();
        };
    }
    void stateFeedBack(const QString& oscAddress,QVariant value) override {

        OSCMessage message;
        message.host = AppConstants::EXTRA_FEEDBACK_HOST;
        message.port = AppConstants::EXTRA_FEEDBACK_PORT;
        message.address = "/dataflow/" + getParentAlias() + "/" + QString::number(getNodeID()) + oscAddress;
        message.value = value;
        OSCSender::instance()->sendOSCMessageWithQueue(message);
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
     * @brief 获取输入值（供Python调用）
     * @param portIndex 输入端口索引
     * @return 输入值的Python对象
     */
    Q_INVOKABLE py::object getInputValuePy(int portIndex) {
        if (portIndex >= 0 && portIndex < static_cast<int>(InPortCount) && in_data.contains(portIndex)) {
            return PythonEngineDefines::variantMapToPyDict(in_data[portIndex]);
        }
        return py::dict();
    }
    
    /**
     * @brief 获取输出值（供Python调用）
     * @param portIndex 输出端口索引
     * @return 输出值的Python对象
     */
    Q_INVOKABLE py::object getOutputValuePy(int portIndex) {
        if (portIndex >= 0 && portIndex < static_cast<int>(OutPortCount) && out_data.contains(portIndex)) {
            return PythonEngineDefines::variantMapToPyDict(out_data[portIndex]);
        }
        return py::dict();
    }
    
    /**
     * @brief 设置输出值（供Python调用）
     * @param portIndex 输出端口索引
     * @param value Python值
     */
    Q_INVOKABLE void setOutputValuePy(int portIndex, const py::object& value) {
        if (portIndex >= 0 && portIndex < static_cast<int>(OutPortCount)) {
            QMetaObject::invokeMethod(this, [this, portIndex, value]() {
                QMutexLocker locker(&m_dataMutex);
                out_data[portIndex] = PythonEngineDefines::pyObjectToVariantMap(value);
                emit dataUpdated(portIndex);
            }, Qt::QueuedConnection);
        }
    }

private:
    QMap<unsigned int, QVariantMap> in_data;  // 输入数据存储
    QMap<unsigned int, QVariantMap> out_data; // 输出数据存储
    PythonScriptInterface *widget;           // 界面控件
    unsigned inputPortIndex;                 // 当前输入端口索引
    
    // 添加PythonWorkerThread相关成员变量
    PythonWorkerThread* m_pythonWorker;      // Python工作线程
    
    QMap<int, QWidget*> m_widgets;           // 存储创建的控件
    int m_widgetCounter = 0;                 // 控件ID计数器
    QString script = R"(
# Python脚本示例
def init_interface():
    print("初始化界面")
    pass

def input_event_handler(index):
    print(f"输入事件: {index}")
    value = worker.get_input_value(index)
    print(f"输入值: {value}")
    pass
)";
    QMutex m_dataMutex;  // 添加互斥锁保护数据访问
};
}

