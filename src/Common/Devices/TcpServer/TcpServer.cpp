//
// Created by bwll1 on 2024/10/10.
//

#include "TcpServer.h"
#include "QThread"
TcpServer::TcpServer(QString dstHost, int dstPort, QObject *parent)
        : QObject(parent), mPort(dstPort), mHost(dstHost), mServer(nullptr)
{
    mThread = new QThread(this);
    this->moveToThread(mThread);
    connect(mThread, &QThread::started, this, &TcpServer::initializeServer);
    connect(mThread, &QThread::finished, this, &TcpServer::cleanup);

    mThread->start();
}

TcpServer::~TcpServer()
{
    cleanup();
}

void TcpServer::initializeServer() {
    mServer = new QTcpServer(this);
    connect(mServer, &QTcpServer::newConnection, this, &TcpServer::onNewConnection);

    if (mServer->listen(QHostAddress(mHost), mPort)) {
        qDebug() << "Server listening on port" << mPort;
    } else {
        qWarning() << "Failed to start server";
    }
}

void TcpServer::cleanup()
{
    // 关闭所有客户端连接
    for (QTcpSocket *clientSocket : mClientSockets) {
        if (clientSocket->isOpen()) {
            clientSocket->close();
        }
        clientSocket->deleteLater();
    }
    mClientSockets.clear();
    // 关闭服务器并删除
    if (mServer) {
        mServer->destroyed();
        mServer->deleteLater();
        mServer = nullptr;
    }
    mThread->quit();
}

void TcpServer::onNewConnection()
{
    QTcpSocket *clientSocket = mServer->nextPendingConnection();
    qDebug()<<"new";
    connect(clientSocket, &QTcpSocket::readyRead, this, &TcpServer::onReadyRead);
    connect(clientSocket, &QTcpSocket::disconnected, this, &TcpServer::onDisconnected);
    mClientSockets.append(clientSocket); // Store the client socket
}

void TcpServer::onReadyRead()
{
    for (QTcpSocket *clientSocket : mClientSockets) {
        if (clientSocket->bytesAvailable() > 0) {
            QByteArray data = clientSocket->readAll();
            std::shared_ptr<QVariantMap> a=std::make_shared<QVariantMap>();

            a->insert("Host", clientSocket->peerAddress().toString());
            a->insert("Default", data);
            emit arrayMsg(data);
            emit recMsg(*a);
            // 可以在此处理数据或回显
        }
    }
}

void TcpServer::onDisconnected()
{
    QTcpSocket *clientSocket = qobject_cast<QTcpSocket*>(sender());
    if (clientSocket) {
        mClientSockets.removeAll(clientSocket);
        clientSocket->deleteLater();
    }
}

void TcpServer::sendMessage(const QString &message)
{
    for (QTcpSocket *clientSocket : mClientSockets) {
        clientSocket->write(message.toUtf8());
    }
}

void TcpServer::setHost(QString address, int port) {
    qDebug()<<"12";
//    cleanup();
    // 关闭所有客户端连接
    for (QTcpSocket *clientSocket : mClientSockets) {
        if (clientSocket->isOpen()) {
            clientSocket->close();
        }
        clientSocket->deleteLater();
    }
    mClientSockets.clear();
    // 关闭服务器并删除
    if (mServer) {
        mServer->destroyed();
        mServer->deleteLater();
        mServer = nullptr;
    }
    mPort=port;
    mHost=address;
    mServer = new QTcpServer(this);
    connect(mServer, &QTcpServer::newConnection, this, &TcpServer::onNewConnection);

    if (mServer->listen(QHostAddress(mHost), mPort)) {
        qDebug() << "Server listening on port" << mPort;
    } else {
        qWarning() << "Failed to start server";
    }
    mThread->start();

}