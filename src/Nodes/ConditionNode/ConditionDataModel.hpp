#pragma once
#include <QtCore/QObject>
#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include "ConditionInterface.hpp"
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
    class ConditionDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:

        ConditionDataModel(){
            InPortCount =1;
            OutPortCount=1;
            Caption="Condition";
            CaptionVisible=true;
            WidgetEmbeddable= true;
            Resizable=true;
            PortEditable=true;
            connect(widget->Editor, &QLineEdit::editingFinished, this, &ConditionDataModel::outDataSlot);
            m_jsEngine = new QJSEngine(this);
        }
        ~ConditionDataModel() override {
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
         * @brief 处理输出数据，使用JS引擎评估表达式，并在表达式成立时计数器+1
         * @param portIndex 端口索引
         * @return 提取后的数据，包含当前计数值
         */
        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex)
            if(m_InData==nullptr) {
                // 输入数据为空时，直接返回当前计数值
                return std::make_shared<VariableData>(count);
            }
            
            QString expression = widget->Editor->text();
            bool expressionResult = false;
            
            // 将整个输入数据注册为JS全局变量$input
            QJSValue jsInput = m_jsEngine->toScriptValue(m_InData->getMap());
            m_jsEngine->globalObject().setProperty("$input", jsInput);
            
            // 执行表达式
            QJSValue result = m_jsEngine->evaluate(expression);
            
            if (result.isError()) {
                qDebug() << "JS表达式错误:" << result.toString();
                return std::make_shared<VariableData>(count);
            }
            
            // 获取表达式结果的布尔值
            expressionResult = result.toBool();
            count=expressionResult;
            // 返回当前计数值
            return std::make_shared<VariableData>(count);
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override {
            {
                Q_UNUSED(portIndex);
                if (data== nullptr){
                    return;
                }
                m_InData = std::dynamic_pointer_cast<VariableData>(data);
                Q_EMIT dataUpdated(0);

            }
        }


        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["expression"] = widget->Editor->text();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }
        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                widget->Editor->setText(v["expression"].toString());
            }
        }

        QWidget *embeddedWidget() override
        {
            return widget;
        }

    private slots:
        void outDataSlot() {
            Q_EMIT dataUpdated(0);
        }
        

    private:
        ConditionInterface *widget=new ConditionInterface();
        std::shared_ptr<VariableData> m_InData;
        bool count=false;
        QJSEngine *m_jsEngine = nullptr;


    };
}