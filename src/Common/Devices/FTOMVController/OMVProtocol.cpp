#include "OMVProtocol.h"

#include <QJsonDocument>
#include <QtEndian>

namespace omv {
namespace {

int extractDeviceIdFromHandshakeOffsets(const QByteArray &frame)
{
    // 抓包归纳：1-based 第 32 字节起为 ASCII 设备 ID
    if (frame.size() < 32) {
        return -1;
    }
    const char first = frame.at(31);
    if (first < '0' || first > '9') {
        return -1;
    }

    if (frame.size() >= 33) {
        const char second = frame.at(32);
        if (second >= '0' && second <= '9') {
            return (first - '0') * 10 + (second - '0');
        }
        if (second == '"') {
            return first - '0';
        }
    }
    return first - '0';
}

int extractDeviceIdFromJson(const QJsonObject &json)
{
    const QStringList keys = {
        QStringLiteral("name"),
        QStringLiteral("id"),
        QStringLiteral("ID"),
        QStringLiteral("na"),
        QStringLiteral("pm")
    };
    for (const QString &key : keys) {
        if (!json.contains(key)) {
            continue;
        }
        const QString value = json.value(key).toString().trimmed();
        if (value.isEmpty()) {
            continue;
        }
        bool ok = false;
        const int id = value.toInt(&ok);
        if (ok && id >= 0) {
            return id;
        }
    }
    return -1;
}

bool looksLikeHandshake(const QByteArray &frame, const QJsonObject &json)
{
    // 偏移特征：1-based 27–28 = "na"
    if (frame.size() >= 28
        && static_cast<unsigned char>(frame.at(26)) == 0x6E
        && static_cast<unsigned char>(frame.at(27)) == 0x61) {
        return true;
    }
    // JSON 特征：com=9
    if (json.value(QStringLiteral("com")).toString() == QLatin1String("9")) {
        return true;
    }
    return false;
}

} // namespace

QByteArray encodeFrame(char typeDigit, const QJsonObject &json)
{
    const QByteArray jsonBytes = QJsonDocument(json).toJson(QJsonDocument::Compact);
    QByteArray payload;
    payload.reserve(1 + jsonBytes.size());
    payload.append(typeDigit);
    payload.append(jsonBytes);

    QByteArray frame;
    frame.reserve(3 + payload.size());
    frame.append(kFrameHeader);

    char lengthBytes[2] = {};
    qToBigEndian(static_cast<quint16>(payload.size()), lengthBytes);
    frame.append(lengthBytes, 2);
    frame.append(payload);
    return frame;
}

QByteArray encodeJson(const QJsonObject &json, char typeDigit)
{
    return encodeFrame(typeDigit, json);
}

QByteArray encodeHandshake()
{
    QJsonObject obj;
    obj.insert(QStringLiteral("tp"), QStringLiteral("1"));
    obj.insert(QStringLiteral("com"), QStringLiteral("9"));
    obj.insert(QStringLiteral("pm"), QString());
    return encodeFrame('1', obj);
}

ParsedFrame classifyFrame(const QByteArray &frame)
{
    ParsedFrame parsed;
    parsed.rawFrame = frame;

    if (frame.size() >= 3 && frame.at(0) == kFrameHeader) {
        const quint16 length = qFromBigEndian<quint16>(
            reinterpret_cast<const uchar *>(frame.constData() + 1));
        if (frame.size() >= 3 + length) {
            parsed.payload = frame.mid(3, length);
        }
    } else if (!frame.isEmpty()) {
        parsed.payload = frame;
    }

    if (!parsed.payload.isEmpty()) {
        parsed.typeDigit = parsed.payload.at(0);
        if (parsed.payload.size() >= 2) {
            const QJsonDocument doc = QJsonDocument::fromJson(parsed.payload.mid(1));
            if (doc.isObject()) {
                parsed.json = doc.object();
            }
        }
    }

    if (looksLikeHandshake(frame, parsed.json)) {
        parsed.kind = FrameKind::Handshake;
        parsed.deviceId = extractDeviceIdFromJson(parsed.json);
        if (parsed.deviceId < 0) {
            parsed.deviceId = extractDeviceIdFromHandshakeOffsets(frame);
        }
    }

    return parsed;
}

void tryDecode(QByteArray &buffer, QVector<ParsedFrame> &out)
{
    out.clear();

    while (true) {
        const int headerIndex = buffer.indexOf(kFrameHeader);
        if (headerIndex < 0) {
            buffer.clear();
            return;
        }
        if (headerIndex > 0) {
            buffer.remove(0, headerIndex);
        }
        if (buffer.size() < 3) {
            return;
        }

        const quint16 length = qFromBigEndian<quint16>(
            reinterpret_cast<const uchar *>(buffer.constData() + 1));
        const int frameSize = 3 + static_cast<int>(length);
        if (frameSize < 3 || length > 64 * 1024) {
            buffer.remove(0, 1);
            continue;
        }
        if (buffer.size() < frameSize) {
            return;
        }

        const QByteArray frame = buffer.left(frameSize);
        buffer.remove(0, frameSize);
        out.push_back(classifyFrame(frame));
    }
}

} // namespace omv
