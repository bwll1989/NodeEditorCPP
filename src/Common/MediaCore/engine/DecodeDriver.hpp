// ============================================================================
//  DecodeDriver.hpp —— 解码驱动线程（渲染/解码线程分离的核心，2026-11-03）
//
//  【架构定位】渲染架构改造：
//      渲染线程（RenderThread）只做：解算（纯数学）→ 读已发布帧 → 合成
//        → 上屏——任何负载下零解码等待；
//      解码驱动线程（本类）做：prepare（打开文件/流探测）+ produce
//        （seek/顺序解码）——解码全部耗时离场渲染线程。
//
//  【工作方式】后台线程按自适应节拍循环：
//      scene = scene_fn()                       （场景快照，与渲染线程同源）
//      t     = time_fn()                        （渲染线程写入的会话时刻）
//      items = buildDrawList(scene, pid, cw, ch, t)   （同一纯函数）
//      逐项：scheduler.produce(asset, media_us)  （ensureSource+解码推进）
//  与渲染线程共用同一 MediaScheduler（源表 shared_mutex 并发）与同一解算
//  纯函数——两端对同一 (scene, t) 解出同一绘制列表，解码目标严格对齐
//  渲染取样时刻（时间轴对齐仍由统一时钟驱动，素材不各自跑时钟）。
//
//  【为什么要独立线程】此前解码嵌在渲染循环里（renderFrame→produce 同步
//  解码）：一路 1080p 软解 ~10ms，prepare（open+探测）数百 ms——多路/高分
//  辨率素材时渲染帧率崩塌、首帧延迟秒级，显示端"迟迟无画面"的结构性根因。
//  拆分后渲染帧率与解码吞吐解耦：解码慢时画面"略旧"（最近已发布帧），
//  不再"卡顿/黑屏"；解码赶上后自动追平。
//
//  【输出几何无关】驱动线程调 buildDrawList 用画布尺寸 1:1（只消费
//  media_us/asset——映射矩形与解码目标无关），避免渲染线程输出尺寸
//  未解析时的依赖倒挂。
//
//  【线程契约】start/stop/poke 任意线程；loop 内 scheduler 的表写与
//  produce 由本线程独占（与渲染线程 find 读并发经 shared_mutex）。
// ============================================================================

#pragma once

#include <atomic>
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>

#include "engine/MediaScheduler.hpp"
#include "engine/ScreenMapper.hpp"
#include "model/SceneModel.hpp"

namespace ms {

class DecodeDriver {
public:
    /// 场景快照提供者（与 RenderThread 同源 provider；nullptr = 无场景）
    using SceneProvider = std::function<std::shared_ptr<const Scene>()>;

    /// 会话时刻提供者（渲染线程每帧写入的栅格时刻——解码目标与之对齐）
    using TimeProvider = std::function<std::int64_t()>;

    /// \param scheduler  媒体源调度器（与渲染线程共享；表操作自带锁）
    /// \param player_id  本机 Player 标识（绘制列表按本机屏过滤）
    DecodeDriver(MediaScheduler& scheduler, std::string player_id)
        : scheduler_(scheduler), player_id_(std::move(player_id)) {}

    DecodeDriver(const DecodeDriver&) = delete;
    DecodeDriver& operator=(const DecodeDriver&) = delete;

    ~DecodeDriver() { stop(); }

    /// 启动驱动线程（幂等）
    /// \param scene_fn 场景快照提供者（须线程安全）
    /// \param time_fn  会话时刻提供者（须线程安全）
    void start(SceneProvider scene_fn, TimeProvider time_fn) {
        if (running_.load(std::memory_order_relaxed)) return; // 幂等
        scene_fn_ = std::move(scene_fn);
        time_fn_  = std::move(time_fn);
        running_.store(true, std::memory_order_relaxed);
        thread_ = std::thread([this] { loop(); });
    }

