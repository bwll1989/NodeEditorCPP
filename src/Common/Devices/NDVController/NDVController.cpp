/**
 * @file NDVController.cpp
 * @brief 对齐 ArtnetTransmitter：常驻单例 + moveToThread，进程退出不主动销毁
 */
#include "NDVController.h"

#include <QCoreApplication>
#include <QDebug>
#include <QHostAddress>
#include <QMetaType>
#include <QMutexLocker>

#include <utility>

namespace {

const QByteArray kPrefix = QByteArray::fromHex("4A480E0010000000000000004A481000484A0100");
const QByteArray kSuffix = QByteArray::fromHex("000000000000");
const QByteArray kStop = QByteArray::fromHex("02000000");
const QByteArray kNext = QByteArray::fromHex("04000000");
const QByteArray kPrev = QByteArray::fromHex("03000000");
const QByteArray kClose = QByteArray::fromHex("09000000");
const QByteArray kPlay = QByteArray::fromHex("1600");
const QByteArray kHandshake = QByteArray::fromHex("00000000");
const QByteArray kLoop = QByteArray::fromHex("1000");
const QByteArray kPause = QByteArray::fromHex("1E000000");
constexpr int kFrameSize = 30;
constexpr int kPlayingTimeoutMs = 800;

} // namespace

NDVController *NDVController::s_instance = nullptr;
QMutex NDVController::s_mutex;

QString NDVController::peerHostOf(const QTcpSocket *socket)
{
    return socket ? socket->peerAddress().toString() : QString();
}

bool NDVController::hostMatches(const QString &peer, const QString &target)
{
    if (peer == target) {
        return true;
    }
    return normalizeIp(peer) == normalizeIp(target);
}

NDVController *NDVController::getInstance()
{
    static bool exitHooked = false;
    NDVController *result = nullptr;
    QCoreApplication *app = nullptr;
    bool needHook = false;
    {
        QMutexLocker locker(&s_mutex);
        if (!s_instance) {
            s_instance = new NDVController();
        }
        result = s_instance;
        if (!exitHooked) {
            exitHooked = true;
            needHook = true;
            app = QCoreApplication::instance();
        }
    }
    if (needHook && app) {
        QObject::connect(app, &QCoreApplication::aboutToQuit, app,
                         []() { NDVController::prepareForAppExit(); },
                         Qt::DirectConnection);
    }
    return result;
}

NDVController *NDVController::instance()
{
    return getInstance();
}

NDVController *NDVController::acquire()
{
    return getInstance();
}

void NDVController::release()
{
    // 对齐 Artnet：节点析构不销毁单例，避免进程退出时拆 TCP 触发 fail-fast
}

void NDVController::prepareForAppExit()
{
    NDVController *inst = nullptr;
    {
        QMutexLocker locker(&s_mutex);
        inst = s_instance;
    }
    if (!inst) {
        return;
    }
    // 先阻断对外信号，再在工作线程停定时器（不销毁线程/套接字，交给进程退出）
    inst->blockSignals(true);
    if (inst->thread() && inst->thread()->isRunning()) {
        QMetaObject::invokeMethod(inst, "silenceForExit", Qt::QueuedConnection);
    }
}

void NDVController::silenceForExit()
{
    if (m_heartbeatTimer) {
        m_heartbeatTimer->stop();
    }
    if (m_playingWatchTimer) {
        m_playingWatchTimer->stop();
    }
}

NDVController::NDVController(QObject *parent)
    : QObject(parent)
{
    qRegisterMetaType<NDVClientInfo>("NDVClientInfo");
    qRegisterMetaType<QVariantMap>("QVariantMap");

    // 对齐 ArtnetTransmitter：单例自身搬到工作线程，套接字在该线程创建
    // QThread 不挂 parent，避免 moveToThread 把 QThread 对象迁入自身
    m_thread = new QThread();
    m_heartbeatTimer = new QTimer(this);
    m_heartbeatTimer->setInterval(5000);
    m_playingWatchTimer = new QTimer(this);
    m_playingWatchTimer->setInterval(500);

    this->moveToThread(m_thread);

    connect(m_thread, &QThread::started, this, &NDVController::initializeServer);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &NDVController::sendHeartbeatToAll);
    connect(m_playingWatchTimer, &QTimer::timeout, this, &NDVController::checkPlayingTimeout);
    connect(this, &NDVController::sendCommandRequested,
            this, &NDVController::handleSendCommand, Qt::QueuedConnection);

    m_thread->start();
    QMetaObject::invokeMethod(m_heartbeatTimer, "start", Qt::QueuedConnection);
    QMetaObject::invokeMethod(m_playingWatchTimer, "start", Qt::QueuedConnection);
}

