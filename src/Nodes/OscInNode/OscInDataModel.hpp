#pragma once

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <iostream>
#include <QPushButton>
#include <QtCore/qglobal.h>
#include "Common/Devices/OSCReceiver/OSCReceiver.h"
#include "OscInInterface.hpp"
#include <QVariantMap>

#include "ConstantDefines.h"
#include "QThread"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
using namespace QtNodes;
namespace Nodes
{
    class OscInDataModel : public AbstractDelegateModel
    {

    public:
        OscInDataModel()
        {
            InPortCount =0;
            OutPortCount=3;
            CaptionVisible=true;
            Caption="OSC Source";
            WidgetEmbeddable=false;
            Resizable=false;
            inData=std::make_shared<VariableData>();
            setup();

        }

        ~OscInDataModel() override {
            delete widget;
        }

        void setup() {

            OSC_Receiver=new OSCReceiver(6000);
            widget=new OscInInterface();
            AbstractDelegateModel::registerOSCControl("/port",widget->portSpinBox);
            AbstractDelegateModel::registerOSCControl("/address",widget->addressEdit);
            AbstractDelegateModel::registerOSCControl("/value",widget->valueEdit);
            connect(OSC_Receiver, &OSCReceiver::receiveOSC, this, &OscInDataModel::getOsc);
            connect(widget,&OscInInterface::portChanged,OSC_Receiver,&OSCReceiver::setPort);

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

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {

            switch (portIndex)
            {
                case 0:
                    return "RESULT";
                case 1:
                    return "ADDRESS";
                case 2:
                    return "VALUE";
                default:
                    return "";
            }
        }

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            switch (port)
            {
                case 0:
                    return inData;
                case 1:
                    return std::make_shared<VariableData>(inData->value("address").toString());
                case 2:
                    return std::make_shared<VariableData>(inData->value().toString());
                default:
                    break;
            }
            return std::make_shared<VariableData>();
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex);
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["Port"] = widget->portSpinBox->value();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                widget->portSpinBox->setValue(v["Port"].toInt());
            }
        }
        QWidget *embeddedWidget() override {

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

    private Q_SLOTS:
        void getOsc(const QVariantMap &data) {
            inData=std::make_shared<VariableData>(data);

            emit dataUpdated(0);
            emit dataUpdated(1);
            emit dataUpdated(2);
            widget->addressEdit->setText(data["address"].toString());
            widget->valueEdit->setText(data["default"].toString());

        }
    private:

        std::shared_ptr<VariableData> inData;
        OSCReceiver *OSC_Receiver;
        OscInInterface *widget;

    };
}