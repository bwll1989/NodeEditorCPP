// ============================================================================
//  SyncFrame.cpp —— UDP 心跳帧打包/解析实现
//  设计要点见 SyncFrame.hpp 文件头；此处只做字段级 memcpy 与合法性校验。
// ============================================================================

#include "sync/SyncFrame.hpp"

#include <algorithm>

namespace ms {

namespace {
// 定长字符串拷贝：源超长截断，目标保证 '\0' 结尾（诊断可读）
void copySessionId(char (&dst)[16], const std::string& src) noexcept {
    const std::size_t n = std::min(src.size(), sizeof(dst) - 1);
    std::memcpy(dst, src.data(), n);
    std::memset(dst + n, 0, sizeof(dst) - n);
}
} // namespace

std::size_t SyncFrameCodec::encode(const SyncFrame& f, void* out) noexcept {
    SyncFrameRaw raw{};
    raw.magic   = kMagic;
    raw.version = kVersion;
    raw.role    = static_cast<std::uint8_t>(f.role);
    copySessionId(raw.session_id, f.session_id);
    raw.state           = static_cast<std::uint8_t>(f.state);
    raw.session_start_us = f.session_start_us;
    raw.session_time_us  = f.session_time_us;
    raw.wall_us          = f.wall_us;
    raw.frame_index      = f.frame_index;
    raw.delta_us         = f.delta_us;
    std::memcpy(out, &raw, sizeof(raw));
    return sizeof(raw);
}

std::optional<SyncFrame> SyncFrameCodec::decode(const void* data,
                                                std::size_t len) noexcept {
    if (len != sizeof(SyncFrameRaw)) return std::nullopt; // 定长校验

    SyncFrameRaw raw{};
    std::memcpy(&raw, data, sizeof(raw));
    if (raw.magic != kMagic)   return std::nullopt; // 非本协议包（静默丢弃）
    if (raw.version != kVersion) return std::nullopt; // 版本不兼容
    // 【值域校验】state/role 超出枚举定义域的帧拒绝——裸 UDP 可被任意
    // 伪造，非法枚举值进入 switch/比较是未定义行为源
    if (raw.state > static_cast<std::uint8_t>(PlayState::Paused))
        return std::nullopt;
    if (raw.role != static_cast<std::uint8_t>(SyncRole::Editor) &&
        raw.role != static_cast<std::uint8_t>(SyncRole::Player))
        return std::nullopt;

    SyncFrame f;
    f.role = static_cast<SyncRole>(raw.role);
    raw.session_id[sizeof(raw.session_id) - 1] = '\0'; // 防御：保证截断
    f.session_id = raw.session_id;
    f.state      = static_cast<PlayState>(raw.state);
    f.session_start_us = raw.session_start_us;
    f.session_time_us  = raw.session_time_us;
    f.wall_us          = raw.wall_us;
    f.frame_index      = raw.frame_index;
    f.delta_us         = raw.delta_us;
    return f;
}

} // namespace ms