NDVController::~NDVController()
{
    // 正常路径不会走到这里（常驻单例）。若被显式 delete：
    if (m_thread) {
        m_thread->quit();
        m_thread->wait();
    }
}

void NDVController::initializeServer()
{
    if (m_server) {
        return;
    }

    m_server = new QTcpServer(this);
    connect(m_server, &QTcpServer::newConnection, this, &NDVController::onNewConnection);

    if (m_server->listen(QHostAddress::Any, static_cast<quint16>(FIXED_PORT))) {
        qDebug() << "NDVController listening on port" << FIXED_PORT;
        m_listening = true;
        Q_EMIT listeningChanged(true);
    } else {
        qWarning() << "NDVController failed to listen on port" << FIXED_PORT
                   << m_server->errorString();
        m_listening = false;
        Q_EMIT listeningChanged(false);
    }
}

void NDVController::onNewConnection()
{
    if (!m_server) {
        return;
    }
    while (m_server->hasPendingConnections()) {
        QTcpSocket *clientSocket = m_server->nextPendingConnection();
        if (!clientSocket) {
            break;
        }
        clientSocket->setParent(this);
        const QString peerIp = peerHostOf(clientSocket);
        // 断开时 peerAddress 可能已空，连接时先缓存
        clientSocket->setProperty("ndvPeerIp", peerIp);
        qDebug() << "NDVController client connected:" << peerIp;
        connect(clientSocket, &QTcpSocket::readyRead, this, &NDVController::onReadyRead);
        connect(clientSocket, &QTcpSocket::disconnected, this, &NDVController::onClientDisconnected);
        m_clientSockets.append(clientSocket);
    }
}

void NDVController::onReadyRead()
{
    auto *clientSocket = qobject_cast<QTcpSocket *>(sender());
    if (!clientSocket || clientSocket->bytesAvailable() <= 0) {
        return;
    }
    const QByteArray data = clientSocket->readAll();
    if (!data.isEmpty()) {
        processBytes(clientSocket, data);
    }
}

void NDVController::onClientDisconnected()
{
    auto *clientSocket = qobject_cast<QTcpSocket *>(sender());
    if (!clientSocket) {
        return;
    }

    const int boundId = clientSocket->property("ndvDeviceId").toInt();
    QString peerIp = clientSocket->property("ndvPeerIp").toString();
    if (peerIp.isEmpty()) {
        peerIp = peerHostOf(clientSocket);
    }
    m_clientSockets.removeAll(clientSocket);
    clientSocket->deleteLater();

    QList<NDVClientInfo> toNotify;
    {
        QMutexLocker locker(&m_mutex);
        if (boundId > 0 && m_clients.contains(boundId) && m_clients.value(boundId).online) {
            NDVClientInfo &info = m_clients[boundId];
            info.online = false;
            info.state = QStringLiteral("Unknown");
            toNotify.append(info);
        } else if (!peerIp.isEmpty()) {
            // 尚未握手绑定 ID 时，按 IP 兜底
            for (auto it = m_clients.begin(); it != m_clients.end(); ++it) {
                if (!hostMatches(it.value().ipAddress, peerIp) || !it.value().online) {
                    continue;
                }
                it.value().online = false;
                it.value().state = QStringLiteral("Unknown");
                toNotify.append(it.value());
            }
        }
    }

    if (!toNotify.isEmpty()) {
        Q_EMIT clientListChanged();
        for (const NDVClientInfo &info : toNotify) {
            Q_EMIT clientStatusChanged(info.deviceId, info);
        }
    }
}

bool NDVController::isOnline(int deviceId) const
{
    QMutexLocker locker(&m_mutex);
    return m_clients.contains(deviceId) && m_clients.value(deviceId).online;
}

NDVClientInfo NDVController::clientInfo(int deviceId) const
{
    QMutexLocker locker(&m_mutex);
    if (m_clients.contains(deviceId)) {
        return m_clients.value(deviceId);
    }
    NDVClientInfo info;
    info.deviceId = deviceId;
    info.online = false;
    info.state = QStringLiteral("Unknown");
    return info;
}

QList<NDVClientInfo> NDVController::clients() const
{
    QMutexLocker locker(&m_mutex);
    return m_clients.values();
}

bool NDVController::sendCommand(const QString &type, int fileIndex, int targetId)
{
    // 节点在主线程调用；实际发包在工作线程（对齐 Artnet 的跨线程投递）
    if (QThread::currentThread() != this->thread()) {
        emit sendCommandRequested(type, fileIndex, targetId);
        return true;
    }
    return sendCommandImpl(type, fileIndex, targetId);
}

