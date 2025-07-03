#pragma once


#include <QtCore/QObject>

#include "DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>

#include <iostream>
#include <QtWidgets/QLineEdit>

#include <QtWidgets/QPushButton>
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
    class BoolPluginDataModel : public NodeDelegateModel
    {
        Q_OBJECT


    public:

        BoolPluginDataModel(): button(new QPushButton("0")){
            InPortCount =1;
            OutPortCount=1;
            CaptionVisible=true;
            Caption="Bool Source";
            WidgetEmbeddable=true;
            Resizable=false;
            button->setCheckable(true);
            button->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
            NodeDelegateModel::registerOSCControl("/bool", button);
            button->setChecked(false);
            connect(button, &QPushButton::clicked, this, &BoolPluginDataModel::onTextEdited);
        }
        ~BoolPluginDataModel(){
            delete button;

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
            return std::make_shared<VariableData>(button->isChecked());
        }
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override{

            if (data== nullptr){
                return;
            }
            auto textData = std::dynamic_pointer_cast<VariableData>(data);
            if (textData->value().canConvert<bool>()) {
                button->setChecked(textData->value().toBool());
            } else {
                button->setChecked(false);
            }
            button->setText(button->isChecked()? "1":"0");
            Q_EMIT dataUpdated(portIndex);
        }


        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["val"] = QString::number(button->isChecked());
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }
        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                button->setChecked(v["val"].toBool(false));
                button->setText(v["val"].toString());
            }
        }
        QWidget *embeddedWidget() override{return button;}

    private Q_SLOTS:

        void onTextEdited(bool const &string)
        {
            button->setText(QString::number(string));
            Q_EMIT dataUpdated(0);
        }

    private:

        QPushButton *button;


    };
}