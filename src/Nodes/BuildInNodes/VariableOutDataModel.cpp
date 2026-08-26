#include "VariableOutDataModel.hpp"

namespace Nodes {

VariableOutDataModel::VariableOutDataModel()
{
    InPortCount = 1;
    OutPortCount = 0;
    CaptionVisible = true;
    Caption = QStringLiteral("Variable Out");
    WidgetEmbeddable = false;
    Resizable = false;
    PortEditable = true;
    setRemarks(QStringLiteral("Variable Out"));
}

NodeDataType VariableOutDataModel::dataType(PortType, PortIndex) const
{
    return NodeDataTypes::VariableData().type();
}

std::shared_ptr<NodeData> VariableOutDataModel::outData(PortIndex port)
{
    return _dataMap[port];
}

void VariableOutDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
{
    _dataMap[portIndex] = std::dynamic_pointer_cast<NodeDataTypes::VariableData>(data);
    Q_EMIT dataUpdated(portIndex);
}

QWidget *VariableOutDataModel::embeddedWidget()
{
    return nullptr;
}

} // namespace Nodes