void NDVController::handleSendCommand(const QString &type, int fileIndex, int targetId)
{
    sendCommandImpl(type, fileIndex, targetId);
}

bool NDVController::sendCommandImpl(const QString &type, int fileIndex, int targetId)
{
    const QByteArray opcode = commandOpcode(type);
    if (opcode.isEmpty()) {
        return false;
    }

    const QString t = type.toLower();
    auto applyOptimistic = [this, &t](int id) {
        NDVClientInfo info;
        {
            QMutexLocker locker(&m_mutex);
            if (!m_clients.contains(id)) {
                return;
            }
            info = m_clients.value(id);
        }
        if (!info.online) {
            return;
        }
        if (t == QLatin1String("stop") || t == QLatin1String("pause") || t == QLatin1String("close")) {
            info.state = QStringLiteral("Stopped");
            info.ignoreProgressUntil = QDateTime::currentDateTime().addMSecs(500);
            upsertClient(info, true);
        } else if (t == QLatin1String("play") || t == QLatin1String("loop")) {
            info.state = QStringLiteral("Playing");
            info.lastProgressAt = QDateTime::currentDateTime();
            info.ignoreProgressUntil = QDateTime();
            upsertClient(info, true);
        }
    };

    if (targetId == 0) {
        QList<int> ids;
        {
            QMutexLocker locker(&m_mutex);
            ids = m_clients.keys();
        }
        if (ids.isEmpty()) {
            return false;
        }
        bool any = false;
        for (int id : ids) {
            const QByteArray packet = buildCommand(opcode, fileIndex, id);
            if (sendRawToId(id, packet)) {
                applyOptimistic(id);
                any = true;
            }
        }
        return any;
    }

    const QByteArray packet = buildCommand(opcode, fileIndex, targetId);
    if (!sendRawToId(targetId, packet)) {
        return false;
    }
    applyOptimistic(targetId);
    return true;
}

void NDVController::processBytes(QTcpSocket *socket, const QByteArray &bytes)
{
    if (!socket || bytes.size() < kFrameSize) {
        return;
    }

    int offset = 0;
    while (offset + kFrameSize <= bytes.size()) {
        if (quint8(bytes[offset]) != 0x4A || quint8(bytes[offset + 1]) != 0x48) {
            ++offset;
            continue;
        }
        handleFrame(socket, bytes.mid(offset, kFrameSize));
        offset += kFrameSize;
    }
}

void NDVController::handleFrame(QTcpSocket *socket, const QByteArray &frame)
{
    if (!socket || frame.size() < kFrameSize) {
        return;
    }

    const quint8 opcode = quint8(frame[20]);
    const QDateTime now = QDateTime::currentDateTime();
    QString peerIp = socket->property("ndvPeerIp").toString();
    if (peerIp.isEmpty()) {
        peerIp = peerHostOf(socket);
        socket->setProperty("ndvPeerIp", peerIp);
    }

    // 握手：把设备 ID 绑定到该 TCP 连接，后续帧按连接区分，不再仅靠 IP
    if (opcode == 0x0A) {
        const int ndvId = quint8(frame[22]);
        if (ndvId <= 0) {
            return;
        }
        socket->setProperty("ndvDeviceId", ndvId);

        NDVClientInfo info;
        {
            QMutexLocker locker(&m_mutex);
            info = m_clients.value(ndvId);
        }
        info.deviceId = ndvId;
        info.ipAddress = peerIp;
        info.online = true;
        info.lastSeen = now;
        info.lastHandshake = now;
        if (info.state != QLatin1String("Playing")) {
            info.state = QStringLiteral("Connected");
        }
        upsertClient(info, true);
        return;
    }

    const int deviceId = socket->property("ndvDeviceId").toInt();
    if (deviceId <= 0) {
        return;
    }

    NDVClientInfo info;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_clients.contains(deviceId)) {
            return;
        }
        info = m_clients.value(deviceId);
    }
    info.lastSeen = now;
    info.online = true;
    info.ipAddress = peerIp;

    if (opcode == 0x0E) {
        // 只作“在播”心跳：不解析进度数值，状态未变则不通知 UI
        if (info.ignoreProgressUntil.isValid() && now < info.ignoreProgressUntil) {
            return;
        }
        info.lastProgressAt = now;
        info.state = QStringLiteral("Playing");
        upsertClient(info, true);
        return;
    }

    if (opcode == 0x20) {
        info.state = QStringLiteral("Stopped");
        info.lastProgressAt = QDateTime();
        info.ignoreProgressUntil = QDateTime();
        upsertClient(info, true);
    }
}

