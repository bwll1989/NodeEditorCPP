#pragma once

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <iostream>
#include <QPushButton>
#include <QtCore/qglobal.h>
#include "Common/Devices/ArtnetReceiver/ArtnetReceiver.h"
#include "ArtnetInInterface.hpp"
#include <QVariantMap>
#include "QThread"
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;


/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class ArtnetInDataModel : public NodeDelegateModel
{

public:
    ArtnetInDataModel()
    {
        InPortCount =0;
        OutPortCount=1;
        CaptionVisible=true;
        Caption="Artnet Source";
        WidgetEmbeddable=true;
        Resizable=false;
        inData=std::make_shared<VariableData>();
        setup();

    }

    virtual ~ArtnetInDataModel() override {
//        OSC_Receiver->deleteLater();
        delete OSC_Receiver;
        delete widget;
    }

    void setup() {

        connect(OSC_Receiver, &ArtnetReceiver::receiveOSC, this, &ArtnetInDataModel::getOsc);
        connect(widget,&ArtnetInInterface::UniverseChanged,OSC_Receiver,&ArtnetReceiver::universeFilter);

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
        return inData;
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
    {
        Q_UNUSED(portIndex);
    }

    QJsonObject save() const override
    {
        QJsonObject modelJson1;
        modelJson1["Universe"] = widget->browser->getProperties("Universe").toInt();
        QJsonObject modelJson  = NodeDelegateModel::save();
        modelJson["values"]=modelJson1;
        return modelJson;
    }

    void load(const QJsonObject &p) override
    {
        QJsonValue v = p["values"];
        if (!v.isUndefined()&&v.isObject()) {
            widget->browser->setProperty("Universe",v["Universe"].toInt());
        }
    }
    QWidget *embeddedWidget() override {

        return widget;
    }
private Q_SLOTS:
    void getOsc(const QVariantMap &data) {
//        inData->insert(data["address"].toString(),data["default"]);
        widget->browser->buildPropertiesFromMap(data);
        Q_EMIT dataUpdated(0);
    }
private:

    std::shared_ptr<VariableData> inData;

    ArtnetReceiver * OSC_Receiver=new ArtnetReceiver();

    ArtnetInInterface * widget=new ArtnetInInterface();

};
