#pragma once


#include <QtCore/QObject>

#include "DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>
#include "AbstractDelegateModel.h"

#include <iostream>
#include <QtWidgets/QLineEdit>

#include <QtWidgets/QPushButton>
#include <QtCore/qglobal.h>
#include "ConstantDefines.h"
#include "StatusContainer/GlobalEventBus.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using QtNodes::ConnectionPolicy;
class QLineEdit;
class QPushButton;
using namespace NodeDataTypes;
namespace Nodes {
    class BoolPluginDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(bool value READ value WRITE setValue NOTIFY valueChanged)

    public:

        BoolPluginDataModel(): button(new QPushButton("Toggle")){
            InPortCount =1;
            OutPortCount=1;
            CaptionVisible=true;
            Caption="Bool Source";
            WidgetEmbeddable=true;
            Resizable=false;
            button->setCheckable(true);
            // button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            button->setMinimumWidth(80);
            AbstractDelegateModel::registerExternalControl("/bool", button);
            button->setChecked(false);
            // 用户点击直接驱动属性 setValue
            connect(button, &QPushButton::clicked, this, &BoolPluginDataModel::setValue);
            // 属性变化统一更新界面并发送反馈、通知输出端口
            connect(this, &BoolPluginDataModel::valueChanged, this, [this](bool){
                // 1. 界面状态和属性自动对齐
                if (button->isChecked() != lastValue.toBool()) {
                    button->setChecked(lastValue.toBool());
                }   
                stateFeedBack("/bool", lastValue);
            });
        }
        ~BoolPluginDataModel(){

        }

    public:

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            Q_UNUSED(portType)
            return VariableData().type();

        }

        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex)
            return std::make_shared<VariableData>(lastValue);
        }
        /**
         * 函数级注释：获取当前布尔属性值
         */
        bool value() const { return lastValue.toBool(); }
        /**
         * 函数级注释：设置当前布尔属性值，仅在变化时发出通知
         */
        void setValue(bool v)
        {
            lastValue = v;
            Q_EMIT dataUpdated(0);
            Q_EMIT valueChanged(v);
        }
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override{

            Q_UNUSED(portIndex)
            if (data== nullptr){
                return;
            }
            auto textData = std::dynamic_pointer_cast<VariableData>(data);
            bool v = false;
            if (textData && textData->value().canConvert<bool>()) {
                v = textData->value().toBool();
            }
            // 端口输入同样通过属性接口驱动业务
            setValue(v);
        }


        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["val"] = button->isChecked();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }
        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                button->setChecked(v["val"].toBool(false));
            }
        }
        QWidget *embeddedWidget() override{return button;}

    signals:
        /**
         * 函数级注释：布尔值属性发生变化时发出的通知信号
         */
        void valueChanged(bool value);

    protected:
        /**
         * 函数级注释：模型就绪后订阅全局事件总线，使用包含正确节点ID的完整地址
         */
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/bool"),
                this,
                SLOT(onValueChanged(GlobalEvent))
            );
        }

    private Q_SLOTS:


        /**
         * 函数级注释：处理外部事件总线输入，调用 setValue 更新布尔属性
         */
        void onValueChanged(const GlobalEvent& ev)
        {
            // 仅处理当前节点地址上的命令事件，避免状态反馈导致重复执行
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }
            if (ev.address != makeFullOscAddress("/bool")) {
                return;
            }
            setValue(ev.payload.toBool());
        }
       
    private:
        QPushButton *button;
        QVariant lastValue=false;
    };
}
