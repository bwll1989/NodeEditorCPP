#include "AJTGatewayDataModel.hpp"

#include <QJsonObject>
#include <QSignalBlocker>
#include <algorithm>

namespace Nodes {

AJTGatewayDataModel::AJTGatewayDataModel()
    : _interface(new AJTGatewayInterface())
    , _dimDebounce(new QTimer(this))
    , _relayDebounce(new QTimer(this))
{
    InPortCount = 4; // NODE 1～4，可编辑增删
    OutPortCount = 1; // STATUS
    PortEditable = true;
    CaptionVisible = true;
    Caption = QStringLiteral("AJT Gateway");
    WidgetEmbeddable = false;
    Resizable = false;

    _statusOut = std::make_shared<NodeDataTypes::VariableData>(false);

    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "host";
        b.control = _interface->_hostEdit;
        AbstractDelegateModel::registerExternalBinding("/host", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "port";
        b.control = _interface->_portEdit;
        AbstractDelegateModel::registerExternalBinding("/port", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "srcAddr";
        b.control = _interface->_srcAddrEdit;
        AbstractDelegateModel::registerExternalBinding("/srcAddr", this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = "connected";
        b.control = _interface->_statusLabel;
        AbstractDelegateModel::registerExternalBinding("/connected", this, b);
    }

    connect(_interface->_hostEdit, &QLineEdit::editingFinished, this, [this]() {
        setHost(_interface->_hostEdit->text());
    });
    connect(_interface->_portEdit, &IntDragValueWidget::valueChanged, this, [this](int val) {
        setPort(val);
    });
    connect(_interface->_srcAddrEdit, &IntDragValueWidget::valueChanged, this, [this](int val) {
        setSrcAddr(val);
    });

    _dimDebounce->setSingleShot(true);
    connect(_dimDebounce, &QTimer::timeout, this, &AJTGatewayDataModel::onDimDebounceTimeout);

    _relayDebounce->setSingleShot(true);
    connect(_relayDebounce, &QTimer::timeout, this, &AJTGatewayDataModel::onRelayDebounceTimeout);

    _interface->_hostEdit->setText(_host);
    _interface->_portEdit->setValue(_port);
    _interface->_srcAddrEdit->setValue(_srcAddr);
    updateDeviceCountUi();
}

AJTGatewayDataModel::~AJTGatewayDataModel()
{
    _modelReady = false;
    _loading = true;
    clearPendingSend();
    if (auto *bus = GlobalEventBus::instance()) {
        bus->unsubscribe(this);
    }
    destroyTcpClient();
}

void AJTGatewayDataModel::ensureTcpClient()
{
    if (_tcpClient || !_modelReady || _loading) {
        return;
    }

    _tcpClient = new TcpClient(_host, _port);
    connect(_tcpClient, &TcpClient::isReady, this, [this](bool ready) {
        if (!_modelReady || _loading || !_tcpClient) {
            return;
        }
        setConnected(ready);
    }, Qt::QueuedConnection);
}

void AJTGatewayDataModel::destroyTcpClient()
{
    if (!_tcpClient) {
        return;
    }
    QObject::disconnect(_tcpClient, nullptr, this, nullptr);
    _tcpClient->disconnectFromServer();
    _tcpClient->stopTimer();
    TcpClient *client = _tcpClient;
    _tcpClient = nullptr;
    _connected = false;
    delete client;
}

void AJTGatewayDataModel::setHost(const QString &host)
{
    if (_host == host) {
        return;
    }
    _host = host;
    if (_interface) {
        QSignalBlocker blocker(_interface->_hostEdit);
        _interface->_hostEdit->setText(_host);
    }
    emit hostChanged(_host);
    if (!_loading) {
        reconnect();
    }
}

void AJTGatewayDataModel::setPort(int port)
{
    if (_port == port) {
        return;
    }
    _port = port;
    if (_interface) {
        QSignalBlocker blocker(_interface->_portEdit);
        _interface->_portEdit->setValue(_port);
    }
    emit portChanged(_port);
    if (!_loading) {
        reconnect();
    }
}

void AJTGatewayDataModel::setSrcAddr(int addr)
{
    addr = std::clamp(addr, 0, 255);
    if (_srcAddr == addr) {
        return;
    }
    _srcAddr = addr;
    if (_interface) {
        QSignalBlocker blocker(_interface->_srcAddrEdit);
        _interface->_srcAddrEdit->setValue(_srcAddr);
    }
    emit srcAddrChanged(_srcAddr);
    if (!_loading) {
        scheduleSend(SendBoth);
    }
}

void AJTGatewayDataModel::setConnected(bool connected)
{
    if (_connected == connected) {
        return;
    }
    _connected = connected;
    if (_interface) {
        _interface->setConnectionStatus(_connected);
    }
    emit connectedChanged(_connected);
    publishStatus();

    // 连上后补发：两类分开发、间隔 ≥500ms
    if (_connected && !_loading && _modelReady) {
        scheduleSend(SendBoth);
    }
}

void AJTGatewayDataModel::reconnect()
{
    if (_loading || !_modelReady) {
        return;
    }
    clearPendingSend();
    ensureTcpClient();
    if (!_tcpClient) {
        return;
    }
    _tcpClient->disconnectFromServer();
    _tcpClient->stopTimer();
    _tcpClient->connectToServer(_host, _port);
}

quint8 AJTGatewayDataModel::sendKindForDevice(const AJTProtocol::DeviceState &dev)
{
    return AJTProtocol::isRelay(dev) ? SendRelay : SendDim;
}

int AJTGatewayDataModel::msUntilSendAllowed() const
{
    if (!_hasSentOnce) {
        return 0;
    }
    const qint64 elapsed = _sinceLastSend.elapsed();
    if (elapsed >= AJTProtocol::kMinSendGapMs) {
        return 0;
    }
    return static_cast<int>(AJTProtocol::kMinSendGapMs - elapsed);
}

void AJTGatewayDataModel::scheduleSend(quint8 kinds)
{
    if (_loading || !_modelReady || kinds == SendNone) {
        return;
    }

    // 同类型：重启 500ms 去重窗口，合并窗口内所有变化
    if (kinds & SendDim) {
        _pendingDim = true;
        if (_dimDebounce) {
            _dimDebounce->start(AJTProtocol::kSendDebounceMs);
        }
    }
    if (kinds & SendRelay) {
        _pendingRelay = true;
        if (_relayDebounce) {
            _relayDebounce->start(AJTProtocol::kSendDebounceMs);
        }
    }
}

void AJTGatewayDataModel::clearPendingSend()
{
    if (_dimDebounce) {
        _dimDebounce->stop();
    }
    if (_relayDebounce) {
        _relayDebounce->stop();
    }
    _pendingDim = false;
    _pendingRelay = false;
}

void AJTGatewayDataModel::onDimDebounceTimeout()
{
    tryFlush(SendDim);
}

void AJTGatewayDataModel::onRelayDebounceTimeout()
{
    tryFlush(SendRelay);
}

void AJTGatewayDataModel::tryFlush(quint8 kind)
{
    if (_loading || !_modelReady) {
        return;
    }

    const bool wantDim = (kind == SendDim) && _pendingDim;
    const bool wantRelay = (kind == SendRelay) && _pendingRelay;
    if (!wantDim && !wantRelay) {
        return;
    }

    // 距上一帧不足 500ms：推迟到间隔满足后再发（异类型错开）
    const int waitMs = msUntilSendAllowed();
    if (waitMs > 0) {
        if (wantDim && _dimDebounce) {
            _dimDebounce->start(waitMs);
        }
        if (wantRelay && _relayDebounce) {
            _relayDebounce->start(waitMs);
        }
        return;
    }

    if (!_connected || !_tcpClient) {
        // 未连接：保留 pending，连上后 scheduleSend 会再触发
        return;
    }

    const auto devices = collectDevices();
    const quint8 src = static_cast<quint8>(_srcAddr & 0xFF);
    const quint8 dst = AJTProtocol::kFrameDstAddr;

    QByteArray frame;
    if (wantDim) {
        frame = AJTProtocol::buildMultiDimFrame(src, dst, devices);
        _pendingDim = false;
    } else {
        frame = AJTProtocol::buildMultiRelayFrame(src, dst, devices);
        _pendingRelay = false;
    }

    if (!frame.isEmpty()) {
        _tcpClient->sendMessage(QString::fromLatin1(frame.toHex()), 0);
        _sinceLastSend.restart();
        _hasSentOnce = true;
    }

    // 另一类仍 pending：至少再隔 500ms 再发
    if (_pendingDim && _dimDebounce && !_dimDebounce->isActive()) {
        _dimDebounce->start(AJTProtocol::kMinSendGapMs);
    }
    if (_pendingRelay && _relayDebounce && !_relayDebounce->isActive()) {
        _relayDebounce->start(AJTProtocol::kMinSendGapMs);
    }
}

void AJTGatewayDataModel::onGlobalEvent(const GlobalEvent &ev)
{
    if (_loading || ev.kind != GlobalEventKind::Command) {
        return;
    }

    const QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
    if (localPath == QLatin1String("host")) {
        setHost(ev.payload.toString());
    } else if (localPath == QLatin1String("port")) {
        setPort(ev.payload.toInt());
    } else if (localPath == QLatin1String("srcAddr")) {
        setSrcAddr(ev.payload.toInt());
    }
}

void AJTGatewayDataModel::afterModelReady()
{
    AbstractDelegateModel::afterModelReady();
    _modelReady = true;

    auto *bus = GlobalEventBus::instance();
    bus->subscribe(makeFullOscAddress("/host"), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress("/port"), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress("/srcAddr"), this, SLOT(onGlobalEvent(GlobalEvent)));

    QTimer::singleShot(0, this, [this]() {
        if (!_modelReady || _loading) {
            return;
        }
        ensureTcpClient();
        if (_connected) {
            scheduleSend(SendBoth);
        }
    });
}

NodeDataType AJTGatewayDataModel::dataType(PortType, PortIndex) const
{
    return NodeDataTypes::VariableData().type();
}

std::shared_ptr<NodeData> AJTGatewayDataModel::outData(PortIndex)
{
    return _statusOut;
}

void AJTGatewayDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex port)
{
    if (_loading) {
        return;
    }

    if (!data) {
        quint8 kinds = SendNone;
        const auto it = _devicesByPort.find(static_cast<int>(port));
        if (it != _devicesByPort.end()) {
            kinds = sendKindForDevice(it->second);
            _devicesByPort.erase(it);
        }
        updateDeviceCountUi();
        scheduleSend(kinds);
        return;
    }

    auto varData = std::dynamic_pointer_cast<NodeDataTypes::VariableData>(data);
    if (!varData) {
        return;
    }

    const QVariant defaultValue = varData->value(QStringLiteral("default"));
    if (!defaultValue.isValid()) {
        return;
    }

    AJTProtocol::DeviceState device;
    if (!AJTProtocol::fromVariantMap(defaultValue.toMap(), device)) {
        return;
    }

    _devicesByPort[static_cast<int>(port)] = device;
    updateDeviceCountUi();
    scheduleSend(sendKindForDevice(device));
}

QString AJTGatewayDataModel::portCaption(PortType portType, PortIndex portIndex) const
{
    if (portType == PortType::Out) {
        return QStringLiteral("STATUS");
    }
    return QStringLiteral("NODE %1").arg(portIndex + 1);
}

QJsonObject AJTGatewayDataModel::save() const
{
    QJsonObject modelJson = NodeDelegateModel::save();
    QJsonObject values;
    values[QStringLiteral("host")] = _host;
    values[QStringLiteral("port")] = _port;
    values[QStringLiteral("srcAddr")] = _srcAddr;
    modelJson[QStringLiteral("values")] = values;
    return modelJson;
}

void AJTGatewayDataModel::load(QJsonObject const &p)
{
    const QJsonValue v = p[QStringLiteral("values")];
    if (!v.isObject()) {
        return;
    }

    _loading = true;
    clearPendingSend();
    const QJsonObject values = v.toObject();
    if (values.contains(QStringLiteral("host"))) {
        setHost(values[QStringLiteral("host")].toString());
    }
    if (values.contains(QStringLiteral("port"))) {
        setPort(values[QStringLiteral("port")].toInt());
    }
    if (values.contains(QStringLiteral("srcAddr"))) {
        setSrcAddr(values[QStringLiteral("srcAddr")].toInt());
    }
    _loading = false;

    if (_modelReady) {
        ensureTcpClient();
        if (_connected) {
            scheduleSend(SendBoth);
        }
    }
}

ConnectionPolicy AJTGatewayDataModel::portConnectionPolicy(PortType portType, PortIndex) const
{
    switch (portType) {
    case PortType::In:
        return ConnectionPolicy::One;
    case PortType::Out:
        return ConnectionPolicy::Many;
    case PortType::None:
        break;
    }
    return ConnectionPolicy::One;
}

void AJTGatewayDataModel::updateDeviceCountUi()
{
    if (_interface) {
        _interface->setDeviceCount(static_cast<int>(_devicesByPort.size()));
    }
}

void AJTGatewayDataModel::publishStatus()
{
    _statusOut = std::make_shared<NodeDataTypes::VariableData>(_connected);
    Q_EMIT dataUpdated(0);
}

std::vector<AJTProtocol::DeviceState> AJTGatewayDataModel::collectDevices() const
{
    std::vector<std::pair<int, AJTProtocol::DeviceState>> ordered;
    ordered.reserve(_devicesByPort.size());
    for (const auto &kv : _devicesByPort) {
        ordered.emplace_back(kv.first, kv.second);
    }
    std::sort(ordered.begin(), ordered.end(),
              [](const auto &a, const auto &b) { return a.first < b.first; });

    std::vector<AJTProtocol::DeviceState> devices;
    devices.reserve(ordered.size());
    for (const auto &kv : ordered) {
        if (kv.first < 0 || kv.first >= static_cast<int>(InPortCount)) {
            continue;
        }
        devices.push_back(kv.second);
    }
    return devices;
}

} // namespace Nodes
