#pragma once
#include <QtCore/QObject>
#include "NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>

#include <iostream>

#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include "QGridLayout"
#include <QtCore/qglobal.h>
#include "JSEngineDefines/JSEngineDefines.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;
using namespace QtNodes;
using namespace NodeDataTypes;
namespace Nodes
{
    class FilterDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:

        FilterDataModel(){
            InPortCount =1;
            OutPortCount=1;
            Caption="Filter";
            CaptionVisible=true;
            WidgetEmbeddable= true;
            Resizable=true;
            PortEditable=true;
            connect(widget->Editor, &QLineEdit::editingFinished, this, &FilterDataModel::outDataSlot);
            m_jsEngine = new QJSEngine(this);
        }
        ~FilterDataModel() override {
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
         * @brief 条件成立时返回输入数据副本，并将 default 设为条件结果
         */
        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex)

            if (!m_outputData) {
                return std::make_shared<VariableData>();
            }
            return m_outputData;
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override {
            {
                Q_UNUSED(portIndex);
                if (data== nullptr){
                    return;
                }
                m_InData = std::dynamic_pointer_cast<VariableData>(data);
                outDataSlot();

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
            if(m_InData==nullptr) {
               return;
            }

            const QString expression = widget->Editor->text();

            // 将整个输入数据注册为JS全局变量$input
            QJSValue jsInput = m_jsEngine->toScriptValue(m_InData->getMap());
            m_jsEngine->globalObject().setProperty("$input", jsInput);

            const QJSValue result = m_jsEngine->evaluate(expression);

            if (result.isError()) {
                qDebug() << "Filter JS表达式错误:" << result.toString();
                return;
            }

            if (result.toBool()) {
                QVariantMap outputMap = m_InData->getMap();
                outputMap.insert(QStringLiteral("default"), result.toBool());
                m_outputData = std::make_shared<VariableData>(outputMap);
                Q_EMIT dataUpdated(0);
            }
        }
        

    private:
        ConditionInterface *widget=new ConditionInterface();
        std::shared_ptr<VariableData> m_InData;
        std::shared_ptr<VariableData> m_outputData;
        QJSEngine *m_jsEngine = nullptr;


    };
}