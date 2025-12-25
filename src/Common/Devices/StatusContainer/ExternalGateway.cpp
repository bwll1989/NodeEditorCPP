#include "ExternalGateway.h"
#include <QJsonParseError>
#include <QCoreApplication>

ExternalGateway* ExternalGateway::s_inst = nullptr;

ExternalGateway* ExternalGateway::instance() {
    if (!s_inst) {
        s_inst = new ExternalGateway();
        if (QCoreApplication::instance())
            s_inst->moveToThread(QCoreApplication::instance()->thread());
    }
    return s_inst;
}

ExternalGateway::ExternalGateway(QObject* parent)
    : QObject(parent)
    , _wsServer(nullptr)
{
    connect(StatusContainer::instance(), &StatusContainer::statusAdded,
            this, &ExternalGateway::onStatusAdded, Qt::QueuedConnection);
}

void ExternalGateway::start(quint16 port) {
    if (_wsServer) return;
    _wsServer = new WebSocketServer();
    connect(_wsServer, &WebSocketServer::newConnection,
            this, &ExternalGateway::onNewConnection, Qt::QueuedConnection);
    connect(_wsServer, &WebSocketServer::messageReceived,
            this, &ExternalGateway::onWebSocketMessage, Qt::QueuedConnection);
    _wsServer->start(port);
}

void ExternalGateway::stop() {
    if (!_wsServer) return;
    _wsServer->stop();
    _wsServer->deleteLater();
    _wsServer = nullptr;
}

void ExternalGateway::broadcastSnapshot(const QString& prefix) {
    auto all = StatusContainer::instance()->queryByPrefix(prefix);
    for (const auto& m : all) broadcastStatus(m);
}

void ExternalGateway::onStatusAdded(const OSCMessage& message) {
    broadcastStatus(message);
}

void ExternalGateway::onNewConnection(QWebSocket* /*socket*/) {
    broadcastSnapshot("/dataflow/");
}

void ExternalGateway::onWebSocketMessage(QWebSocket* socket, const QByteArray& payload) {
    Q_UNUSED(socket);
    handleWebSocketMessage(socket, payload);
}

void ExternalGateway::handleWebSocketMessage(QWebSocket* /*socket*/, const QByteArray& payload) {
    QJsonParseError err;
    const auto doc = QJsonDocument::fromJson(payload, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject()) return;
    const auto obj = doc.object();
    const auto address = obj.value(QStringLiteral("address")).toString();
    const auto value = obj.value(QStringLiteral("value")).toVariant();
    if (address.isEmpty()) return;

    OSCMessage msg;
    msg.address = address;
    msg.value = value;
    ExternalControler::instance()->parseOSC(msg);
}

void ExternalGateway::broadcastStatus(const OSCMessage& message) {
    if (!_wsServer) return;
    QJsonObject o;
    o.insert("address", message.address);
    o.insert("type", message.type);
    o.insert("value", QJsonValue::fromVariant(message.value));
    const auto text = QJsonDocument(o).toJson(QJsonDocument::Compact);
    _wsServer->broadcastMessage(text, /*messageType=text*/ 1);
}