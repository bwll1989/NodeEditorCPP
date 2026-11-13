// ============================================================================
//  RenderThread.hpp —— Player 渲染线程（§5.6 渲染管线运行期装配）
//
//  设计依据：《media-server-sync-design.md》§5.2/§5.5/§5.6
//    把 P1 打通的渲染管线（FramePacer 相位对齐 + ScreenMapper 映射 +
//    MediaScheduler 取帧 + SoftRenderer 合成）装配成一条独立线程，
//    挂在 PlayerRuntime 侧与网络/tick 并行运行：
//
//      渲染线程（本类自持，60fps）：
//          now = clock()                          （对齐时钟）
//          scene = scene_provider()               （场景快照，锁内拷 shared_ptr）
//          ph    = playhead_provider()            （播放头快照，锁内拷贝）
//          session_t = pacer.frameSessionTimeUs(now)   ← 共享时钟栅格推导
//          renderFrame(scene, id, ow, oh, session_t, sched, renderer)
//          pacer.waitUntil(clock, nextPresentTime)   ← present 相位对齐
//
//  【时间语义】（与 PlayerRuntime 播放头状态机的关系）：
//    * Playing：会话时刻由 FramePacer 从共享时钟直接推导（栅格时刻）——
//      各端对同一帧序号解算同一会话时刻（§5.6"各端同帧"根基），
//      不经过 10ms tick 状态机（渲染分辨率高于 tick 粒度）；
//    * Paused：直接使用播放头冻结值——暂停画面静止；
//    * session_start 变化（重锚/seek）→ FramePacer epoch 自动跟随重建。
//
//  【线程模型】（渲染架构改造 2026-11-03：解码与渲染线程分离）
//    * 渲染线程（本类自持）：解算（纯数学）→ 读已发布帧（composeFrame，
//      零解码）→ 软件合成 → frame_sink——任何负载下不被解码阻塞；
//    * 解码驱动线程（DecodeDriver，本类自持）：prepare + produce 全部
//      解码耗时——与渲染线程共享 MediaScheduler（源表 shared_mutex），
//      会话时刻取渲染线程写入的原子值（解码目标 = 渲染取样时刻，同源
//      对齐）；源内部帧经原子发布槽交付（FfmpegMediaSource::published_）；
//    * 场景与播放头经 provider 函数每帧快照获取（provider 由
//      PlayerRuntime 提供，内部持 state_mutex_ 拷贝 shared_ptr/值）——
//      渲染/解码线程永不直接触碰 Runtime 的可变状态；
//    * stop() 先停驱动线程再 join 渲染线程，最坏等一个 present 周期。
//
//  【测试策略】renderOnce(now) 是确定性单帧入口（时间显式注入，不起线程；
//  驱动线程不启动——测试用 scheduler() 手动 prime 帧后断言合成内容）；
//  媒体源经 factory() 注入 Stub（assets_root 留空则不注册 FFmpeg 源）；
//  线程生命周期用假时钟/计数器断言。诊断帧导出 BMP（32bpp BGRX）供
//  三进程联测核对"各端画面内容"。
// ============================================================================

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <thread>

#include "engine/DecodeDriver.hpp"
#include "engine/FramePacer.hpp"
#include "engine/IComposeRenderer.hpp" // 合成后端接口（②阶段 GL 管线注入点）
#include "engine/MediaScheduler.hpp"
#include "engine/RenderLoop.hpp"     // renderFrame/composeFrame + FrameStats
#include "engine/SoftRenderer.hpp"
#include "audio/AudioTypes.hpp"     // AudioPoolEntry（音源池条目）
#include "engine/TimelineEngine.hpp" // isActive（音频片段活跃判定，P3）
#include "media/HwAccel.hpp"         // 硬解后端配置（透传到媒体源工厂）
#include "model/SceneModel.hpp"
#include "sync/SyncFrame.hpp"        // PlayState

#include <map>
#include <limits>

