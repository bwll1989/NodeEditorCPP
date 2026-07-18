#include "FTOMVController.h"
#include "OMVProtocol.h"

#include <QHash>
#include <QHostAddress>
#include <QJsonDocument>
#include <QMetaType>
#include <QMutexLocker>
#include <QTcpServer>
#include <QTcpSocket>
#include <QThread>
#include <QTimer>
#include <QVariantMap>

FTOMVController *FTOMVController::s_instance = nullptr;
int FTOMVController::s_refCount = 0;
QMutex FTOMVController::s_refMutex;

namespace {

QString peerHostOf(const QTcpSocket *socket)
{
    if (!socket) {
        return {};
    }
    const QHostAddress addr = socket->peerAddress();
    bool ok = false;
    const quint32 v4 = addr.toIPv4Address(&ok);
    if (ok) {
        return QHostAddress(v4).toString();
    }
    return addr.toString();
}

/**
 * @brief 网络工作线程：监听、粘包解帧、设备登记与按 ID 写帧
 */
class OmvTcpWorker : public QObject
{
    Q_OBJECT

public:
    explicit OmvTcpWorker(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    ~OmvTcpWorker() override
    {
        cleanup();
    }

public slots:
    void initialize(const QString &host, quint16 port)
    {
        cleanup();

        m_server = new QTcpServer(this);
        connect(m_server, &QTcpServer::newConnection, this, &OmvTcpWorker::onNewConnection);

        const QHostAddress address = host.isEmpty() ? QHostAddress::Any : QHostAddress(host);
        if (m_server->listen(address, port)) {
            qDebug() << "FTOMVController listening on" << host << port;
            emit serverReady(true, port);
        } else {
            qWarning() << "FTOMVController failed to listen on" << host << port
                       << m_server->errorString();
            emit serverReady(false, port);
        }
    }

    void cleanup()
    {
        const QList<QTcpSocket *> sockets = m_clientSockets;
        for (QTcpSocket *socket : sockets) {
            const int deviceId = m_socketToDevice.value(socket, -1);
            const QString host = peerHostOf(socket);
            if (socket->isOpen()) {
                socket->close();
            }
            socket->deleteLater();
            emit clientGone(deviceId, host);
        }
        m_clientSockets.clear();
        m_receiveBuffers.clear();
        m_socketToDevice.clear();
        m_deviceToSocket.clear();

        if (m_server) {
            m_server->close();
            m_server->deleteLater();
            m_server = nullptr;
        }
    }

    void sendFrameToDevice(int deviceId, const QByteArray &frame)
    {
        QTcpSocket *socket = m_deviceToSocket.value(deviceId, nullptr);
        if (!socket || socket->state() != QAbstractSocket::ConnectedState) {
            qWarning() << "FTOMVController device not connected:" << deviceId;
            return;
        }
        socket->write(frame);
    }

    void broadcastFrame(const QByteArray &frame)
    {
        for (auto it = m_deviceToSocket.cbegin(); it != m_deviceToSocket.cend(); ++it) {
            QTcpSocket *socket = it.value();
            if (socket && socket->state() == QAbstractSocket::ConnectedState) {
                socket->write(frame);
            }
        }
    }

private slots:
    void onNewConnection()
    {
        while (m_server && m_server->hasPendingConnections()) {
            QTcpSocket *socket = m_server->nextPendingConnection();
            const QString host = peerHostOf(socket);
            qDebug() << "FTOMVController client connected:" << host;

            connect(socket, &QTcpSocket::readyRead, this, &OmvTcpWorker::onReadyRead);
            connect(socket, &QTcpSocket::disconnected, this, &OmvTcpWorker::onDisconnected);

            m_clientSockets.append(socket);
            m_receiveBuffers.insert(socket, QByteArray());
            emit clientArrived(host);
        }
    }

    void onReadyRead()
    {
        auto *socket = qobject_cast<QTcpSocket *>(sender());
        if (!socket) {
            return;
        }

        QByteArray &buffer = m_receiveBuffers[socket];
        buffer.append(socket->readAll());

        QVector<omv::ParsedFrame> frames;
        omv::tryDecode(buffer, frames);

        const QString host = peerHostOf(socket);
        for (const omv::ParsedFrame &frame : frames) {
            handleFrame(socket, host, frame);
        }

        // 无标准帧头时：仅尝试识别握手（用于设备登记）
        if (frames.isEmpty() && buffer.size() >= 28) {
            const omv::ParsedFrame legacy = omv::classifyFrame(buffer);
            if (legacy.kind == omv::FrameKind::Handshake) {
                handleFrame(socket, host, legacy);
                buffer.clear();
            }
        }
    }

    void onDisconnected()
    {
        auto *socket = qobject_cast<QTcpSocket *>(sender());
        if (!socket) {
            return;
        }

        const int deviceId = m_socketToDevice.value(socket, -1);
        const QString host = peerHostOf(socket);

        if (deviceId >= 0) {
            m_deviceToSocket.remove(deviceId);
        }
        m_socketToDevice.remove(socket);
        m_clientSockets.removeAll(socket);
        m_receiveBuffers.remove(socket);
        socket->deleteLater();

        emit clientGone(deviceId, host);
    }

private:
    void handleFrame(QTcpSocket *socket, const QString &host, const omv::ParsedFrame &frame)
    {
        int deviceId = m_socketToDevice.value(socket, -1);

        if (frame.kind == omv::FrameKind::Handshake && frame.deviceId >= 0) {
            deviceId = frame.deviceId;
            const int oldId = m_socketToDevice.value(socket, -1);
            if (oldId >= 0 && oldId != deviceId) {
                m_deviceToSocket.remove(oldId);
            }
            if (QTcpSocket *oldSocket = m_deviceToSocket.value(deviceId, nullptr)) {
                if (oldSocket != socket) {
                    m_socketToDevice.remove(oldSocket);
                }
            }
            m_socketToDevice.insert(socket, deviceId);
            m_deviceToSocket.insert(deviceId, socket);
            emit deviceHandshaked(deviceId, host);
            socket->write(omv::encodeHandshake());
        }

        QVariantMap msg;
        msg.insert(QStringLiteral("host"), host);
        msg.insert(QStringLiteral("deviceId"), deviceId >= 0 ? deviceId : frame.deviceId);
        msg.insert(QStringLiteral("isHandshake"), frame.kind == omv::FrameKind::Handshake);
        msg.insert(QStringLiteral("typeDigit"),
                   frame.typeDigit ? QString(QChar(frame.typeDigit)) : QString());
        msg.insert(QStringLiteral("hex"), QString::fromLatin1(frame.rawFrame.toHex()));
        if (!frame.json.isEmpty()) {
            msg.insert(QStringLiteral("json"), frame.json.toVariantMap());
        }
        emit messageReceived(msg);
    }

signals:
    void serverReady(bool ready, quint16 port);
    void clientArrived(const QString &peerHost);
    void deviceHandshaked(int deviceId, const QString &peerHost);
    void clientGone(int deviceId, const QString &peerHost);
    void messageReceived(const QVariantMap &msg);

private:
    QTcpServer *m_server = nullptr;
    QList<QTcpSocket *> m_clientSockets;
    QHash<QTcpSocket *, QByteArray> m_receiveBuffers;
    QHash<QTcpSocket *, int> m_socketToDevice;
    QHash<int, QTcpSocket *> m_deviceToSocket;
};

} // namespace

FTOMVController *FTOMVController::instance()
{
    QMutexLocker locker(&s_refMutex);
    return s_instance;
}

FTOMVController *FTOMVController::acquire()
{
    QMutexLocker locker(&s_refMutex);
    if (!s_instance) {
        s_instance = new FTOMVController();
    }
    ++s_refCount;
    return s_instance;
}

void FTOMVController::release()
{
    FTOMVController *toDelete = nullptr;
    {
        QMutexLocker locker(&s_refMutex);
        if (s_refCount > 0) {
            --s_refCount;
            if (s_refCount == 0) {
                toDelete = s_instance;
                s_instance = nullptr;
            }
        }
    }
    if (toDelete) {
        delete toDelete;
    }
}

QJsonObject FTOMVController::toJsonObject(const QVariantMap &map)
{
    return QJsonObject::fromVariantMap(map);
}

FTOMVController::FTOMVController(QObject *parent)
    : QObject(parent)
    , m_keepAliveTimer(new QTimer(this))
{
    qRegisterMetaType<QVariantMap>("QVariantMap");
    qRegisterMetaType<QJsonObject>("QJsonObject");

    m_keepAliveTimer->setInterval(omv::kKeepAliveIntervalMs);
    connect(m_keepAliveTimer, &QTimer::timeout, this, &FTOMVController::broadcastHandshake);

    setupWorker();
    startServer(m_host, m_port);
}

FTOMVController::~FTOMVController()
{
    if (m_keepAliveTimer) {
        m_keepAliveTimer->stop();
    }
    teardownWorker();
}

void FTOMVController::setupWorker()
{
    m_thread = new QThread(this);
    auto *worker = new OmvTcpWorker();
    m_worker = worker;
    worker->moveToThread(m_thread);

    connect(m_thread, &QThread::finished, worker, &QObject::deleteLater);

    connect(worker, &OmvTcpWorker::serverReady, this, [this](bool ready, quint16 port) {
        {
            QMutexLocker locker(&m_mutex);
            m_listening = ready;
            m_port = port;
        }
        if (ready) {
            if (!m_keepAliveTimer->isActive()) {
                m_keepAliveTimer->start();
            }
        } else {
            m_keepAliveTimer->stop();
        }
        emit isReady(ready);
    });

    connect(worker, &OmvTcpWorker::clientArrived, this, &FTOMVController::clientConnected);
    connect(worker, &OmvTcpWorker::deviceHandshaked, this, [this](int deviceId, const QString &host) {
        {
            QMutexLocker locker(&m_mutex);
            m_deviceHosts.insert(deviceId, host);
        }
        emit deviceRegistered(deviceId, host);
    });
    connect(worker, &OmvTcpWorker::clientGone, this, [this](int deviceId, const QString &host) {
        {
            QMutexLocker locker(&m_mutex);
            if (deviceId >= 0) {
                m_deviceHosts.remove(deviceId);
            }
        }
        emit clientDisconnected(deviceId, host);
    });
    connect(worker, &OmvTcpWorker::messageReceived, this, [this](const QVariantMap &msg) {
        emit frameReceived(msg);
        emit recMsg(msg);
    });

    m_thread->start();
}

void FTOMVController::teardownWorker()
{
    if (!m_thread) {
        return;
    }
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "cleanup", Qt::BlockingQueuedConnection);
    }
    m_thread->quit();
    m_thread->wait();
    m_thread = nullptr;
    m_worker = nullptr;
}

