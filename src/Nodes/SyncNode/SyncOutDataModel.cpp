/**
 * @file SyncOutDataModel.cpp
 */

#include "SyncOutDataModel.hpp"
#include "SyncPairHub.hpp"
#include "SyncProtocol.hpp"

#include <QDateTime>
#include <QSignalBlocker>
#include <QtNetwork/QAbstractSocket>
#include <QtNetwork/QHostAddress>

namespace Nodes {

SyncOutDataModel::SyncOutDataModel()
{
    InPortCount = 1;
    OutPortCount = 0;
    CaptionVisible = true;
    Caption = QStringLiteral("Sync Out");
    WidgetEmbeddable = false;
    Resizable = false;
    PortEditable = true;

    setupUi();

    _schemaTimer.setInterval(300);
    connect(&_schemaTimer, &QTimer::timeout, this, [this]() {
        if (_active && _listening && static_cast<int>(InPortCount) != _lastAnnouncedN) {
            announceSchema();
            SyncPairHub::instance()->syncPortCountFromOut(this);
        }
    });
    _schemaTimer.start();

    _heartbeatTimer.setInterval(SyncProtocol::HeartbeatIntervalMs);
    connect(&_heartbeatTimer, &QTimer::timeout, this, &SyncOutDataModel::onHeartbeatTick);

    _clientWatchTimer.setInterval(1000);
    connect(&_clientWatchTimer, &QTimer::timeout, this, &SyncOutDataModel::onClientWatchTick);
}

SyncOutDataModel::~SyncOutDataModel()
{
    SyncPairHub::instance()->unregisterOut(this);
    GlobalEventBus::instance()->unsubscribe(this);
    stopServer();
}

void SyncOutDataModel::setupUi()
{
    _widget = new SyncOutInterface();
    refreshStatus();

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
        b.member = QStringLiteral("push");
        b.control = _widget->pushButton;
        AbstractDelegateModel::registerExternalBinding(QStringLiteral("/push"), this, b);
    }

    connect(_widget->portSpin, &IntDragValueWidget::valueChanged, this, &SyncOutDataModel::setPort);
    connect(_widget->activeCheck, &QCheckBox::toggled, this, &SyncOutDataModel::setActive);
    connect(_widget->pushButton, &QPushButton::clicked, this, [this]() { setPush(true); });
}

void SyncOutDataModel::afterModelReady()
{
    AbstractDelegateModel::afterModelReady();
    SyncPairHub::instance()->registerOut(this);

    auto *bus = GlobalEventBus::instance();
    bus->subscribe(makeFullOscAddress(QStringLiteral("/port")), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress(QStringLiteral("/active")), this, SLOT(onGlobalEvent(GlobalEvent)));
    bus->subscribe(makeFullOscAddress(QStringLiteral("/push")), this, SLOT(onGlobalEvent(GlobalEvent)));

    if (_active)
        startServer();
}

NodeDataType SyncOutDataModel::dataType(PortType, PortIndex) const
{
    return NodeDataTypes::VariableData().type();
}

std::shared_ptr<NodeData> SyncOutDataModel::outData(PortIndex)
{
    return nullptr;
}

void SyncOutDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
{
    auto var = std::dynamic_pointer_cast<NodeDataTypes::VariableData>(data);
    if (!var) {
        _cache.erase(portIndex);
        return;
    }
    _cache[portIndex] = var;
    sendPort(portIndex, var);
}

QWidget *SyncOutDataModel::embeddedWidget()
{
    return _widget;
}

void SyncOutDataModel::setPort(int port)
{
    if (port <= 0 || port > 65535 || _port == port)
        return;
    _port = port;
    if (_widget) {
        QSignalBlocker blocker(_widget->portSpin);
        _widget->portSpin->setValue(_port);
    }
    Q_EMIT portChanged(_port);
    if (_active)
        startServer();
}

void SyncOutDataModel::setActive(bool active)
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
        startServer();
    else
        stopServer();
}

void SyncOutDataModel::refreshStatus()
{
    // 清掉已销毁的指针
    for (int i = _clients.size() - 1; i >= 0; --i) {
        if (!_clients.at(i))
            _clients.removeAt(i);
    }
    const int n = _clients.size();
    setConnected(n > 0);
    if (_widget)
        _widget->updateStatus(_listening, n);
}

