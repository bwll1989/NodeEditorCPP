#pragma once

#include "DataTypes/NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <QtWidgets/QComboBox>
#include <iostream>
#include <QtConcurrent/QtConcurrent>
#include <QAbstractScrollArea>
#include <vector>
#include <QtCore/qglobal.h>
#include "PluginDefinition.hpp"
#include "ImageSwitchInterface.hpp"
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
using namespace std;
namespace Nodes
{
    class ImageSwitchDataModel : public NodeDelegateModel
    {
        Q_OBJECT

        public:
        ImageSwitchDataModel()
        {
            InPortCount =2;
            OutPortCount=1;
            CaptionVisible=true;
            Caption=PLUGIN_NAME;
            WidgetEmbeddable= false;
            Resizable=false;
            PortEditable= true;
            in_dictionary=std::unordered_map<unsigned int,  std::shared_ptr<ImageData>>();
            connect(widget->IndexEdit,&QSpinBox::valueChanged,this,[this](){
                for(unsigned int i=0;i<OutPortCount;i++){
                    Q_EMIT dataUpdated(i);
                }
            });
            NodeDelegateModel::registerOSCControl("/index",widget->IndexEdit);

        }

        virtual ~ImageSwitchDataModel() override{}

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch(portType)
            {
            case PortType::In:
                if (portIndex==InPortCount-1){
                    return "INDEX";
                }
                return "IMAGE "+QString::number(portIndex);
            case PortType::Out:
                return "IMAGE "+QString::number(portIndex);
            default:
                return "";
            }

        }
        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            // Q_UNUSED(portIndex);
            // Q_UNUSED(portType);
            switch(portType){
            case PortType::In:
                if(portIndex==InPortCount-1){
                    return VariableData().type();
                }
                return  ImageData().type();
            case PortType::Out:
                return ImageData().type();
            default:
                return ImageData().type();
            }
        }



        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            Q_UNUSED(port);
            return in_dictionary[widget->IndexEdit->value()];
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (portIndex==InPortCount-1){
                if(data== nullptr){
                    widget->IndexEdit->setValue(0);
                }
                else{
                    widget->IndexEdit->setValue(std::dynamic_pointer_cast<VariableData>(data)->value().toInt());
                }
            }
            if (data== nullptr){
                in_dictionary[portIndex]=nullptr;
            }
            else{
                in_dictionary[portIndex]=std::dynamic_pointer_cast<ImageData>(data);
            }
            if (widget->IndexEdit->value()==portIndex){
                for(unsigned int i = 0; i < OutPortCount; ++i){
                    Q_EMIT dataUpdated(i);
                }
            }
        }


        QWidget *embeddedWidget() override
        {
            return widget;
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson1["index"] = widget->IndexEdit->value();
            modelJson["values"]=modelJson1;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {

                widget->IndexEdit->setValue(v.toObject()["index"].toInt());
            }
        }

    private:
        QFutureWatcher<double>* m_watcher = nullptr;
        ImageSwitchInterface *widget=new ImageSwitchInterface();
        std::unordered_map<unsigned int,  std::shared_ptr<ImageData>> in_dictionary;
    };
}