    /// 停止并 join（幂等；最多等一个驱动节拍）
    void stop() {
        running_.store(false, std::memory_order_relaxed);
        {
            std::lock_guard<std::mutex> lock(cv_mutex_);
        }
        cv_.notify_all();
        if (thread_.joinable()) thread_.join();
    }

    /// 唤醒驱动线程立即跑一拍（场景更新/seek 后调用——新素材立即
    /// prepare、新时刻立即解码，不等下一节拍；非阻塞）
    void poke() { cv_.notify_all(); }

    /// 上一拍耗时（us；诊断观测——解码压力的直接指标）
    std::int64_t lastBeatCostUs() const noexcept {
        return beat_cost_us_.load(std::memory_order_relaxed);
    }

private:
    /// 驱动主循环：解算 → 逐项驱动解码 → 自适应等待
    void loop() {
        for (;;) {
            if (!running_.load(std::memory_order_relaxed)) break;

            // ---- 一拍：解算 + 驱动解码（渲染线程同源输入）----
            const auto beat_tp = std::chrono::steady_clock::now();
            const bool busy = driveOnce();
            beat_cost_us_.store(
                std::chrono::duration_cast<std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - beat_tp)
                    .count(),
                std::memory_order_relaxed);
            // 【诊断 2026-11-05】解码拍超帧预算（16.7ms）打点——解码压力
            // 渐进恶化（NVDEC surface 池紧张/解码降速）的直接证据。
            // MS_CANVAS_DBG=1 门控；500ms 节流防风暴（拍间 8ms，连续超限
            // 时最多 2 条/秒）。
            if (beat_cost_us_.load(std::memory_order_relaxed) > 16'667) {
                static const bool s_dbg =
                    std::getenv("MS_CANVAS_DBG") != nullptr;
                static std::int64_t s_last = 0;
                const auto now = std::chrono::duration_cast<
                    std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch())
                    .count();
                if (s_dbg && now - s_last > 500) {
                    s_last = now;
                    std::fprintf(stderr,
                                 "[decode] 拍耗时 %lldus（超 60fps 预算，"
                                 "解码压力大）\n",
                                 static_cast<long long>(
                                     beat_cost_us_.load(
                                         std::memory_order_relaxed)));
                    std::fflush(stderr);
                }
            }