namespace ms {

class AudioEngine;
class IAudioOutput;
struct AudioVoice;

class RenderThread {
public:
    // -----------------------------------------------------------------------
    // 启动配置
    // -----------------------------------------------------------------------
    struct Config {
        std::string player_id;         ///< 本机 Player 标识（screen 匹配键）
        int fps = 60;                  ///< 目标帧率（<=0 退化 30，同 FramePacer）
        int out_w = 0, out_h = 0;      ///< 输出分辨率（0 = 首个绑定 screen 1:1）
        /// 诊断帧导出目录（空 = 不导出）；文件名 frame_<帧号>.bmp
        std::string capture_dir;
        /// 导帧最小间隔（us）：默认 1s 一帧，避免写盘影响渲染节奏
        std::int64_t capture_interval_us = 1'000'000;
        /// 最多导出帧数（上限保护，防止长跑联测写爆磁盘）
        int capture_max = 12;
        /// 素材根目录（非空 → 注册 FfmpegMediaSource；测试留空注入 Stub）
        std::string assets_root;
        /// 帧输出回调（显示端预览窗口/平台 present 钩子）：每帧合成完成后
        /// 同步调用（渲染线程上下文——回调内只做互斥快照拷贝，不得长阻塞，
        /// 否则拖累 present 相位）。参数：宽、高、RGBA 像素（行主序 top-down、
        /// stride = 宽×4 字节）。空 = 不回调（editor 端/纯联测不受影响）。
        std::function<void(int, int, const std::uint8_t*, std::uint32_t)>
            frame_sink;
        /// 硬解后端配置（透传给 FFmpeg 媒体源：探测失败自动软解——运行
        /// 行为永远有兜底，本字段只表达"意图"）。库层默认 Soft（测试
        /// 不受硬解环境影响）；display 端装配默认 D3D11VA。
        HwAccelConfig hw_accel;
        /// 合成后端工厂（第②阶段 GL 渲染管线注入点）：
        ///   * 空（默认）= 软渲染基线——renderer_ 画布直接参与合成，
        ///     frame_sink 回调生效（GDI 预览窗上屏）；测试直调 renderOnce
        ///     走同一路径，逐像素断言语义不变；
        ///   * 非空 = 在渲染线程 loop 开头调用一次，产出活动后端（GL
        ///     上下文线程亲和——窗口/上下文必须在渲染线程创建，故工厂
        ///     时机锁死在 loop 内而非构造函数）。画面经后端 endFrame
        ///     （SwapBuffers）直上窗口，frame_sink 不再回调（互斥）；
        ///     工厂返回 nullptr → 回落软渲染（运行行为永远有兜底）。
        std::function<std::unique_ptr<IComposeRenderer>()> backend_factory;

        // ---- 音频输出（P3，2026-11-10）----------------------------------
        /// true = 渲染拍顺路混音并接音频输出（与画面同一 session_t 解算
        /// ——同一拍顺手产出音频，避免第二次解算分叉；方案 §3 P3）。
        /// 前提：渲染线程已启用（render_fps>0）——音频拍挂在渲染拍上。
        bool audio_enable = false;
        /// 音频设备名（空 = 系统默认播放设备；FriendlyName 包含匹配）
        std::string audio_device;
        /// 音频输出实现工厂（测试注入点——方案 P1"抽 IAudioSink 薄接口"
        /// 同款：单测注入 Null/记录器替代 WASAPI，混音产出可确定性断言；
        /// 空 = 真实 AudioOut）。工厂产出 open 失败 → 自动回退 Null
        /// （电平/混音链路照常，行为永远有兜底）。
        std::function<std::unique_ptr<IAudioOutput>()> audio_output_factory;
    };

    /// 对齐时钟读取函数（返回对齐时钟 us；PlayerRuntime 注入 ClockProvider）
    using ClockFn = std::function<std::int64_t()>;

    /// 场景快照提供者（返回当前场景；nullptr = 尚未收到场景）
    using SceneProvider = std::function<std::shared_ptr<const Scene>()>;

