//
// Created by bwll1 on 2024/9/1.
//

#include "FTDAWController.h"
#include <QMutexLocker>
#include <QMetaType>

FTDAWController* FTDAWController::s_instance = nullptr;
int FTDAWController::s_refCount = 0;
QMutex FTDAWController::s_refMutex;

/**
 * 函数级注释：返回当前全局单例指针（不改变引用计数）
 */
FTDAWController* FTDAWController::instance()
{
    QMutexLocker locker(&s_refMutex);
    return s_instance;
}

/**
 * 函数级注释：获取或创建全局单例并增加引用计数
 */
FTDAWController* FTDAWController::acquire()
{
    QMutexLocker locker(&s_refMutex);
    if (!s_instance) {
        s_instance = new FTDAWController();
    }
    ++s_refCount;
    return s_instance;
}

/**
 * 函数级注释：释放一次引用计数，为零时销毁全局单例
 */
void FTDAWController::release()
{
    FTDAWController* toDelete = nullptr;
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

FTDAWController::FTDAWController(QObject *parent)
    : QObject(parent)
    , m_client(nullptr)
    , m_heartbeatTimer(new QTimer(this))
    , m_host(QStringLiteral("127.0.0.1"))
{
    qRegisterMetaType<QVariantMap>("QVariantMap");

    m_heartbeatTimer->setInterval(2000);
    connect(m_heartbeatTimer, &QTimer::timeout, this, &FTDAWController::sendHeartbeat);

    m_client = new TcpClient(m_host, FIXED_PORT, this);

    connect(m_client, &TcpClient::isReady,
            this, &FTDAWController::isReady,
            Qt::QueuedConnection);
    connect(m_client, &TcpClient::isReady,
            this, &FTDAWController::onConnectionStateChanged,
            Qt::QueuedConnection);
    connect(m_client, &TcpClient::recMsg,
            this, &FTDAWController::recMsg,
            Qt::QueuedConnection);
}

FTDAWController::~FTDAWController()
{
    QMutexLocker locker(&m_mutex);
    if (m_client) {
        m_client->disconnectFromServer();
        m_client = nullptr;
    }
}

void FTDAWController::connectToServer(const QString &host)
{
    QMutexLocker locker(&m_mutex);
    m_host = host;
    if (!m_client) {
        m_client = new TcpClient(m_host, FIXED_PORT, this);

        connect(m_client, &TcpClient::isReady,
                this, &FTDAWController::isReady,
                Qt::QueuedConnection);
        connect(m_client, &TcpClient::isReady,
                this, &FTDAWController::onConnectionStateChanged,
                Qt::QueuedConnection);
        connect(m_client, &TcpClient::recMsg,
                this, &FTDAWController::recMsg,
                Qt::QueuedConnection);
                emit onHostChanged(m_host);
    } else {
        m_client->connectToServer(m_host, FIXED_PORT);
        emit onHostChanged(m_host);
    }
}

void FTDAWController::disconnectFromServer()
{
    QMutexLocker locker(&m_mutex);
    if (m_client) {
        m_client->disconnectFromServer();
    }
}

void FTDAWController::sendMessage(const QString &message, int format)
{
    QMutexLocker locker(&m_mutex);
    if (m_client) {
        m_client->sendMessage(message, format);
    }
}

void FTDAWController::sendHeartbeat()
{
    sendMessage(QStringLiteral("heartbeat"), 1);
}

void FTDAWController::onConnectionStateChanged(bool ready)
{
    if (ready) {
        if (!m_heartbeatTimer->isActive()) {
            m_heartbeatTimer->start();
        }
    } else {
        m_heartbeatTimer->stop();
    }
}
