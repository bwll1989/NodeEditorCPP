#include "ImageOutDataModel.hpp"

namespace Nodes {

ImageOutDataModel::ImageOutDataModel()
{
    InPortCount = 1;
    OutPortCount = 0;
    CaptionVisible = true;
    Caption = QStringLiteral("Image Out");
    WidgetEmbeddable = false;
    Resizable = false;
    PortEditable = true;
    setRemarks(QStringLiteral("Image Out"));
}

NodeDataType ImageOutDataModel::dataType(PortType, PortIndex) const
{
    return NodeDataTypes::ImageData().type();
}

std::shared_ptr<NodeData> ImageOutDataModel::outData(PortIndex port)
{
    return _dataMap[port];
}

void ImageOutDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
{
    _dataMap[portIndex] = std::dynamic_pointer_cast<NodeDataTypes::ImageData>(data);
    Q_EMIT dataUpdated(portIndex);
}

QWidget *ImageOutDataModel::embeddedWidget()
{
    return nullptr;
}

} // namespace Nodes
