#pragma once

#include "Nodes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include "ValueDisplayInterface.hpp"
#include <iostream>
#include <vector>
#include <QtCore/qglobal.h>

using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;


/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class ValueDisplayDataModel : public NodeDelegateModel
{
    Q_OBJECT

public:
    ValueDisplayDataModel()
    {
        InPortCount =1;
        OutPortCount=1;
        CaptionVisible=true;
        Caption=PLUGIN_NAME;
        WidgetEmbeddable=true;
        Resizable=false;
//        qDebug()<<Data->NodeValues.toString();

    }

    virtual ~ValueDisplayDataModel() override{}

public:

    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        QString in = "➩";
        QString out = "➩";
        switch (portType) {
            case PortType::In:
                return in;
            case PortType::Out:
                return out;
            default:
                break;
        }
        return "";
    }

public:
    unsigned int nPorts(PortType portType) const override
    {
        unsigned int result = 1;

        switch (portType) {
            case PortType::In:
                result = InPortCount;
                break;

            case PortType::Out:
                result = OutPortCount;

            default:
                break;
        }
        return result;
    }

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
        Q_UNUSED(portIndex)
        switch (portType) {
            case PortType::In:
                return VariantData().type();
            case PortType::Out:
                return VariantData().type();
            case PortType::None:
                break;
            default:
                break;
        }
        // FIXME: control may reach end of non-void function [-Wreturn-type]

        return StringData().type();
    }

    std::shared_ptr<NodeData> outData(PortIndex const port) override
    {
        Q_UNUSED(port);
        return std::make_shared<VariantData>(inData);
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
    {
        Q_UNUSED(portIndex);

        if (data== nullptr){
            return;
        }
        if (auto textData = std::dynamic_pointer_cast<VariantData>(data)) {

            if (textData->NodeValues.canConvert<QString>()) {
                widget->boolDisplay->setText(textData->NodeValues.toString());
            } else {
                widget->boolDisplay->setText("");
            }
            if (textData->NodeValues.canConvert<bool>()) {
                widget->boolDisplay->setChecked(textData->NodeValues.toBool());
                widget->boolDisplay->setStyleSheet(textData->NodeValues.toBool() ? "QPushButton{background-color: #00FF00;}" : "QPushButton{background-color: #FF0000;}");
            } else {
                widget->boolDisplay->setChecked(false);
                widget->boolDisplay->setStyleSheet(textData->NodeValues.toBool() ? "QPushButton{background-color: #00FF00;}" : "QPushButton{background-color: #FF0000;}");
            }
            if (textData->NodeValues.canConvert<float>()) {
                widget->floatDisplay->setVal(textData->NodeValues.toFloat());
            } else {
                widget->floatDisplay->setValue(0.000);
            }
            if (textData->NodeValues.canConvert<int>()) {
                widget->intDisplay->setVal(textData->NodeValues.toInt());
            } else {
                widget->intDisplay->setValue(0);
            }
        }
        widget->adjustSize();
        Q_EMIT dataUpdated(0);

    }

    QWidget *embeddedWidget() override {return widget; }


private:

//    QLabel *widget=new QLabel("Resulting Text");
    ValueDisplayInterface *widget=new ValueDisplayInterface();
    QVariant inData;
};
