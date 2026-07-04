#include "PeripProtoHandler.h"

#include <QJsonDocument>
#include <QJsonObject>

#include <google/protobuf/util/json_util.h>

PeripProtoHandler::PeripProtoHandler(QObject *parent)
    : QObject(parent)
{
}

bool PeripProtoHandler::parseBody(const peripheral::proto::PeripProtoPacket &packet,
                                  google::protobuf::Message &message)
{
    if (packet.body.isEmpty()) {
        return true;
    }

    if (!message.ParseFromArray(packet.body.constData(), packet.body.size())) {
        emit parseError(packet.peerHost, packet.cmd, QStringLiteral("protobuf parse failed"));
        return false;
    }
    return true;
}

QVariantMap PeripProtoHandler::messageToVariantMap(const google::protobuf::Message &message)
{
    std::string json;
    google::protobuf::util::JsonPrintOptions options;
    options.always_print_fields_with_no_presence = true;
    const auto status = google::protobuf::util::MessageToJsonString(message, &json, options);
    if (!status.ok()) {
        return {};
    }

    QJsonParseError error;
    const QJsonDocument doc = QJsonDocument::fromJson(QByteArray::fromStdString(json), &error);
    if (error.error != QJsonParseError::NoError || !doc.isObject()) {
        return {};
    }
    return doc.object().toVariantMap();
}

void PeripProtoHandler::sendReply(const QString &peerHost,
                                  int32_t cmd,
                                  const google::protobuf::Message &message)
{
    const std::string serialized = message.SerializeAsString();
    emit replyRequested(peerHost,
                        cmd,
                        QByteArray(serialized.data(), static_cast<int>(serialized.size())));
}

void PeripProtoHandler::handlePacket(const peripheral::proto::PeripProtoPacket &packet)
{
    using namespace peripheral::proto;

    switch (static_cast<CmdType>(packet.cmd)) {
    case CMD_HEARTBEAT: {
        Heartbeat msg;
        if (!parseBody(packet, msg)) {
            return;
        }
        Heartbeat reply;
        sendReply(packet.peerHost, CMD_HEARTBEAT, reply);
        break;
    }
    case CMD_P2S_BOAT_POSITIONING_SYS_REGISTER: {
        P2SBoatPositioningSysRegister msg;
        if (!parseBody(packet, msg)) {
            return;
        }
        S2PBoatPositioningSysRegister reply;
        reply.set_ret(1);
        sendReply(packet.peerHost, CMD_S2P_BOAT_POSITIONING_SYS_REGISTER, reply);
        break;
    }
    case CMD_P2S_BOAT_POSITIONING_SYS_UPDATE_POS: {
        P2SBoatPositioningSysUpdatePos msg;
        if (!parseBody(packet, msg)) {
            return;
        }
        emit posReceived(messageToVariantMap(msg));
        break;
    }
    case CMD_P2S_BOAT_POSITIONING_SYS_UPDATE_POINT: {
        P2SBoatPositioningSysUpdatePoint msg;
        if (!parseBody(packet, msg)) {
            return;
        }
        emit pointReceived(messageToVariantMap(msg));
        break;
    }
    default:
        emit parseError(packet.peerHost,
                        packet.cmd,
                        QStringLiteral("unsupported command"));
        break;
    }
}
