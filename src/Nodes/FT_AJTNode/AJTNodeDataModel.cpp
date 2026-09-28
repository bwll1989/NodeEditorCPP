#include "AJTNodeDataModel.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QSignalBlocker>
#include <algorithm>

namespace Nodes {

namespace {
constexpr int kUiDebounceMs = 16;
}

AJTNodeDataModel::AJTNodeDataModel()
    : _interface(new AJTNodeInterface())
    , _uiDebounce(new QTimer(this))
{
    InPortCount = kChannelCount + 1; // CH1～CH6 + 全开/全关
    OutPortCount = 1;                // DATA
    PortEditable = false;
    CaptionVisible = true;
    Caption = QStringLiteral("AJT Dimming Node");
    WidgetEmbeddable = false;
    Resizable = false;

    _device.kind = QString::fromLatin1(AJTProtocol::kKindDim);
    _device.id = 0x15;
    _device.enable = false;
    _device.channels.fill(0);
    _deviceOut = std::make_shared<NodeDataTypes::VariableData>();

    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "deviceId";
        b.control = _interface->_idEdit;
        AbstractDelegateModel::registerExternalBinding("/id", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "enable";
        b.control = _interface->_enableButton;
        AbstractDelegateModel::registerExternalBinding("/enable", this, b);
    }

    connect(_interface->_idEdit, &IntDragValueWidget::valueChanged, this, [this](int val) {
        setDeviceId(val);
    });
    connect(_interface->_enableButton, &QPushButton::toggled, this, [this](bool checked) {
        setEnable(checked);
    });

    for (int i = 0; i < kChannelCount; ++i) {
        {
            NodeDelegateModel::ExternalBinding b;
            b.control = _interface->_channelEdits[i];
            AbstractDelegateModel::registerExternalBinding(QString("/CH%1").arg(i + 1), nullptr, b);
        }
        connect(_interface->_channelEdits[i], &IntDragValueWidget::valueChanged, this, [this, i](int val) {
            setChannelLevel(i, val);
        });
    }

    _uiDebounce->setSingleShot(true);
    _uiDebounce->setInterval(kUiDebounceMs);
    connect(_uiDebounce, &QTimer::timeout, this, &AJTNodeDataModel::flushPendingUi);

    _interface->setDeviceId(_device.id);
    _interface->setEnableChecked(_device.enable);
    publishDevice();
}

AJTNodeDataModel::~AJTNodeDataModel()
{
    _loading = true;
    if (_uiDebounce) {
        _uiDebounce->stop();
    }
    if (auto *bus = GlobalEventBus::instance()) {
        bus->unsubscribe(this);
    }
}

void AJTNodeDataModel::setDeviceId(int id)
{
    id = std::clamp(id, 0, 255);
    if (_device.id == id) {
        return;
    }
    _device.id = id;
    if (_interface) {
        _interface->setDeviceId(_device.id);
    }
    emit deviceIdChanged(_device.id);
    if (!_loading) {
        publishDevice();
    }
}

void AJTNodeDataModel::setEnable(bool enable)
{
    if (_device.enable == enable) {
        return;
    }
    _device.enable = enable;
    if (_interface) {
        _interface->setEnableChecked(_device.enable);
    }
    emit enableChanged(_device.enable);
    if (!_loading) {
        publishDevice();
    }
}

void AJTNodeDataModel::afterModelReady()
{
    AbstractDelegateModel::afterModelReady();
    auto *bus = GlobalEventBus::instance();
    bus->subscribe(makeFullOscAddress("/id"), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress("/enable"), this, SLOT(onGlobalEvent(GlobalEvent)));
    for (int i = 0; i < kChannelCount; ++i) {
        bus->subscribe(makeFullOscAddress(QString("/CH%1").arg(i + 1)), this, SLOT(onGlobalEvent(GlobalEvent)));
    }
}

void AJTNodeDataModel::onGlobalEvent(const GlobalEvent &ev)
{
    if (_loading || ev.kind != GlobalEventKind::Command) {
        return;
    }

    const QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
    if (localPath == QLatin1String("id")) {
        setDeviceId(ev.payload.toInt());
    } else if (localPath == QLatin1String("enable")) {
        setEnable(ev.payload.toBool());
    } else if (localPath.startsWith(QLatin1String("CH"))) {
        bool ok = false;
        const int ch = localPath.mid(2).toInt(&ok);
        if (ok && ch >= 1 && ch <= kChannelCount) {
            setChannelLevel(ch - 1, ev.payload.toInt());
        }
    }
}

NodeDataType AJTNodeDataModel::dataType(PortType, PortIndex) const
{
    return NodeDataTypes::VariableData().type();
}

std::shared_ptr<NodeData> AJTNodeDataModel::outData(PortIndex)
{
    return _deviceOut;
}

void AJTNodeDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex port)
{
    if (_loading || port >= InPortCount) {
        return;
    }

    if (port == kEnablePort) {
        auto varData = std::dynamic_pointer_cast<NodeDataTypes::VariableData>(data);
        if (!varData) {
            setEnable(false);
            return;
        }
        setEnable(varData->asBool());
        return;
    }

    auto varData = std::dynamic_pointer_cast<NodeDataTypes::VariableData>(data);
    if (!varData || port < 0 || port >= kChannelCount) {
        return;
    }
    setChannelLevel(port, varData->asInt());
}

