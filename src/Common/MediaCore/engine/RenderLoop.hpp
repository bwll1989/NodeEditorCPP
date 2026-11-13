// ============================================================================
//  RenderLoop.hpp —— 渲染主循环编排（§5.2+§5.5+§5.6 的每帧组装，两端同源）
//
//  两种每帧编排（渲染架构改造 2026-11-03 后并存，按线程选用）：
//
//  ① renderFrame()——"解算→驱动解码→取帧→绘制"一体化（单线程模式）：
//      aligned_now（对齐时钟）
//        → FramePacer 推导本帧序号/目标会话时刻（相位对齐，各端一致）
//        → ScreenMapper.buildDrawList(...)            （画布映射，§5.2）
//        → MediaScheduler.produce(asset, media_us)    （查建实例+解码推进）
//        → source.frameView() → Renderer.draw(item)   （合成，§5.6）
//        → 等待到帧栅格时刻 → present                  （相位对齐提交）
//    适用：editor 预览 worker（解码渲染同线程独占）/ 单线程测试。
//
//  ② composeFrame()——"解算→读已发布帧→绘制"只读合成（渲染线程模式）：
//      解码已剥离到 DecodeDriver 独立线程（自适应节拍 8ms/100ms + poke），
//      渲染线程只做映射（纯数学）+ find/frameView 读已发布帧 + 合成，
//      任何负载下不被解码阻塞。适用：Player 渲染线程（RenderThread）。
//
//  两函数共用同一 buildDrawList 纯函数（同源原则，§2.1：预览和实际上屏
//  不一致的经典 bug 从根上杜绝——输入相同则绘制列表相同）。
//
//  Player 侧主循环现状（渲染线程 + 解码驱动线程双线程）：
//      RenderThread::loop（渲染线程）:
//          composeFrame(scene, player_id, ow, oh, 栅格时刻, scheduler, renderer);
//            ↑ GL 后端：backend_->endFrame()（SwapBuffers 呈现）在
//              composeFrame 返回后立即调用（2026-11-04 黑屏根因修复——
//              旧 platformPresent 独立步骤在双线程改造中丢失，见
//              RenderThread.cpp 呈现修复注释）
//          pacer.waitUntil(...);                        // present 相位对齐
//      DecodeDriver（驱动线程）:
//          循环 buildDrawList → 逐项 scheduler.produce（解码推进）
// ============================================================================

#pragma once

#include <cstddef>
#include <string>

#include "engine/MediaScheduler.hpp"
#include "engine/ScreenMapper.hpp"

