#include "SocketTransmitter.h"
#include <QGlobalStatic>


// 声明全局静态实例
SocketTransmitter* SocketTransmitter::socketInstance = nullptr;

DLL_EXPORT SocketTransmitter* getClientControlInstance()
{
    return SocketTransmitter::getInstance();
}

SocketTransmitter::SocketTransmitter(QObject* parent)
    : QObject(parent), m_server(nullptr)
{
    // 在构造函数中自动启动服务器
    if (!startServer(34455)) {
        qWarning() << "Failed to start WebSocket server on port 34455";
    }
}

SocketTransmitter* SocketTransmitter::getInstance()
{
    if (socketInstance == nullptr) {
        socketInstance = new SocketTransmitter();
    }
    return socketInstance;
}

bool SocketTransmitter::startServer(quint16 port)
{
    if (m_server) {
        return false;
    }

    m_server = new QWebSocketServer("SocketTransmitter", QWebSocketServer::NonSecureMode, this);
    if (!m_server->listen(QHostAddress::Any, port)) {
        delete m_server;
        m_server = nullptr;
        return false;
    }

    connect(m_server, &QWebSocketServer::newConnection, this, &SocketTransmitter::onNewConnection);
    return true;
}

void SocketTransmitter::stopServer()
{
    if (m_server) {
        m_server->close();
        delete m_server;
        m_server = nullptr;
    }
}

void SocketTransmitter::enqueueJson(const QJsonDocument& json)
{
    QMutexLocker locker(&m_mutex);
    m_sendQueue.enqueue(json);
    QMetaObject::invokeMethod(this, "processQueue", Qt::QueuedConnection);
}

void SocketTransmitter::processQueue()
{
    QMutexLocker locker(&m_mutex);
    while (!m_sendQueue.isEmpty()) {
        QJsonDocument json = m_sendQueue.dequeue();
        for (QWebSocket* client : m_clients) {
            if (client && client->isValid()) {
                client->sendTextMessage(json.toJson(QJsonDocument::Compact));
            }
        }
    }
}

void SocketTransmitter::onNewConnection()
{
    QWebSocket* socket = m_server->nextPendingConnection();
    connect(socket, &QWebSocket::disconnected, this, &SocketTransmitter::onClientDisconnected);
    m_clients.append(socket);
    qDebug() << "New client connected";
    emit newConnection(socket);
}

void SocketTransmitter::onClientDisconnected()
{
    QWebSocket* socket = qobject_cast<QWebSocket*>(sender());
    qDebug() << "Client disconnected";
    if (socket) {
        m_clients.removeAll(socket);
        socket->deleteLater();
    }
}