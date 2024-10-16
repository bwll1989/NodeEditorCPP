#pragma once


#include <QtCore/QObject>

#include "Nodes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>

#include <iostream>
#include <QtWidgets/QLineEdit>

#include <QtWidgets/QPushButton>
#include <QtCore/qglobal.h>
#include <QToolBox>
#include "src/timelinewidget.hpp"
using namespace std;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;
class QPushButton;
/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class TimeLineDataModel : public NodeDelegateModel
{
Q_OBJECT

public:

    TimeLineDataModel(){
        InPortCount =1;
        OutPortCount=1;
        CaptionVisible=true;
        Caption=PLUGIN_NAME;
        WidgetEmbeddable= false;
        Resizable= false;
        PortEditable=true;

//        widget->model->createTrack(MediaType::CONTROL);
//
//        widget->model->addClip(0,0,10,"123");
//        widget->model->addClip(1,0,30,"456");

    }

    ~TimeLineDataModel()
    {

        delete widget;
        delete tab;


    }
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
        return std::make_shared<VariantData>();
    }
    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override{
        if (data== nullptr){
            return;
        }
        if (auto textData = std::dynamic_pointer_cast<VariantData>(data)) {
            if (textData->NodeValues.canConvert<bool>()) {
                qDebug()<<textData->NodeValues.toBool();
            } else {

            }
        } else if (auto boolData = std::dynamic_pointer_cast<BoolData>(data)) {

        } else {


        }

        Q_EMIT dataUpdated(portIndex);
    }


    QJsonObject save() const override
    {
        QJsonObject modelJson1;
        modelJson1["Clips"] = "sda";
        QJsonObject modelJson  = NodeDelegateModel::save();
        modelJson["values"]=modelJson1;
        return modelJson;
    }

    void load(const QJsonObject &p) override
    {
        QJsonValue v = p["values"];
        if (!v.isUndefined()&&v.isObject()) {
            value=v["Clips"].toString();


        }
    }
    QWidget *embeddedWidget() override{

        return widget;
}

private Q_SLOTS:

    void onTextEdited(bool const &string)
    {
        Q_EMIT dataUpdated(0);
    }

private:
    timelinewidget *widget=new timelinewidget();
    QTabWidget *tab=new QTabWidget();
    unordered_map<unsigned int, QVariant> in_dictionary;
    unordered_map<unsigned int, QVariant> out_dictionary;
    QString value;
};
