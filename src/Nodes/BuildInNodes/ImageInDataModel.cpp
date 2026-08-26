#include "ImageInDataModel.hpp"

#include "StatusContainer/GlobalEventBus.hpp"

namespace Nodes {

ImageInDataModel::ImageInDataModel()
{
    InPortCount = 0;
    OutPortCount = 1;
    CaptionVisible = true;
    Caption = QStringLiteral("Image In");
    WidgetEmbeddable = false;
    Resizable = false;
    PortEditable = true;
    setRemarks(QStringLiteral("Image In"));
}

QString ImageInDataModel::remarks() const
{
    return getRemarks();
}

void ImageInDataModel::setRemarks(const QString &remarks)
{
    QString trimmed = remarks.trimmed();
    if (trimmed.isEmpty())
        trimmed = QStringLiteral("Image In");
    if (trimmed == getRemarks())
        return;
    NodeDelegateModel::setRemarks(trimmed);
    Q_EMIT remarksChanged(trimmed);
}

NodeDataType ImageInDataModel::dataType(PortType, PortIndex) const
{
    return NodeDataTypes::ImageData().type();
}

std::shared_ptr<NodeData> ImageInDataModel::outData(PortIndex port)
{
    return _dataMap[port];
}

QWidget *ImageInDataModel::embeddedWidget()
{
    return nullptr;
}

void ImageInDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
{
    _dataMap[portIndex] = std::dynamic_pointer_cast<NodeDataTypes::ImageData>(data);
    Q_EMIT dataUpdated(portIndex);
}

void ImageInDataModel::afterModelReady()
{
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/input"),
                                          this,
                                          SLOT(onGlobalEvent(GlobalEvent)));
}

void ImageInDataModel::onGlobalEvent(const GlobalEvent &ev)
{
    if (ev.kind != GlobalEventKind::Command)
        return;
    if (ev.address != makeFullOscAddress("/input"))
        return;
    setRemarks(ev.payload.toString());
}

} // namespace Nodes
