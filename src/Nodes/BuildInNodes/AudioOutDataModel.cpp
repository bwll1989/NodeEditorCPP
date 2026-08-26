#include "AudioOutDataModel.hpp"

namespace Nodes {

AudioOutDataModel::AudioOutDataModel()
{
    InPortCount = 1;
    OutPortCount = 0;
    CaptionVisible = true;
    Caption = QStringLiteral("Audio Out");
    WidgetEmbeddable = false;
    Resizable = false;
    PortEditable = true;
    setRemarks(QStringLiteral("Audio Out"));
}

NodeDataType AudioOutDataModel::dataType(PortType, PortIndex) const
{
    return NodeDataTypes::AudioData().type();
}

std::shared_ptr<NodeData> AudioOutDataModel::outData(PortIndex port)
{
    return _dataMap[port];
}

void AudioOutDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
{
    auto audioData = std::dynamic_pointer_cast<NodeDataTypes::AudioData>(data);
    if (audioData && audioData->isConnectedToSharedBuffer())
        _dataMap[portIndex] = audioData;
    else
        _dataMap[portIndex] = nullptr;
    Q_EMIT dataUpdated(portIndex);
}

QWidget *AudioOutDataModel::embeddedWidget()
{
    return nullptr;
}

} // namespace Nodes
