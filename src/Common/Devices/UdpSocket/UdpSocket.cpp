//
// Created by bwll1 on 2024/10/10.
//

#include "UdpSocket.h"

UdpSocket::UdpSocket(QString dstHost, int dstPort, QObject *parent): QUdpSocket(parent)
{

    host=QHostAddress(dstHost);
    port=dstPort;
    startSocket(dstHost,port);
    connect(this,&QUdpSocket::readyRead, this, &UdpSocket::processPendingDatagrams);
}

UdpSocket::~UdpSocket()
{
    close();
    deleteLater();
}

void UdpSocket::processPendingDatagrams()
{
    while (this->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(this->pendingDatagramSize());

        QHostAddress sender;
        quint16 senderPort;
        this->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        emit arrayMsg(datagram);
        emit recMsg(QString::fromUtf8(datagram));
        // 发送回显数据到客户端
//            QByteArray response = "Echo: " + datagram;
//            this->writeDatagram(response, sender, senderPort);
    }
}

void UdpSocket::startSocket(const QString &dstHost,int dstPort)
{
    close();
    host=QHostAddress(dstHost);
    port=dstPort;
    if(this->bind(host, dstPort))
    {

        emit isReady(true);
        qDebug()<<"UDP Socket is ready";
        emit recMsg("UDP Socket is ready!");
    }else
    {
        emit isReady(false);
        qDebug()<<"Error! please check port or host!"<<dstHost<<dstPort;
        emit recMsg("Error! please check port or host!");

    }

}

void UdpSocket::startListen(){
    close();

    if(this->bind(host, port))
    {

        emit isReady(true);
        qDebug()<<"UDP Socket is ready";
        emit recMsg("UDP Socket is ready!");
    }else
    {
        emit isReady(false);
        qDebug()<<"Error! please check port or host!"<<host<<port;
        emit recMsg("Error! please check port or host!");

    }

}

void UdpSocket::sendMessage(const QString &message)
{
    this->writeDatagram(message.toUtf8(),host,port);
}

