//
// Created by bwll1 on 2024/9/1.
//

#include "ArtnetReceiver.h"
#include <QJsonObject>
#include <QByteArray>
#include <QHostAddress>
#include "tinyosc.h"
#include <QThread>
#include <QtEndian>
ArtnetReceiver::ArtnetReceiver(QObject *parent)
        : QObject(parent), mUniverse(-1), mSocket(nullptr) ,result(QVariantMap()){
    // 启动线程
    qRegisterMetaType<QVariantMap >("QVariantMap&");
    //注册信号传递数值类型
    mThread = new QThread(this);
    this->moveToThread(mThread);
    connect(mThread, &QThread::started, this, &ArtnetReceiver::initializeSocket);
    connect(mThread, &QThread::finished, this, &ArtnetReceiver::cleanup);

    mThread->start();
}

ArtnetReceiver::~ArtnetReceiver() {
    mThread->quit();
    mThread->wait();
}

void ArtnetReceiver::initializeSocket() {
    mSocket = new QUdpSocket(this);

    if (mSocket->bind(QHostAddress::AnyIPv4, 6454,QUdpSocket::ShareAddress | QUdpSocket::ReuseAddressHint)) {
        connect(mSocket, &QUdpSocket::readyRead, this, &ArtnetReceiver::processPendingDatagrams);
    }
}

void ArtnetReceiver::cleanup() {
    if (mSocket) {
        mSocket->close();
        mSocket->deleteLater();
        mSocket = nullptr;
    }
}

void ArtnetReceiver::processPendingDatagrams() {
    while (mSocket && mSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(mSocket->pendingDatagramSize());
        QHostAddress sender;
        quint16 senderPort;
        mSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);
        // 解析 Art-Net 数据包
        if (datagram.size() < 18) {
//            qWarning() << "Invalid Art-Net packet";
            continue;
        }

        // 检查标识符 "Art-Net"
        const char *header = datagram.constData();
        if (strncmp(header, "Art-Net", 7) != 0) {
//            qWarning() << "Not an Art-Net packet";
            continue;
        }
        // 提取操作码
        quint16 opCode = qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(header + 8));
        result.clear();

        if (opCode == 0x5000) { // OpDmx
            result["universe"] = qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(header + 14));
//            result["sequence"] = static_cast<quint8>(header[12]);
//            result["physical"] = static_cast<quint8>(header[13]);
            result["Host"]=sender.toString();
            quint16 dmxDataLength = qFromLittleEndian<quint16>(reinterpret_cast<const uchar *>(header + 16));
            if (datagram.size() < 18 + dmxDataLength) {
//                qWarning() << "Invalid DMX data length";
                continue;
            }
            QByteArray dmxData = datagram.mid(18, dmxDataLength);
            result["data"] = dmxData; // 包含 DMX 数据
            if(mUniverse==-1||mUniverse==result["universe"].toInt())
                emit receiveOSC(result);

        }
    }
}

void ArtnetReceiver::universeFilter(const int &uni) {
    mUniverse = uni;
}