void SyncOutDataModel::setConnected(bool connected)
{
    if (_connected == connected)
        return;
    _connected = connected;
    Q_EMIT connectedChanged(_connected);
}

void SyncOutDataModel::setPush(bool v)
{
    if (!v)
        return;
    if (_push) {
        pushAll();
        return;
    }
    _push = true;
    Q_EMIT pushChanged(true);
    pushAll();
    _push = false;
    Q_EMIT pushChanged(false);
}

void SyncOutDataModel::onGlobalEvent(const GlobalEvent &ev)
{
    if (ev.kind != GlobalEventKind::Command)
        return;
    const QString addr = ev.address;
    if (addr == makeFullOscAddress(QStringLiteral("/port")))
        setPort(ev.payload.toInt());
    else if (addr == makeFullOscAddress(QStringLiteral("/active")))
        setActive(ev.payload.toBool());
    else if (addr == makeFullOscAddress(QStringLiteral("/push"))) {
        if (!ev.payload.isValid() || ev.payload.toBool())
            setPush(true);
    }
}

void SyncOutDataModel::stopServer()
{
    _heartbeatTimer.stop();
    _clientWatchTimer.stop();

    for (auto const &c : _clients) {
        if (!c)
            continue;
        QObject::disconnect(c.data(), nullptr, this, nullptr);
        c->abort();
        c->deleteLater();
    }
    _clients.clear();
    _clientLastRxMs.clear();
    _clientRxBuffers.clear();

    if (_server) {
        QObject::disconnect(_server.get(), nullptr, this, nullptr);
        _server->close();
        _server.reset();
    }
    _listening = false;
    _lastAnnouncedN = -1;
    refreshStatus();
}

void SyncOutDataModel::startServer()
{
    stopServer();
    if (!_active)
        return;

    _server = std::make_unique<QTcpServer>();
    connect(_server.get(), &QTcpServer::newConnection, this, &SyncOutDataModel::onNewConnection);
    const bool ok = _server->listen(QHostAddress::AnyIPv4, static_cast<quint16>(_port));
    _listening = ok;
    if (!ok) {
        _server.reset();
        refreshStatus();
        return;
    }
    _heartbeatTimer.start();
    _clientWatchTimer.start();
    refreshStatus();
    SyncPairHub::instance()->syncPortCountFromOut(this);
}

void SyncOutDataModel::onNewConnection()
{
    if (!_server)
        return;
    while (_server->hasPendingConnections()) {
        QTcpSocket *incoming = _server->nextPendingConnection();
        if (!incoming)
            continue;
        incoming->setParent(this);
        incoming->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
        _clients.append(incoming);
        touchClient(incoming);
        connect(incoming, &QTcpSocket::disconnected, this, &SyncOutDataModel::onClientDisconnected);
        connect(incoming, &QTcpSocket::readyRead, this, &SyncOutDataModel::onClientReadyRead);
        pushToClient(incoming);
    }
    refreshStatus();
}

void SyncOutDataModel::touchClient(QTcpSocket *sock)
{
    if (!sock)
        return;
    _clientLastRxMs.insert(sock, QDateTime::currentMSecsSinceEpoch());
}

void SyncOutDataModel::dropClient(QTcpSocket *sock)
{
    if (!sock)
        return;
    QObject::disconnect(sock, nullptr, this, nullptr);
    _clientLastRxMs.remove(sock);
    _clientRxBuffers.remove(sock);
    for (int i = _clients.size() - 1; i >= 0; --i) {
        if (!_clients.at(i) || _clients.at(i).data() == sock)
            _clients.removeAt(i);
    }
    sock->abort();
    sock->deleteLater();
    refreshStatus();
}

void SyncOutDataModel::onClientDisconnected()
{
    dropClient(qobject_cast<QTcpSocket *>(sender()));
}

