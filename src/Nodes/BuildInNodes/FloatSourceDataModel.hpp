#pragma once


#include <QtCore/QObject>

#include "Common/DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>
#include <iostream>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include "QGridLayout"
#include <QtCore/qglobal.h>
#include "OSCSender/OSCSender.h"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
#include "../GUI/Elements/FloatDragValueWidget/FloatDragValueWidget.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using QtNodes::ConnectionPolicy;
class QLineEdit;

using namespace NodeDataTypes;
namespace Nodes
{
class FloatSourceDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(double value READ value WRITE setValue NOTIFY valueChanged)

    public:

        FloatSourceDataModel(){
            InPortCount =1;
            OutPortCount=1;
            CaptionVisible=true;
            Caption="Float Source";
            WidgetEmbeddable= true;
            Resizable=false;
            widget->setFixedSize(80,30);
            widget->setValue(m_value);
            connect(widget, &FloatDragValueWidget::valueChanged, this, &FloatSourceDataModel::setValue);
            NodeDelegateModel::ExternalBinding binding;
            binding.member = "value";
            binding.control=widget;
            AbstractDelegateModel::registerExternalBinding("/float", this, binding);
            // AbstractDelegateModel::registerExternalControl("/float",widget);
            // registerExternalControl("/float",widget);
            
            connect(this, &FloatSourceDataModel::valueChanged, this, [this](double){
                    widget->blockSignals(true);
                   widget->setValue(m_value);
                   widget->blockSignals(false);
            });
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

        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex)
            return std::make_shared<VariableData>(value());
        }

        /**
         * 函数级注释：获取当前浮点属性值
         */
        double value() const
        {
            return m_value;
        }

        /**
         * 函数级注释：设置当前浮点属性值，仅在变化时发出通知
         */
        void setValue(double v)
        {

            m_value = v;
            Q_EMIT dataUpdated(0);
            Q_EMIT valueChanged(v);
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override {
            {
                Q_UNUSED(portIndex);
                if (data== nullptr){
                    setValue(0.0);
                    return;
                }

                auto textData = std::dynamic_pointer_cast<VariableData>(data);
                double v = 0.0;
                if (textData && textData->value().canConvert<double>()) {
                    v = textData->value().toDouble();
                }
                setValue(v);

            }
        }


        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["val"] = value();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }
        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                setValue(v["val"].toDouble());

            }
        }

        QWidget *embeddedWidget() override {return widget;}

    signals:
        /**
         * 函数级注释：浮点值属性发生变化时发出的通知信号
         */
        void valueChanged(double value);

    protected:
        /**
         * 函数级注释：模型就绪后订阅浮点地址的命令事件
         */
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/float"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
        }

    private Q_SLOTS:

    public Q_SLOTS:
        /**
         * 函数级注释：处理来自事件总线的浮点命令事件，更新控件与状态
         */
        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }
            if (ev.address != makeFullOscAddress("/float")) {
                return;
            }
            bool ok = false;
            double v = ev.payload.toDouble(&ok);
            if (!ok) {
                return;
            }
            setValue(v);
        }

    private:
        FloatDragValueWidget *widget=new FloatDragValueWidget();
        double m_value = 0.0;


    };
}
