#pragma once

#include "Nodes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>

#include <QtCore/QObject>
#include <QtWidgets/QComboBox>
#include <iostream>
#include <vector>
#include <QtCore/qglobal.h>
#if defined(UNTITLED_LIBRARY)
#  define UNTITLED_EXPORT Q_DECL_EXPORT
#else
#  define UNTITLED_EXPORT Q_DECL_IMPORT
#endif
using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;


/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class MathOperationDataModel : public NodeDelegateModel
{
Q_OBJECT

public:
    MathOperationDataModel()
    {
        InPortCount =2;
        OutPortCount=1;
        CaptionVisible=true;
        Caption="Math Operation";
        WidgetEmbeddable= true;
        Resizable=false;
        PortEditable= false;
//        method->setStyleSheet("QComboBox{background-color: transparent;}");
        widget->addItems(*methods);
//        method->setFlat(true);
        connect(widget,&QComboBox::currentIndexChanged,this,&MathOperationDataModel::methodChanged);

    }

    virtual ~MathOperationDataModel() override{}

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
    unsigned int nPorts(PortType portType) const override {
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
    }


    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {
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

        return BoolData().type();
    }

    std::shared_ptr<NodeData> outData(PortIndex const port) override
    {
        Q_UNUSED(port);
        return std::make_shared<VariantData>(val);
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
    {

        if (data== nullptr){
            return;
        }
        if (auto textData = std::dynamic_pointer_cast<VariantData>(data)) {
            in_dictionary[portIndex]=textData->NodeValues;
            for(auto kv:in_dictionary){
                if(kv.first!=portIndex){

                    switch (widget->currentIndex()) {
                        case 0:
                            val=textData->NodeValues.toFloat()+kv.second.toFloat();
                            break;
                        case 1:
                            val=textData->NodeValues.toFloat()-kv.second.toFloat();
                            break;
                        case 2:
                            val=textData->NodeValues.toFloat()*kv.second.toFloat();
                            break;
                        case 3:
                            val=textData->NodeValues.toFloat()/kv.second.toFloat();
                            break;
                        case 4:
                            val=fmod(textData->NodeValues.toFloat(),kv.second.toFloat());
                            break;
                        case 5:
                            val=pow(textData->NodeValues.toFloat(),kv.second.toFloat());
                            break;
                    }

                }
            }

        }
//        method->setText(QString::number(val));
//        method->setStyleSheet(val ? "QComboBox{background-color: #00FF00;}" : "QPushButton{background-color: #FF0000;}");

        Q_EMIT dataUpdated(0);
    }
    void methodChanged()
    {

        for(auto kv:in_dictionary){
            if(kv.first!=0){
                switch (widget->currentIndex()) {
                    case 0:
                        val=in_dictionary[0].toFloat()+kv.second.toFloat();
                        break;
                    case 1:
                        val=in_dictionary[0].toFloat()-kv.second.toFloat();
                        break;
                    case 2:
                        val=in_dictionary[0].toFloat()*kv.second.toFloat();
                        break;
                    case 3:
                        val=in_dictionary[0].toFloat()/kv.second.toFloat();
                        break;
                    case 4:
                        val=fmod(in_dictionary[0].toFloat(),kv.second.toFloat());
                        break;
                    case 5:
                        val=pow(in_dictionary[0].toFloat(),kv.second.toFloat());
                        break;
                }

            }
        }
        Q_EMIT dataUpdated(0);
    }
    QWidget *embeddedWidget() override {

        return widget; }

private:

    QComboBox *widget=new QComboBox();
    QStringList *methods=new QStringList {"+","-","*","/","%","^"};
    std::unordered_map<unsigned int, QVariant> in_dictionary;
    QVariant val;
};
