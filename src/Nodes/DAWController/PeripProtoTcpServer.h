#pragma once

#include "PeripProtoPacket.h"

#include <QObject>
#include <QThread>

class PeripProtoTcpServer : public QObject
{
    Q_OBJECT

public:
    explicit PeripProtoTcpServer(QString host = QStringLiteral("0.0.0.0"),
                                 int port = 9001,
                                 QObject *parent = nullptr);
    ~PeripProtoTcpServer() override;

public slots:
    void initializeServer();
    void cleanup();
    void setHost(const QString &host, int port);
    void sendPacket(int32_t cmd, const QByteArray &body);
    void sendPacketToHost(const QString &host, int32_t cmd, const QByteArray &body);
    void sendMessage(const QString &host, int32_t cmd, const google::protobuf::Message &message);
    void broadcastMessage(int32_t cmd, const google::protobuf::Message &message);

signals:
    void isReady(bool ready);
    void packetReceived(const peripheral::proto::PeripProtoPacket &packet);
    void clientConnected(const QString &host);
    void clientDisconnected(const QString &host);

    void initializeRequested(const QString &host, int port);
    void cleanupRequested();
    void sendPacketRequested(const QByteArray &packet);
    void sendPacketToHostRequested(const QString &host, const QByteArray &packet);

private:
    QString mHost;
    int mPort = 9001;
    QThread *mThread = nullptr;
};
