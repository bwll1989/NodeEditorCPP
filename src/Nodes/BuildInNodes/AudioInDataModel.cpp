#include "AudioInDataModel.hpp"

#include "StatusContainer/GlobalEventBus.hpp"

namespace Nodes {

AudioInDataModel::AudioInDataModel()
{
    InPortCount = 0;
    OutPortCount = 1;
    CaptionVisible = true;
    Caption = QStringLiteral("Audio In");
    WidgetEmbeddable = false;
    Resizable = false;
    PortEditable = true;
    setRemarks(QStringLiteral("Audio In"));
}

QString AudioInDataModel::remarks() const
{
    return getRemarks();
}

void AudioInDataModel::setRemarks(const QString &remarks)
{
    QString trimmed = remarks.trimmed();
    if (trimmed.isEmpty())
        trimmed = QStringLiteral("Audio In");
    if (trimmed == getRemarks())
        return;
    NodeDelegateModel::setRemarks(trimmed);
    Q_EMIT remarksChanged(trimmed);
}

NodeDataType AudioInDataModel::dataType(PortType, PortIndex) const
{
    return NodeDataTypes::AudioData().type();
}

std::shared_ptr<NodeData> AudioInDataModel::outData(PortIndex port)
{
    return _dataMap[port];
}

QWidget *AudioInDataModel::embeddedWidget()
{
    return nullptr;
}

void AudioInDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
{
    auto audioData = std::dynamic_pointer_cast<NodeDataTypes::AudioData>(data);
    if (audioData && audioData->isConnectedToSharedBuffer())
        _dataMap[portIndex] = audioData;
    else
        _dataMap[portIndex] = nullptr;
    Q_EMIT dataUpdated(portIndex);
}

void AudioInDataModel::afterModelReady()
{
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/input"),
                                          this,
                                          SLOT(onGlobalEvent(GlobalEvent)));
}

void AudioInDataModel::onGlobalEvent(const GlobalEvent &ev)
{
    if (ev.kind != GlobalEventKind::Command)
        return;
    if (ev.address != makeFullOscAddress("/input"))
        return;
    setRemarks(ev.payload.toString());
}

} // namespace Nodes
