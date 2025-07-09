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

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;

using namespace NodeDataTypes;
namespace Nodes
{
    class ExtractDataModel : public NodeDelegateModel
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
            Q_UNUSED(portIndex)
            if(m_proprtyData==nullptr) {
                // m_proprtyData默认为空指针
                return std::make_shared<VariableData>();
            }
            
            QString expression = widget->text();
            // 如果表达式不包含点号，使用原始的value方法
            if (!expression.contains(".")) {
                auto data = m_proprtyData->value(expression);
                return std::make_shared<VariableData>(data);
            }
            
            // 处理包含点号的JS表达式
            QStringList parts = expression.split(".");
            QString rootKey = parts.first();
            QVariant rootValue = m_proprtyData->value(rootKey);
            
            if (rootValue.isNull()) {
                return std::make_shared<VariableData>();
            }
            
            // 将数据转换为JS对象
            QJSValue jsObject;
            if (rootValue.typeId() == QMetaType::QVariantMap) {
                jsObject = JSEngineDefines::variantMapToJSValue(m_jsEngine, rootValue.toMap());
            } else {
                jsObject = m_jsEngine->toScriptValue(rootValue);
            }
            
            // 构建完整的JS表达式
            QString jsExpression = "obj";
            for (int i = 1; i < parts.size(); ++i) {
                jsExpression += "." + parts[i];
            }
            
            // 设置JS上下文并执行表达式
            m_jsEngine->globalObject().setProperty("obj", jsObject);
            QJSValue result = m_jsEngine->evaluate(jsExpression);
            
            if (result.isError()) {
                qDebug() << "JS表达式错误:" << result.toString();
                return std::make_shared<VariableData>();
            }
            // 返回结果
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
        QLineEdit *widget=new QLineEdit("default");
        std::shared_ptr<VariableData> m_proprtyData;
        QJSEngine *m_jsEngine = nullptr;


    };
}