    /// 播放头快照（PlayerRuntime 锁内拷贝的三元组）
    struct Playhead {
        PlayState   state             = PlayState::Idle;
        std::int64_t session_start_us = 0; ///< 会话起点（对齐时钟域）
        std::int64_t session_time_us  = 0; ///< 本地播放头（Paused 冻结值）
    };
    using PlayheadProvider = std::function<Playhead()>;

    /// 场景序号提供者（每次场景被应用自增；0/缺省 = 退回指针判新）
    using SceneSerialProvider = std::function<std::uint64_t()>;

    /// 生效 player id 提供者（指派模型 2026-11-06）：每帧取当前 id——
    /// Editor 指派换 id 后，选屏过滤立即跟随，无需重建渲染线程。
    /// 空 = 退回 Config::player_id（测试/旧调用方兼容）。
    using PlayerIdProvider = std::function<std::string()>;

    // -----------------------------------------------------------------------
    // 生命周期
    // -----------------------------------------------------------------------

    /// \param cfg          配置
    /// \param scene_fn     场景快照提供者（须线程安全）
    /// \param playhead_fn  播放头快照提供者（须线程安全）
    /// \param clock_fn     对齐时钟（须线程安全）
    /// \param serial_fn    场景序号提供者（可空：空则用指针变化判"新场景"，
    ///                     地址被分配器复用时可能误判——生产路径必须传）
    /// \param id_fn        生效 id 提供者（可空：空则用 Config.player_id；
    ///                     指派模型下由 PlayerRuntime 注入 effectiveId）
    RenderThread(Config cfg, SceneProvider scene_fn,
                 PlayheadProvider playhead_fn, ClockFn clock_fn,
                 SceneSerialProvider serial_fn = {},
                 PlayerIdProvider id_fn = {});

    ~RenderThread(); // stop()
    RenderThread(const RenderThread&) = delete;
    RenderThread& operator=(const RenderThread&) = delete;

    /// 启动渲染线程 + 解码驱动线程（建捕获目录；幂等）
    void start();

    /// 停止并 join（幂等；先停解码驱动（停止解码消耗）再 join 渲染线程）
    void stop();

    // -----------------------------------------------------------------------
    // 单帧渲染（确定性测试入口；渲染线程主循环每拍调用）
    // -----------------------------------------------------------------------

    /// 渲染一帧：快照场景/播放头 → 栅格解算时刻 → composeFrame（只读合成，
    /// 零解码）→ 可选导帧。解码由驱动线程异步推进；源未就绪的项占位跳过。
    /// \param now_us 对齐时钟当前值（显式注入——测试传假时钟，运行传真实值）
    /// \return 本帧统计；无场景/未绑定 screen/Idle 节流跳过时返回全零
    FrameStats renderOnce(std::int64_t now_us);

    // ---- 诊断观测（跨线程原子读）------------------------------------------

    std::uint64_t frames()   const noexcept { return frames_.load(std::memory_order_relaxed); }
    std::uint32_t captured() const noexcept { return captured_.load(std::memory_order_relaxed); }
    int outWidth()  const noexcept { return out_w_; }
    int outHeight() const noexcept { return out_h_; }

    /// 画布只读访问（测试逐像素断言渲染内容）
    const SoftRenderer& renderer() const noexcept { return renderer_; }

    /// 局部媒体源工厂（测试在 start 前注册 Stub 源；assets_root 为空时
    /// 构造函数不注册任何源——留白给测试注入）
    MediaFactory& factory() noexcept { return factory_; }

    /// 调度器访问（测试手动 prime 帧：注册 Stub 源 → prepare+produce
    /// 一帧 → renderOnce 断言合成内容；驱动线程模式下由驱动线程调用）
    MediaScheduler& scheduler() noexcept { return scheduler_; }

    /// 最近一帧的会话时刻（渲染线程发布给解码驱动线程的值——测试断言
    /// "栅格/冻结时刻正确传递给解码侧"）
    std::int64_t lastSessionTimeUs() const noexcept {
        return session_time_shared_.load(std::memory_order_relaxed);
    }

