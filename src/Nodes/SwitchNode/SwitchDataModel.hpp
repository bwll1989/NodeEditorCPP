#pragma once


#include <QtCore/QObject>

#include "DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>

#include <iostream>
#include <QtWidgets/QLineEdit>

#include <QtWidgets/QSpinBox>
#include <QtCore/qglobal.h>

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;
class QPushButton;
using namespace NodeDataTypes;
namespace Nodes
{
    class SwitchDataModel : public NodeDelegateModel
    {
        Q_OBJECT


    public:

        SwitchDataModel(){
            InPortCount =3;
            OutPortCount=1;
            CaptionVisible=true;
            Caption= PLUGIN_NAME;
            WidgetEmbeddable= false;
            Resizable=false;
            PortEditable= true;

            NodeDelegateModel::registerOSCControl("/index",widget);
            connect(widget,&QSpinBox::valueChanged, this, &SwitchDataModel::onIndexEdited);
        }
        ~SwitchDataModel(){    }

    public:

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            Q_UNUSED(portType)
            return VariableData().type();

        }

        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex)
            return  in_dictionary[widget->value()];
        }
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override{

            if (data== nullptr){
                return;
            }

            auto textData = std::dynamic_pointer_cast<VariableData>(data);
            if(portIndex==InPortCount-1){
                widget->setValue(textData->value().toInt());
            }
            else{
                in_dictionary[portIndex]=std::dynamic_pointer_cast<VariableData>(textData);
                for(unsigned int i = 0; i < OutPortCount; ++i){
                    Q_EMIT dataUpdated(i);
                }
            }

        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch(portType)
            {
            case PortType::In:
                if (portIndex==InPortCount-1){
                    return "INDEX";
                }
                return "INPUT "+QString::number(portIndex);
            case PortType::Out:
                return "OUTPUT "+QString::number(portIndex);
            default:
                return "";
            }

        }
        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["val"] = widget->value();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }
        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                //            button->setChecked(v["val"].toBool(false));
                widget->setValue(v["val"].toInt());
            }
        }
        QWidget *embeddedWidget() override{return widget;}

    private Q_SLOTS:

        void onIndexEdited(int const &string)
        {
            for(unsigned int i = 0; i < OutPortCount; ++i){
                Q_EMIT dataUpdated(i);
            }
        }

    private:

        QSpinBox *widget=new QSpinBox();
        std::unordered_map<unsigned int,  std::shared_ptr<VariableData>> in_dictionary;

    };
}
