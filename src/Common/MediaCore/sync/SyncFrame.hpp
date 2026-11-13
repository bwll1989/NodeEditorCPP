// ============================================================================
//  SyncFrame.hpp —— UDP 心跳帧（极简二进制，非 Protobuf）
//
//  设计依据：《media-server-sync-design.md》§6 表格：
//    "高频同步帧用极简二进制"——心跳 ~1Hz 走 UDP，56 字节定长，
//    打包/解析零依赖（不经过 Protobuf），丢包可直接忽略（下帧即最新）。
//
//  角色（§6）：
//    * Editor 定期向每个已注册 Player 单播心跳（学到注册时的来源地址）；
//    * Player 收到心跳：① 迟到加入时学习 session_start（软对齐）；
//                        ② 残余 offset 的软重锚吸收；
//    * Player 也用同一帧格式向 Editor 上报自己的 session_time 与 delta，
//      Editor 据此做健康检测与"谁偏了、偏多少"的集中观测（§7.1①）。
//
//  字节布局（ little-endian，static_assert 保证 56 字节）：
//    [0..1]   magic 'MS'   [2] version   [3] role
//    [4..19]  session_id（定长，'\0' 填充）
//    [20]     state        [21..23] pad
//    [24..31] session_start_us
//    [32..39] session_time_us   （发送方当前播放头）
//    [40..47] wall_us           （发送方对齐时钟，诊断用）
//    [48..51] frame_index       （发送方已渲染帧号）
//    [52..55] delta_us          （Player 上报 target-current；Editor 发送填 0）
// ============================================================================

#pragma once

#include <cstdint>
#include <cstring>
#include <optional>
#include <string>

namespace ms {

// 播放状态（与 MsgReAnchor::Cmd 对应但独立编码，便于 UDP 裸传输）
enum class PlayState : uint8_t {
    Idle    = 0, ///< 未开始/已停止
    Playing = 1, ///< 推进中
    Paused  = 2, ///< 冻结
};

enum class SyncRole : uint8_t {
    Editor = 1,
    Player = 2,
};

// ---------------------------------------------------------------------------
// 逻辑帧（平台无关，收发两侧的结构化视图）
// ---------------------------------------------------------------------------
struct SyncFrame {
    SyncRole   role = SyncRole::Editor;
    std::string session_id;
    PlayState   state = PlayState::Idle;
    std::int64_t session_start_us  = 0;
    std::int64_t session_time_us   = 0; ///< 发送方当前播放头
    std::int64_t wall_us           = 0; ///< 发送方对齐时钟（诊断）
    std::uint32_t frame_index      = 0;
    std::int32_t delta_us          = 0; ///< Player：target - current
};

// ---------------------------------------------------------------------------
// 线格式（pack(1) 保证无填充；static_assert 锁定 56 字节布局）
// ---------------------------------------------------------------------------
#pragma pack(push, 1)
struct SyncFrameRaw {
    std::uint16_t magic;
    std::uint8_t  version;
    std::uint8_t  role;
    char          session_id[16];
    std::uint8_t  state;
    std::uint8_t  pad[3];
    std::int64_t  session_start_us;
    std::int64_t  session_time_us;
    std::int64_t  wall_us;
    std::uint32_t frame_index;
    std::int32_t  delta_us;
};
#pragma pack(pop)

static_assert(sizeof(SyncFrameRaw) == 56,
              "SyncFrameRaw 必须为 56 字节（UDP 心跳定长布局）");

// ---------------------------------------------------------------------------
// 打包 / 解析
// ---------------------------------------------------------------------------
class SyncFrameCodec {
public:
    static constexpr std::uint16_t kMagic   = 0x4D53; // 'MS'
    static constexpr std::uint8_t  kVersion = 1;

    /// 线格式定长字节数（与 SyncFrameRaw 的 static_assert 布局一致）
    static constexpr std::size_t kWireSize = sizeof(SyncFrameRaw); // = 56

    /// 逻辑帧 → 56 字节线格式（写入 out，返回字节数恒为 56）
    static std::size_t encode(const SyncFrame& frame, void* out) noexcept;

    /// 56 字节线格式 → 逻辑帧
    /// \return nullopt 表示魔数/版本不符（静默丢弃，下帧即最新）
    static std::optional<SyncFrame> decode(const void* data,
                                           std::size_t len) noexcept;
};

} // namespace ms
