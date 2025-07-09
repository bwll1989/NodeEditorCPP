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
using namespace NodeDataTypes;
namespace Nodes
{

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
            widget->setFixedWidth(80);
            widget->addItems(*methods);
            //        method->setFlat(true);
            val=QVariant(false);
            connect(widget,&QComboBox::currentIndexChanged,this,&LogicOperationDataModel::methodChanged);
            NodeDelegateModel::registerOSCControl("/method",widget);
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
        }
        void methodChanged()
        {
            bool tempVal=false;
            for(auto kv:in_dictionary){
                if(kv.first!=0){
                    switch (widget->currentIndex()) {
                    case 0:
                        tempVal=in_dictionary[0].toString()==kv.second.toString();
                        break;
                    case 1:
                        tempVal=in_dictionary[0].toBool()||kv.second.toBool();
                        break;
                    case 2:
                        tempVal=in_dictionary[0].toString()!=kv.second.toString();
                        break;
                    case 3:
                        tempVal=qMax(in_dictionary[0].toDouble(),kv.second.toDouble());
                        break;
                    case 4:
                        tempVal=qMin(in_dictionary[0].toDouble(),kv.second.toDouble());
                        break;
                    case 5:
                        tempVal=in_dictionary[0].toFloat()<kv.second.toFloat();
                        break;
                    case 6:
                        tempVal=in_dictionary[0].toFloat()<=kv.second.toFloat();
                        break;
                    case 7:
                        tempVal=in_dictionary[0].toFloat()>kv.second.toFloat();
                        break;
                    case 8:
                        tempVal=in_dictionary[0].toFloat()>=kv.second.toFloat();
                        break;

                    }

                }
            }
            if (tempVal!=val.toBool())
            {
                qDebug()<<"tempVal:"<<tempVal;
                val = tempVal;
                Q_EMIT dataUpdated(0);
            }

        }
        QJsonObject save() const override
        {
            QJsonObject modelJson;
            modelJson["method"] = widget->currentIndex();
            return modelJson;
        }

        void load(QJsonObject const &p) override
        {
            int val = p["method"].toInt();
            widget->setCurrentIndex(val);
        }
        QWidget *embeddedWidget() override {
            return widget; }

    private:

        QComboBox *widget=new QComboBox();
        QStringList *methods=new QStringList {"AND","OR","NOT","MAX","MIN","<","<=",">",">="};
        std::unordered_map<unsigned int, QVariant> in_dictionary;
        QVariant val;
    };
}