#pragma once

#include "DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include "QTimer"
#include <iostream>
#include <vector>
#include <QtCore/qglobal.h>
#include "QSpinBox"
#include "DelayInterface.hpp"
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
namespace Nodes
{
    /// The model dictates the number of inputs and outputs for the Node.
    /// In this example it has no logic.
    class DelayDataModel : public NodeDelegateModel
    {
        Q_OBJECT


    public:
        DelayDataModel()
        {
            InPortCount =1;
            OutPortCount=1;
            CaptionVisible=true;
            Caption="Delay";
            WidgetEmbeddable= true;
            Resizable=false;
            PortEditable= false;


        }

        virtual ~DelayDataModel() override{
            if(timer->isActive())
            {
                timer->stop();
            }
            deleteLater();

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

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            //        Q_UNUSED(port);
            return  inData;
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {

            if (data== nullptr){
                return;
            }
            if ((inData = std::dynamic_pointer_cast<VariableData>(data))) {
                if(timer->isActive())
                {
                    timer->stop();
                }
                timer->setInterval(widget->value->value());
                connect(timer,&QTimer::timeout,this,&DelayDataModel::delayFinished);
                timer->setSingleShot(true);
                //            定时器单次执行
                timer->start();
            }
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["delay"] = widget->value->value();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }
        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                widget->value->setValue(v["delay"].toInt());

            }
        }

        QWidget *embeddedWidget() override{

            return widget;
        }

        void delayFinished(){
            Q_EMIT dataUpdated(0);
        }
        DelayInterface *widget=new DelayInterface();
        shared_ptr<VariableData> inData;
        QTimer *timer=new QTimer();
    };
}