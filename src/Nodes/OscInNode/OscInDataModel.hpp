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
#include "QThread"
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
namespace Nodes
{
    class OscInDataModel : public NodeDelegateModel
    {

    public:
        OscInDataModel()
        {
            InPortCount =0;
            OutPortCount=1;
            CaptionVisible=true;
            Caption="OSC Source";
            WidgetEmbeddable=true;
            Resizable=false;
            inData=std::make_shared<VariableData>();
            setup();

        }

        virtual ~OscInDataModel() override {
            //        OSC_Receiver->deleteLater();
            delete OSC_Receiver;
            delete widget;
        }

        void setup() {

            OSC_Receiver=new OSCReceiver(6000);
            widget=new OscInInterface();
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

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            Q_UNUSED(port);
            return std::make_shared<VariableData>(widget->browser->exportToMap());
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex);
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["Port"] = widget->browser->getProperties("Port").toInt();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                widget->browser->setProperty("Port",v["Port"].toInt());
            }
        }
        QWidget *embeddedWidget() override {

            return widget;
        }
    private Q_SLOTS:
        void getOsc(const QVariantMap &data) {
            inData->insert(data["address"].toString(),data["default"]);

            widget->browser->buildPropertiesFromMap(inData->getMap());
            Q_EMIT dataUpdated(0);
        }
    private:

        std::shared_ptr<VariableData> inData;

        OSCReceiver *OSC_Receiver;
        OscInInterface *widget;

    };
}