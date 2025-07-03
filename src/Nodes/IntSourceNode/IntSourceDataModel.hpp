#pragma once


#include <QtCore/QObject>

#include "NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include "IntSourceInterface.hpp"
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
    /// The model dictates the number of inputs and outputs for the Node.
    /// In this example it has no logic.
    class IntSourceDataModel : public NodeDelegateModel
    {
        Q_OBJECT

    public:

        IntSourceDataModel():widget(new IntSourceInterface()){
            InPortCount =1;
            OutPortCount=1;
            CaptionVisible=true;
            Caption="Int Source";
            WidgetEmbeddable=true;
            Resizable=false;
            registerOSCControl("/int",widget->intDisplay);
            connect(widget->intDisplay, &QLineEdit::textChanged, this, &IntSourceDataModel::onIntEdited);
        }


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
            return std::make_shared<VariableData>(widget->intDisplay->text().toInt());
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override {
            {
                Q_UNUSED(portIndex);
                if (data== nullptr){
                    widget->intDisplay->setText("0");
                    return;
                }
                auto textData = std::dynamic_pointer_cast<VariableData>(data);
                if (textData->value().canConvert<int>()) {

                    widget->intDisplay->setText(QString::number(textData->value().toInt()));
                } else {

                    widget->intDisplay->setText("0");
                }

                widget->adjustSize();
                Q_EMIT dataUpdated(0);

            }
        }


        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["val"] = widget->intDisplay->text().toInt();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }
        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                widget->intDisplay->setText(QString::number(v["val"].toInt()));

            }
        }
        QWidget *embeddedWidget() override {return widget;}
    private Q_SLOTS:

        void onIntEdited(QString const &string)
        {
            //        widget->currentVal->setNum(string);
            //        Q_UNUSED(string);
            //        this->embeddedWidgetSizeUpdated();
            Q_EMIT dataUpdated(0);
        }

    private:
        IntSourceInterface *widget;

    };
}