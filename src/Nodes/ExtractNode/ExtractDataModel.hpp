#pragma once


#include <QtCore/QObject>

#include "DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>
#include <iostream>

#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include "QGridLayout"
#include <QtCore/qglobal.h>
#include "JSEngineDefines/JSEngineDefines.hpp"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;
using namespace QtNodes;
using namespace NodeDataTypes;
namespace Nodes
{
    class ExtractDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:

        ExtractDataModel(){
            InPortCount =1;
            OutPortCount=1;
            Caption="Extract";
            CaptionVisible=true;
            WidgetEmbeddable= true;
            Resizable=false;
            widget->setPlaceholderText("JS Expression (e.g., \"input['key']\")");
            connect(widget, &QLineEdit::editingFinished, this, &ExtractDataModel::outDataSlot);
            m_jsEngine = new QJSEngine(this);
        }
        ~ExtractDataModel() override {
            if(m_jsEngine) {
                delete m_jsEngine;
                m_jsEngine = nullptr;
            }
        }
    public:
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            switch (portType) {
            case PortType::In:
                return VariableData().type();
            case PortType::Out:
                return VariableData().type();
            case PortType::None:
                break;
            default:
                break;
            }
            // FIXME: control may reach end of non-void function [-Wreturn-type]

            return VariableData().type();
        }

        /**
         * @brief 处理输出数据，支持JS表达式解析
         * @param portIndex 端口索引
         * @return 提取后的数据
         */
        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            // 函数说明：
            // 1. 将 VariableData（QVariantMap）整体导出为 JS 对象，注入到 QJSEngine 的全局上下文；
            // 2. 同时把顶层键作为全局变量注入，支持直接使用键名访问（兼容之前的用法）；
            // 3. 对用户输入的 JS 表达式进行 evaluate，返回结果作为 VariableData；
            // 4. 表达式错误时返回空 VariableData，并打印错误日志。
            Q_UNUSED(portIndex)
            if (m_proprtyData == nullptr) {
                return std::make_shared<VariableData>();
            }
        
            // 获取输入数据的完整映射
            const QVariantMap dataMap = m_proprtyData->getMap();
        
            // 获取表达式（支持任意 JS 表达式）
            const QString expression = widget->text().trimmed();
        
            // 如果表达式为空，直接返回整份 VariableData，便于下游节点整体使用
            if (expression.isEmpty()) {
                return std::make_shared<VariableData>(dataMap);
            }
        
            // 将整份 VariableData 导出为 JS 对象并注入上下文
            QJSValue jsData = JSEngineDefines::variantMapToJSValue(m_jsEngine, dataMap);
            QJSValue global = m_jsEngine->globalObject();
        
            // 保持兼容：提供 data 与 obj 两个入口变量
            global.setProperty("input", jsData);
        
            // 便捷访问：注入顶层键到全局对象，支持直接编写 key.subkey 的表达式
            for (auto it = dataMap.begin(); it != dataMap.end(); ++it) {
                global.setProperty(it.key(), m_jsEngine->toScriptValue(it.value()));
            }
        
            // 执行用户表达式
            QJSValue result = m_jsEngine->evaluate(expression);
            if (result.isError()) {
                qDebug() << "JS表达式错误:" << result.toString();
                return std::make_shared<VariableData>();
            }
        
            // 返回结果（自动保持原始类型，如对象/数组/数值/字符串/布尔等）
            return std::make_shared<VariableData>(result.toVariant());
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override {
            {
                Q_UNUSED(portIndex);
                if (data== nullptr){
                    return;
                }
                m_proprtyData = std::dynamic_pointer_cast<VariableData>(data);
                Q_EMIT dataUpdated(0);

            }
        }


        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["key"] = widget->text();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }
        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                widget->setText(v["key"].toString());

            }
        }

        QWidget *embeddedWidget() override {return widget;}

    private slots:
        void outDataSlot() {
            Q_EMIT dataUpdated(0);
        }
    private:
        QLineEdit *widget=new QLineEdit();
        std::shared_ptr<VariableData> m_proprtyData;
        QJSEngine *m_jsEngine = nullptr;


    };
}