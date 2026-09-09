#pragma once

/**
 * @file SyncProtocol.hpp
 * @brief Sync Out / Sync In 的 TCP 行分隔 JSON 编解码。
 *
 * - 一对 Sync = 一个 TCP 端口（Out 监听 port；In 连接该 port）。
 * - Out 为服务端可一对多广播；In 为客户端。
 * - 传输：一行一条 Compact JSON（以 '\\n' 结尾）。
 * - 心跳：{"t":"hb"}，默认 2s 一次；对端 6s 无任何包则判定掉线。
 */

#include <QtCore/QByteArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonParseError>
#include <QtCore/QJsonValue>
#include <QtCore/QList>
#include <QtCore/QVariant>

#include "Common/DataTypes/NodeDataList.hpp"

namespace Nodes {
namespace SyncProtocol {

inline constexpr int DefaultPort = 9100;
inline constexpr int HeartbeatIntervalMs = 2000;
inline constexpr int HeartbeatTimeoutMs = 6000;

inline QJsonValue encodeValue(const NodeDataTypes::VariableData &data)
{
    return QJsonValue::fromVariant(data.value());
}

inline QJsonValue encodeValue(const std::shared_ptr<NodeDataTypes::VariableData> &data)
{
    if (!data)
        return QJsonValue();
    return encodeValue(*data);
}

inline std::shared_ptr<NodeDataTypes::VariableData> decodeValue(const QJsonValue &v)
{
    if (v.isUndefined() || v.isNull())
        return std::make_shared<NodeDataTypes::VariableData>();
    return std::make_shared<NodeDataTypes::VariableData>(v.toVariant());
}

/** 数据帧：{"n":4,"i":0,"v":123} */
inline QByteArray makeDataPacket(int portCount, int index, const QJsonValue &value)
{
    QJsonObject o;
    o.insert(QStringLiteral("n"), portCount);
    o.insert(QStringLiteral("i"), index);
    o.insert(QStringLiteral("v"), value);
    return QJsonDocument(o).toJson(QJsonDocument::Compact);
}

/** 口数宣告：{"n":4} */
inline QByteArray makeSchemaPacket(int portCount)
{
    QJsonObject o;
    o.insert(QStringLiteral("n"), portCount);
    return QJsonDocument(o).toJson(QJsonDocument::Compact);
}

/** 心跳：{"t":"hb"} */
inline QByteArray makeHeartbeatPacket()
{
    QJsonObject o;
    o.insert(QStringLiteral("t"), QStringLiteral("hb"));
    return QJsonDocument(o).toJson(QJsonDocument::Compact);
}

/** TCP 写：JSON + '\\n' */
inline QByteArray frameLine(const QByteArray &json)
{
    return json + '\n';
}

/** 从缓冲中取出完整行（不含 '\\n'） */
inline QList<QByteArray> takeLines(QByteArray &buffer)
{
    QList<QByteArray> lines;
    for (;;) {
        const int idx = buffer.indexOf('\n');
        if (idx < 0)
            break;
        lines.append(buffer.left(idx).trimmed());
        buffer.remove(0, idx + 1);
    }
    return lines;
}

struct Packet {
    int portCount = -1;
    int index = -1;
    QJsonValue value;
    bool hasIndex = false;
    bool isHeartbeat = false;
    bool valid = false;
};

inline Packet parsePacket(const QByteArray &bytes)
{
    Packet p;
    if (bytes.isEmpty())
        return p;
    QJsonParseError err{};
    const QJsonDocument doc = QJsonDocument::fromJson(bytes, &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return p;
    const QJsonObject o = doc.object();
    if (o.value(QStringLiteral("t")).toString() == QStringLiteral("hb")) {
        p.isHeartbeat = true;
        p.valid = true;
        return p;
    }
    p.portCount = o.value(QStringLiteral("n")).toInt(-1);
    if (o.contains(QStringLiteral("i"))) {
        p.hasIndex = true;
        p.index = o.value(QStringLiteral("i")).toInt(-1);
        p.value = o.value(QStringLiteral("v"));
    }
    p.valid = (p.portCount >= 0);
    return p;
}

} // namespace SyncProtocol
} // namespace Nodes
