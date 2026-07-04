#pragma once

#include "PeripProtoPacket.h"
#include "perip2s.pb.h"

#include <QObject>
#include <QVariantMap>

class PeripProtoHandler : public QObject
{
    Q_OBJECT

public:
    explicit PeripProtoHandler(QObject *parent = nullptr);

public slots:
    void handlePacket(const peripheral::proto::PeripProtoPacket &packet);

signals:
    void posReceived(const QVariantMap &data);
    void pointReceived(const QVariantMap &data);
    void parseError(const QString &peerHost, int32_t cmd, const QString &reason);

    void replyRequested(const QString &peerHost, int32_t cmd, const QByteArray &body);

private:
    bool parseBody(const peripheral::proto::PeripProtoPacket &packet,
                   google::protobuf::Message &message);
    void sendReply(const QString &peerHost, int32_t cmd, const google::protobuf::Message &message);
    static QVariantMap messageToVariantMap(const google::protobuf::Message &message);
};