namespace ms {

// ---------------------------------------------------------------------------
// FrameStats —— 单帧渲染统计（诊断日志/测试断言用）
// ---------------------------------------------------------------------------
struct FrameStats {
    std::size_t items    = 0; ///< 解算出的绘制项数
    std::size_t drawn    = 0; ///< 实际绘制（取得帧并写入）的项数
    std::size_t no_frame = 0; ///< 源未就绪/该时刻无帧而跳过的项数
    std::size_t invalid  = 0; ///< 素材缺失等非法项数
};

/// 渲染一帧：解算 → 取帧 → 绘制（纯编排，时间由调用方注入）
///
/// \param scene           完整场景
/// \param player_id       本机 Player 标识（决定渲染哪些 screen）
/// \param out_w/out_h     本机输出分辨率（像素）
/// \param session_time_us 本帧会话时刻（FramePacer 对齐后的栅格时刻）
/// \param scheduler       媒体源调度器（取帧驱动）
/// \param renderer        渲染后端（需提供 draw(const DrawItem&, const FrameView&)
///                        → bool；SoftRenderer 满足，GL 后端同签名）
/// \param mode            画布→本机映射模式
/// \return 本帧统计
template <typename RendererT>
FrameStats renderFrame(const Scene& scene,
                       const std::string& player_id,
                       int out_w, int out_h,
                       std::int64_t session_time_us,
                       MediaScheduler& scheduler,
                       RendererT& renderer,
                       FitMode mode = FitMode::Stretch) {
    FrameStats stats;

    // ---- 0. 帧开始：清屏（画布初始化；GL 后端同义 = glClear）----
    renderer.beginFrame(out_w, out_h);

    // ---- 1. 解算：活跃 clip（z 序）× 画布映射 → 本机绘制项（§5.2/§5.3）----
    const std::vector<DrawItem> items =
        buildDrawList(scene, player_id, out_w, out_h, session_time_us, mode);
    stats.items = items.size();

    // ---- 2. 逐项取帧并绘制（z 序即绘制序，后画覆盖先画）----
    for (const DrawItem& di : items) {
        // 素材必须存在（SceneLoader 校验过引用完整性，此处防御）
        if (di.asset == nullptr || di.clip == nullptr) { ++stats.invalid; continue; }

        // 取帧：ensureSource（查/建实例，shared_ptr 保活）+ produce 解码推进。
        // 【查表合并】原实现 produce()（内部 ensure）成功后再 find() 二次
        // 查表——每帧每项两次锁内 map 查找；合并为复用 ensureSource 返回
        // 的实例（同一张表同一实例），省一半表查找。
        auto src = scheduler.ensureSource(*di.asset);
        FrameView fv;
        if (src == nullptr || !src->produce(di.media_us) ||
            !src->frameView(fv) || !fv.valid()) {
            ++stats.no_frame; // 源未就绪/该时刻无载荷 → 跳过（故障隔离）
            continue;
        }

        // 合成：后端把帧按 DrawItem 矩形写入帧缓冲
        if (renderer.draw(di, fv)) ++stats.drawn;
        else                       ++stats.invalid;
    }
    return stats;
}

/// 只读合成一帧：解算 → 取【已发布帧】→ 绘制（不驱动解码）
///
/// 【渲染架构：解码与渲染线程分离】与 renderFrame 的唯一差别：
/// 不调用 scheduler.produce（解码驱动是 DecodeDriver 线程的职责），
/// 只经 find+frameView 读取解码线程已发布的最近帧——渲染线程在
/// 任何负载下都只做映射（纯数学）+ 读帧 + 合成，永不被解码阻塞；
/// 源未就绪的 clip 由上层按占位跳过（no_frame 计数），解码线程
/// 就绪后自动换真画面。
///
/// \return 本帧统计（同 renderFrame；no_frame = 源未就绪或无已发布帧）
template <typename RendererT>
FrameStats composeFrame(const Scene& scene,
                        const std::string& player_id,
                        int out_w, int out_h,
                        std::int64_t session_time_us,
                        MediaScheduler& scheduler,
                        RendererT& renderer,
                        FitMode mode = FitMode::Stretch) {
    FrameStats stats;

    // ---- 0. 帧开始：清屏 ----
    // 【瞬卡探针 2026-11-05】beginFrame 耗时（GL 后端含窗口就绪化/清屏）——
    // 丢帧归因第三层：compose 慢时分清"beginFrame / 解算 / draw"。
    // MS_CANVAS_DBG 门控 + 500ms 节流（>8ms 才打）。
    const auto t_begin0 = std::chrono::steady_clock::now();
    renderer.beginFrame(out_w, out_h);
    {
        const auto begin_us = std::chrono::duration_cast<
            std::chrono::microseconds>(std::chrono::steady_clock::now() -
                                       t_begin0)
                                  .count();
        static const bool s_dbg = std::getenv("MS_CANVAS_DBG") != nullptr;
        if (s_dbg && begin_us > 8'000) {
            static std::int64_t s_last_log_ms = 0;
            const std::int64_t now_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch())
                    .count();
            if (now_ms - s_last_log_ms > 500) {
                s_last_log_ms = now_ms;
                std::fprintf(stderr,
                             "[compose-probe] beginFrame %lldus（>8ms）\n",
                             static_cast<long long>(begin_us));
                std::fflush(stderr);
            }
        }
    }

    // ---- 1. 解算：活跃 clip（z 序）× 画布映射 → 本机绘制项 ----
    // 与 renderFrame 同一纯函数（两端同源；映射结果与解码驱动线程
    // 各自调用互不影响——输入相同则绘制列表相同）
    const std::vector<DrawItem> items =
        buildDrawList(scene, player_id, out_w, out_h, session_time_us, mode);
    stats.items = items.size();

    // ---- 2. 逐项取【已发布帧】并绘制（不解码——零阻塞）----
    // 【瞬卡探针】逐项 draw 耗时（GL 后端含 interop map/D2D/unmap + 着色
    // 器绘制）——>8ms 打点（500ms 节流）：渲染丢帧的合成层细分布点。
    for (const DrawItem& di : items) {
        if (di.asset == nullptr || di.clip == nullptr) { ++stats.invalid; continue; }

        // 只读查表（读锁 <1µs；shared_ptr 保活）+ 读原子发布槽（acquire，无锁）
        const auto t_find0 = std::chrono::steady_clock::now();
        auto src = scheduler.find(di.asset->id);
        FrameView fv;
        const bool got = src != nullptr && src->frameView(fv) && fv.valid();
        const auto t_find1 = std::chrono::steady_clock::now();
        if (!got) {
            ++stats.no_frame; // 解码线程尚未发布帧：占位跳过（自愈）
            continue;
        }

        if (renderer.draw(di, fv)) ++stats.drawn;
        else                       ++stats.invalid;
        const auto draw_us = std::chrono::duration_cast<
            std::chrono::microseconds>(std::chrono::steady_clock::now() -
                                       t_find1)
                                 .count();
        const auto find_us = std::chrono::duration_cast<
            std::chrono::microseconds>(t_find1 - t_find0)
                                 .count();
        static const bool s_probe =
            std::getenv("MS_CANVAS_DBG") != nullptr;
        if (s_probe && (draw_us > 8'000 || find_us > 8'000)) {
            static std::int64_t s_last_log_ms = 0;
            const std::int64_t now_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch())
                    .count();
            if (now_ms - s_last_log_ms > 500) {
                s_last_log_ms = now_ms;
                std::fprintf(stderr,
                             "[compose-probe] draw %lldus find %lldus"
                             "（>8ms 项内慢）\n",
                             static_cast<long long>(draw_us),
                             static_cast<long long>(find_us));
                std::fflush(stderr);
            }
        }
    }
    return stats;
}

} // namespace ms
