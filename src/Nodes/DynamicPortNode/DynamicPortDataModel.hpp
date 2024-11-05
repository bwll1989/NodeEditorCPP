#pragma once


#include <QtCore/QObject>

#include "DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>

#include <iostream>
#include <QtWidgets/QLineEdit>

#include <QtWidgets/QPushButton>
#include <QtCore/qglobal.h>

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;
class QPushButton;
/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class DynamicPortDataModel : public NodeDelegateModel
{
    Q_OBJECT

public:

    DynamicPortDataModel(){
        InPortCount =2;
        OutPortCount=2;
        CaptionVisible=true;
        Caption="Dynamic Port";
        WidgetEmbeddable= false;
        PortEditable= true;
        Resizable= true;
    }
    ~DynamicPortDataModel(){

    }

public:

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {

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

        return VariantData(QVariant()).type();

    }

    std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
    {
        Q_UNUSED(portIndex)
        return std::make_shared<VariantData>();
    }
    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override{
        if (data== nullptr){
            return;
        }


        Q_EMIT dataUpdated(portIndex);
    }


    QJsonObject save() const override
    {

        QJsonObject modelJson1;

        QJsonObject modelJson  = NodeDelegateModel::save();
        modelJson["values"]=modelJson1;
        modelJson["InPutCount"] = static_cast<qint64>(InPortCount);
        modelJson["OutPutCount"] = static_cast<qint64>(OutPortCount);
        return modelJson;



    }
    void load(const QJsonObject &p) override
    {
        QJsonValue v = p["values"];
        if (!v.isUndefined() && v.isObject()) {
            v = p["InPutCount"];
            InPortCount=v.toInt();
            v = p["OutPutCount"];
            OutPortCount=v.toInt();
        }


    }
    QWidget *embeddedWidget() override{

        return widget;
    }
    QWidget *widget=new QWidget();
private Q_SLOTS:

    void onTextEdited(bool const &string)
    {
        Q_EMIT dataUpdated(0);
    }

};
