//
// Created by bwll1 on 2024/10/10.
//

#include "TcpClient.h"
TcpClient::TcpClient(QString dstHost,int dstPort,QObject *parent ): QObject(parent), tcpClient(new QTcpSocket(this))
    {
        host=dstHost;
        port=dstPort;
        connect(tcpClient, &QTcpSocket::readyRead, this, &TcpClient::onReadyRead);
        connect(tcpClient, &QTcpSocket::connected, this, &TcpClient::onConnected);
        connect(tcpClient, &QTcpSocket::disconnected, this, &TcpClient::onDisconnected);
        connect(tcpClient, qOverload<QAbstractSocket::SocketError>(&QTcpSocket::errorOccurred), this, &TcpClient::onErrorOccurred);
        connectToServer(host,port);
        m_timer->setInterval(5000); // 每隔5秒重连一次
        connect(m_timer, &QTimer::timeout, this, &TcpClient::reConnect);
    }

TcpClient::~TcpClient()
    {

        tcpClient->close();
//
        this->deleteLater();


    }


void TcpClient::connectToServer(const QString &dstHost,int dstPort)
    {

//        tcpClient->close();
        host=dstHost;
        port=dstPort;
        tcpClient->close();
        tcpClient->connectToHost(host, port);

        if(tcpClient->waitForConnected(500))
        {
            if(m_timer->isActive()){
                m_timer->stop();
            }
            isConnected = true;
        }
        else{
            m_timer->start();
            isConnected = false;
        }
        emit isReady(isConnected);
    }

void TcpClient::sendMessage(const QString &message)
    {
        if(tcpClient->state() == QAbstractSocket::ConnectedState)
        {
            tcpClient->write(message.toUtf8());
        }
    }

void TcpClient::onReadyRead()
    {
        QByteArray data = tcpClient->readAll();
        emit recMsg(QString(data));
    }

void TcpClient::onConnected()
    {
        isConnected=true;
        m_timer->stop();
        emit isReady(isConnected);
    }

void TcpClient::onDisconnected()
    {
        isConnected=false;
        emit isReady(isConnected);
        m_timer->start();
//        reConnect();

    }
void TcpClient::onErrorOccurred(QAbstractSocket::SocketError socketError)
    {
        qWarning() << "Socket error:" << socketError;
        isConnected=false;
        emit isReady(isConnected);
    }
void TcpClient::reConnect()
    {
        qDebug()<<"reconnect";
        connectToServer(host,port);
    }