void FTOMVController::startServer(const QString &host, quint16 port)
{
    QString bindHost;
    quint16 bindPort = 0;
    {
        QMutexLocker locker(&m_mutex);
        m_host = host.isEmpty() ? QStringLiteral("0.0.0.0") : host;
        m_port = port == 0 ? static_cast<quint16>(omv::kDefaultPort) : port;
        bindHost = m_host;
        bindPort = m_port;
    }
    if (!m_worker) {
        return;
    }
    QMetaObject::invokeMethod(m_worker, "initialize", Qt::QueuedConnection,
                              Q_ARG(QString, bindHost),
                              Q_ARG(quint16, bindPort));
}

void FTOMVController::stopServer()
{
    if (m_keepAliveTimer) {
        m_keepAliveTimer->stop();
    }
    {
        QMutexLocker locker(&m_mutex);
        m_listening = false;
        m_deviceHosts.clear();
    }
    if (m_worker) {
        QMetaObject::invokeMethod(m_worker, "cleanup", Qt::QueuedConnection);
    }
    emit isReady(false);
}

bool FTOMVController::enqueueFrame(int deviceId, const QByteArray &frame)
{
    if (!m_worker || deviceId < 0 || frame.isEmpty()) {
        return false;
    }
    return QMetaObject::invokeMethod(m_worker, "sendFrameToDevice", Qt::QueuedConnection,
                                     Q_ARG(int, deviceId),
                                     Q_ARG(QByteArray, frame));
}

