#pragma once
#include <QtCore/QObject>
#include "Common/DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include "CountInterface.hpp"
#include <iostream>

#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include "QGridLayout"
#include <QtCore/qglobal.h>
#include "JSEngineDefines/JSEngineDefines.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;

using namespace NodeDataTypes;
namespace Nodes
{
    class CountDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(int count READ count WRITE setCount NOTIFY countChanged)
        Q_PROPERTY(bool clear READ clear WRITE setClear NOTIFY clearChanged)

    public:

        CountDataModel(){
            InPortCount =1;
            OutPortCount=1;
            Caption="Count";
            CaptionVisible=true;
            WidgetEmbeddable= false;
            Resizable=false;
            PortEditable=true;

            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "clear";
                b.control = widget->Clear;
                AbstractDelegateModel::registerExternalBinding("/clear", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "count";
                b.control = widget->countDisplay;
                AbstractDelegateModel::registerExternalBinding("/count", this, b);
            }

            connect(widget->Editor, &QLineEdit::editingFinished, this, &CountDataModel::outDataSlot);
            connect(widget->Clear, &QPushButton::clicked, this, [this]() { setClear(true); });
            m_jsEngine = new QJSEngine(this);
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
                return std::make_shared<VariableData>(m_count);
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
                return std::make_shared<VariableData>(m_count);
            }
            
            // 获取表达式结果的布尔值
            expressionResult = result.toBool();
            
            // 如果表达式结果为true，计数器+1
            if (expressionResult) {
                setCount(m_count + 1);
            }

            return std::make_shared<VariableData>(m_count);
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
    protected:
        /**
         * 函数级注释：模型就绪后订阅全局事件总线，使用包含正确节点ID的完整地址
         */
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/clear"),
                this,
                SLOT(clearExternalCommand(GlobalEvent))
            );
        }
    private slots:
        void outDataSlot() {
            Q_EMIT dataUpdated(0);
        }

        void clearExternalCommand(const GlobalEvent& ev) {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }
            if (ev.payload.toBool()) {
                setClear(true);
            }
        }

        /**
         * @brief 清除计数器值
         */
        void clearCount() {
            setCount(0);
            m_InData=nullptr;
            Q_EMIT dataUpdated(0);
        }

        int count() const { return m_count; }

        void setCount(int v)
        {
            if (m_count == v) {
                return;
            }
            m_count = v;
            {
                const QSignalBlocker blocker(widget->countDisplay);
                widget->countDisplay->setValue(m_count);
            }
            Q_EMIT countChanged(m_count);
        }

        bool clear() const { return m_clear; }

        void setClear(bool v)
        {
            if (!v) {
                return;
            }
            if (m_clear) {
                return;
            }
            m_clear = true;
            Q_EMIT clearChanged(true);
            clearCount();
            m_clear = false;
            Q_EMIT clearChanged(false);
        }

    signals:
        void countChanged(int v);
        void clearChanged(bool v);

    private:
        CountInterface *widget=new CountInterface();
        std::shared_ptr<VariableData> m_InData;
        int m_count=0;
        bool m_clear=false;
        QJSEngine *m_jsEngine = nullptr;


    };
}