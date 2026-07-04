#pragma once

#include <QByteArray>
#include <qmetatype.h>
#include <QString>

#include <google/protobuf/message.h>

namespace peripheral {
namespace proto {

struct PeripProtoPacket
{
    int32_t cmd = 0;
    QByteArray body;
    QString peerHost;
};

/**
 * @brief perip2s TCP 帧格式（大端序）
 * [4 data_len][4 msg_id][msg_body]
 * data_len = msg_id(4) + msg_body.len
 */
class PeripProtoCodec
{
public:
    static constexpr int kDataLenFieldSize = 4;
    static constexpr int kMsgIdFieldSize = 4;
    static constexpr int kMinDataLen = kMsgIdFieldSize;
    static constexpr uint32_t kMaxBodySize = 1024 * 1024;

    static QByteArray encode(int32_t cmd, const QByteArray &body);
    static QByteArray encode(int32_t cmd, const google::protobuf::Message &message);

    // 返回已消费字节数；0 表示数据不完整；-1 表示当前帧无效并已丢弃
    static int tryDecode(QByteArray &buffer, PeripProtoPacket &out);
};

} // namespace proto
} // namespace peripheral

Q_DECLARE_METATYPE(peripheral::proto::PeripProtoPacket)
