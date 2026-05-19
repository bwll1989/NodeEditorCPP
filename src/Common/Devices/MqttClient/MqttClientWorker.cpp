#include "MqttClientWorker.h"
#include <QDebug>
#include <QUuid>
#include <QDateTime>
#include <QtGlobal>

MqttClientWorker::MqttClientWorker(QObject *parent) : QObject(parent)
{
    m_timer = new QTimer(this);
    m_timer->setInterval(2000); // 2 seconds reconnect interval
    connect(m_timer, &QTimer::timeout, this, &MqttClientWorker::reConnect);
}

MqttClientWorker::~MqttClientWorker()
{
    if (m_timer) {
        m_timer->stop();
    }
    if (m_client) {
        m_client->disconnectFromHost();
        m_client->deleteLater();
    }
}

void MqttClientWorker::initialize()
{
    m_client = new QMQTT::Client(QHostAddress::LocalHost, 1883, this);
    // 使用 Worker 自身的重连逻辑，避免与 QMQTT 内置 autoReconnect 重复调用 connectToHost
    m_client->setAutoReconnect(false);
    // 每个实例使用唯一 ClientId，避免多节点/外部控制同时连接同一 Broker 时互相踢线
    m_clientId = QStringLiteral("Flow_%1")
                     .arg(QUuid::createUuid().toString(QUuid::WithoutBraces));
    m_client->setClientId(m_clientId);
    m_client->setCleanSession(true);
    m_client->setKeepAlive(60);

    connect(m_client, &QMQTT::Client::connected, this, &MqttClientWorker::onConnected);
    connect(m_client, &QMQTT::Client::disconnected, this, &MqttClientWorker::onDisconnected);
    connect(m_client, &QMQTT::Client::received, this, &MqttClientWorker::onReceived);
    connect(m_client, &QMQTT::Client::error, this, &MqttClientWorker::onError);
}

bool MqttClientWorker::shouldAttemptConnect() const
{
    if (!m_client || m_host.isEmpty()) {
        return false;
    }
    if (m_connectInProgress) {
        return false;
    }
    if (m_client->isConnectedToHost()) {
        return false;
    }
    if (m_client->connectionState() == QMQTT::STATE_CONNECTING) {
        return false;
    }
    return true;
}

QString MqttClientWorker::normalizeHost(const QString& host) const
{
    QString normalizedHost = host.trimmed();
    static const QStringList prefixes = {
        QStringLiteral("mqtts://"), QStringLiteral("mqtt://"),
        QStringLiteral("ssl://"), QStringLiteral("tcp://"),
        QStringLiteral("ws://"), QStringLiteral("wss://"),
    };
    for (const QString& prefix : prefixes) {
        if (normalizedHost.startsWith(prefix, Qt::CaseInsensitive)) {
            normalizedHost = normalizedHost.mid(prefix.size());
            break;
        }
    }
    const int slash = normalizedHost.indexOf(QLatin1Char('/'));
    if (slash > 0) {
        normalizedHost = normalizedHost.left(slash);
    }
    const int colon = normalizedHost.lastIndexOf(QLatin1Char(':'));
    if (colon > 0) {
        bool ok = false;
        normalizedHost.mid(colon + 1).toUShort(&ok);
        if (ok) {
            normalizedHost = normalizedHost.left(colon);
        }
    }
    return normalizedHost.trimmed();
}

bool MqttClientWorker::isSameConnectionTarget(const QString& host, int port,
                                              const QString& username,
                                              const QString& password) const
{
    return normalizeHost(host) == m_host
        && port == m_port
        && username.trimmed() == m_username
        && password == m_password;
}

