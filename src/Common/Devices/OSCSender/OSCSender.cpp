//
// Created by bwll1 on 2024/9/1.
//

#include "OSCSender.h"
#include <QJsonObject>
#include <QByteArray>
#include <QHostAddress>
#include "Common/Devices/UdpSocket/UdpSocket.h"
#include "tinyosc.h"
OSCSender::OSCSender(QString dstHost,quint16 port, QObject *parent):
        mPort(port),
        mHost(dstHost) {
        qRegisterMetaType<QVariantMap >("QVariantMap&");
        //注册信号传递数值类型
        mThread = new QThread(this);
        this->moveToThread(mThread);
        connect(mThread, &QThread::started, this, &OSCSender::initializeSocket);
        connect(mThread, &QThread::finished, this, &OSCSender::cleanup);

        mThread->start();

}

OSCSender::~OSCSender() {
    mThread->quit();
    mThread->wait();
}

void OSCSender::initializeSocket() {
    mSocket = new QUdpSocket(this);

    if (mSocket->bind(QHostAddress(mHost), mPort)) {
    } else {
        qWarning() << "Failed to bind to port" << mPort;
    }
}
void OSCSender::cleanup() {
    if (mSocket) {
        mSocket->close();
        mSocket->deleteLater();
        mSocket = nullptr;
    }
}
void OSCSender::setHost(QString address,int port) {
    mHost=address;
    if (mSocket) {
        mSocket->close();
    }
    mPort = port;
    if (mSocket) {
        mSocket->bind(QHostAddress(mHost), mPort);
    }
}


bool OSCSender::sendMessage(QVariantMap &data){
    char buffer[1024];
    for (auto it = data.cbegin(); it != data.cend(); ++it) {
        const QString &address = it.key();
        const QVariant &value = it.value();

        QString format;
        std::vector<float> floatArgs;
        std::vector<int32_t> intArgs;
        std::vector<const char*> stringArgs;

        // 构造格式字符串并根据值类型准备参数
        if (value.typeId() == QMetaType::Double) {
            format += "f";
            floatArgs.push_back(static_cast<float>(value.toDouble()));
        } else if (value.typeId() == QMetaType::Int) {
            format += "i";
            intArgs.push_back(value.toInt());
        } else if (value.typeId() == QMetaType::QString) {
            format += "s";
            QByteArray ba = value.toString().toLatin1(); // must
            stringArgs.push_back(ba.data());
        } else if (value.typeId() == QMetaType::Bool) {
            format += "i";
            intArgs.push_back(value.toInt());
        } else {
            qWarning() << "Unsupported value type in QVariantMap:" << value;
            continue; // 跳过不支持的类型
        }

        // 写入 OSC 消息
        uint32_t len = 0;
        if (floatArgs.size() + intArgs.size() + stringArgs.size() == 0) {
            len = tosc_writeMessage(buffer, sizeof(buffer), address.toStdString().c_str(), "");
        } else {
            // 动态展开参数
            if (!floatArgs.empty()) {
                len = tosc_writeMessage(buffer, sizeof(buffer), address.toStdString().c_str(),
                                        format.toStdString().c_str(), floatArgs[0]);
            } else if (!intArgs.empty()) {
                len = tosc_writeMessage(buffer, sizeof(buffer), address.toStdString().c_str(),
                                        format.toStdString().c_str(), intArgs[0]);
            } else if (!stringArgs.empty()) {
                len = tosc_writeMessage(buffer, sizeof(buffer), address.toStdString().c_str(),
                                        format.toStdString().c_str(), stringArgs[0]);
            }
        }

        // 检查写入是否成功
        if (len == 0) {
            qWarning() << "Failed to write OSC message for address" << address;
            continue; // 跳过失败的消息
        }

        // 通过 QUdpSocket 发送数据
        qint64 bytesSent = mSocket->writeDatagram(buffer, len, QHostAddress(mHost), mPort);
        if (bytesSent < 0) {
            qWarning() << "Failed to send OSC message for address" << address << "via QUdpSocket.";
            return false;
        }
    }

    return true;

}