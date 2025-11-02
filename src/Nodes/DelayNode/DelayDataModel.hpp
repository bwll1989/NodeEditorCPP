#pragma once

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include "QTimer"
#include <iostream>
#include <QtCore/qglobal.h>
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
            InPortCount =2;
            OutPortCount=1;
            CaptionVisible=true;
            Caption="Delay";
            WidgetEmbeddable= false;
            Resizable=false;
            PortEditable= false;
        }

        ~DelayDataModel() override{
            if(timer->isActive())
            {
                timer->stop();
            }
            deleteLater();
        }

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            Q_UNUSED(portType)
            return VariableData().type();
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch(portType)
            {
            case PortType::In:
                switch(portIndex)
                {
                case 0:
                        return "INPUT";
                case 1:
                        return "TIME";
                default:
                        return "";
                }
            case PortType::Out:
                return "OUTPUT";
            default:
                return "";
            }

        }
        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            Q_UNUSED(port);
            return  inData;
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (data== nullptr){
                return;
            }
            switch(portIndex)
            {
            case 0:
                inData = std::dynamic_pointer_cast<VariableData>(data);
                if(timer->isActive())
                {
                    timer->stop();
                }
                timer->setInterval(widget->value->text().toInt());
                connect(timer,&QTimer::timeout,this,&DelayDataModel::delayFinished);
                timer->setSingleShot(true);
                //            定时器单次执行
                timer->start();
                break;
            case 1:
                widget->value->setText(std::dynamic_pointer_cast<VariableData>(data)->value().toString());
            }

        }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["delay"] = widget->value->text().toInt();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }
        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                widget->value->setText(v.toString());

            }
        }

        QWidget *embeddedWidget() override
        {
            return widget;
        }

        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex index) const override {
            auto result = ConnectionPolicy::One;
            switch (portType) {
                case PortType::In:
                    result = ConnectionPolicy::Many;
                    break;
                case PortType::Out:
                    result = ConnectionPolicy::Many;
                    break;
                case PortType::None:
                    break;
            }

            return result;
        }
        void delayFinished(){
            Q_EMIT dataUpdated(0);
        }
        DelayInterface *widget=new DelayInterface();
        std::shared_ptr<VariableData> inData;
        QTimer *timer=new QTimer();
    };
}