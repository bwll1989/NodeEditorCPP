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
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
#include "../GUI/Elements/IntDragValueWidget/IntDragValueWidget.hpp"
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;
using namespace NodeDataTypes;
namespace Nodes
{
    /// The model dictates the number of inputs and outputs for the Node.
    /// In this example it has no logic.
    class IntVariableDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(int value READ value WRITE setValue NOTIFY valueChanged)

    public:

        IntVariableDataModel():widget(new IntDragValueWidget()){
            InPortCount =1;
            OutPortCount=1;
            CaptionVisible=true;
            Caption="Int Variable";
            WidgetEmbeddable=true;
            Resizable=false;
            widget->setFixedSize(80,30);
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "value";
            binding.control=widget;
            AbstractDelegateModel::registerExternalBinding("/int", this, binding);
            // AbstractDelegateModel::registerExternalControl("/int",widget);
            widget->setValue(m_value);
            connect(widget, &IntDragValueWidget::valueChanged, this, &IntVariableDataModel::setValue);
            connect(this, &IntVariableDataModel::valueChanged, this, [this](int){
                if (widget->value()!=m_value)
                    widget->blockSignals(true);
                    widget->setValue(m_value);
                    widget->blockSignals(false);
            });
        }


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

        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex)
            return std::make_shared<VariableData>(value());
        }

        /**
         * 函数级注释：获取当前整型属性值
         */
        int value() const
        {
            return m_value;
        }



        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override {
            {
                Q_UNUSED(portIndex);
                if (data== nullptr){
                    setValue(0);
                    return;
                }
                auto textData = std::dynamic_pointer_cast<VariableData>(data);
                int v = 0;
                if (textData && textData->value().canConvert<int>()) {
                    v = textData->value().toInt();
                }
                setValue(v);

            }
        }


        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            return modelJson1;
        }
        void load(const QJsonObject &p) override
        {
        }
        QWidget *embeddedWidget() override {return widget;}

    signals:
        /**
         * 函数级注释：整型值属性发生变化时发出的通知信号
         */
        void valueChanged(int value);

    protected:
        /**
         * 函数级注释：模型就绪后订阅整型地址的命令事件
         */
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/int"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
        }

    private Q_SLOTS:



    public Q_SLOTS:
        /**
         * 函数级注释：处理来自事件总线的整型命令事件，更新控件与状态
         */
        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }
            if (ev.address != makeFullOscAddress("/int")) {
                return;
            }
            bool ok = false;
            int v = ev.payload.toInt(&ok);
            if (!ok) {
                return;
            }
            setValue(v);
        }
        /**
         * 函数级注释：设置当前整型属性值，仅在变化时发出通知
         */
        void setValue(int v)
        {
            m_value = v;
            Q_EMIT dataUpdated(0);
            Q_EMIT valueChanged(v);
        }

    private:
        IntDragValueWidget *widget;
        int m_value = 0;

    };
}
