//
// Created by bwll1 on 2024/9/1.
//

#include "OSCSender.h"
#include <QJsonObject>
#include <QByteArray>
#include <QHostAddress>
#include "tinyosc.h"
/**
 * @brief OSCSender构造函数 - 自动启动传输器
 */
OSCSender::OSCSender(QString dstHost,quint16 port, QObject *parent):
        mPort(port),
        mHost(dstHost),
        m_timer(new QTimer(this))
        {
        qRegisterMetaType<QVariantMap >("QVariantMap&");
        //注册信号传递数值类型
        mThread = new QThread(this);
        this->moveToThread(mThread);
        m_timer->setInterval(PROCESS_INTERVAL);
        m_timer->moveToThread(mThread);
        connect(mThread, &QThread::started, this, &OSCSender::initializeSocket);
        connect(mThread, &QThread::finished, this, &OSCSender::cleanup);
        connect(m_timer, &QTimer::timeout, this, &OSCSender::processQueue);
        
        // 自动启动传输器
        mThread->start();
        // 在主线程中启动定时器
        QMetaObject::invokeMethod(m_timer, "start", Qt::QueuedConnection);
}

OSCSender::~OSCSender() {
    cleanup();
    mThread->quit();
    mThread->wait();
}

void OSCSender::initializeSocket() {
    mSocket = new QUdpSocket(this);
    if (mSocket->bind(QHostAddress::AnyIPv4, 0,QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint)) {
    }
}
void OSCSender::cleanup() {
    if (m_timer) {
        m_timer->stop();
        delete m_timer;
        m_timer = nullptr;
    }
    if (mSocket) {
        mSocket->close();
        mSocket->deleteLater();
        mSocket = nullptr;
    }
}
void OSCSender::setHost(QString address,int port) {
    mHost=address;
    mPort = port;
}

void OSCSender::processQueue(){

    // 处理队列中的消息，首先需要加锁，避免多线程冲突
    QMutexLocker locker(&m_mutex);
    //当队列不为空时，处理队列中的消息，直到队列为空
    while (!m_messageQueue.isEmpty()){
        OSCMessage msg = m_messageQueue.dequeue();
        char buffer[1024];
        mHost = msg.host;
        mPort = msg.port;
        const QString &address = msg.address;
        const QVariant &value = msg.value;

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
            qWarning() << "Failed to send OSC message for address" << address << "via QUdpSocket."<<mHost;
            
        }
    }
    
}

bool OSCSender::sendOSCMessageWithQueue(const OSCMessage &message){
    // 将OSCMessage加入队列，首先需要加锁，避免多线程冲突
    QMutexLocker locker(&m_mutex);
    //将OSCMessage加入队列
    m_messageQueue.enqueue(message);
    return true;
}

bool OSCSender::sendOSCMessageDirectly(const OSCMessage &message){
    // 直接发送消息，不加入队列，首先需要加锁，避免多线程冲突
    QMutexLocker locker(&m_mutex);
    char buffer[1024];
    mHost = message.host;
    mPort = message.port;
    const QString &address = message.address;
    const QVariant &value = message.value;

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
        return false; // 跳过不支持的类型
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
        return false; // 跳过失败的消息
    }
    // 通过 QUdpSocket 发送数据
    qint64 bytesSent = mSocket->writeDatagram(buffer, len, QHostAddress(mHost), mPort);
    if (bytesSent < 0) {
        qWarning() << "Failed to send OSC message for address" << address << "via QUdpSocket."<<mHost;
        return false;
    }
    return true;
}