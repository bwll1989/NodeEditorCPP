#include "VariableInDataModel.hpp"

#include "StatusContainer/GlobalEventBus.hpp"

namespace Nodes {

VariableInDataModel::VariableInDataModel()
{
    InPortCount = 0;
    OutPortCount = 1;
    CaptionVisible = true;
    Caption = QStringLiteral("Variable In");
    WidgetEmbeddable = false;
    Resizable = false;
    PortEditable = true;
    setRemarks(QStringLiteral("Variable In"));
}

QString VariableInDataModel::remarks() const
{
    return getRemarks();
}

void VariableInDataModel::setRemarks(const QString &remarks)
{
    QString trimmed = remarks.trimmed();
    if (trimmed.isEmpty())
        trimmed = QStringLiteral("Variable In");
    if (trimmed == getRemarks())
        return;
    NodeDelegateModel::setRemarks(trimmed);
    Q_EMIT remarksChanged(trimmed);
}

NodeDataType VariableInDataModel::dataType(PortType, PortIndex) const
{
    return NodeDataTypes::VariableData().type();
}

std::shared_ptr<NodeData> VariableInDataModel::outData(PortIndex port)
{
    return _dataMap[port];
}

QWidget *VariableInDataModel::embeddedWidget()
{
    return nullptr;
}

void VariableInDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
{
    _dataMap[portIndex] = std::dynamic_pointer_cast<NodeDataTypes::VariableData>(data);
    Q_EMIT dataUpdated(portIndex);
}

void VariableInDataModel::afterModelReady()
{
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/input"),
                                          this,
                                          SLOT(onGlobalEvent(GlobalEvent)));
}

void VariableInDataModel::onGlobalEvent(const GlobalEvent &ev)
{
    if (ev.kind != GlobalEventKind::Command)
        return;
    if (ev.address != makeFullOscAddress("/input"))
        return;
    setRemarks(ev.payload.toString());
}

} // namespace Nodes
