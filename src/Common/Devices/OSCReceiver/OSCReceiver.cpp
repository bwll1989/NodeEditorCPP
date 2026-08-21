//
// Created by bwll1 on 2024/9/1.
//

#include "OSCReceiver.h"
#include <QByteArray>
#include <QHostAddress>
#include <QVariantList>
#include "tinyosc.h"
#include <QThread>

OSCReceiver::OSCReceiver(quint16 port, QObject *parent)
    : QObject(parent)
    , mPort(port)
    , mHost("0.0.0.0")
    , mSocket(nullptr)
    , mThread(nullptr)
    , m_ownerThread(nullptr)
{
    qRegisterMetaType<QVariantMap>("QVariantMap&");

    m_ownerThread = QThread::currentThread();
    mThread = new QThread();
    this->moveToThread(mThread);
    connect(mThread, &QThread::started, this, &OSCReceiver::initializeSocket);
    mThread->start();
}

OSCReceiver::~OSCReceiver()
{
    if (mThread && mThread->isRunning()) {
        QMetaObject::invokeMethod(this, "prepareToQuit", Qt::BlockingQueuedConnection);
        mThread->quit();
        mThread->wait();
    }
    delete mThread;
    mThread = nullptr;
}

void OSCReceiver::prepareToQuit()
{
    cleanup();
    if (m_ownerThread) {
        moveToThread(m_ownerThread);
    }
}

void OSCReceiver::initializeSocket()
{
    if (mSocket) {
        return;
    }
    mSocket = new QUdpSocket(this);

    if (mSocket->bind(QHostAddress(mHost), mPort,
                      QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint)) {
        connect(mSocket, &QUdpSocket::readyRead, this, &OSCReceiver::processPendingDatagrams);
    }
}

void OSCReceiver::cleanup()
{
    if (mSocket) {
        mSocket->disconnect();
        mSocket->close();
        delete mSocket;
        mSocket = nullptr;
    }
}

void OSCReceiver::processPendingDatagrams()
{
    while (mSocket && mSocket->hasPendingDatagrams()) {
        QByteArray datagram;
        datagram.resize(mSocket->pendingDatagramSize());

        QHostAddress sender;
        quint16 senderPort;
        mSocket->readDatagram(datagram.data(), datagram.size(), &sender, &senderPort);

        tosc_message oscMessage;
        if (tosc_parseMessage(&oscMessage, datagram.data(), datagram.size()) == 0) {
            const char *address = tosc_getAddress(&oscMessage);
            result.insert("address", address);
            message.address = QString(address);
            message.port = mPort;

            const char *format = tosc_getFormat(&oscMessage);
            QVariantList values;
            QString lastType;

            for (int i = 0; format && format[i] != '\0'; i++) {
                const char type = format[i];
                if (type == 'f') {
                    lastType = QStringLiteral("Float");
                    values.append(tosc_getNextFloat(&oscMessage));
                } else if (type == 'i') {
                    lastType = QStringLiteral("Int");
                    values.append(QVariant::fromValue(tosc_getNextInt32(&oscMessage)));
                } else if (type == 's') {
                    lastType = QStringLiteral("String");
                    values.append(QString::fromUtf8(tosc_getNextString(&oscMessage)));
                }
            }

            result.insert("type", lastType);
            if (values.size() <= 1) {
                const QVariant v = values.isEmpty() ? QVariant() : values.first();
                result.insert("default", v);
                message.value = v;
            } else {
                result.insert("default", values);
                message.value = values;
            }
            emit receiveOSC(result);
            emit receiveOSCMessage(message);
        }
    }
}

void OSCReceiver::setPort(const int &port)
{
    if (QThread::currentThread() != this->thread()) {
        QMetaObject::invokeMethod(this, [this, port]() { setPort(port); }, Qt::QueuedConnection);
        return;
    }

    mPort = port;
    if (!mSocket) {
        return;
    }
    mSocket->close();
    mSocket->bind(QHostAddress(mHost), mPort,
                  QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint);
}
