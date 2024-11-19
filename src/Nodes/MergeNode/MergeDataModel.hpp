#pragma once


#include <QtCore/QObject>

#include "DataTypes/NodeDataList.hpp"

#include <QtNodes/NodeDelegateModel>
#include "MergeInterface.hpp"
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
class MergeDataModel : public NodeDelegateModel
{
    Q_OBJECT

public:

    MergeDataModel(){
        InPortCount =5;
        OutPortCount=1;
        Caption="Merge";
        CaptionVisible=true;
        WidgetEmbeddable= true;
        PortEditable=true;
        Resizable=true;

        // connect(widget, &QLineEdit::editingFinished, this, &MergeDataModel::outDataSlot);
        connect(widget,&MergeInterface::tableChanged,this,&MergeDataModel::processAll);
        // 连接 itemChanged 信号
        connect(widget,&MergeInterface::rowChanged,this,&MergeDataModel::processRow);


    }
    ~MergeDataModel() override {};
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
        // if(m_proprtyData==nullptr) {
        //     // m_proprtyData默认为空指针
        //     return std::make_shared<VariableData>();
        // }
        // auto data=m_proprtyData->value("widget->text()");

        return m_proprtyData;
    }

    void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override {
        {
            Q_UNUSED(portIndex);
            if (data== nullptr){
                return;
            }
            in_dictionary[portIndex]=std::dynamic_pointer_cast<VariableData>(data);
            processAll();
            Q_EMIT dataUpdated(0);
        }
    }


    void processAll() {
        m_proprtyData=std::make_shared<VariableData>("");
        for(int i=0;i<widget->rowCount();i++) {
            auto keys=widget->getRowValues(i);
            if(!in_dictionary[QString(keys[0]).toInt()])
                break;
            if(!keys[0].isEmpty()||!keys[1].isEmpty()){
                if(keys[2].isEmpty()){
                    m_proprtyData->insert(keys[1],in_dictionary[QString(keys[0]).toInt()]->value(keys[1]));
                }else {
                    m_proprtyData->insert(keys[2],in_dictionary[QString(keys[0]).toInt()]->value(keys[1]));
                }
            }
        }
    }

    void processRow(int row) {
        auto keys=widget->getRowValues(row);

        if(!in_dictionary[QString(keys[0]).toInt()])
            return;
        if(!keys[0].isEmpty()||!keys[1].isEmpty()){
            if(keys[2].isEmpty()){
                m_proprtyData->insert(keys[1],in_dictionary[QString(keys[0]).toInt()]->value(keys[1]));
            }else {
                m_proprtyData->insert(keys[2],in_dictionary[QString(keys[0]).toInt()]->value(keys[1]));
            }
        }
        Q_EMIT dataUpdated(0);

    }
    QJsonObject save() const override
    {
        QJsonObject modelJson1;
        modelJson1["key"] = widget->exportToJson();
        QJsonObject modelJson  = NodeDelegateModel::save();
        modelJson["values"]=modelJson1;
        return modelJson;
    }
    void load(const QJsonObject &p) override
    {
        QJsonValue v = p["values"];
        if (!v.isUndefined()&&v.isObject()) {
            widget->importFromJson(v["key"].toObject());
//
        }
    }

    QWidget *embeddedWidget() override {return widget;}
private slots:
    void outDataSlot() {
        Q_EMIT dataUpdated(0);
    }
private:
    MergeInterface *widget=new MergeInterface();
    std::shared_ptr<VariableData> m_proprtyData;
    std::unordered_map<unsigned int,  std::shared_ptr<VariableData>> in_dictionary;
};
