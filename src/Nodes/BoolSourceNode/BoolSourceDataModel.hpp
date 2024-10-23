#pragma once


#include <QtCore/QObject>

#include "Nodes/NodeDataList.hpp"

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
class BoolSourceDataModel : public NodeDelegateModel
{
    Q_OBJECT


public:

    BoolSourceDataModel():button(new QPushButton("0")){
        InPortCount =1;
        OutPortCount=1;
        CaptionVisible=true;
        Caption="Bool Input";
        WidgetEmbeddable=true;
        Resizable=false;
        button->setCheckable(true);
        button->setChecked(false);
        connect(button, &QPushButton::clicked, this, &BoolSourceDataModel::onTextEdited);
    }
    ~BoolSourceDataModel(){
        delete button;

    }

public:


    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        QString in = "In "+QString::number(portIndex);
        QString out = "Out "+QString::number(portIndex);
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
                return BoolData().type();
            case PortType::Out:
                return VariantData().type();
            case PortType::None:
                break;
            default:
                break;
        }
        // FIXME: control may reach end of non-void function [-Wreturn-type]

        return BoolData().type();

    }

    std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
    {
        Q_UNUSED(portIndex)
        return std::make_shared<VariantData>(button->isChecked());
    }
    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override{
        if (data== nullptr){
            return;
        }
        if (auto textData = std::dynamic_pointer_cast<VariantData>(data)) {
            if (textData->NodeValues.canConvert<bool>()) {
                button->setChecked(textData->NodeValues.toBool());
                button->setText(button->isChecked()? "1":"0");
            } else {
                button->setChecked(false);
            }
        } else if (auto boolData = std::dynamic_pointer_cast<BoolData>(data)) {
            button->setChecked(boolData->NodeValues);
            button->setText(button->isChecked()? "1":"0");
        } else {
            button->setChecked(false);

        }

        Q_EMIT dataUpdated(portIndex);
    }


    QJsonObject save() const override
    {
        QJsonObject modelJson1;
        modelJson1["val"] = QString::number(button->isChecked());
        QJsonObject modelJson  = NodeDelegateModel::save();
        modelJson["values"]=modelJson1;
        return modelJson;
    }
    void load(const QJsonObject &p) override
    {
        QJsonValue v = p["values"];
        if (!v.isUndefined()&&v.isObject()) {
            button->setChecked(v["val"].toBool(false));
            button->setText(v["val"].toString());
        }
    }
    QWidget *embeddedWidget() override{return button;}

private Q_SLOTS:

    void onTextEdited(bool const &string)
    {

        button->setText(QString::number(string));
        Q_EMIT dataUpdated(0);
    }

private:

    QPushButton *button;


};