            // ---- 节拍等待：有活跃素材 8ms 快拍（追赶 30/60fps 素材），
            //      无素材 100ms 慢拍（待机近零负载）；stop/poke 立即醒 ----
            // 【poke 生效修复 2026-11-05】wait_for(lock, wait, pred) 的语义
            // 是"谓词为真才返回"——poke() 的 notify_all 只把人叫醒，谓词
            // （!running_）仍为假就继续睡到超时，等价 poke 完全无效：场景
            // 更新/seek 后新素材要等满一整拍（无素材时 100ms）才开始
            // prepare/decode，编辑期表现为"display 画面迟一拍"。改为无谓词
            // 等待：任何 notify 立即返回，循环首行自行复查 running_。
            const auto wait = busy ? std::chrono::milliseconds(8)
                                   : std::chrono::milliseconds(100);
            std::unique_lock<std::mutex> lock(cv_mutex_);
            cv_.wait_for(lock, wait);
        }
    }

    /// 单拍驱动：解算本机绘制列表 → 逐项 ensureSource + produce
    /// \return 本拍是否驱动了活跃素材（节拍自适应依据）
    bool driveOnce() {
        if (!scene_fn_ || !time_fn_) return false;
        const std::shared_ptr<const Scene> scene = scene_fn_();
        if (!scene) return false;

        // 会话时刻：渲染线程每帧写入的栅格时刻（同源对齐——解码目标帧
        // = 渲染线程正要取的帧；时间轴对齐由统一时钟保证）
        const std::int64_t t = time_fn_();

        // 绘制列表：与渲染线程同一纯函数（画布 1:1，只消费 media_us/asset
        // ——输出矩形与解码无关；本机屏过滤/覆盖判断/可见性与渲染一致）
        const std::vector<DrawItem> items = buildDrawList(
            *scene, player_id_,
            scene->canvas_w > 0 ? static_cast<int>(scene->canvas_w) : 1,
            scene->canvas_h > 0 ? static_cast<int>(scene->canvas_h) : 1,
            t, FitMode::Stretch);

        bool busy = false;
        for (const DrawItem& di : items) {
            if (di.asset == nullptr || di.clip == nullptr) continue;
            // 【瞬卡探针 2026-11-05】media_us 跳变检测——同一素材相邻两拍
            // 的解码目标差正常 = 拍间隔(8ms) + 至多一帧跨度（60fps≈25ms
            // 上限）；差 >100ms 说明解码目标本身在跳变（会话时钟跳变/
            // 重锚换轴/纠偏硬 seek/clip 切换），与 FfmpegMediaSource 的
            // [probe] seek 打点互证：大跳差 + seekTo = 时钟跳变。
            // 本线程独占 per-asset 记录（driveOnce 串行），无锁。
            if (s_probe_on) {
                const std::int64_t prev = last_media_us_[di.asset->id];
                if (prev != 0) { // 0 = 首见素材（首帧跳变属正常，不报）
                    const std::int64_t d = di.media_us - prev;
                    if (d > 100'000 || d < -100'000) {
                        static std::atomic<std::int64_t> s_last_ms{0};
                        const std::int64_t now =
                            std::chrono::duration_cast<
                                std::chrono::milliseconds>(
                                std::chrono::steady_clock::now()
                                    .time_since_epoch())
                                .count();
                        std::int64_t prev_ms =
                            s_last_ms.load(std::memory_order_relaxed);
                        if (now - prev_ms >= 200 &&
                            s_last_ms.compare_exchange_strong(prev_ms, now)) {
                            std::fprintf(stderr,
                                         "[probe] media_us 跳变 %+.3lldms"
                                         " (asset=%s %lld→%lld)\n",
                                         static_cast<long long>(d / 1000),
                                         di.asset->id.c_str(),
                                         static_cast<long long>(prev),
                                         static_cast<long long>(di.media_us));
                            std::fflush(stderr);
                        }
                    }
                }
                last_media_us_[di.asset->id] = di.media_us;
            }
            // ensureSource（创建+prepare，失败可重试）+ produce（解码推进）
            // ——MediaScheduler::produce 内聚此逻辑；渲染线程零触发
            if (scheduler_.produce(*di.asset, di.media_us)) busy = true;
        }
        return busy;
    }

    MediaScheduler& scheduler_;      ///< 与渲染线程共享（源表锁内并发）
    std::string      player_id_;     ///< 本机 Player 标识（屏过滤键）
    SceneProvider    scene_fn_;      ///< 场景快照（start 注入）
    TimeProvider     time_fn_;       ///< 会话时刻（start 注入）

    // ---- 瞬卡探针（2026-11-05）：media_us 跳变检测的 per-asset 记录 ----
    // 仅 s_probe_on（MS_CANVAS_DBG=1）时读写；driveOnce 独占线程串行
    // 访问，无锁。key=asset id，value=上一拍解码目标（0=首见）。
    static inline const bool s_probe_on =
        std::getenv("MS_CANVAS_DBG") != nullptr;
    std::unordered_map<std::string, std::int64_t> last_media_us_;

    std::thread       thread_;       ///< 驱动线程
    std::atomic<bool> running_{false};
    std::mutex        cv_mutex_;     ///< 节拍等待锁（stop/poke 唤醒用）
    std::condition_variable cv_;     ///< 节拍条件变量

    std::atomic<std::int64_t> beat_cost_us_{0}; ///< 上一拍耗时（诊断）
};

} // namespace ms