void NDVController::checkPlayingTimeout()
{
    QList<NDVClientInfo> snapshot;
    {
        QMutexLocker locker(&m_mutex);
        snapshot = m_clients.values();
    }
    const QDateTime now = QDateTime::currentDateTime();
    for (const NDVClientInfo &client : snapshot) {
        if (client.state != QLatin1String("Playing")) {
            continue;
        }
        if (!client.lastProgressAt.isValid()
            || client.lastProgressAt.msecsTo(now) < kPlayingTimeoutMs) {
            continue;
        }
        NDVClientInfo updated = client;
        updated.state = QStringLiteral("Stopped");
        updated.lastProgressAt = QDateTime();
        upsertClient(updated, true);
    }
}

void NDVController::sendHeartbeatToAll()
{
    QList<NDVClientInfo> snapshot;
    {
        QMutexLocker locker(&m_mutex);
        snapshot = m_clients.values();
    }
    if (snapshot.isEmpty()) {
        return;
    }

    QByteArray packet;
    packet.append(kPrefix);
    packet.append(kHandshake);
    packet.append(kSuffix);

    const QDateTime now = QDateTime::currentDateTime();
    for (const NDVClientInfo &client : snapshot) {
        if (client.ipAddress.isEmpty()) {
            continue;
        }
        sendRawToIp(client.ipAddress, packet);

        QMutexLocker locker(&m_mutex);
        if (m_clients.contains(client.deviceId)) {
            m_clients[client.deviceId].lastHandshake = now;
        }
    }
}

void NDVController::upsertClient(const NDVClientInfo &info, bool emitStatus)
{
    bool listChanged = false;
    bool statusChanged = false;
    {
        QMutexLocker locker(&m_mutex);
        const bool existed = m_clients.contains(info.deviceId);
        const NDVClientInfo prev = existed ? m_clients.value(info.deviceId) : NDVClientInfo{};
        listChanged = !existed
            || prev.ipAddress != info.ipAddress
            || prev.online != info.online;
        statusChanged = listChanged || prev.state != info.state;
        m_clients[info.deviceId] = info;
    }
    if (listChanged) {
        Q_EMIT clientListChanged();
    }
    if (emitStatus && statusChanged) {
        Q_EMIT clientStatusChanged(info.deviceId, info);
    }
}

QString NDVController::normalizeIp(const QString &ip)
{
    QString s = ip.trimmed();
    if (s.startsWith(QLatin1String("::ffff:"), Qt::CaseInsensitive)) {
        s = s.mid(7);
    }
    return s;
}

QByteArray NDVController::commandOpcode(const QString &type) const
{
    const QString t = type.toLower();
    if (t == QLatin1String("play")) return kPlay;
    if (t == QLatin1String("stop")) return kStop;
    if (t == QLatin1String("loop")) return kLoop;
    if (t == QLatin1String("close")) return kClose;
    if (t == QLatin1String("next")) return kNext;
    if (t == QLatin1String("prev")) return kPrev;
    if (t == QLatin1String("handshake")) return kHandshake;
    if (t == QLatin1String("pause")) return kPause;
    return {};
}

QByteArray NDVController::buildCommand(const QByteArray &opcode, int fileIndex, int targetId) const
{
    QByteArray full;
    full.append(kPrefix);
    full.append(opcode);

    if (opcode == kPlay || opcode == kLoop) {
        full.append(static_cast<char>(fileIndex & 0xFF));
        full.append(static_cast<char>(0x00));
    } else if (opcode == kHandshake) {
        full.append(static_cast<char>(targetId & 0xFF));
        full.append(static_cast<char>(0x00));
    }

    full.append(kSuffix);
    return full;
}

bool NDVController::sendRawToId(int targetId, const QByteArray &packet)
{
    QString ip;
    {
        QMutexLocker locker(&m_mutex);
        if (!m_clients.contains(targetId)) {
            return false;
        }
        ip = m_clients.value(targetId).ipAddress;
    }
    return sendRawToIp(ip, packet);
}

bool NDVController::sendRawToIp(const QString &ip, const QByteArray &packet)
{
    if (ip.isEmpty() || packet.isEmpty()) {
        return false;
    }
    for (QTcpSocket *clientSocket : std::as_const(m_clientSockets)) {
        if (!clientSocket || clientSocket->state() != QAbstractSocket::ConnectedState) {
            continue;
        }
        if (hostMatches(peerHostOf(clientSocket), ip)) {
            clientSocket->write(packet);
            return true;
        }
    }
    return false;
}
