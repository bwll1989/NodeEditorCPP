// ============================================================================
//  AnchorScheduler.hpp —— anchor_time 锚点调度器（核心同步原语，§6.1）
//
//  设计依据：《media-server-sync-design.md》§6.1 / §7.1
//    核心原则：**不要"收到命令就执行"，而是"在本机对齐时钟走到
//    anchor_time 这个全场约定的未来时刻才执行"**。
//    同步 = 共享 NTP 时钟 + 把状态变更调度到一个共同的未来时刻。
//
//  职责（纯内存、无 IO，便于单元测试）：
//    * post()   ：收到 MsgReAnchor 入队（同 anchor_time 去重，新值覆盖旧值
//                 ——Editor 重发同锚点时以后到为准）
//    * poll()   ：主循环每帧调用，返回所有"到点"的锚点（最早优先）
//    * 迟到处理 ：aligned_now 已超过 anchor_time 时直接返回（迟到的
//                 anchored_session_time 仍是权威值，应用后播放头会
//                 跳到 anchored + 迟到时长，对 PLAY 是正确追帧语义）
//    * clear()  ：STOP 时清空未应用的锚点
//
//  应用逻辑（由调用方 Runtime 执行，此处只做"何时应用"的裁决）：
//      session_start = anchor_time_us - anchored_session_time_us
//      state         = cmd 对应的播放状态
// ============================================================================

#pragma once

#include <cstdint>
#include <map>
#include <optional>

namespace ms {

// 与 MsgReAnchor::Cmd 一致的独立枚举（调度器不依赖 protobuf）
enum class AnchorCmd : int {
    None  = 0,
    Play  = 1,
    Pause = 2,
    Seek  = 3,
    Stop  = 4,
};

// 待应用的锚点
struct Anchor {
    std::int64_t anchor_time_us          = 0; ///< 全场约定的应用时刻
    std::int64_t anchored_session_time_us = 0; ///< 该时刻播放头应处位置
    AnchorCmd    cmd                      = AnchorCmd::None;
};

class AnchorScheduler {
public:
    /// 锚点迟到的判定阈值：aligned_now 超过 anchor_time 视为"到点"，
    /// 无论迟到多久都立即应用（迟到的 anchored_session_time 仍是权威）
    /// 此处无需配置；调用方以 aligned_now >= anchor_time 为准。

    /// 收到重锚命令入队。
    /// \note 同一 anchor_time_us 的重复下发会覆盖（Editor 重发的幂等性）
    void post(const Anchor& anchor) {
        pending_[anchor.anchor_time_us] = anchor;
    }

    /// 取出最早的一个"已到点"锚点（anchor_time <= aligned_now）。
    /// \return nullopt 表示当前没有需要应用的锚点（最常见的每帧路径）
    std::optional<Anchor> poll(std::int64_t aligned_now_us) {
        if (pending_.empty()) return std::nullopt;
        const auto it = pending_.begin(); // map 按时间升序 → 最早锚点
        if (it->first > aligned_now_us) return std::nullopt; // 未到点
        const Anchor a = it->second;
        pending_.erase(it);
        return a;
    }

    /// 清空全部未应用锚点（CMD_STOP / 会话切换时）
    void clear() { pending_.clear(); }

    /// 是否有待应用锚点（诊断用）
    bool empty() const { return pending_.empty(); }

    /// 待应用数量（诊断用）
    std::size_t size() const { return pending_.size(); }

private:
    // key = anchor_time_us：升序排列保证"最早到点先应用"
    std::map<std::int64_t, Anchor> pending_;
};

} // namespace ms
