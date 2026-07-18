#include "SnapshotModel.hpp"

#include <QSignalBlocker>
#include <QTimer>
#include <algorithm>

using namespace Nodes;

SnapshotModel::SnapshotModel()
{
    InPortCount = 2;
    OutPortCount = 1;
    CaptionVisible = true;
    Caption = QStringLiteral("Snapshot");
    WidgetEmbeddable = true;
    Resizable = true;
    PortEditable = false;

    m_activeOutput = std::make_shared<VariableData>(QVariant(0));

    ensureDefaultPreset();
    syncUi();

    connect(widget, &SnapshotInterface::presetRecallRequested, this, &SnapshotModel::recallPreset);
    connect(widget, &SnapshotInterface::captureRequested, this, &SnapshotModel::captureActivePreset);
    connect(widget, &SnapshotInterface::addPresetRequested, this, &SnapshotModel::addPreset);
    connect(widget, &SnapshotInterface::removePresetRequested, this, &SnapshotModel::removeActivePreset);

    {
        NodeDelegateModel::ExternalBinding binding;
        binding.member = QStringLiteral("activeIndex");
        AbstractDelegateModel::registerExternalBinding("/index", this, binding);
    }
}

NodeDataType SnapshotModel::dataType(PortType portType, PortIndex portIndex) const
{
    Q_UNUSED(portType)
    Q_UNUSED(portIndex)
    return VariableData().type();
}

QString SnapshotModel::portCaption(PortType portType, PortIndex portIndex) const
{
    if (portType == PortType::In) {
        switch (portIndex) {
        case 0: return QStringLiteral("INDEX");
        case 1: return QStringLiteral("CAPTURE");
        default: return QString();
        }
    }
    if (portType == PortType::Out) {
        return portIndex == 0 ? QStringLiteral("ACTIVE") : QString();
    }
    return QString();
}

std::shared_ptr<NodeData> SnapshotModel::outData(PortIndex port)
{
    Q_UNUSED(port)
    return m_activeOutput;
}

void SnapshotModel::setInData(std::shared_ptr<NodeData> nodeData, PortIndex port)
{
    if (!nodeData) {
        return;
    }

    auto var = std::dynamic_pointer_cast<VariableData>(nodeData);
    if (!var) {
        return;
    }

    switch (port) {
    case 0:
        recallPreset(var->value().toInt());
        break;
    case 1:
        if (var->value().toBool()) {
            captureActivePreset();
        }
        break;
    default:
        break;
    }
}

QJsonObject SnapshotModel::save() const
{
    QJsonArray presetsJson;
    for (const SnapshotPreset &preset : m_presets) {
        QJsonObject item;
        item[QStringLiteral("name")] = preset.name;
        item[QStringLiteral("nodes")] = preset.nodes;
        presetsJson.append(item);
    }

    QJsonObject values;
    values[QStringLiteral("activeIndex")] = m_activeIndex;
    values[QStringLiteral("presets")] = presetsJson;

    QJsonObject modelJson = NodeDelegateModel::save();
    modelJson[QStringLiteral("values")] = values;
    return modelJson;
}

void SnapshotModel::load(const QJsonObject &p)
{
    m_suppressRecall = true;

    const QJsonObject values = p.value(QStringLiteral("values")).toObject();
    m_activeIndex = values.value(QStringLiteral("activeIndex")).toInt(0);
    m_presets.clear();

    const QJsonArray presetsJson = values.value(QStringLiteral("presets")).toArray();
    for (const QJsonValue &value : presetsJson) {
        if (!value.isObject()) {
            continue;
        }
        const QJsonObject obj = value.toObject();
        SnapshotPreset preset;
        preset.name = obj.value(QStringLiteral("name")).toString();
        preset.nodes = obj.value(QStringLiteral("nodes")).toArray();
        m_presets.append(preset);
    }

    ensureDefaultPreset();
    syncUi();
    m_suppressRecall = false;

    // 工程加载完成后在 afterModelReady 中召回当前活动预设
    if (m_activeIndex >= 0 && m_activeIndex < m_presets.size()) {
        m_recallActiveOnReady = !m_presets.at(m_activeIndex).nodes.isEmpty();
    }
}

ConnectionPolicy SnapshotModel::portConnectionPolicy(PortType portType, PortIndex index) const
{
    Q_UNUSED(index)
    switch (portType) {
    case PortType::In:
    case PortType::Out:
        return ConnectionPolicy::Many;
    default:
        return ConnectionPolicy::One;
    }
}

void SnapshotModel::setActiveIndex(int index)
{
    if (index < 0 || index >= m_presets.size()) {
        return;
    }
    if (m_activeIndex == index) {
        return;
    }
    m_activeIndex = index;
    syncUi();
    Q_EMIT activeIndexChanged(index);
}

