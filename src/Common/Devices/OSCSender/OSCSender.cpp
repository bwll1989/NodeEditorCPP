//
// Created by bwll1 on 2024/9/1.
//

#include "OSCSender.h"
#include <QByteArray>
#include <QHostAddress>
#include <QMetaType>
#include <QVariantList>
#include <QtEndian>
#include <cstring>
#include <limits>

namespace {

void pad4(QByteArray &packet)
{
    while (packet.size() % 4 != 0) {
        packet.append('\0');
    }
}

void appendBigEndianU32(QByteArray &packet, quint32 value)
{
    const quint32 be = qToBigEndian(value);
    packet.append(reinterpret_cast<const char *>(&be), 4);
}

void appendOscFloat(QByteArray &packet, float value)
{
    quint32 bits = 0;
    memcpy(&bits, &value, sizeof(bits));
    appendBigEndianU32(packet, bits);
}

void appendOscInt(QByteArray &packet, int32_t value)
{
    appendBigEndianU32(packet, static_cast<quint32>(value));
}

bool flattenVariant(const QVariant &value, QString &format, QVariantList &args)
{
    const int typeId = value.typeId();

    if (typeId == QMetaType::QVariantList || typeId == QMetaType::QStringList) {
        const QVariantList list = value.toList();
        for (const QVariant &item : list) {
            if (!flattenVariant(item, format, args)) {
                return false;
            }
        }
        return true;
    }

    switch (typeId) {
    case QMetaType::Float:
    case QMetaType::Double:
        format += QLatin1Char('f');
        args.append(value.toFloat());
        return true;
    case QMetaType::Bool:
    case QMetaType::Int:
    case QMetaType::UInt:
        format += QLatin1Char('i');
        args.append(value.toInt());
        return true;
    case QMetaType::LongLong: {
        const qint64 v = value.toLongLong();
        if (v < std::numeric_limits<int32_t>::min() || v > std::numeric_limits<int32_t>::max()) {
            return false;
        }
        format += QLatin1Char('i');
        args.append(static_cast<int>(v));
        return true;
    }
    case QMetaType::ULongLong: {
        const quint64 v = value.toULongLong();
        if (v > static_cast<quint64>(std::numeric_limits<int32_t>::max())) {
            return false;
        }
        format += QLatin1Char('i');
        args.append(static_cast<int>(v));
        return true;
    }
    case QMetaType::QString:
        format += QLatin1Char('s');
        args.append(value.toString());
        return true;
    default:
        if (value.canConvert<double>() && value.userType() != QMetaType::QString) {
            format += QLatin1Char('f');
            args.append(float(value.toDouble()));
            return true;
        }
        return false;
    }
}

QByteArray encodeOscMessage(const QString &address, const QVariant &value, QString *error)
{
    QString format;
    QVariantList args;
    if (value.isValid() && !flattenVariant(value, format, args)) {
        if (error) {
            *error = QStringLiteral("Unsupported OSC value type: %1").arg(value.typeName());
        }
        return {};
    }

    QByteArray packet;
    packet.append(address.toUtf8());
    packet.append('\0');
    pad4(packet);

    packet.append(',');
    packet.append(format.toLatin1());
    packet.append('\0');
    pad4(packet);

    for (int i = 0; i < format.size(); ++i) {
        const QChar t = format.at(i);
        const QVariant &arg = args.at(i);
        if (t == QLatin1Char('f')) {
            appendOscFloat(packet, arg.toFloat());
        } else if (t == QLatin1Char('i')) {
            appendOscInt(packet, static_cast<int32_t>(arg.toInt()));
        } else if (t == QLatin1Char('s')) {
            packet.append(arg.toString().toUtf8());
            packet.append('\0');
            pad4(packet);
        }
    }
    return packet;
}

} // namespace

OSCSender* OSCSender::instance() {
    static OSCSender* sender = nullptr;
    if (!sender) {
        sender = new OSCSender();
    }
    return sender;
}

