#include "PeripProtoPacket.h"

#include <QDataStream>
#include <QIODevice>

namespace peripheral {
namespace proto {

QByteArray PeripProtoCodec::encode(int32_t cmd, const QByteArray &body)
{
    const quint32 dataLen = static_cast<quint32>(kMsgIdFieldSize + body.size());

    QByteArray packet;
    QDataStream stream(&packet, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream << dataLen << static_cast<quint32>(cmd);
    if (!body.isEmpty()) {
        stream.writeRawData(body.constData(), body.size());
    }
    return packet;
}

QByteArray PeripProtoCodec::encode(int32_t cmd, const google::protobuf::Message &message)
{
    std::string serialized;
    message.SerializeToString(&serialized);
    return encode(cmd, QByteArray(serialized.data(), static_cast<int>(serialized.size())));
}

int PeripProtoCodec::tryDecode(QByteArray &buffer, PeripProtoPacket &out)
{
    while (buffer.size() >= kDataLenFieldSize) {
        QDataStream stream(buffer);
        stream.setByteOrder(QDataStream::BigEndian);

        quint32 dataLen = 0;
        stream >> dataLen;

        if (dataLen < kMinDataLen || dataLen > kMaxBodySize + kMsgIdFieldSize) {
            buffer.remove(0, kDataLenFieldSize);
            return -1;
        }

        const int packetSize = kDataLenFieldSize + static_cast<int>(dataLen);
        if (buffer.size() < packetSize) {
            return 0;
        }

        quint32 msgId = 0;
        stream.device()->seek(kDataLenFieldSize);
        stream >> msgId;

        const int bodySize = static_cast<int>(dataLen) - kMsgIdFieldSize;
        out.cmd = static_cast<int32_t>(msgId);
        out.body = buffer.mid(kDataLenFieldSize + kMsgIdFieldSize, bodySize);
        buffer.remove(0, packetSize);
        return packetSize;
    }

    return 0;
}

} // namespace proto
} // namespace peripheral
