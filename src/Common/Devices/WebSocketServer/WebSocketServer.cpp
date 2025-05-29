#include "WebSocketServer.h"
#include <QDebug>

WebSocketServer::WebSocketServer(QObject *parent)
    : QObject(parent)
    , m_server(new QWebSocketServer("WebSocketServer", QWebSocketServer::NonSecureMode, this))
{
    connect(m_server, &QWebSocketServer::newConnection, this, &WebSocketServer::onNewConnection);
}

WebSocketServer::~WebSocketServer()
{
    stop();
}

void WebSocketServer::start(quint16 port)
{
    if (m_server->listen(QHostAddress::Any, port)) {
        qDebug() << "WebSocketServer started on port" << port;
    } else {
        qDebug() << "Failed to start WebSocketServer on port" << port;
    }
}

void WebSocketServer::stop()
{
    m_server->close();
    qDebug() << "WebSocketServer stopped";
}

void WebSocketServer::onNewConnection()
{
    QWebSocket *socket = m_server->nextPendingConnection();
    if (!socket) return;
    qDebug() << "New connection from" << socket->peerAddress().toString() << ":" << socket->peerPort();
    QMutexLocker locker(&m_mutex);
    QThread *thread = new QThread;
    m_clientThreads.insert(socket, thread);
    socket->moveToThread(thread);
    thread->start();

    connect(socket, &QWebSocket::binaryMessageReceived, this, &WebSocketServer::onMessageReceived);
    connect(socket, &QWebSocket::disconnected, this, &WebSocketServer::onSocketDisconnected);

    emit newConnection(socket);
}

void WebSocketServer::onMessageReceived(const QByteArray &message)
{
    qDebug() << "Received message:" << message;
    QWebSocket *socket = qobject_cast<QWebSocket*>(sender());
    if (socket) {

        emit messageReceived(socket, message);
    }
}

void WebSocketServer::onSocketDisconnected()
{
    QWebSocket *socket = qobject_cast<QWebSocket*>(sender());
    if (socket) {
        QMutexLocker locker(&m_mutex);
        QThread *thread = m_clientThreads.take(socket);
        if (thread) {
            thread->quit();
            thread->wait();
            delete thread;
        }
        socket->deleteLater();
        emit connectionClosed(socket);
    }
}