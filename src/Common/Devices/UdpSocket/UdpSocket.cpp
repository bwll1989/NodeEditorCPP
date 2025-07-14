//
// Created by bwll1 on 2024/10/10.
//

#include "UdpSocket.h"

#include <QVariant>

UdpSocket::UdpSocket(QString dstHost, int dstPort, QObject *parent): QObject(parent), mPort(dstPort), mHost(dstHost), mSocket(nullptr)
{
    // 启动线程
//    qRegisterMetaType<QVariantMap >("QVariantMap&");
    //注册信号传递数值类型
    mThread = new QThread(this);
    this->moveToThread(mThread);
    connect(mThread, &QThread::started, this, &UdpSocket::initializeSocket);
    connect(mThread, &QThread::finished, this, &UdpSocket::cleanup);

    mThread->start();
}

UdpSocket::~UdpSocket()
{
    mThread->quit();
    mThread->wait();
}
void UdpSocket::initializeSocket() {
    mSocket = new QUdpSocket(this);
    connect(mSocket, &QUdpSocket::readyRead, this, &UdpSocket::processPendingDatagrams);
    if (mSocket->bind(QHostAddress::AnyIPv4, mPort,QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint)) {
    }
}
void UdpSocket::cleanup() {
    if (mSocket) {
        mSocket->deleteLater();
        mSocket = nullptr;
    }
}
void UdpSocket::setHost(QString address,int port) {
    mHost=address;
    if (mSocket) {
        mSocket->close();
    }
    mPort = port;
    if (mSocket) {
        mSocket->bind(QHostAddress::AnyIPv4, mPort,QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint);

    }
}
void UdpSocket::processPendingDatagrams()
{
    while (mSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(mSocket->pendingDatagramSize());

        QHostAddress sender;
        quint16 senderPort;
        mSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        QVariantMap dataMap;
        dataMap.insert("host", sender.toString());
        dataMap.insert("hex", QString(datagram.toHex())); // 转换为QString类型
        dataMap.insert("utf-8", QString::fromUtf8(datagram)); // 修复UTF-8解码方式
        dataMap.insert("ansi", QString::fromLocal8Bit(datagram)); // 修复ANSI解码方式
        dataMap.insert("default", datagram);
        emit recMsg(dataMap);
        emit arrayMsg(datagram);

//        emit recMsg(QString::fromUtf8(datagram));
        // 发送回显数据到客户端
//            QByteArray response = "Echo: " + datagram;
//            this->writeDatagram(response, sender, senderPort);
    }
}



void UdpSocket::sendMessage(const QString &host,const int &port,const QString &message,const int &format)
{
    QByteArray data;
    switch (format) {
    case 0: // HEX格式
        data = QByteArray::fromHex(message.toUtf8());
        break;
    case 1: // UTF-8格式
        data = message.toUtf8();  // 修复冒号错误为括号
        break;
    case 2: // 新增ANSI格式
        data = message.toLocal8Bit(); // 使用本地编码（ANSI）
        break;
    default:
        data = message.toUtf8();  // 默认使用UTF-8
        break;
    }
    mSocket->writeDatagram(data,QHostAddress(host),port);
}