QString AJTNodeDataModel::portCaption(PortType portType, PortIndex portIndex) const
{
    if (portType == PortType::Out) {
        return QStringLiteral("DATA");
    }
    if (portType == PortType::In && portIndex == kEnablePort) {
        return QStringLiteral("ALL ON/OFF");
    }
    if (portIndex >= 0 && portIndex < kChannelCount) {
        return QString("CH%1").arg(portIndex + 1);
    }
    return {};
}

QJsonObject AJTNodeDataModel::save() const
{
    QJsonObject modelJson = NodeDelegateModel::save();
    QJsonObject values;
    values[QStringLiteral("id")] = _device.id;
    values[QStringLiteral("enable")] = _device.enable;
    QJsonArray levels;
    for (int i = 0; i < kChannelCount; ++i) {
        levels.append(_device.channels[i]);
    }
    values[QStringLiteral("levels")] = levels;
    modelJson[QStringLiteral("values")] = values;
    return modelJson;
}

void AJTNodeDataModel::load(QJsonObject const &p)
{
    const QJsonValue v = p[QStringLiteral("values")];
    if (!v.isObject()) {
        return;
    }

    _loading = true;
    const QJsonObject values = v.toObject();
    if (values.contains(QStringLiteral("id"))) {
        setDeviceId(values[QStringLiteral("id")].toInt());
    }
    if (values.contains(QStringLiteral("levels")) && values[QStringLiteral("levels")].isArray()) {
        const QJsonArray levels = values[QStringLiteral("levels")].toArray();
        for (int i = 0; i < kChannelCount && i < levels.size(); ++i) {
            setChannelLevel(i, levels[i].toInt());
        }
    }
    if (values.contains(QStringLiteral("enable"))) {
        setEnable(values[QStringLiteral("enable")].toBool());
    }
    _loading = false;
    flushPendingUi();
    publishDevice();
}

ConnectionPolicy AJTNodeDataModel::portConnectionPolicy(PortType portType, PortIndex) const
{
    switch (portType) {
    case PortType::In:
    case PortType::Out:
        return ConnectionPolicy::Many;
    case PortType::None:
        break;
    }
    return ConnectionPolicy::One;
}

void AJTNodeDataModel::setChannelLevel(int index, int level)
{
    if (index < 0 || index >= kChannelCount) {
        return;
    }
    level = AJTProtocol::clampLevel(level);
    if (_device.channels[index] == level) {
        return;
    }
    _device.channels[index] = level;
    scheduleUiUpdate(index);
    AbstractDelegateModel::stateFeedBack(QString("/CH%1").arg(index + 1), level);
    if (!_loading) {
        publishDevice();
    }
}

void AJTNodeDataModel::scheduleUiUpdate(int index)
{
    if (index < 0 || index >= kChannelCount) {
        return;
    }
    _uiDirtyMask |= static_cast<quint8>(1u << index);
    if (_loading) {
        return;
    }
    if (_uiDebounce) {
        _uiDebounce->start();
    }
}

void AJTNodeDataModel::flushPendingUi()
{
    if (!_interface || _uiDirtyMask == 0) {
        return;
    }
    const quint8 mask = _uiDirtyMask;
    _uiDirtyMask = 0;
    for (int i = 0; i < kChannelCount; ++i) {
        if (mask & (1u << i)) {
            _interface->setChannelLevel(i, _device.channels[i]);
        }
    }
}

void AJTNodeDataModel::publishDevice()
{
    _device.kind = QString::fromLatin1(AJTProtocol::kKindDim);
    auto out = std::make_shared<NodeDataTypes::VariableData>();
    out->insert(QStringLiteral("default"), AJTProtocol::toVariantMap(_device));
    _deviceOut = out;
    Q_EMIT dataUpdated(0);
}

} // namespace Nodes
