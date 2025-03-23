#pragma once

#include "DataTypes/NodeDataList.hpp"

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
class LogicOperationDataModel : public NodeDelegateModel
{
Q_OBJECT

public:
    LogicOperationDataModel()
    {
        InPortCount =2;
        OutPortCount=1;
        CaptionVisible=true;
        Caption="Logic Operation";
        WidgetEmbeddable= true;
        Resizable=false;
        PortEditable= false;
//        method->setStyleSheet("QComboBox{background-color: transparent;}");
        widget->addItems(*methods);
//        method->setFlat(true);
        val=QVariant(false);
        connect(widget,&QComboBox::currentIndexChanged,this,&LogicOperationDataModel::methodChanged);
        registerOSCControl("/method",widget);
    }

    virtual ~LogicOperationDataModel() override{}


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

        return VariableData().type();
    }

    std::shared_ptr<NodeData> outData(PortIndex const port) override
    {
        Q_UNUSED(port);
        return std::make_shared<VariableData>(val);
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
    {
        if (data== nullptr){
            return;
        }
        if (auto textData = std::dynamic_pointer_cast<VariableData>(data)) {
            in_dictionary[portIndex]=textData->value();
            methodChanged();

        }

        Q_EMIT dataUpdated(0);
    }
    void methodChanged()
    {

        for(auto kv:in_dictionary){
            if(kv.first!=0){
                switch (widget->currentIndex()) {
                    case 0:
                        val=in_dictionary[0].toString()==kv.second.toString();
                        break;
                    case 1:
                        val=in_dictionary[0].toBool()||kv.second.toBool();
                        break;
                    case 2:
                        val=in_dictionary[0].toString()!=kv.second.toString();
                        break;
                    case 3:
                        val=qMax(in_dictionary[0].toDouble(),kv.second.toDouble());
                        break;
                    case 4:
                        val=qMin(in_dictionary[0].toDouble(),kv.second.toDouble());
                        break;
                    case 5:
                        val=in_dictionary[0].toFloat()<kv.second.toFloat();
                        break;
                    case 6:
                        val=in_dictionary[0].toFloat()<=kv.second.toFloat();
                        break;
                    case 7:
                        val=in_dictionary[0].toFloat()>kv.second.toFloat();
                        break;
                    case 8:
                        val=in_dictionary[0].toFloat()>=kv.second.toFloat();
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
    QStringList *methods=new QStringList {"AND","OR","NOT","MAX","MIN","<","<=",">",">="};
    std::unordered_map<unsigned int, QVariant> in_dictionary;
    QVariant val;
};
