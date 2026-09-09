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

            // 计数仅显示，不可拖动/编辑
            widget->countDisplay->setEnabled(false);
            widget->countDisplay->setAttribute(Qt::WA_TransparentForMouseEvents, true);

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
         * @brief 输出当前计数值
         */
        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex)
            return std::make_shared<VariableData>(m_count);
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override {
            Q_UNUSED(portIndex);
            if (data == nullptr) {
                return;
            }
            m_InData = std::dynamic_pointer_cast<VariableData>(data);
            if (!m_InData) {
                return;
            }
            evaluateAndMaybeCount();
            Q_EMIT dataUpdated(0);
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
            if (m_InData) {
                evaluateAndMaybeCount();
            }
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
         * @brief 清除计数器
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

        /** 用 JS 条件求值；为真则计数 +1 */
        void evaluateAndMaybeCount()
        {
            if (!m_InData || !m_jsEngine) {
                return;
            }

            const QString expression = widget->Editor->text();
            QJSValue jsInput = m_jsEngine->toScriptValue(m_InData->asMap());
            m_jsEngine->globalObject().setProperty("$input", jsInput);

            QJSValue result = m_jsEngine->evaluate(expression);
            if (result.isError()) {
                qDebug() << "JS表达式错误" << result.toString();
                return;
            }

            if (result.toBool()) {
                setCount(m_count + 1);
            }
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
