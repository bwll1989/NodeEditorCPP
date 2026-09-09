/**
 * @file SyncInDataModel.cpp
 */

#include "SyncInDataModel.hpp"
#include "SyncPairHub.hpp"
#include "SyncProtocol.hpp"

#include <QDateTime>
#include <QSignalBlocker>
#include <QtNetwork/QHostAddress>

namespace Nodes {

SyncInDataModel::SyncInDataModel()
{
    InPortCount = 0;
    OutPortCount = 1;
    CaptionVisible = true;
    Caption = QStringLiteral("Sync In");
    WidgetEmbeddable = false;
    Resizable = false;
    PortEditable = false;

    setupUi();

    _reconnectTimer.setInterval(1000);
    connect(&_reconnectTimer, &QTimer::timeout, this, &SyncInDataModel::onReconnectTick);

    _heartbeatWatchTimer.setInterval(1000);
    connect(&_heartbeatWatchTimer, &QTimer::timeout, this, &SyncInDataModel::onHeartbeatWatchTick);
}

SyncInDataModel::~SyncInDataModel()
{
    SyncPairHub::instance()->unregisterIn(this);
    GlobalEventBus::instance()->unsubscribe(this);
    disconnectSocket();
}

void SyncInDataModel::setupUi()
{
    _widget = new SyncInInterface();
    _widget->updateConnectionStatus(false);

    {
        NodeDelegateModel::ExternalBinding b;
        b.member = QStringLiteral("host");
        b.control = _widget->hostEdit;
        AbstractDelegateModel::registerExternalBinding(QStringLiteral("/host"), this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = QStringLiteral("port");
        b.control = _widget->portSpin;
        AbstractDelegateModel::registerExternalBinding(QStringLiteral("/port"), this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = QStringLiteral("active");
        b.control = _widget->activeCheck;
        AbstractDelegateModel::registerExternalBinding(QStringLiteral("/active"), this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = QStringLiteral("connected");
        b.control = _widget->connectionLabel;
        AbstractDelegateModel::registerExternalBinding(QStringLiteral("/connected"), this, b);
    }
    {
        NodeDelegateModel::ExternalBinding b;
        b.member = QStringLiteral("lastSource");
        AbstractDelegateModel::registerExternalBinding(QStringLiteral("/lastSource"), this, b);
    }

    connect(_widget->hostEdit, &QLineEdit::editingFinished, this, [this]() {
        setHost(_widget->hostEdit->text());
    });
    connect(_widget->portSpin, &IntDragValueWidget::valueChanged, this, &SyncInDataModel::setPort);
    connect(_widget->activeCheck, &QCheckBox::toggled, this, &SyncInDataModel::setActive);
}

void SyncInDataModel::afterModelReady()
{
    AbstractDelegateModel::afterModelReady();
    SyncPairHub::instance()->registerIn(this);

    auto *bus = GlobalEventBus::instance();
    bus->subscribe(makeFullOscAddress(QStringLiteral("/host")), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress(QStringLiteral("/port")), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress(QStringLiteral("/active")), this, SLOT(onGlobalEvent(GlobalEvent)));
    // 兼容旧地址
    bus->subscribe(makeFullOscAddress(QStringLiteral("/listenPort")), this, SLOT(onGlobalEvent(GlobalEvent)));

    SyncPairHub::instance()->syncPortCountForListenPort(_port);
    if (_active)
        connectSocket();
}

NodeDataType SyncInDataModel::dataType(PortType, PortIndex) const
{
    return NodeDataTypes::VariableData().type();
}

std::shared_ptr<NodeData> SyncInDataModel::outData(PortIndex port)
{
    auto it = _cache.find(port);
    if (it != _cache.end())
        return it->second;
    return nullptr;
}

void SyncInDataModel::setInData(std::shared_ptr<NodeData>, PortIndex)
{
}

QWidget *SyncInDataModel::embeddedWidget()
{
    return _widget;
}

void SyncInDataModel::applyPortCountFromPair(unsigned int newCount)
{
    applyPortCount(newCount);
}

void SyncInDataModel::setHost(const QString &host)
{
    const QString h = host.trimmed().isEmpty() ? QStringLiteral("127.0.0.1") : host.trimmed();
    if (_host == h)
        return;
    _host = h;
    if (_widget) {
        QSignalBlocker blocker(_widget->hostEdit);
        _widget->hostEdit->setText(_host);
    }
    Q_EMIT hostChanged(_host);
    if (_active)
        connectSocket();
}

void SyncInDataModel::setPort(int port)
{
    if (port <= 0 || port > 65535 || _port == port)
        return;
    _port = port;
    if (_widget) {
        QSignalBlocker blocker(_widget->portSpin);
        _widget->portSpin->setValue(_port);
    }
    Q_EMIT portChanged(_port);
    SyncPairHub::instance()->syncPortCountForListenPort(_port);
    if (_active)
        connectSocket();
}

void SyncInDataModel::setActive(bool active)
{
    if (_active == active)
        return;
    _active = active;
    if (_widget) {
        QSignalBlocker blocker(_widget->activeCheck);
        _widget->activeCheck->setChecked(_active);
    }
    Q_EMIT activeChanged(_active);
    if (_active)
        connectSocket();
    else
        disconnectSocket();
}

void SyncInDataModel::setConnected(bool connected)
{
    if (_connected == connected)
        return;
    _connected = connected;
    if (_widget)
        _widget->updateConnectionStatus(_connected);
    Q_EMIT connectedChanged(_connected);
}

void SyncInDataModel::setLastSource(const QString &source)
{
    if (_lastSource == source)
        return;
    _lastSource = source;
    if (_widget)
        _widget->updateSource(_lastSource);
    Q_EMIT lastSourceChanged(_lastSource);
}

void SyncInDataModel::onGlobalEvent(const GlobalEvent &ev)
{
    if (ev.kind != GlobalEventKind::Command)
        return;
    const QString addr = ev.address;
    if (addr == makeFullOscAddress(QStringLiteral("/host")))
        setHost(ev.payload.toString());
    else if (addr == makeFullOscAddress(QStringLiteral("/port"))
             || addr == makeFullOscAddress(QStringLiteral("/listenPort")))
        setPort(ev.payload.toInt());
    else if (addr == makeFullOscAddress(QStringLiteral("/active")))
        setActive(ev.payload.toBool());
}

void SyncInDataModel::disconnectSocket()
{
    _reconnectTimer.stop();
    _heartbeatWatchTimer.stop();
    if (!_socket)
        return;
    QObject::disconnect(_socket.get(), nullptr, this, nullptr);
    _socket->abort();
    _socket.reset();
    _rxBuffer.clear();
    _lastRxMs = 0;
    setConnected(false);
    setLastSource({});
}

void SyncInDataModel::connectSocket()
{
    disconnectSocket();
    if (!_active)
        return;

    _socket = std::make_unique<QTcpSocket>();
    _socket->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    connect(_socket.get(), &QTcpSocket::connected, this, &SyncInDataModel::onSocketConnected);
    connect(_socket.get(), &QTcpSocket::disconnected, this, &SyncInDataModel::onSocketDisconnected);
    connect(_socket.get(),
            QOverload<QAbstractSocket::SocketError>::of(&QAbstractSocket::errorOccurred),
            this,
            &SyncInDataModel::onSocketError);
    connect(_socket.get(), &QTcpSocket::readyRead, this, &SyncInDataModel::onReadyRead);

    _socket->connectToHost(_host, static_cast<quint16>(_port));
    scheduleReconnect();
}

void SyncInDataModel::scheduleReconnect()
{
    if (_active && !_connected)
        _reconnectTimer.start();
    else
        _reconnectTimer.stop();
}

void SyncInDataModel::onReconnectTick()
{
    if (!_active || _connected)
        return;
    if (!_socket)
        return;
    if (_socket->state() == QAbstractSocket::ConnectingState
        || _socket->state() == QAbstractSocket::HostLookupState)
        return;
    _socket->abort();
    _socket->connectToHost(_host, static_cast<quint16>(_port));
}

void SyncInDataModel::onSocketConnected()
{
    setConnected(true);
    _reconnectTimer.stop();
    touchRx();
    _heartbeatWatchTimer.start();
    setLastSource(QStringLiteral("%1:%2").arg(_host).arg(_port));
    SyncPairHub::instance()->syncPortCountForListenPort(_port);
}

void SyncInDataModel::onSocketDisconnected()
{
    _heartbeatWatchTimer.stop();
    setConnected(false);
    setLastSource({});
    _rxBuffer.clear();
    _lastRxMs = 0;
    scheduleReconnect();
}

void SyncInDataModel::onSocketError(QAbstractSocket::SocketError)
{
    if (_socket && _socket->state() != QAbstractSocket::ConnectedState)
        setConnected(false);
    scheduleReconnect();
}

void SyncInDataModel::touchRx()
{
    _lastRxMs = QDateTime::currentMSecsSinceEpoch();
}

void SyncInDataModel::replyHeartbeat()
{
    if (!_socket || _socket->state() != QAbstractSocket::ConnectedState)
        return;
    _socket->write(SyncProtocol::frameLine(SyncProtocol::makeHeartbeatPacket()));
}

void SyncInDataModel::onHeartbeatWatchTick()
{
    if (!_active || !_connected || !_socket)
        return;
    if (_lastRxMs <= 0)
        return;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    if ((now - _lastRxMs) <= SyncProtocol::HeartbeatTimeoutMs)
        return;
    // 半开连接：主动断开并走重连
    connectSocket();
}

void SyncInDataModel::onReadyRead()
{
    if (!_socket)
        return;
    _rxBuffer.append(_socket->readAll());
    const QList<QByteArray> lines = SyncProtocol::takeLines(_rxBuffer);
    for (const QByteArray &line : lines)
        handleLine(line);
}

void SyncInDataModel::handleLine(const QByteArray &line)
{
    const SyncProtocol::Packet packet = SyncProtocol::parsePacket(line);
    if (!packet.valid)
        return;

    touchRx();
    if (packet.isHeartbeat) {
        replyHeartbeat();
        return;
    }

    if (packet.portCount >= 0)
        applyPortCount(static_cast<unsigned int>(packet.portCount));

    if (!packet.hasIndex || packet.index < 0)
        return;
    if (static_cast<unsigned int>(packet.index) >= OutPortCount)
        return;

    const PortIndex idx = static_cast<PortIndex>(packet.index);
    _cache[idx] = SyncProtocol::decodeValue(packet.value);
    Q_EMIT dataUpdated(idx);
}

void SyncInDataModel::applyPortCount(unsigned int newCount)
{
    const unsigned int oldCount = OutPortCount;
    if (newCount == oldCount)
        return;

    if (newCount > oldCount) {
        Q_EMIT portsAboutToBeInserted(PortType::Out, oldCount, newCount - 1);
        OutPortCount = newCount;
        Q_EMIT portsInserted();
    } else {
        Q_EMIT portsAboutToBeDeleted(PortType::Out, newCount, oldCount - 1);
        OutPortCount = newCount;
        Q_EMIT portsDeleted();
        for (auto it = _cache.begin(); it != _cache.end();) {
            if (it->first >= newCount)
                it = _cache.erase(it);
            else
                ++it;
        }
    }
    Q_EMIT embeddedWidgetSizeUpdated();
}

QJsonObject SyncInDataModel::save() const
{
    QJsonObject values;
    values.insert(QStringLiteral("host"), _host);
    values.insert(QStringLiteral("port"), _port);
    values.insert(QStringLiteral("active"), _active);
    QJsonObject modelJson = NodeDelegateModel::save();
    modelJson.insert(QStringLiteral("values"), values);
    return modelJson;
}

void SyncInDataModel::load(QJsonObject const &p)
{
    const QJsonObject values = p.value(QStringLiteral("values")).toObject();
    if (!values.isEmpty()) {
        setHost(values.value(QStringLiteral("host")).toString(_host));
        // 兼容旧字段 listenPort
        const int port = values.contains(QStringLiteral("port"))
                             ? values.value(QStringLiteral("port")).toInt(_port)
                             : values.value(QStringLiteral("listenPort")).toInt(_port);
        setPort(port);
        setActive(values.value(QStringLiteral("active")).toBool(_active));
    }
}

} // namespace Nodes