void MqttClientWorker::doConnect()
{
    if (!m_client || !shouldAttemptConnect()) {
        return;
    }

    m_connectInProgress = true;
    if (!m_clientId.isEmpty()) {
        m_client->setClientId(m_clientId);
    }
    m_client->setHostName(m_host);
    m_client->setPort(static_cast<quint16>(m_port));
    if (!m_username.isEmpty()) {
        m_client->setUsername(m_username);
        m_client->setPassword(m_password.toUtf8());
    } else {
        m_client->setUsername(QString());
        m_client->setPassword(QByteArray());
    }
    m_client->connectToHost();
}

void MqttClientWorker::connectToHost(const QString &host, int port, const QString &username, const QString &password)
{
    if (!m_client) {
        qWarning() << "MqttClientWorker: Client not initialized!";
        return;
    }

    m_autoReconnect = true;
    if (m_timer->isActive()) {
        m_timer->stop();
    }

    const QString newHost = normalizeHost(host);
    const QString newUser = username.trimmed();

    if (newHost.isEmpty()) {
        qWarning() << "MqttClientWorker: MQTT host is empty after normalization, original:" << host;
        return;
    }

    const bool sameTarget = isSameConnectionTarget(host, port, username, password);
    if (sameTarget
        && (m_isConnected
            || m_client->isConnectedToHost()
            || m_connectInProgress
            || m_client->connectionState() == QMQTT::STATE_CONNECTING)) {
        return;
    }

    m_host = newHost;
    m_port = port;
    m_username = newUser;
    m_password = password;

    if (m_client->isConnectedToHost()) {
        qInfo() << "MqttClientWorker: reconnecting to" << m_host << ":" << m_port;
        m_pendingConnect = true;
        m_client->disconnectFromHost();
        return;
    }

    if (m_client->connectionState() == QMQTT::STATE_CONNECTING) {
        m_pendingConnect = true;
        return;
    }

    qInfo() << "MqttClientWorker: connecting to" << m_host << ":" << m_port;
    doConnect();
}

void MqttClientWorker::disconnectFromHost()
{
    m_autoReconnect = false;
    m_pendingConnect = false;
    m_connectInProgress = false;
    if (m_timer->isActive()) {
        m_timer->stop();
    }
    if (m_client) {
        m_client->disconnectFromHost();
    }
}

void MqttClientWorker::publish(const QString &topic, const QString &message, int qos, bool retain)
{
    if (m_client && m_client->isConnectedToHost()) {
        QMQTT::Message msg(0, topic, message.toUtf8(), qos, retain, false);
        m_client->publish(msg);
    }
}

void MqttClientWorker::subscribe(const QString &topic, int qos)
{
    const QString trimmedTopic = topic.trimmed();
    if (trimmedTopic.isEmpty()) {
        return;
    }
    if (m_client && m_client->isConnectedToHost()) {
        m_client->subscribe(trimmedTopic, qos);
    }
}

void MqttClientWorker::unsubscribe(const QString &topic)
{
    if (m_client && m_client->isConnectedToHost()) {
        m_client->unsubscribe(topic);
    }
}

void MqttClientWorker::stopTimer()
{
    if (m_timer) {
        m_timer->stop();
    }
}

void MqttClientWorker::onConnected()
{
    if (m_isConnected) {
        return;
    }
    m_connectInProgress = false;
    m_pendingConnect = false;
    m_isConnected = true;
    m_lastConnectedAt = QDateTime::currentMSecsSinceEpoch();
    m_flapCount = 0;
    m_timer->setInterval(2000);
    m_timer->stop();
    qInfo() << "MqttClientWorker: connected to" << m_host << ":" << m_port
            << "clientId:" << m_clientId;
    emit isConnectedChanged(true);
}

void MqttClientWorker::onDisconnected()
{
    const bool wasConnected = m_isConnected;
    m_connectInProgress = false;
    m_isConnected = false;

    if (wasConnected) {
        emit isConnectedChanged(false);
    }

    if (m_pendingConnect) {
        m_pendingConnect = false;
        doConnect();
        return;
    }

    if (m_autoReconnect && !m_timer->isActive()) {
        const qint64 now = QDateTime::currentMSecsSinceEpoch();
        const qint64 sessionMs = (m_lastConnectedAt > 0) ? (now - m_lastConnectedAt) : 0;
        if (wasConnected && sessionMs > 0 && sessionMs < 5000) {
            m_timer->setInterval(qMin(30000, 2000 * (1 << qMin(m_flapCount++, 4))));
        } else {
            m_timer->setInterval(2000);
            m_flapCount = 0;
        }
        m_timer->start();
    }
}

