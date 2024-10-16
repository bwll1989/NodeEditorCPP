//
// Created by bwll1 on 2024/10/10.
//

#include "TcpServer.h"
TcpServer::TcpServer(int port, QObject *parent) : QTcpServer(parent)
{
    listenPort=port;
    startServer();
}
TcpServer::~TcpServer()
{
    stopServer();

    this->deleteLater();

}

void TcpServer::stopServer()
{
    for (QTcpSocket *clientSocket : m_clients) {
        clientSocket->disconnectFromHost();
        emit clientRemoved(clientSocket->peerAddress().toString());
    }
    close();
    emit serverStoped();

};
void TcpServer::startServer()
//    切换启动或停止客户端
{
//        qDebug()<<"server 进程："<<QThread::currentThreadId();
    if(!isListening()){
        if (!listen(QHostAddress::Any, listenPort)) {
            qDebug() << "Error starting server:" << errorString();
            emit serverError(errorString());
        } else {
//                qDebug() << "Server started on port "<<listenPort;
        }
    } else{
        stopServer();
    }};

void TcpServer::sendMessage(const QString &client,const QString &message)
{
    QTcpSocket *clientSocket;
    for(QTcpSocket *Socket:m_clients)
        if(Socket->peerAddress().toString()==client){
            clientSocket=Socket;
            break;
        }
    clientSocket->write(message.toStdString().c_str());
    clientSocket->flush();

}
void TcpServer::setPort(const int &port)
{
    for (QTcpSocket *clientSocket : m_clients) {
        clientSocket->disconnectFromHost();
        emit clientRemoved(clientSocket->peerAddress().toString());
    }
    close();
//        先停止服务器
    listenPort=port;
//        设置端口后重新启动
    if (!listen(QHostAddress::Any, listenPort)) {
        emit serverError(errorString());
    } else {
        qInfo() << "Server restarted on port "<<listenPort;
    }
}

void TcpServer:: incomingConnection(qintptr socketDescriptor)
{
    // 创建一个新的 QTcpSocket 对象来处理新的连接
    QTcpSocket *clientSocket = new QTcpSocket(this);
    if (!clientSocket->setSocketDescriptor(socketDescriptor)) {
        qDebug() << "Error setting socket descriptor:" << clientSocket->errorString();
        delete clientSocket;
        return;
    }

    // 存储连接
    m_clients.append(clientSocket);
    emit clientInserted(clientSocket->peerAddress().toString());
    // 当客户端断开连接时，删除 QTcpSocket 对象并从列表中移除
    connect(clientSocket, &QTcpSocket::disconnected, [this, clientSocket]() {
        //            qDebug() << "Client disconnected:" << clientSocket->peerAddress().toString();
        m_clients.removeAll(clientSocket);
        emit clientRemoved(clientSocket->peerAddress().toString());
        clientSocket->deleteLater();
    });

    // 当有数据到达时，读取数据
    connect(clientSocket, &QTcpSocket::readyRead, this, [=]() {
        while (clientSocket->bytesAvailable() > 0) {
            QByteArray data = clientSocket->readAll();
            emit serverMessage(data);
            // 向客户端发送数据
            clientSocket->write("Hello from server!");
            clientSocket->flush();

        }
    });
}