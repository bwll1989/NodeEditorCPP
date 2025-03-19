//
// Created by bwll1 on 2024/9/1.
//

#include "OSCReceiver.h"
#include <QJsonObject>
#include <QByteArray>
#include <QHostAddress>
#include "tinyosc.h"
#include <QThread>

OSCReceiver::OSCReceiver(quint16 port, QObject *parent)
        : QObject(parent), mPort(port), mHost("0.0.0.0"), mSocket(nullptr) {
    // 启动线程
    qRegisterMetaType<QVariantMap >("QVariantMap&");
    //注册信号传递数值类型
    mThread = new QThread(this);
    this->moveToThread(mThread);
    connect(mThread, &QThread::started, this, &OSCReceiver::initializeSocket);
    connect(mThread, &QThread::finished, this, &OSCReceiver::cleanup);

    mThread->start();
}

OSCReceiver::~OSCReceiver() {
    mThread->quit();
    mThread->wait();
}

void OSCReceiver::initializeSocket() {
    mSocket = new QUdpSocket(this);

    if (mSocket->bind(QHostAddress(mHost), mPort,QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint)) {
        connect(mSocket, &QUdpSocket::readyRead, this, &OSCReceiver::processPendingDatagrams);
    }
}

void OSCReceiver::cleanup() {
    if (mSocket) {
        mSocket->close();
        mSocket->deleteLater();
        mSocket = nullptr;
    }
}

void OSCReceiver::processPendingDatagrams() {
    while (mSocket && mSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(mSocket->pendingDatagramSize());

        QHostAddress sender;
        quint16 senderPort;
        mSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        // 使用 tinyosc 解析 OSC 数据
        tosc_message oscMessage;
        if (tosc_parseMessage(&oscMessage, datagram.data(), datagram.size()) == 0) {
            const char *address = tosc_getAddress(&oscMessage);
            result.insert("address", address);
            message.address = QString(address);
            message.port = mPort;

            const char *format = tosc_getFormat(&oscMessage);

            for (int i = 0; format[i] != '\0'; i++) {
                const char type = format[i];
                if (type == 'f') {
                    double value = tosc_getNextFloat(&oscMessage);
                    result.insert("type", "Float");
                    result.insert("default", value);
                    message.value = value;
                } else if (type == 'i') {
                    int32_t value = tosc_getNextInt32(&oscMessage);
                    result.insert("type", "Int");
                    result.insert("default", QVariant::fromValue(value));
                    message.value = value;
                } else if (type == 's') {
                    const char *value = tosc_getNextString(&oscMessage);
                    result.insert("type", "String");
                    result.insert("default", value);
                    message.value = QString(value);
                }
            }
            emit receiveOSC(result);
            emit receiveOSCMessage(message);
        }
    }
}



void OSCReceiver::setPort(const int &port) {
    if (mSocket) {
        mSocket->close();
    }
    mPort = port;
    if (mSocket) {
        mSocket->bind(QHostAddress(mHost), mPort);
    }
}
