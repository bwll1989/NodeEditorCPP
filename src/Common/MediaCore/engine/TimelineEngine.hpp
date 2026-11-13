// ============================================================================
//  TimelineEngine.hpp —— 播放头解算纯函数（Engine Core 组件）
//
//  设计依据：《media-server-sync-design.md》§4.4 / §5.2 / §7
//    * evaluate(t) 是【纯函数】：给定播放头 t，输出"此刻该渲染什么"的确定性
//      结果——无内部状态、无副作用、不触碰时钟/IO；这是 Editor 预览与
//      Player 渲染行为一致的根基（§4.4 "关键不变量"）；
//    * 主时钟只决定 t 每秒增长多少（session_time = aligned_now - session_start，
//      §4.4），本引擎不关心时间从哪来——时钟逻辑在 clock/ClockProvider；
//    * 无外部依赖（设计文档 §2.2 组件表："TimelineEngine — 无外部依赖"）。
//
//  使用方式（两端同源）：
//    Editor(Player 角色, 全画布预览) 与 Player(单 screen) 各自持有 Scene，
//    每帧用自己的对齐时钟算出 t 后调用 evaluate(t)，得到活跃片段及素材内
//    进度，再交给 MediaScheduler 取帧 / ScreenMapper 映射输出（§5.6）。
// ============================================================================

#pragma once

#include <cstdint>
#include <vector>

#include "model/SceneModel.hpp"

namespace ms {

// ---------------------------------------------------------------------------
// 活跃片段 —— evaluate(t) 对单个片段的解算结果
// ---------------------------------------------------------------------------
struct ActiveClip {
    const Clip*  clip  = nullptr; ///< 命中的片段（借用指针，指向 Scene 内部）
    std::int64_t media_us = 0;    ///< 素材内部时间：in_us + (t - start_us)
    double local_progress = 0.0;  ///< 片段局部进度 [0,1)：(t - start)/duration
    // 【2026-11-05 Tween】解算后的完整变换：静态 transform 被该片段的
    // tween 通道逐帧覆写后的结果（TweenEngine::resolveTransform 纯函数）。
    // 下游（ScreenMapper/渲染端）一律消费 resolved，不再直读 clip->transform。
    Transform resolved;
};

// ---------------------------------------------------------------------------
// 解算结果 —— 该播放头时刻的全部活跃片段
// 排序规则：按叠放层级 z 升序（小 z 在前先绘制，大 z 在后覆盖其上）；
// z 相同时保持轨道声明顺序 → 轨道内片段声明顺序（稳定语义，便于跨端一致）。
// ---------------------------------------------------------------------------
struct EvalResult {
    std::vector<ActiveClip> active;
};

// ---------------------------------------------------------------------------
// TimelineEngine —— 播放头解算器
// ---------------------------------------------------------------------------
class TimelineEngine {
public:
    /// 持有场景的【不可变借用】；要求 scene 生命周期覆盖本引擎使用期，
    /// 且期间不得修改 scene（否则借用指针悬空）
    explicit TimelineEngine(const Scene& scene) noexcept : scene_(&scene) {}

    /// 解算播放头 t 时刻的活跃片段集（纯函数：同一 (scene, t) 恒得同一结果）
    /// \param t_us 播放头（微秒，通常为 session_time）
    EvalResult evaluate(std::int64_t t_us) const;

    // ---- 以下为可独立测试的静态纯函数（无 this 状态）----

    /// 片段在 t 是否活跃【前闭后开】：start <= t < start + duration
    static bool isActive(const Clip& c, std::int64_t t_us) noexcept;

    /// t 时刻应取素材的内部时间（us）：in_us + (t - start_us)
    static std::int64_t mediaTimeUs(const Clip& c, std::int64_t t_us) noexcept;

    /// 片段局部进度 [0,1)：(t - start) / duration（duration<=0 视为 0）
    static double localProgress(const Clip& c, std::int64_t t_us) noexcept;

    /// 推算时间轴总时长（us）：所有片段 end 的最大值；空时间轴返回 0
    static std::int64_t computeDurationUs(const Timeline& tl) noexcept;

private:
    const Scene* scene_; ///< 场景借用指针（不可变）
};

} // namespace ms
