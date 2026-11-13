// ============================================================================
//  FrameCodec.cpp —— TCP 分帧编解码实现
// ============================================================================

#include "net/FrameCodec.hpp"

#include <cstring>

namespace ms {

namespace {

/// 小端写入 4 字节长度（x86 原生小端；显式逐字节保证跨平台一致）
void putU32Le(std::string& out, std::uint32_t v) {
    out.push_back(static_cast<char>(v & 0xFF));
    out.push_back(static_cast<char>((v >> 8) & 0xFF));
    out.push_back(static_cast<char>((v >> 16) & 0xFF));
    out.push_back(static_cast<char>((v >> 24) & 0xFF));
}

/// 小端读取 4 字节长度
std::uint32_t readU32Le(const char* p) {
    return static_cast<std::uint32_t>(static_cast<unsigned char>(p[0])) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(p[1])) << 8) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(p[2])) << 16) |
           (static_cast<std::uint32_t>(static_cast<unsigned char>(p[3])) << 24);
}

} // namespace

std::string FrameCodec::encodePayload(const ms_proto::Envelope& envelope) {
    return envelope.SerializeAsString();
}

std::string FrameCodec::encode(const ms_proto::Envelope& envelope) {
    const std::string payload = encodePayload(envelope);
    std::string frame;
    frame.reserve(kHeaderSize + payload.size());
    putU32Le(frame, static_cast<std::uint32_t>(payload.size()));
    frame.append(payload);
    return frame;
}

int FrameCodec::tryExtract(std::string& buffer, ms_proto::Envelope& out) {
    // 半包：不足一个长度头
    if (buffer.size() < kHeaderSize) return 0;

    const std::uint32_t len = readU32Le(buffer.data());

    // 长度非法：流已损坏（对端协议错误/恶意输入）→ 调用方必须断开
    if (len == 0 || len > kMaxPayload) return -1;

    // 半包：payload 还没收全
    if (buffer.size() < kHeaderSize + len) return 0;

    // 完整帧：解析 payload 并从缓冲头部消费掉
    const bool ok = out.ParseFromArray(buffer.data() + kHeaderSize,
                                       static_cast<int>(len));
    buffer.erase(0, kHeaderSize + len);
    return ok ? 1 : -1;
}

} // namespace ms