void SyncOutDataModel::onClientReadyRead()
{
    auto *sock = qobject_cast<QTcpSocket *>(sender());
    if (!sock)
        return;
    _clientRxBuffers[sock].append(sock->readAll());
    const QList<QByteArray> lines = SyncProtocol::takeLines(_clientRxBuffers[sock]);
    for (const QByteArray &line : lines) {
        const SyncProtocol::Packet packet = SyncProtocol::parsePacket(line);
        if (!packet.valid)
            continue;
        touchClient(sock);
        // 服务端只消费心跳；业务数据单向 Out→In
    }
}

void SyncOutDataModel::onHeartbeatTick()
{
    if (!_listening || _clients.isEmpty())
        return;
    writeLine(SyncProtocol::makeHeartbeatPacket());
}

void SyncOutDataModel::onClientWatchTick()
{
    if (!_listening)
        return;
    const qint64 now = QDateTime::currentMSecsSinceEpoch();
    QList<QTcpSocket *> dead;
    for (auto const &c : _clients) {
        QTcpSocket *sock = c.data();
        if (!sock) {
            dead.append(nullptr);
            continue;
        }
        const qint64 last = _clientLastRxMs.value(sock, 0);
        if (last > 0 && (now - last) > SyncProtocol::HeartbeatTimeoutMs)
            dead.append(sock);
    }
    for (QTcpSocket *sock : dead) {
        if (sock)
            dropClient(sock);
        else
            refreshStatus();
    }
}

void SyncOutDataModel::writeLine(const QByteArray &json)
{
    const QByteArray line = SyncProtocol::frameLine(json);
    for (int i = _clients.size() - 1; i >= 0; --i) {
        QTcpSocket *sock = _clients.at(i).data();
        if (!sock || sock->state() != QAbstractSocket::ConnectedState) {
            if (sock) {
                _clientLastRxMs.remove(sock);
                _clientRxBuffers.remove(sock);
            }
            _clients.removeAt(i);
            continue;
        }
        sock->write(line);
    }
}

void SyncOutDataModel::announceSchema()
{
    if (!_active || !_listening)
        return;
    const int n = static_cast<int>(InPortCount);
    writeLine(SyncProtocol::makeSchemaPacket(n));
    _lastAnnouncedN = n;
    SyncPairHub::instance()->syncPortCountFromOut(this);
}

void SyncOutDataModel::sendPort(PortIndex index,
                                const std::shared_ptr<NodeDataTypes::VariableData> &data)
{
    if (!_active || !_listening || !data || _clients.isEmpty())
        return;
    const int n = static_cast<int>(InPortCount);
    if (n != _lastAnnouncedN)
        announceSchema();

    writeLine(SyncProtocol::makeDataPacket(
        n, static_cast<int>(index), SyncProtocol::encodeValue(data)));
}

void SyncOutDataModel::pushToClient(QTcpSocket *sock)
{
    if (!sock || sock->state() != QAbstractSocket::ConnectedState)
        return;
    const int n = static_cast<int>(InPortCount);
    sock->write(SyncProtocol::frameLine(SyncProtocol::makeSchemaPacket(n)));
    _lastAnnouncedN = n;
    for (auto const &[idx, data] : _cache) {
        if (idx < InPortCount && data) {
            sock->write(SyncProtocol::frameLine(SyncProtocol::makeDataPacket(
                n, static_cast<int>(idx), SyncProtocol::encodeValue(data))));
        }
    }
}

void SyncOutDataModel::pushAll()
{
    if (!_active || !_listening || _clients.isEmpty())
        return;
    announceSchema();
    for (auto const &[idx, data] : _cache) {
        if (idx < InPortCount && data)
            sendPort(idx, data);
    }
}

QJsonObject SyncOutDataModel::save() const
{
    QJsonObject values;
    values.insert(QStringLiteral("port"), _port);
    values.insert(QStringLiteral("active"), _active);
    QJsonObject modelJson = NodeDelegateModel::save();
    modelJson.insert(QStringLiteral("values"), values);
    return modelJson;
}

void SyncOutDataModel::load(QJsonObject const &p)
{
    const QJsonObject values = p.value(QStringLiteral("values")).toObject();
    if (!values.isEmpty()) {
        setPort(values.value(QStringLiteral("port")).toInt(_port));
        setActive(values.value(QStringLiteral("active")).toBool(_active));
    }
}

} // namespace Nodes
