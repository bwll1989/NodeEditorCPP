#pragma once


#include <QtCore/QObject>

#include "DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>
#include <iostream>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QSlider>
#include <QtWidgets/QSpinBox>
#include "QGridLayout"
#include <QtCore/qglobal.h>
#include "ConstantDefines.h"
#include "OSCSender/OSCSender.h"
#include "AbstractDelegateModel.h"
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using QtNodes::ConnectionPolicy;
class QLineEdit;

using namespace NodeDataTypes;
namespace Nodes
{
class FloatSourceDataModel : public AbstractDelegateModel
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
            connect(widget, &QLineEdit::textChanged, this, &FloatSourceDataModel::onFloatEdited);
            registerOSCControl("/float",widget);
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
            return std::make_shared<VariableData>(widget->text().toDouble());
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override {
            {
                Q_UNUSED(portIndex);
                if (data== nullptr){
                    return;
                }

                auto textData = std::dynamic_pointer_cast<VariableData>(data);
                if (textData->value().canConvert<double>()) {

                    widget->setText(QString::number(textData->value().toDouble()));
                } else {

                    widget->setText("0");
                }
                widget->adjustSize();
                Q_EMIT dataUpdated(0);

            }
        }


        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["val"] = widget->text().toDouble();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }
        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                widget->setText(QString::number(v["val"].toDouble()));

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
        QLineEdit *widget=new QLineEdit();


    };
}