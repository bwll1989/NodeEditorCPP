#pragma once


#include <QtCore/QObject>

#include "DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>

#include <iostream>
#include <QtWidgets/QLineEdit>


#include <QtCore/qglobal.h>


using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;
using namespace NodeDataTypes;
namespace Nodes
{
    class TextSourceDataModel : public NodeDelegateModel
    {
        Q_OBJECT

    public:

        TextSourceDataModel():_lineEdit(new QLineEdit("")){
            InPortCount =1;
            OutPortCount=1;
            CaptionVisible=true;
            Caption=PLUGIN_NAME;
            WidgetEmbeddable=true;
            Resizable=false;
            NodeDelegateModel::registerOSCControl("/string",_lineEdit);
            connect(_lineEdit, &QLineEdit::textChanged, this, &TextSourceDataModel::onTextEdited);
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
            return std::make_shared<VariableData>(_lineEdit->text());
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override{
            if (data== nullptr){
                _lineEdit->setText("");
                return;
            }
            auto textData = std::dynamic_pointer_cast<VariableData>(data);
            if (textData->value().canConvert<QString>()) {
                _lineEdit->setText(textData->value().toString());
            } else {
                _lineEdit->setText("");
            }

            Q_EMIT dataUpdated(portIndex);
        }


        QJsonObject save() const override
        {
            QJsonObject modelJson1;
            modelJson1["text"] = _lineEdit->text();
            QJsonObject modelJson  = NodeDelegateModel::save();
            modelJson["values"]=modelJson1;
            return modelJson;
        }
        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined()&&v.isObject()) {
                _lineEdit->setText(v["text"].toString());
            }
        }
        QWidget *embeddedWidget() override{return _lineEdit;}

    private Q_SLOTS:

        void onTextEdited(QString const &string)
        {
            Q_UNUSED(string);

            Q_EMIT dataUpdated(0);
        }

    private:
        QLineEdit *_lineEdit;

    };
}