#include "AJTRelayDataModel.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QSignalBlocker>
#include <algorithm>

namespace Nodes {

namespace {
constexpr int kUiDebounceMs = 16;
}

AJTRelayDataModel::AJTRelayDataModel()
    : _interface(new AJTRelayInterface())
    , _uiDebounce(new QTimer(this))
{
    InPortCount = kChannelCount + 1; // CH1～CH12 + 全开/全关
    OutPortCount = 1;                // DATA
    PortEditable = false;
    CaptionVisible = true;
    Caption = QStringLiteral("AJT Relay Node");
    WidgetEmbeddable = false;
    Resizable = false;

    _device.kind = QString::fromLatin1(AJTProtocol::kKindRelay);
    _device.id = 0x01;
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
            setChannelState(i, val);
        });
    }

    _uiDebounce->setSingleShot(true);
    _uiDebounce->setInterval(kUiDebounceMs);
    connect(_uiDebounce, &QTimer::timeout, this, &AJTRelayDataModel::flushPendingUi);

    _interface->setDeviceId(_device.id);
    _interface->setEnableChecked(_device.enable);
    publishDevice();
}

AJTRelayDataModel::~AJTRelayDataModel()
{
    _loading = true;
    if (_uiDebounce) {
        _uiDebounce->stop();
    }
    if (auto *bus = GlobalEventBus::instance()) {
        bus->unsubscribe(this);
    }
}

void AJTRelayDataModel::setDeviceId(int id)
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

void AJTRelayDataModel::setEnable(bool enable)
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

void AJTRelayDataModel::afterModelReady()
{
    AbstractDelegateModel::afterModelReady();
    auto *bus = GlobalEventBus::instance();
    bus->subscribe(makeFullOscAddress("/id"), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress("/enable"), this, SLOT(onGlobalEvent(GlobalEvent)));
    for (int i = 0; i < kChannelCount; ++i) {
        bus->subscribe(makeFullOscAddress(QString("/CH%1").arg(i + 1)), this, SLOT(onGlobalEvent(GlobalEvent)));
    }
}

void AJTRelayDataModel::onGlobalEvent(const GlobalEvent &ev)
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
            setChannelState(ch - 1, ev.payload.toInt());
        }
    }
}

NodeDataType AJTRelayDataModel::dataType(PortType, PortIndex) const
{
    return NodeDataTypes::VariableData().type();
}

std::shared_ptr<NodeData> AJTRelayDataModel::outData(PortIndex)
{
    return _deviceOut;
}

void AJTRelayDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex port)
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
    setChannelState(port, varData->asInt());
}

QString AJTRelayDataModel::portCaption(PortType portType, PortIndex portIndex) const
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

QJsonObject AJTRelayDataModel::save() const
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

void AJTRelayDataModel::load(QJsonObject const &p)
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
            setChannelState(i, levels[i].toInt());
        }
    }
    if (values.contains(QStringLiteral("enable"))) {
        setEnable(values[QStringLiteral("enable")].toBool());
    }
    _loading = false;
    flushPendingUi();
    publishDevice();
}

ConnectionPolicy AJTRelayDataModel::portConnectionPolicy(PortType portType, PortIndex) const
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

void AJTRelayDataModel::setChannelState(int index, int on)
{
    if (index < 0 || index >= kChannelCount) {
        return;
    }
    on = AJTProtocol::clampSwitch(on);
    if (_device.channels[index] == on) {
        return;
    }
    _device.channels[index] = on;
    scheduleUiUpdate(index);
    AbstractDelegateModel::stateFeedBack(QString("/CH%1").arg(index + 1), on);
    if (!_loading) {
        publishDevice();
    }
}

void AJTRelayDataModel::scheduleUiUpdate(int index)
{
    if (index < 0 || index >= kChannelCount) {
        return;
    }
    _uiDirtyMask |= static_cast<quint16>(1u << index);
    if (_loading) {
        return;
    }
    if (_uiDebounce) {
        _uiDebounce->start();
    }
}

void AJTRelayDataModel::flushPendingUi()
{
    if (!_interface || _uiDirtyMask == 0) {
        return;
    }
    const quint16 mask = _uiDirtyMask;
    _uiDirtyMask = 0;
    for (int i = 0; i < kChannelCount; ++i) {
        if (mask & (1u << i)) {
            _interface->setChannelLevel(i, _device.channels[i]);
        }
    }
}

void AJTRelayDataModel::publishDevice()
{
    _device.kind = QString::fromLatin1(AJTProtocol::kKindRelay);
    auto out = std::make_shared<NodeDataTypes::VariableData>();
    out->insert(QStringLiteral("default"), AJTProtocol::toVariantMap(_device));
    _deviceOut = out;
    Q_EMIT dataUpdated(0);
}

} // namespace Nodes