void SnapshotModel::recallPreset(int index)
{
    if (index < 0 || index >= m_presets.size()) {
        widget->setStatusText(tr("预设索引无效"));
        return;
    }

    setActiveIndex(index);

    const QString title = sceneTitle();
    if (title.isEmpty()) {
        widget->setStatusText(tr("无法确定当前 dataflow"));
        return;
    }

    const SnapshotPreset &preset = m_presets.at(index);
    if (preset.nodes.isEmpty()) {
        widget->setStatusText(tr("预设「%1」为空，请先捕获").arg(preset.name));
        return;
    }

    if (!GraphSnapshotBridge::instance()->applySnapshot(title, preset.nodes)) {
        widget->setStatusText(tr("召回失败：未找到对应 dataflow"));
        return;
    }

    m_activeOutput = std::make_shared<VariableData>(QVariant(index));
    Q_EMIT dataUpdated(0);
    widget->setStatusText(tr("已召回预设「%1」").arg(preset.name));
}

void SnapshotModel::captureActivePreset()
{
    if (m_activeIndex < 0 || m_activeIndex >= m_presets.size()) {
        return;
    }

    const QString title = sceneTitle();
    if (title.isEmpty()) {
        widget->setStatusText(tr("无法确定当前 dataflow"));
        return;
    }

    const QVector<NodeId> nodeIds = collectCaptureNodeIds();
    if (nodeIds.isEmpty()) {
        widget->setStatusText(tr("请先在画布中选中要捕获的节点"));
        return;
    }

    m_presets[m_activeIndex].nodes = GraphSnapshotBridge::instance()->captureNodes(title, nodeIds);
    widget->setStatusText(tr("已捕获 %1 个节点到「%2」")
                              .arg(m_presets[m_activeIndex].nodes.size())
                              .arg(m_presets[m_activeIndex].name));
}

void SnapshotModel::addPreset()
{
    SnapshotPreset preset;
    preset.name = tr("预设 %1").arg(m_presets.size() + 1);
    m_presets.append(preset);
    m_activeIndex = m_presets.size() - 1;
    syncUi();
}

void SnapshotModel::removeActivePreset()
{
    if (m_presets.size() <= 1) {
        widget->setStatusText(tr("至少保留一个预设"));
        return;
    }

    m_presets.removeAt(m_activeIndex);
    if (m_activeIndex >= m_presets.size()) {
        m_activeIndex = m_presets.size() - 1;
    }
    syncUi();
}

void SnapshotModel::onGlobalEvent(const GlobalEvent &ev)
{
    if (ev.kind != GlobalEventKind::Command) {
        return;
    }

    const QString addr = ev.address;
    if (addr == makeFullOscAddress("/index")) {
        recallPreset(ev.payload.toInt());
    } else if (addr == makeFullOscAddress("/capture")) {
        if (ev.payload.toBool()) {
            captureActivePreset();
        }
    }
}

void SnapshotModel::afterModelReady()
{
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/index"), this, SLOT(onGlobalEvent(GlobalEvent)));
    GlobalEventBus::instance()->subscribe(makeFullOscAddress("/capture"), this, SLOT(onGlobalEvent(GlobalEvent)));

    if (!m_recallActiveOnReady) {
        return;
    }
    m_recallActiveOnReady = false;

    // 延迟召回，确保 VST3 等节点完成 loadPlugin / 音频初始化
    QTimer::singleShot(100, this, [this]() {
        recallPreset(m_activeIndex);
    });
}

void SnapshotModel::ensureDefaultPreset()
{
    if (m_presets.isEmpty()) {
        SnapshotPreset preset;
        preset.name = tr("预设 1");
        m_presets.append(preset);
        m_activeIndex = 0;
    }

    if (m_activeIndex < 0 || m_activeIndex >= m_presets.size()) {
        m_activeIndex = 0;
    }
}

void SnapshotModel::syncUi()
{
    QStringList names;
    names.reserve(m_presets.size());
    for (const SnapshotPreset &preset : m_presets) {
        names.append(preset.name);
    }

    widget->setPresets(names, m_activeIndex);
    m_activeOutput = std::make_shared<VariableData>(QVariant(m_activeIndex));
}

QString SnapshotModel::sceneTitle() const
{
    return getParentAlias();
}

QVector<NodeId> SnapshotModel::collectCaptureNodeIds() const
{
    const QString title = sceneTitle();
    QVector<NodeId> nodeIds = GraphSnapshotBridge::instance()->selectedNodes(title);
    const NodeId selfId = getNodeID();

    nodeIds.erase(std::remove(nodeIds.begin(), nodeIds.end(), selfId), nodeIds.end());
    return nodeIds;
}