void MqttClientWorker::onReceived(const QMQTT::Message &message)
{
    emit messageReceived(message.topic(), message.payload());
}

void MqttClientWorker::onError(QMQTT::ClientError error)
{
    m_connectInProgress = false;

    QString errorStr;
    switch (error) {
    case QMQTT::UnknownError: errorStr = "UnknownError"; break;
    case QMQTT::SocketConnectionRefusedError: errorStr = "SocketConnectionRefusedError"; break;
    case QMQTT::SocketRemoteHostClosedError: errorStr = "SocketRemoteHostClosedError"; break;
    case QMQTT::SocketHostNotFoundError: errorStr = "SocketHostNotFoundError"; break;
    case QMQTT::SocketAccessError: errorStr = "SocketAccessError"; break;
    case QMQTT::SocketResourceError: errorStr = "SocketResourceError"; break;
    case QMQTT::SocketTimeoutError: errorStr = "SocketTimeoutError"; break;
    case QMQTT::SocketDatagramTooLargeError: errorStr = "SocketDatagramTooLargeError"; break;
    case QMQTT::SocketNetworkError: errorStr = "SocketNetworkError"; break;
    case QMQTT::SocketAddressInUseError: errorStr = "SocketAddressInUseError"; break;
    case QMQTT::SocketAddressNotAvailableError: errorStr = "SocketAddressNotAvailableError"; break;
    case QMQTT::SocketUnsupportedSocketOperationError: errorStr = "SocketUnsupportedSocketOperationError"; break;
    case QMQTT::SocketUnfinishedSocketOperationError: errorStr = "SocketUnfinishedSocketOperationError"; break;
    case QMQTT::SocketProxyAuthenticationRequiredError: errorStr = "SocketProxyAuthenticationRequiredError"; break;
    case QMQTT::SocketSslHandshakeFailedError: errorStr = "SocketSslHandshakeFailedError"; break;
    case QMQTT::SocketProxyConnectionRefusedError: errorStr = "SocketProxyConnectionRefusedError"; break;
    case QMQTT::SocketProxyConnectionClosedError: errorStr = "SocketProxyConnectionClosedError"; break;
    case QMQTT::SocketProxyConnectionTimeoutError: errorStr = "SocketProxyConnectionTimeoutError"; break;
    case QMQTT::SocketProxyNotFoundError: errorStr = "SocketProxyNotFoundError"; break;
    case QMQTT::SocketProxyProtocolError: errorStr = "SocketProxyProtocolError"; break;
    case QMQTT::MqttUnacceptableProtocolVersionError: errorStr = "MqttUnacceptableProtocolVersionError"; break;
    case QMQTT::MqttIdentifierRejectedError: errorStr = "MqttIdentifierRejectedError"; break;
    case QMQTT::MqttServerUnavailableError: errorStr = "MqttServerUnavailableError"; break;
    case QMQTT::MqttBadUserNameOrPasswordError: errorStr = "MqttBadUserNameOrPasswordError"; break;
    case QMQTT::MqttNotAuthorizedError: errorStr = "MqttNotAuthorizedError"; break;
    case QMQTT::MqttNoPingResponse: errorStr = "MqttNoPingResponse"; break;
    default: errorStr = QString("Error code: %1").arg(error); break;
    }

    emit errorOccurred(errorStr);

    if (m_autoReconnect && !m_isConnected && !m_timer->isActive()) {
        m_timer->start();
    }
}

void MqttClientWorker::reConnect()
{
    doConnect();
}
