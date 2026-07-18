#pragma once

#include <QByteArray>
#include <QJsonObject>
#include <QString>
#include <QVector>

/**
 * @brief 安卓播放器（OMV）TCP 帧协议：0xA0 + 大端长度 + (typeDigit + JSON)
 *
 * 业务 JSON 由调用方决定；本模块仅提供组包/解包与握手帧约定。
 */
namespace omv {

constexpr char kFrameHeader = static_cast<char>(0xA0);
constexpr int kDefaultPort = 9090;
constexpr int kKeepAliveIntervalMs = 4000;

enum class FrameKind {
    Unknown,   ///< 普通业务帧 / 未识别
    Handshake  ///< 握手登记相关
};

struct ParsedFrame {
    FrameKind kind = FrameKind::Unknown;
    int deviceId = -1;       ///< 握手成功时尽量解析出的设备 ID
    char typeDigit = 0;      ///< Payload 首位类型字符（常见 '1'/'2'）
    QByteArray rawFrame;
    QByteArray payload;
    QJsonObject json;
};

/** @brief 组包：payload = typeDigit + JSON；frame = 0xA0 + be16(len) + payload */
QByteArray encodeFrame(char typeDigit, const QJsonObject &json);

/** @brief 组包：直接封装调用方提供的 JSON 对象（默认 typeDigit='1'） */
QByteArray encodeJson(const QJsonObject &json, char typeDigit = '1');

/** @brief 已知握手/心跳帧：{"tp":"1","com":"9","pm":""} */
QByteArray encodeHandshake();

/**
 * @brief 从缓冲中尽量解出完整帧（粘包/半包）
 * 未完整的部分留在 buffer
 */
void tryDecode(QByteArray &buffer, QVector<ParsedFrame> &out);

/** @brief 解析单帧：提取 JSON；仅额外识别握手特征 */
ParsedFrame classifyFrame(const QByteArray &frame);

} // namespace omv