bool FTOMVController::enqueueBroadcast(const QByteArray &frame)
{
    if (!m_worker || frame.isEmpty()) {
        return false;
    }
    return QMetaObject::invokeMethod(m_worker, "broadcastFrame", Qt::QueuedConnection,
                                     Q_ARG(QByteArray, frame));
}

bool FTOMVController::sendJson(int deviceId, const QJsonObject &json, char typeDigit)
{
    return enqueueFrame(deviceId, omv::encodeJson(json, typeDigit));
}

bool FTOMVController::sendJson(int deviceId, const QVariantMap &json, char typeDigit)
{
    return sendJson(deviceId, toJsonObject(json), typeDigit);
}

bool FTOMVController::broadcastJson(const QJsonObject &json, char typeDigit)
{
    return enqueueBroadcast(omv::encodeJson(json, typeDigit));
}

bool FTOMVController::broadcastJson(const QVariantMap &json, char typeDigit)
{
    return broadcastJson(toJsonObject(json), typeDigit);
}

bool FTOMVController::sendFrame(int deviceId, const QByteArray &frame)
{
    return enqueueFrame(deviceId, frame);
}

bool FTOMVController::handshake(int deviceId)
{
    return enqueueFrame(deviceId, omv::encodeHandshake());
}

void FTOMVController::broadcastHandshake()
{
    enqueueBroadcast(omv::encodeHandshake());
}

QList<int> FTOMVController::connectedDeviceIds() const
{
    QMutexLocker locker(&m_mutex);
    return m_deviceHosts.keys();
}

bool FTOMVController::isListening() const
{
    QMutexLocker locker(&m_mutex);
    return m_listening;
}

quint16 FTOMVController::port() const
{
    QMutexLocker locker(&m_mutex);
    return m_port;
}

#include "FTOMVController.moc"
