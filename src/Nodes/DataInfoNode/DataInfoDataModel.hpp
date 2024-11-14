#pragma once

#include "DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>
#include "qtpropertymanager.h"
#include "qtvariantproperty.h"
#include "Common/GUI/QPropertyBrowser/QPropertyBrowser.h"
#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include <iostream>
#include <qtreeview.h>
#include <QVBoxLayout>
#include <vector>
#include <QtCore/qglobal.h>
#include <QJsonModel/QJsonModel.hpp>
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;


/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class DataInfoDataModel : public NodeDelegateModel
{
    Q_OBJECT

public:
    DataInfoDataModel()
    {
        InPortCount =1;
        OutPortCount=1;
        CaptionVisible=true;
        Caption="Data Info";
        WidgetEmbeddable=true;
        Resizable=false;
//        qDebug()<<Data->NodeValues.toString();

    }

    virtual ~DataInfoDataModel() override{}

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
        if (inData = std::dynamic_pointer_cast<VariableData>(data)) {
            model=inData->getMap();
            widget->buildPropertiesFromMap(model);
            Q_EMIT dataUpdated(0);

        }

    }

    QWidget *embeddedWidget() override {

        return widget;
    }


private:

//    QLabel *widget=new QLabel("Resulting Text");
    QVariantMap model;
    QPropertyBrowser *widget=new QPropertyBrowser();
    QtVariantPropertyManager *variantManager;
     std::shared_ptr<VariableData> inData;
};
