#include "PeripProtoTcpServer.h"

#include <QHash>
#include <QHostAddress>
#include <QTcpServer>
#include <QTcpSocket>
#include <QDebug>

namespace {

class PeripProtoTcpWorker : public QObject
{
    Q_OBJECT

public:
    explicit PeripProtoTcpWorker(QObject *parent = nullptr)
        : QObject(parent)
    {
    }

    ~PeripProtoTcpWorker() override
    {
        cleanup();
    }

public slots:
    void initialize(const QString &host, int port)
    {
        cleanup();

        mServer = new QTcpServer(this);
        connect(mServer, &QTcpServer::newConnection, this, &PeripProtoTcpWorker::onNewConnection);

        const QHostAddress address = host.isEmpty() ? QHostAddress::Any : QHostAddress(host);
        if (mServer->listen(address, static_cast<quint16>(port))) {
            qDebug() << "PeripProtoTcpServer listening on" << host << port;
            emit serverReady(true);
        } else {
            qWarning() << "PeripProtoTcpServer failed to listen on" << host << port
                       << mServer->errorString();
            emit serverReady(false);
        }
    }

    void cleanup()
    {
        for (QTcpSocket *clientSocket : mClientSockets) {
            if (clientSocket->isOpen()) {
                clientSocket->close();
            }
            clientSocket->deleteLater();
        }
        mClientSockets.clear();
        mReceiveBuffers.clear();

        if (mServer) {
            mServer->close();
            mServer->deleteLater();
            mServer = nullptr;
        }
    }

    void sendPacketToClients(const QByteArray &packet)
    {
        for (QTcpSocket *clientSocket : mClientSockets) {
            clientSocket->write(packet);
        }
    }

    void sendPacketToClient(const QString &host, const QByteArray &packet)
    {
        for (QTcpSocket *clientSocket : mClientSockets) {
            if (clientSocket->peerAddress().toString() == host) {
                clientSocket->write(packet);
                return;
            }
        }
        qDebug() << "PeripProtoTcpServer host not found:" << host;
    }

private slots:
    void onNewConnection()
    {
        while (mServer && mServer->hasPendingConnections()) {
            QTcpSocket *clientSocket = mServer->nextPendingConnection();
            const QString peerHost = clientSocket->peerAddress().toString();
            qDebug() << "PeripProtoTcpServer client connected:" << peerHost;

            connect(clientSocket, &QTcpSocket::readyRead, this, &PeripProtoTcpWorker::onReadyRead);
            connect(clientSocket, &QTcpSocket::disconnected, this, &PeripProtoTcpWorker::onDisconnected);

            mClientSockets.append(clientSocket);
            mReceiveBuffers.insert(clientSocket, QByteArray());
            emit clientConnected(peerHost);
        }
    }

    void onReadyRead()
    {
        auto *clientSocket = qobject_cast<QTcpSocket *>(sender());
        if (!clientSocket) {
            return;
        }

        QByteArray &buffer = mReceiveBuffers[clientSocket];
        buffer.append(clientSocket->readAll());

        peripheral::proto::PeripProtoPacket packet;
        int consumed = 0;
        while ((consumed = peripheral::proto::PeripProtoCodec::tryDecode(buffer, packet)) != 0) {
            if (consumed < 0) {
                continue;
            }
            packet.peerHost = clientSocket->peerAddress().toString();
            emit packetReceived(packet);
        }
    }

    void onDisconnected()
    {
        auto *clientSocket = qobject_cast<QTcpSocket *>(sender());
        if (!clientSocket) {
            return;
        }

        const QString peerHost = clientSocket->peerAddress().toString();
        mClientSockets.removeAll(clientSocket);
        mReceiveBuffers.remove(clientSocket);
        clientSocket->deleteLater();
        emit clientDisconnected(peerHost);
    }

signals:
    void serverReady(bool ready);
    void packetReceived(const peripheral::proto::PeripProtoPacket &packet);
    void clientConnected(const QString &host);
    void clientDisconnected(const QString &host);

private:
    QTcpServer *mServer = nullptr;
    QList<QTcpSocket *> mClientSockets;
    QHash<QTcpSocket *, QByteArray> mReceiveBuffers;
};

} // namespace

PeripProtoTcpServer::PeripProtoTcpServer(QString host, int port, QObject *parent)
    : QObject(parent)
    , mHost(std::move(host))
    , mPort(port)
{
    qRegisterMetaType<peripheral::proto::PeripProtoPacket>();

    mThread = new QThread(this);
    auto *worker = new PeripProtoTcpWorker();
    worker->moveToThread(mThread);

    connect(mThread, &QThread::started, this, [this]() {
        emit initializeRequested(mHost, mPort);
    });
    connect(mThread, &QThread::finished, worker, &QObject::deleteLater);

    connect(worker, &PeripProtoTcpWorker::serverReady, this, &PeripProtoTcpServer::isReady);
    connect(worker, &PeripProtoTcpWorker::packetReceived, this, &PeripProtoTcpServer::packetReceived);
    connect(worker, &PeripProtoTcpWorker::clientConnected, this, &PeripProtoTcpServer::clientConnected);
    connect(worker, &PeripProtoTcpWorker::clientDisconnected, this, &PeripProtoTcpServer::clientDisconnected);

    connect(this, &PeripProtoTcpServer::initializeRequested, worker, &PeripProtoTcpWorker::initialize);
    connect(this, &PeripProtoTcpServer::cleanupRequested, worker, &PeripProtoTcpWorker::cleanup);
    connect(this, &PeripProtoTcpServer::sendPacketRequested, worker, &PeripProtoTcpWorker::sendPacketToClients);
    connect(this, &PeripProtoTcpServer::sendPacketToHostRequested, worker, &PeripProtoTcpWorker::sendPacketToClient);

    mThread->start();
}

PeripProtoTcpServer::~PeripProtoTcpServer()
{
    emit cleanupRequested();
    mThread->quit();
    mThread->wait();
}

void PeripProtoTcpServer::initializeServer()
{
    emit initializeRequested(mHost, mPort);
}

void PeripProtoTcpServer::cleanup()
{
    emit cleanupRequested();
}

void PeripProtoTcpServer::setHost(const QString &host, int port)
{
    mHost = host;
    mPort = port;
    emit initializeRequested(mHost, mPort);
}

void PeripProtoTcpServer::sendPacket(int32_t cmd, const QByteArray &body)
{
    emit sendPacketRequested(peripheral::proto::PeripProtoCodec::encode(cmd, body));
}

void PeripProtoTcpServer::sendPacketToHost(const QString &host, int32_t cmd, const QByteArray &body)
{
    emit sendPacketToHostRequested(host, peripheral::proto::PeripProtoCodec::encode(cmd, body));
}

void PeripProtoTcpServer::sendMessage(const QString &host, int32_t cmd, const google::protobuf::Message &message)
{
    sendPacketToHost(host, cmd, QByteArray(message.SerializeAsString().data(),
                                           static_cast<int>(message.ByteSizeLong())));
}

void PeripProtoTcpServer::broadcastMessage(int32_t cmd, const google::protobuf::Message &message)
{
    sendPacket(cmd, QByteArray(message.SerializeAsString().data(),
                               static_cast<int>(message.ByteSizeLong())));
}

#include "PeripProtoTcpServer.moc"