    // ---- 合成耗时观测（性能基线：软渲染 vs GL 后端对比的数据源）--------
    // 语义：composeFrame 全程（映射解算 + 读发布帧 + 后端合成）微秒数；
    //   last = 逐帧原始值（抖动大）；ema = 指数滑动平均（α=1/16，帧级
    //   平滑——性能记录用稳态值而非尖峰）。

    /// 最近一帧合成耗时（us；0 = 尚无样本）
    std::uint64_t lastComposeCostUs() const noexcept {
        return last_compose_cost_us_.load(std::memory_order_relaxed);
    }

    /// 合成耗时的指数滑动平均（us；稳态性能指标）
    std::uint64_t composeCostEmaUs() const noexcept {
        return ema_compose_cost_us_.load(std::memory_order_relaxed);
    }

    // ---- MPCDI 输出几何投递（M4 收编自 MMC；HTTP 线程 → 渲染线程）------
    /// 线程安全投递 MPCDI 顶点网格（mode=2；由 MpcdiService 经
    /// mpcdi_to_warp 转换产出）。渲染线程每帧取用并仲裁：MPCDI active
    /// 时**优先于**场景 screen 的手动网格 warp（同屏输出变形二选一，
    /// 手动网格与校准包叠加无定义，校准包是显式结果）。nullptr = 撤销
    /// （回落场景 warp）。锁内拷贝 shared_ptr，每帧成本可忽略——
    /// GlRenderer 内部比对去重，网格未变零副作用。
    void setMpcdiWarp(std::shared_ptr<const WarpSpec> warp);

private:
    /// 取当前 MPCDI 网格（渲染线程每帧调用；空 = 未激活/已撤销）。
    std::shared_ptr<const WarpSpec> mpcdiWarp() const;

    /// 渲染线程主循环：renderOnce → 等待下一栅格点 → 循环
    void loop();

    /// 解析输出尺寸（每帧调用——resize/换屏下一帧即跟随）。三级优先：
    ///   1. Config 显式指定（>0）：Editor 预览窗/NDI 虚拟输出/测试等
    ///      固定分辨率场景——用户明确意图最优先；
    ///   2. 【显示分辨率模型 2026-11-10】后端实际输出表面（GL 窗口客户区/
    ///      全屏铺满 = 设备光栅）：display 生产路径——渲染光栅恒 = 输出
    ///      设备实际尺寸，SwapBuffers 恒点对点 1:1，取景矩形（Screen.w/h）
    ///      只决定画布内容映射进该光栅，比例失配被吸收进取景映射段；
    ///   3. 兜底：首个绑定本机（生效 id）screen 的画布尺寸（1:1 拉伸
    ///      映射 = 拼接屏常态）——软渲染基线（无窗口概念）与 headless。
    /// 场景未绑定本机 → false。\param id 本帧生效 id（指派模型下 = Editor
    /// 指派值）\param now_us 对齐时钟当前值（失配提示节流锚点，与渲染
    /// 线程其余告警同钟域）
    bool ensureOutputSize(const Scene& scene, const std::string& id,
                          std::int64_t now_us);

    /// 按间隔/上限节流导出诊断帧（渲染线程内调用）
    void maybeCapture(std::int64_t now_us);

    /// RGBA → 32bpp BMP（BGRX、底行优先，BI_RGB 无压缩；54 字节手写头，
    /// 不依赖平台头文件——测试可跨平台复现并校验字节）
    static bool writeBmp(const std::string& path, int w, int h,
                         const std::uint8_t* rgba, std::uint32_t stride);

    // ---- 成员（声明序 = 构造序：factory_ 先于 scheduler_ 先于 driver_）----
    Config          cfg_;
    SceneProvider   scene_fn_;
    PlayheadProvider playhead_fn_;
    ClockFn         clock_fn_;
    SceneSerialProvider serial_fn_;  ///< 场景序号（可空→指针判新回退）
    PlayerIdProvider    id_fn_;      ///< 生效 id（可空→cfg_.player_id 回退）

