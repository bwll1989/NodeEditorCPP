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
            InPortCount =5;
            OutPortCount=1;
            CaptionVisible=true;
            Caption= PLUGIN_NAME;
            WidgetEmbeddable=true;
            Resizable=false;
            PortEditable= true;
            index=new QSpinBox();
            NodeDelegateModel::registerOSCControl("/index",index);
            connect(index,&QSpinBox::valueChanged, this, &SwitchDataModel::onIndexEdited);
        }
        ~SwitchDataModel(){    }

    public:

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

        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            Q_UNUSED(portIndex)
            return  in_dictionary[index->value()];
        }
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override{

            if (data== nullptr){
                return;
            }

            auto textData = std::dynamic_pointer_cast<VariableData>(data);
            if(portIndex==0){
                index->setValue(textData->value().toInt());
            }
            else{
                in_dictionary[portIndex-1]=std::dynamic_pointer_cast<VariableData>(textData);
                for(unsigned int i = 0; i < OutPortCount; ++i){
                    Q_EMIT dataUpdated(i);
                }
            }

        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            if(portType==PortType::In&&portIndex==0){
                return "index";
            }

            return "info";
        }
        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["val"] = index->value();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }
        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                //            button->setChecked(v["val"].toBool(false));
                index->setValue(v["val"].toInt());
            }
        }
        QWidget *embeddedWidget() override{return index;}

    private Q_SLOTS:

        void onIndexEdited(int const &string)
        {

            Q_EMIT dataUpdated(0);
        }

    private:

        QSpinBox *index;
        std::unordered_map<unsigned int,  std::shared_ptr<VariableData>> in_dictionary;

    };
}
