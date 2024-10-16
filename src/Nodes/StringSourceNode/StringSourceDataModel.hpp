#pragma once


#include <QtCore/QObject>

#include "Nodes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>

#include <iostream>
#include <QtWidgets/QLineEdit>


#include <QtCore/qglobal.h>


using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
class QLineEdit;

/// The model dictates the number of inputs and outputs for the Node.
/// In this example it has no logic.
class TextSourceDataModel : public NodeDelegateModel
{
    Q_OBJECT

public:

    TextSourceDataModel(){
        InPortCount =1;
        OutPortCount=1;
        CaptionVisible=true;
        Caption=PLUGIN_NAME;
        WidgetEmbeddable=true;
        Resizable=false;
        connect(_lineEdit, &QLineEdit::textEdited, this, &TextSourceDataModel::onTextEdited);
    }

public:

    QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
    {
        QString in = "➩";
        QString out = "➩";
        switch (portType) {
            case PortType::In:
                return in;
            case PortType::Out:
                return out;
            default:
                break;
        }
        return "";
    }

public:
    unsigned int nPorts(PortType portType) const override
    {
        unsigned int result = 1;
        switch (portType) {
            case PortType::In:
                result = InPortCount;
                break;
            case PortType::Out:
                result = OutPortCount;
            default:
                break;
        }
        return result;
    }

    NodeDataType dataType(PortType portType, PortIndex portIndex) const override
    {

        Q_UNUSED(portIndex)
        switch (portType) {
            case PortType::In:
                return StringData().type();
            case PortType::Out:
                return VariantData().type();
            case PortType::None:
                break;
            default:
                break;
        }
        // FIXME: control may reach end of non-void function [-Wreturn-type]

        return StringData().type();
    }

    std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
    {
        Q_UNUSED(portIndex)
        return std::make_shared<VariantData>(_lineEdit->text());
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override{
        if (data== nullptr){
            return;
        }
        if (auto textData = std::dynamic_pointer_cast<VariantData>(data)) {
            if (textData->NodeValues.canConvert<QString>()) {
                _lineEdit->setText(textData->NodeValues.toString());
            } else {
                _lineEdit->setText("");
            }
        } else if (auto boolData = std::dynamic_pointer_cast<StringData>(data)) {
            _lineEdit->setText(boolData->NodeValues);

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
    QLineEdit *_lineEdit=new QLineEdit("");

};
