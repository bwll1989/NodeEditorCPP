#pragma once
#include <QtCore/QObject>
#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include "CountInterface.hpp"
#include <iostream>

#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include "QGridLayout"
#include <QtCore/qglobal.h>
#include "JSEngineDefines/JSEngineDefines.hpp"
#include "ConstantDefines.h"
#include "OSCSender/OSCSender.h"
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;

using namespace NodeDataTypes;
namespace Nodes
{
    class CountDataModel : public NodeDelegateModel
    {
        Q_OBJECT

    public:

        CountDataModel(){
            InPortCount =1;
            OutPortCount=1;
            Caption="Count";
            CaptionVisible=true;
            WidgetEmbeddable= true;
            Resizable=true;
            PortEditable=true;
            NodeDelegateModel::registerOSCControl("/clear",widget->Clear);
            connect(widget->Editor, &QLineEdit::editingFinished, this, &CountDataModel::outDataSlot);
            connect(widget->Clear, &QPushButton::clicked, this, &CountDataModel::clearCount);
            m_jsEngine = new QJSEngine(this);
            clearCount();
        }
        ~CountDataModel() override {
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
            
            // 如果表达式结果为true，计数器+1
            if (expressionResult) {
                count++;
            }
            
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
            modelJson1["count"] = count; // 保存当前计数值
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }
        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                widget->Editor->setText(v["expression"].toString());
                // 加载保存的计数值
                if (!v["count"].isUndefined()) {
                    count = v["count"].toInt();
                }
            }
        }

        QWidget *embeddedWidget() override
        {
            return widget;
        }

        void stateFeedBack(const QString& oscAddress,QVariant value) override {

            OSCMessage message;
            message.host = AppConstants::EXTRA_FEEDBACK_HOST;
            message.port = AppConstants::EXTRA_FEEDBACK_PORT;
            message.address = "/dataflow/" + getParentAlias() + "/" + QString::number(getNodeID()) + oscAddress;
            message.value = value;
            OSCSender::instance()->sendOSCMessageWithQueue(message);
        }
    private slots:
        void outDataSlot() {
            Q_EMIT dataUpdated(0);
        }
        
        /**
         * @brief 清除计数器值
         */
        void clearCount() {
            count = 0;
            Q_EMIT dataUpdated(0);
        }
    private:
        CountInterface *widget=new CountInterface();
        std::shared_ptr<VariableData> m_InData;
        int count=0;
        QJSEngine *m_jsEngine = nullptr;


    };
}