    MediaFactory    factory_;    ///< 局部工厂（测试隔离；assets_root 装配 FFmpeg 源）
    MediaScheduler  scheduler_;  ///< 源实例缓存（源表锁内并发：驱动写/渲染读）
    SoftRenderer    renderer_;   ///< 软件合成画布（渲染线程独占；soft 基线 +
                                 ///< SoftRendererBackend 的包装对象）
    std::unique_ptr<FramePacer> pacer_; ///< 帧栅格（epoch=session_start，变化时重建）

    /// 活动合成后端（渲染线程独占，loop 开头创建/退出销毁——GL 上下文
    /// 线程亲和：创建与析构必须同线程）。空 = 软渲染基线（renderOnce
    /// 直接用 renderer_；测试直调路径与既有逐像素断言全兼容）。
    std::unique_ptr<IComposeRenderer> backend_;

    /// 解码驱动线程（渲染线程的"帧供给方"）：prepare+produce 全部解码
    /// 在此线程；与会话时刻原子值（下方）同源对齐。声明在 scheduler_ 后
    /// （构造引用它），析构先于 scheduler_（成员析构逆序——驱动线程先停
    /// 再拆调度器，join 保证无悬垂）。
    DecodeDriver driver_;

    std::thread      thread_;        ///< 渲染线程（start 起，stop join）
    std::atomic<bool> running_{false};

    // ---- 渲染→解码的会话时刻共享（渲染线程写 / 驱动线程读）--------------
    // 渲染线程每帧写入本帧栅格时刻（§5.6 相位对齐产物）——解码驱动线程
    // 以此为解码目标（同一 (scene,t) 同一绘制列表 → 解码帧=渲染要取的帧）
    std::atomic<std::int64_t> session_time_shared_{0};

    // ---- 诊断计数（渲染线程写、外部读 → 原子）------------------------------
    std::atomic<std::uint64_t> frames_{0};    ///< 累计渲染帧数
    std::atomic<std::uint32_t> captured_{0};  ///< 累计导出帧数

    // ---- 合成耗时观测（渲染线程写、外部读 → 原子；性能对比数据源）----
    std::atomic<std::uint64_t> last_compose_cost_us_{0}; ///< 最近帧合成耗时
    std::atomic<std::uint64_t> ema_compose_cost_us_{0};  ///< 指数滑动平均（α=1/16）
    /// 最近帧呈现耗时（us；endFrame 内 SwapBuffers+泵消息——渲染丢帧
    /// 归因观测：帧间隔超限时对照本值区分"合成慢"vs"呈现慢"）
    std::atomic<std::uint64_t> last_swap_cost_us_{0};

    int out_w_ = 0;                 ///< 解析后的输出宽（ensureOutputSize 维护）
    int out_h_ = 0;                 ///< 解析后的输出高

    // ---- MPCDI 输出几何（HTTP 线程写 / 渲染线程读 → 锁内 shared_ptr）----
    // setMpcdiWarp 投递、mpcdiWarp 取用；渲染线程每帧锁内拷贝一次
    // shared_ptr（热路径成本可忽略），比对去重由 GlRenderer 承担。
    mutable std::mutex mpcdi_mu_;
    std::shared_ptr<const WarpSpec> mpcdi_warp_;  ///< 空 = 未激活/已撤销

