#include "VariableOutDataModel.hpp"

#include <QtWidgets/QLabel>
namespace Nodes {
    VariableOutDataModel::VariableOutDataModel()
    {
        InPortCount =2;
        OutPortCount=0;
        CaptionVisible=true;
        Caption="Variable Out";
        WidgetEmbeddable= true;
        Resizable=false;
        PortEditable= true;
        setRemarks(QString("VariableExport"));
        ModelDataBridge::instance().registerExportDelegate(this);

    }

    VariableOutDataModel::~VariableOutDataModel()
    {
        ModelDataBridge::instance().unregisterExportDelegate(this);
    }


    NodeDataType VariableOutDataModel::dataType(PortType, PortIndex) const
    {
        return NodeDataTypes::VariableData().type();
    }

    std::shared_ptr<NodeData> VariableOutDataModel::outData(PortIndex port)
    {
        return  _dataMap[port];

    }

    void VariableOutDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
    {
        _dataMap[portIndex] = std::dynamic_pointer_cast<NodeDataTypes::VariableData>(data);
        emit dataUpdated(portIndex);
    }

    QWidget *VariableOutDataModel::embeddedWidget()
    {
        return nullptr;
    }
    void VariableOutDataModel::setRemarks(const QString& remarks){
        NodeDelegateModel::setRemarks(remarks);
        ModelDataBridge::instance().updateRemarksForDelegate(this,false,getRemarks());
    };
}