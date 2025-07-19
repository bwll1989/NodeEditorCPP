#include "WebSocketServer.h"
#include <QDebug>
#include <QThread>

WebSocketServer::WebSocketServer(QObject *parent)
    : QObject(parent)
{
    mThread = new QThread(this);
    WebSocketWorker *worker = new WebSocketWorker();
    worker->moveToThread(mThread);
    connect(mThread, &QThread::finished, worker, &QObject::deleteLater);
    connect(this, &WebSocketServer::initializeRequested, worker, &WebSocketWorker::initialize);
    connect(this, &WebSocketServer::cleanupRequested, worker, &WebSocketWorker::cleanup);
    connect(this, &WebSocketServer::broadcastRequested, worker, &WebSocketWorker::broadcast);
    connect(worker, &WebSocketWorker::newConnection, this, &WebSocketServer::newConnection);
    connect(worker, &WebSocketWorker::messageReceived, this, &WebSocketServer::messageReceived);
    connect(worker, &WebSocketWorker::connectionClosed, this, &WebSocketServer::connectionClosed);
    mThread->start();
    emit initializeRequested(mPort);
}

WebSocketServer::~WebSocketServer()
{
    emit cleanupRequested();
    mThread->quit();
    mThread->wait();
}

void WebSocketServer::initialize(quint16 port) {
    mPort = port;
    emit initializeRequested(port);
}
void WebSocketServer::cleanup() {
    emit cleanupRequested();
}
void WebSocketServer::setPort(quint16 port) {
    mPort = port;
    emit initializeRequested(port);
}
void WebSocketServer::start(quint16 port) {
    setPort(port);
}
void WebSocketServer::stop() {
    emit cleanupRequested();
}
void WebSocketServer::broadcastMessage(const QByteArray &message,const int &messageType) {
    emit broadcastRequested(message,messageType);
}


WebSocketWorker::WebSocketWorker(QObject *parent) : QObject(parent), m_server(nullptr) {}

WebSocketWorker::~WebSocketWorker() { cleanup(); }


void WebSocketWorker::initialize(quint16 port) {
    cleanup();
    m_server = new QWebSocketServer("WebSocketServer", QWebSocketServer::NonSecureMode);
    connect(m_server, &QWebSocketServer::newConnection, this, &WebSocketWorker::onNewConnection);
    if (m_server->listen(QHostAddress::Any, port)) {
        qDebug() << "WebSocketWorker listening on port" << port;
    } else {
        qWarning() << "WebSocketWorker failed to start on port" << port;
    }
}
void WebSocketWorker::cleanup() {
    for (QWebSocket *client : m_clients) {
        if (client->isValid()) client->close();
        client->deleteLater();
    }
    m_clients.clear();
    if (m_server) {
        m_server->close();
        m_server->deleteLater();
        m_server = nullptr;
    }
}
void WebSocketWorker::broadcast(const QByteArray &msg,const int &messageType) {
    if (messageType == 1) {
        for (QWebSocket *client : m_clients) {
            if (client->state() == QAbstractSocket::ConnectedState) {
                client->sendTextMessage(msg);
            }
        }
    }else
    {
        for (QWebSocket *client : m_clients) {
            if (client->state() == QAbstractSocket::ConnectedState) {
                client->sendBinaryMessage(msg);
            }
        }
    }

}

void WebSocketWorker::onNewConnection() {
    QWebSocket *client = m_server->nextPendingConnection();
    if (!client) return;
    connect(client, &QWebSocket::binaryMessageReceived, this, &WebSocketWorker::onMessageReceived);
    connect(client, &QWebSocket::textMessageReceived, this, [this, client](const QString &msg) {
        emit messageReceived(client, msg.toUtf8());
    });
    connect(client, &QWebSocket::disconnected, this, &WebSocketWorker::onDisconnected);
    m_clients.append(client);
    emit newConnection(client);
}

void WebSocketWorker::onMessageReceived(const QByteArray &msg) {
    QWebSocket *client = qobject_cast<QWebSocket*>(sender());
    if (client)
        emit messageReceived(client, msg);
}

void WebSocketWorker::onDisconnected() {
    QWebSocket *client = qobject_cast<QWebSocket*>(sender());
    if (client) {
        m_clients.removeAll(client);
        client->deleteLater();
        emit connectionClosed(client);
    }
}