    /// 本帧选屏 id（指派模型，2026-11-06）：id_fn_ 非空取其值（空返回值
    /// 也回退），未注入退回 cfg_.player_id（测试/旧调用方兼容）。
    /// renderOnce 每帧取一次——指派换 id 后下一帧立即跟随。
    std::string effectiveId() const {
        if (id_fn_) {
            std::string v = id_fn_();
            if (!v.empty()) return v;
        }
        return cfg_.player_id;
    }
    std::int64_t last_capture_us_ = 0; ///< 上次导帧时刻（渲染线程独占，免原子）
    /// Idle 静帧上一次渲染时刻（ms，渲染线程独占免原子）：静帧节流窗口锚点
    /// （初值极小 → 收到场景后首帧立即渲染，不等 500ms）
    std::int64_t last_idle_render_ms_ = -1'000'000'000;
    /// 上一次【已合成】的场景序号（渲染线程独占）：静帧节流的变化检测基准
    /// ——序号变化 = 收到新 MsgScene（编辑下发），本次必须立即合成，不受
    /// 500ms 静帧节流窗口约束（2026-11-05 编辑期"display 慢半拍"修复）。
    /// 用序号而非对象地址：上一帧 shared_ptr 释放后 make_shared 会复用同一
    /// 块地址 → 地址相等 → 误判"没换场景" → 节流重新生效（今日复测发现）。
    std::uint64_t last_composed_serial_ = 0;
    /// 指针判新的回退基准（只在未装配 serial_fn_ 的测试路径使用）
    const void* last_composed_scene_ = nullptr;
    /// "场景无本 display 的 screen"上次告警时刻（ms，渲染线程独占）：
    /// 周期性醒目告警的节流锚点（初值极小 → 首帧立即告警，此后 15s 一次
    /// 直至 id 对上）——一次性输出会被心跳/重锚日志滚出可视区，用户实测
    /// 根本看不到（2026-11-04 白屏诊断二次修订）
    std::int64_t last_no_screen_warn_ms_ = -1'000'000'000;
    /// 合成异常上次告警时刻（ms，渲染线程独占）：FrameStats 异常态
    /// （items==0 / no_frame>0 / invalid>0）的 2s 节流锚点——黑屏排查
    /// 定位器，区分映射层/解码层/绘制层/上屏层故障（2026-11-04 黑屏诊断）
    std::int64_t last_stats_warn_ms_ = -1'000'000'000;
    /// 取景/输出比例失配上次提示时刻（ms，渲染线程独占）：显示分辨率模型
    /// 下失配是预期行为（内容 Stretch 铺满），提示只为醒目知会（10s 限频）
    std::int64_t last_ratio_warn_ms_ = -1'000'000'000;

    // ---- 音频（P3，2026-11-10）--------------------------------------------
    /// 音源池（clip id → 条目）：同片段跨拍同源（解码游标/seek 状态跨拍
    /// 连续——AudioMediaSource::read 的前后跳消化依赖游标连续性）；条目
    /// 带素材路径指纹，换绑素材（同 clip id 新 asset）时旧源失效重建。
    /// 【池超限铁律】兜底清空必须在取声源指针之前（unique_ptr 清空会使
    /// 早先 voices 里的裸指针悬垂——与编辑器 tone_sources_ 同款）。
    std::map<std::string, AudioPoolEntry> audio_sources_;
    /// 每拍声源清单复用缓冲（渲染拍 60fps，避免逐拍堆分配）
    std::vector<AudioVoice> audio_voices_;
    /// 上次音频拍的会话时刻（窗长 = 会话时钟推进量——会话锁定产出；
    /// INT64_MIN = 尚未打过音频拍，首窗用标准窗长）
    std::int64_t last_audio_t_us_ = std::numeric_limits<std::int64_t>::min();
    /// 音频拍（renderOnce 每帧调用）：收集活跃音频片段 → 可变窗混音
    void audioTick(const Scene& scene, std::int64_t session_t, bool playing);
    /// 音频混音引擎（audio_enable 时 start() 装配；unique_ptr——头文件
    /// 只前向声明，AudioEngine.hpp 不泄入本头）
    std::unique_ptr<AudioEngine> audio_engine_;
    /// 音频观测（2026-11-12 实测补齐）：voice 数变化即打 + 电平每 60 拍
    /// 一行（≈1s@60fps）——display 音频链路此前零日志，出声与否无从
    /// 排障（RMS 电平的 UI 上报链属 P4，观测先行走 stderr）。
    std::size_t last_voice_cnt_ = static_cast<std::size_t>(-1);
    int audio_log_div_ = 0;
};

} // namespace ms
