#pragma once

#include <QByteArray>
#include <QString>
#include <QVariantMap>
#include <QVariantList>
#include <array>
#include <vector>

namespace Nodes {
namespace AJTProtocol {

constexpr int kDimChannelCount = 6;
constexpr int kRelayChannelCount = 12;
constexpr int kRelayPackedBytes = 3; // 每字节 4 路 × 2bit

constexpr quint8 kTypeMultiDim = 0xF2;
constexpr quint8 kTypeMultiRelay = 0xF1;
constexpr quint8 kFuncMulti = 0x15;
constexpr quint8 kFrameDstAddr = 0x00; // 多设备帧固定目的地址（广播）
/// 厂家：500ms 内多条指令会丢包 → 同类型 500ms 去重合并；异类型至少间隔 500ms
constexpr int kSendDebounceMs = 500;
constexpr int kMinSendGapMs = 500;

// 兼容旧名
constexpr int kChannelCount = kDimChannelCount;
constexpr quint8 kFuncMultiDim = kFuncMulti;

inline constexpr const char *kKindDim = "dim";
inline constexpr const char *kKindRelay = "relay";

/// 通道开关编码：00 无效 / 01 开 / 10 关
enum class SwitchCode : quint8 {
    Invalid = 0b00,
    On = 0b01,
    Off = 0b10,
};

struct DeviceState {
    QString kind = QString::fromLatin1(kKindDim);
    int id = 0x15;
    bool enable = false;
    /// dim: [0..5] 亮度 0～255；relay: [0..11] 0=关 1=开
    std::array<int, kRelayChannelCount> channels{};
};

inline int clampLevel(int level)
{
    if (level < 0) return 0;
    if (level > 255) return 255;
    return level;
}

inline int clampSwitch(int value)
{
    return value ? 1 : 0;
}

inline bool isRelay(const DeviceState &dev)
{
    return dev.kind == QLatin1String(kKindRelay);
}

inline bool isDim(const DeviceState &dev)
{
    return !isRelay(dev);
}

inline QVariantMap toVariantMap(const DeviceState &dev)
{
    const int count = isRelay(dev) ? kRelayChannelCount : kDimChannelCount;
    QVariantList ch;
    ch.reserve(count);
    for (int i = 0; i < count; ++i) {
        ch.append(dev.channels[i]);
    }
    QVariantMap map;
    map.insert(QStringLiteral("kind"), dev.kind);
    map.insert(QStringLiteral("id"), dev.id);
    map.insert(QStringLiteral("enable"), dev.enable);
    map.insert(QStringLiteral("channels"), ch);
    return map;
}

inline bool fromVariantMap(const QVariantMap &map, DeviceState &out)
{
    if (map.isEmpty()) {
        return false;
    }
    out.kind = map.value(QStringLiteral("kind"), QString::fromLatin1(kKindDim)).toString();
    if (out.kind.isEmpty()) {
        out.kind = QString::fromLatin1(kKindDim);
    }
    out.id = map.value(QStringLiteral("id"), 0).toInt() & 0xFF;
    out.enable = map.value(QStringLiteral("enable"), false).toBool();
    out.channels.fill(0);

    const QVariantList ch = map.value(QStringLiteral("channels")).toList();
    const int count = isRelay(out) ? kRelayChannelCount : kDimChannelCount;
    for (int i = 0; i < count && i < ch.size(); ++i) {
        out.channels[i] = isRelay(out) ? clampSwitch(ch[i].toInt())
                                       : clampLevel(ch[i].toInt());
    }
    return true;
}

inline QByteArray buildFrame(quint8 src, quint8 dst, quint8 type, quint8 func,
                             const QByteArray &payload)
{
    QByteArray body;
    body.reserve(4 + payload.size());
    body.append(static_cast<char>(src));
    body.append(static_cast<char>(dst));
    body.append(static_cast<char>(type));
    body.append(static_cast<char>(func));
    body.append(payload);

    const quint8 len = static_cast<quint8>(body.size() + 1);
    quint16 sum = len;
    for (int i = 0; i < body.size(); ++i) {
        sum += static_cast<quint8>(body.at(i));
    }

    QByteArray frame;
    frame.reserve(body.size() + 4);
    frame.append(static_cast<char>(0xF7));
    frame.append(static_cast<char>(len));
    frame.append(body);
    frame.append(static_cast<char>(sum & 0xFF));
    frame.append(static_cast<char>(0xFD));
    return frame;
}

/// F7 | LEN | SRC | DST | F2 | 15 | (ID CH1..CH6)*N | CSUM | FD
inline QByteArray buildMultiDimFrame(quint8 src, quint8 dst,
                                     const std::vector<DeviceState> &devices)
{
    QByteArray payload;
    payload.reserve(static_cast<int>(devices.size()) * (1 + kDimChannelCount));
    for (const DeviceState &dev : devices) {
        if (!isDim(dev)) {
            continue;
        }
        payload.append(static_cast<char>(dev.id & 0xFF));
        for (int i = 0; i < kDimChannelCount; ++i) {
            const int level = dev.enable ? clampLevel(dev.channels[i]) : 0;
            payload.append(static_cast<char>(level));
        }
    }
    if (payload.isEmpty()) {
        return {};
    }
    return buildFrame(src, dst, kTypeMultiDim, kFuncMulti, payload);
}

/// 将 CH(base+1..base+4) 打成一字节：高位对为 CH4… 低位对为 CH1…（相对 base）
inline quint8 packRelayNibble(const DeviceState &dev, int base)
{
    auto code = [&](int index) -> quint8 {
        if (!dev.enable) {
            return static_cast<quint8>(SwitchCode::Off);
        }
        return dev.channels[base + index]
                   ? static_cast<quint8>(SwitchCode::On)
                   : static_cast<quint8>(SwitchCode::Off);
    };
    // 字节内顺序：4.3.2.1 → bits76,54,32,10
    return static_cast<quint8>((code(3) << 6) | (code(2) << 4) | (code(1) << 2) | code(0));
}

/// F7 | LEN | SRC | DST | F1 | 15 | (ID B0 B1 B2)*N | CSUM | FD
inline QByteArray buildMultiRelayFrame(quint8 src, quint8 dst,
                                       const std::vector<DeviceState> &devices)
{
    QByteArray payload;
    payload.reserve(static_cast<int>(devices.size()) * (1 + kRelayPackedBytes));
    for (const DeviceState &dev : devices) {
        if (!isRelay(dev)) {
            continue;
        }
        payload.append(static_cast<char>(dev.id & 0xFF));
        payload.append(static_cast<char>(packRelayNibble(dev, 0)));  // CH1～4
        payload.append(static_cast<char>(packRelayNibble(dev, 4)));  // CH5～8
        payload.append(static_cast<char>(packRelayNibble(dev, 8)));  // CH9～12
    }
    if (payload.isEmpty()) {
        return {};
    }
    return buildFrame(src, dst, kTypeMultiRelay, kFuncMulti, payload);
}

} // namespace AJTProtocol
} // namespace Nodes
