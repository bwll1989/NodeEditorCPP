#pragma once

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <iostream>
#include <QPushButton>
#include <QtCore/qglobal.h>
#include "Common/Devices/OSCSender/OSCSender.h"
#include "OscOutInterface.hpp"
#include <QVariantMap>
#include "QThread"
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;


/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class OscOutDataModel : public NodeDelegateModel
{

    Q_OBJECT
public:
    OscOutDataModel()
    {
        InPortCount =1;
        OutPortCount=0;
        CaptionVisible=true;
        Caption="OSC Output";
        WidgetEmbeddable=true;
        Resizable=false;
        inData=std::make_shared<VariableData>();
        setup();

    }

    virtual ~OscOutDataModel() override {

    }

    void setup() {

      
        widget=new OscOutInterface();
        // connect(widget,&OscOutInterface::hostChanged,OSC_Sender,&OSCSender::setHost);
        connect(this,&OscOutDataModel::onHasOSC,this,[this](OSCMessage &msg){
            qDebug()<<"sendOSCMessageWithQueue";
            OSCSender::instance()->sendOSCMessageWithQueue(msg);
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

    std::shared_ptr<NodeData> outData(PortIndex const port) override
    {
        Q_UNUSED(port);
        return inData;
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
    {

        Q_UNUSED(portIndex);
        if (data== nullptr){
            return;
        }
        auto textData = std::dynamic_pointer_cast<VariableData>(data);
        auto da=textData->getMap();
        OSCMessage msg;
        msg.host = widget->browser->getProperties("Host").toString();
        msg.port = widget->browser->getProperties("Port").toInt();
        msg.address = da["address"].toString();
        msg.value = da["value"];
        emit onHasOSC(msg);
    }

    QJsonObject save() const override
    {
        QJsonObject modelJson1;
        modelJson1["Port"] = widget->browser->getProperties("Port").toInt();
        modelJson1["Host"] = widget->browser->getProperties("Host").toString();
        QJsonObject modelJson  = NodeDelegateModel::save();
        modelJson["values"]=modelJson1;
        return modelJson;
    }

    void load(const QJsonObject &p) override
    {
        QJsonValue v = p["values"];
        if (!v.isUndefined()&&v.isObject()) {
            widget->browser->setProperty("Port",v["Port"].toInt());
            widget->browser->setProperty("Host",v["Host"].toString());
        }
    }

    QWidget *embeddedWidget() override {
        return widget;
    }
Q_SIGNALS:
    void onHasOSC(OSCMessage &data);
private:
    std::shared_ptr<VariableData> inData;
    OscOutInterface *widget;

};