OSCSender::OSCSender(QString dstHost, quint16 port, QObject *parent)
    : QObject(parent)
    , m_timer(nullptr)
    , mPort(port)
    , mHost(dstHost)
    , mThread(nullptr)
    , mSocket(nullptr)
{
    qRegisterMetaType<QVariantMap>("QVariantMap&");

    // 记录创建线程（通常是主线程）；QThread 必须留在该线程
    QThread *ownerThread = QThread::currentThread();
    mThread = new QThread();
    m_timer = new QTimer();
    m_timer->setInterval(PROCESS_INTERVAL);

    connect(mThread, &QThread::started, this, &OSCSender::initializeSocket);
    connect(m_timer, &QTimer::timeout, this, &OSCSender::processQueue);

    this->moveToThread(mThread);
    m_timer->moveToThread(mThread);

    mThread->start();
    QMetaObject::invokeMethod(m_timer, "start", Qt::QueuedConnection);

    // 供析构时迁回（捕获 ownerThread，避免在错误线程调用 moveToThread）
    m_ownerThread = ownerThread;
}

OSCSender::~OSCSender()
{
    if (mThread && mThread->isRunning()) {
        // moveToThread 必须在「对象当前线程」里调用，故在工作线程内完成 cleanup + 迁回
        QMetaObject::invokeMethod(this, "prepareToQuit", Qt::BlockingQueuedConnection);
        mThread->quit();
        mThread->wait();
    }

    delete m_timer;
    m_timer = nullptr;
    delete mThread;
    mThread = nullptr;
}

void OSCSender::prepareToQuit()
{
    cleanup();
    if (!m_ownerThread) {
        return;
    }
    if (m_timer) {
        m_timer->moveToThread(m_ownerThread);
    }
    moveToThread(m_ownerThread);
}

void OSCSender::initializeSocket()
{
    if (mSocket) {
        return;
    }
    mSocket = new QUdpSocket(this);
    mSocket->bind(QHostAddress::AnyIPv4, 0,
                  QAbstractSocket::ShareAddress | QAbstractSocket::ReuseAddressHint);
}

void OSCSender::cleanup()
{
    if (m_timer) {
        m_timer->stop();
    }
    if (mSocket) {
        mSocket->disconnect();
        mSocket->close();
        delete mSocket;
        mSocket = nullptr;
    }
}

void OSCSender::setHost(QString address, int port)
{
    mHost = address;
    mPort = port;
}

bool OSCSender::writeAndSend(const OSCMessage &msg)
{
    if (!mSocket) {
        return false;
    }

    mHost = msg.host;
    mPort = msg.port;

    QString error;
    const QByteArray packet = encodeOscMessage(msg.address, msg.value, &error);
    if (packet.isEmpty()) {
        qWarning() << (error.isEmpty() ? QStringLiteral("Failed to encode OSC message") : error)
                   << "address:" << msg.address << "value:" << msg.value;
        return false;
    }

    const qint64 bytesSent = mSocket->writeDatagram(packet, QHostAddress(mHost), mPort);
    if (bytesSent < 0) {
        qWarning() << "Failed to send OSC message for address" << msg.address << "via QUdpSocket." << mHost;
        return false;
    }
    return true;
}

void OSCSender::processQueue()
{
    QMutexLocker locker(&m_mutex);
    while (!m_messageQueue.isEmpty()) {
        const OSCMessage msg = m_messageQueue.dequeue();
        writeAndSend(msg);
    }
}

bool OSCSender::sendOSCMessageWithQueue(const OSCMessage &message)
{
    QMutexLocker locker(&m_mutex);
    m_messageQueue.enqueue(message);
    emit messageSent(message);
    return true;
}

bool OSCSender::sendOSCMessageDirectly(const OSCMessage &message)
{
    // 直接发送必须在 socket 所在线程执行
    if (QThread::currentThread() != thread()) {
        bool ok = false;
        QMetaObject::invokeMethod(this, [this, message, &ok]() {
            ok = sendOSCMessageDirectly(message);
        }, Qt::BlockingQueuedConnection);
        return ok;
    }

    QMutexLocker locker(&m_mutex);
    if (!writeAndSend(message)) {
        return false;
    }
    emit messageSent(message);
    return true;
}
