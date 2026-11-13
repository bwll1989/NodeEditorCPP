// ============================================================================
//  TimelineEngine.cpp —— 播放头解算纯函数实现
//
//  设计要点（对应设计文档 §4.4 / §5.2）：
//    * evaluate(t) 无状态：不做任何缓存/推进，调用方每帧重算即可（t 由
//      对齐时钟算出，seek/pause 都只是 t 的变化，引擎零感知）；
//    * 前闭后开区间：t == start 活跃，t == end 不活跃；
//    * 排序：按 z 升序稳定排序；z 相同时按"轨道声明顺序 + 片段声明顺序"，
//      这保证 Editor 与 Player 对同一 t 得到完全一致的绘制次序（跨端一致
//      是帧同步渲染的前提）。
// ============================================================================

#include "engine/TimelineEngine.hpp"

#include <algorithm>

#include "engine/TweenEngine.hpp" // Tween 通道逐帧插值（resolveTransform）

namespace ms {

// ---------------------------------------------------------------------------
// 静态纯函数：片段在 t 是否活跃【前闭后开】
//   t ∈ [start, start + duration) → 活跃
//   例：start=1000us, duration=500us → [1000, 1500) 活跃；1500 不活跃。
//   边界正确性由 test_timeline_engine.cpp 的边界用例锁定。
// ---------------------------------------------------------------------------
bool TimelineEngine::isActive(const Clip& c, std::int64_t t_us) noexcept {
    return t_us >= c.start_us && t_us < c.start_us + c.duration_us;
}

// ---------------------------------------------------------------------------
// 静态纯函数：t 时刻素材内部时间
//   media_us = in_us + (t - start_us)
//   例：素材入点 in=500us，clip 从 start=1000us 开始，
//       t=1200us → 素材应取 500 + (1200-1000) = 700us 处的帧
//   seek 时 t 突变，此映射自动给出正确帧位（无需引擎维护任何状态）。
// ---------------------------------------------------------------------------
std::int64_t TimelineEngine::mediaTimeUs(const Clip& c,
                                         std::int64_t t_us) noexcept {
    return c.in_us + (t_us - c.start_us);
}

// ---------------------------------------------------------------------------
// 静态纯函数：片段局部进度 [0,1)
//   progress = (t - start) / duration
//   duration <= 0（非法配置）时按 0 处理，避免除零（防御式编程）。
// ---------------------------------------------------------------------------
double TimelineEngine::localProgress(const Clip& c, std::int64_t t_us) noexcept {
    if (c.duration_us <= 0) return 0.0;
    const double elapsed = static_cast<double>(t_us - c.start_us);
    return elapsed / static_cast<double>(c.duration_us);
}

// ---------------------------------------------------------------------------
// 推算时间轴总时长：max(所有片段的 end)；显式 duration_us 为 0 时调用方可用
// 此结果回填（SceneLoader 加载阶段自动推算，见 loader/SceneLoader.cpp）
// ---------------------------------------------------------------------------
std::int64_t TimelineEngine::computeDurationUs(const Timeline& tl) noexcept {
    std::int64_t max_end = 0;
    for (const auto& track : tl.tracks) {
        for (const auto& clip : track.clips) {
            const std::int64_t end = clip.start_us + clip.duration_us;
            if (end > max_end) max_end = end;
        }
    }
    return max_end;
}

// ---------------------------------------------------------------------------
// evaluate：遍历全部轨道/片段，收集 t 时刻活跃项并按 z 升序稳定排序。
//
// 复杂度 O(片段总数)；片段量级（数百）下单帧遍历成本可忽略。
// 若未来场景规模显著增长，可按 start 排序 + 二分定位优化（记录于 §5.5）。
// ---------------------------------------------------------------------------
EvalResult TimelineEngine::evaluate(std::int64_t t_us) const {
    EvalResult result;
    result.active.reserve(16); // 典型场景同帧活跃片段个位/十数量级，预留减少重分配

    // ---- 1. 线性收集：遍历每条轨道的每个片段，判定活跃并预计算进度 ----
    for (const auto& track : scene_->timeline.tracks) {
        for (const auto& clip : track.clips) {
            if (!isActive(clip, t_us)) continue; // 前闭后开判定

            ActiveClip ac;
            ac.clip           = &clip;
            ac.media_us       = mediaTimeUs(clip, t_us);
            ac.local_progress = localProgress(clip, t_us);
            // 【Tween 集成点（唯一）】逐帧解算完整变换：无 tween 通道时
            // resolved == clip.transform（零开销语义），有通道时被覆写。
            // 排序/绘制/可见性全部改用 resolved（见下方排序与本文件头说明）。
            ac.resolved       = TweenEngine::resolveTransform(clip, t_us);
            result.active.push_back(ac);
        }
    }

    // ---- 2. 绘制次序：按 z 升序稳定排序 ----
    // stable_sort 保证 z 相等时保持收集顺序（即"轨道声明序 → 片段声明序"），
    // 与遍历顺序配合实现跨端一致的绘制次序（见文件头说明）。
    // 按 resolved.z 排序（Tween 不开放 z 通道时 resolved.z == 静态 z；
    // 未来开放 z 通道后自动获得逐帧重排能力，无需改这里）。
    std::stable_sort(result.active.begin(), result.active.end(),
                     [](const ActiveClip& a, const ActiveClip& b) {
                         return a.resolved.z < b.resolved.z;
                     });

    return result;
}

} // namespace ms
