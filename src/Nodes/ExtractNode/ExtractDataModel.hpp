#pragma once


#include <QtCore/QObject>

#include "DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>
#include "ExtractInterface.hpp"
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
    class ExtractDataModel : public NodeDelegateModel
    {
        Q_OBJECT

    public:

        ExtractDataModel(){
            InPortCount =1;
            OutPortCount=1;
            Caption="Extract";
            CaptionVisible=true;
            WidgetEmbeddable= true;
            Resizable=false;
            connect(widget, &QLineEdit::editingFinished, this, &ExtractDataModel::outDataSlot);

        }
        ~ExtractDataModel() override {};
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
            if(m_proprtyData==nullptr) {
                // m_proprtyData默认为空指针
                return std::make_shared<VariableData>();
            }
            auto data=m_proprtyData->value(widget->text());
            return std::make_shared<VariableData>(data);
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override {
            {
                Q_UNUSED(portIndex);
                if (data== nullptr){
                    return;
                }
                m_proprtyData = std::dynamic_pointer_cast<VariableData>(data);
                Q_EMIT dataUpdated(0);

            }
        }


        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["key"] = widget->text();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }
        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                widget->setText(v["key"].toString());

            }
        }

        QWidget *embeddedWidget() override {return widget;}
    private slots:
        void outDataSlot() {
            Q_EMIT dataUpdated(0);
        }
    private:
        QLineEdit *widget=new QLineEdit("default");
        std::shared_ptr<VariableData> m_proprtyData;


    };
}