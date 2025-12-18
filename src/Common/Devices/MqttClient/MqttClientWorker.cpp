#include "MqttClientWorker.h"
#include <QDebug>

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
    
    connect(m_client, &QMQTT::Client::connected, this, &MqttClientWorker::onConnected);
    connect(m_client, &QMQTT::Client::disconnected, this, &MqttClientWorker::onDisconnected);
    connect(m_client, &QMQTT::Client::received, this, &MqttClientWorker::onReceived);
    connect(m_client, &QMQTT::Client::error, this, &MqttClientWorker::onError);
}

void MqttClientWorker::connectToHost(const QString &host, int port, const QString &username, const QString &password)
{
    if (!m_client) {
        qWarning() << "MqttClientWorker: Client not initialized!";
        return;
    }

    if (m_timer->isActive()) {
        m_timer->stop();
    }

    if (m_client->isConnectedToHost()) {
        m_client->disconnectFromHost();
    }

    m_host = host;
    m_port = port;
    m_username = username;
    m_password = password;

    m_client->setHostName(m_host);
    m_client->setPort(m_port);
    if (!m_username.isEmpty()) {
        m_client->setUsername(m_username);
        m_client->setPassword(m_password.toUtf8());
    }

    m_client->connectToHost();
}

void MqttClientWorker::disconnectFromHost()
{
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
    if (m_client && m_client->isConnectedToHost()) {
        m_client->subscribe(topic, qos);
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
    m_isConnected = true;
    m_timer->stop();
    emit isConnectedChanged(true);
}

void MqttClientWorker::onDisconnected()
{
    m_isConnected = false;
    emit isConnectedChanged(false);
    
    // Auto reconnect
    if (!m_timer->isActive()) {
        m_timer->start();
    }
}

void MqttClientWorker::onReceived(const QMQTT::Message &message)
{
    emit messageReceived(message.topic(), message.payload());
}

void MqttClientWorker::onError(QMQTT::ClientError error)
{
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
    
    // Trigger reconnect on error
    if (!m_isConnected && !m_timer->isActive()) {
        m_timer->start();
    }
}

void MqttClientWorker::reConnect()
{
    if (m_client) {
        // qDebug() << "MqttClientWorker: Reconnecting to" << m_host << ":" << m_port;
        m_client->connectToHost();
    }
}
