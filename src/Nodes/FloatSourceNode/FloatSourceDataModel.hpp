#pragma once


#include <QtCore/QObject>

#include "DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>
#include "FloatSourceInterface.hpp"
#include <iostream>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include "QGridLayout"
#include <QtCore/qglobal.h>


using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;

using namespace NodeDataTypes;
namespace Nodes
{
    class FloatSourceDataModel : public NodeDelegateModel
    {
        Q_OBJECT

    public:

        FloatSourceDataModel(){
            InPortCount =1;
            OutPortCount=1;
            CaptionVisible=true;
            Caption="Float Source";
            WidgetEmbeddable= true;
            Resizable=false;
            connect(widget->floatDisplay, &QLineEdit::textChanged, this, &FloatSourceDataModel::onFloatEdited);
            registerOSCControl("/float",widget->floatDisplay);
        }

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
            return std::make_shared<VariableData>(widget->floatDisplay->text().toDouble());
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override {
            {
                Q_UNUSED(portIndex);
                if (data== nullptr){
                    return;
                }

                auto textData = std::dynamic_pointer_cast<VariableData>(data);
                if (textData->value().canConvert<double>()) {

                    widget->floatDisplay->setText(QString::number(textData->value().toDouble()));
                } else {

                    widget->floatDisplay->setText("0");
                }
                widget->adjustSize();
                Q_EMIT dataUpdated(0);

            }
        }


        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["val"] = widget->floatDisplay->text().toDouble();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }
        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                widget->floatDisplay->setText(QString::number(v["val"].toDouble()));

            }
        }

        QWidget *embeddedWidget() override {return widget;}
    private Q_SLOTS:

        void onFloatEdited(QString const &string)
        {
            //        widget->currentVal->setNum(string);
            //        Q_UNUSED(string);
            //        this->embeddedWidgetSizeUpdated();
            Q_EMIT dataUpdated(0);
        }

    private:
        FloatSourceInterface *widget=new FloatSourceInterface();


    };
}