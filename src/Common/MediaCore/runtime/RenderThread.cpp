// ============================================================================
//  RenderThread.cpp —— Player 渲染线程实现
//
//  职责链（每帧）：
//    快照场景/播放头 → FramePacer 栅格解算会话时刻（§5.6 相位对齐）→
//    composeFrame（映射/读帧/合成编排）→ 节流导出诊断 BMP →
//    waitUntil 下一栅格点（present 对齐提交）
//
//  诊断 BMP 格式说明（writeBmp）：
//    32bpp BGRX、BI_RGB 无压缩、biHeight 正值 = 底行优先（BMP 惯例）。
//    画布内部是 RGBA 行主序（SoftRenderer 约定），导出时逐像素 R/B 互换
//    并补不透明。手写 54 字节头（14B FILEHEADER + 40B INFOHEADER），
//    小端逐字节拼装——不 include windows.h，测试可按字节校验头结构。
// ============================================================================

#include "runtime/RenderThread.hpp"

#include "runtime/AssetPath.hpp" // u8Path：capture_dir/帧文件路径可含中文（MSVC 窄串按 ACP 解码）

#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <string>
#include <string_view>
#include <typeinfo>
#include <vector>

#include "media/FfmpegMediaSource.hpp"
#include "media/TextMediaSource.hpp" // 文本源注册（无条件，无文件依赖）
#include "media/LiveMediaSource.hpp" // 实时源注册（无条件：NDI 直收/Hub 转发消费端）
#include "engine/TweenEngine.hpp"    // resolveGain（音频增益解算，P3）
#include "audio/AudioEngine.hpp"     // 混音引擎（P3 音频拍）
#include "audio/AudioCasting.hpp"    // 选源出口（两端共用，含换绑素材指纹）
#include "audio/AudioMediaSource.hpp"// 素材解码声源（P2）
#include "audio/AudioOut.hpp"        // WASAPI 真实输出（默认装配）
#include "audio/IAudioOutput.hpp"    // 输出解偶缝（Null 兑底用）
#include "audio/SineAudioSource.hpp"// 正弦回退（无音轨素材）

namespace ms {

namespace {

// 【排障】stderr 时戳（毫秒）：渲染线程状态沿打点与 [player]/[editor]
// 日志对齐时间线（2026-11-17 停止后画面延迟二分定位）
long long tsMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now().time_since_epoch())
        .count();
}

// ---------------------------------------------------------------------------
// BMP 小端写入辅助（头字段逐字节拼装，天然小端，跨平台一致）
// ---------------------------------------------------------------------------

void put16(std::vector<std::uint8_t>& v, std::uint32_t x) {
    v.push_back(static_cast<std::uint8_t>(x & 0xFF));
    v.push_back(static_cast<std::uint8_t>((x >> 8) & 0xFF));
}

void put32(std::vector<std::uint8_t>& v, std::uint32_t x) {
    v.push_back(static_cast<std::uint8_t>(x & 0xFF));
    v.push_back(static_cast<std::uint8_t>((x >> 8) & 0xFF));
    v.push_back(static_cast<std::uint8_t>((x >> 16) & 0xFF));
    v.push_back(static_cast<std::uint8_t>((x >> 24) & 0xFF));
}

// ---------------------------------------------------------------------------
// 进程内存水位（KB）：/proc/self/status 的 VmRSS/VmHWM（Android/Linux 有效，
// 其余平台恒 -1）。【OOM 取证 2026-11-24】「同步媒体闪退」已符号化实锤为
// 堆耗尽：帧心跳处的无害小字符串分配失败，连 bad_alloc 异常对象都分配不
// 出 → __cxa_allocate_exception 直呼 terminate（无活动异常签名）——心跳
// 带出 RSS 曲线，爬升段即元凶活动段。
// ---------------------------------------------------------------------------
long procStatusKB(const char* key) {
#if defined(__ANDROID__) || defined(__linux__)
    std::FILE* f = std::fopen("/proc/self/status", "r");
    if (!f) return -1;
    char line[256];
    long kb = -1;
    const std::size_t klen = std::strlen(key);
    while (std::fgets(line, sizeof(line), f)) {
        if (std::strncmp(line, key, klen) == 0) {
            kb = std::strtol(line + klen, nullptr, 10);
            break;
        }
    }
    std::fclose(f);
    return kb;
#else
    (void)key;
    return -1;
#endif
}

} // namespace

// ---------------------------------------------------------------------------
// 构造 / 析构
// ---------------------------------------------------------------------------
RenderThread::RenderThread(Config cfg, SceneProvider scene_fn,
                           PlayheadProvider playhead_fn, ClockFn clock_fn,
                           SceneSerialProvider serial_fn,
                           PlayerIdProvider id_fn)
    : cfg_(std::move(cfg)),
      scene_fn_(std::move(scene_fn)),
      playhead_fn_(std::move(playhead_fn)),
      // 【判空归一】clock_fn_ 允许传空（测试直构路径）——loop() 里 137 行
      // 判空而 waitUntil 处不判空，空函数对象在 145 行会抛 bad_function_call。
      // 与 PreviewRenderer 的构造兑底模式一致：空则归一为 steady 时钟，
      // 之后所有调用点无需再判空。
      clock_fn_(clock_fn ? std::move(clock_fn)
                         : [] {
                             return std::chrono::duration_cast<
                                        std::chrono::microseconds>(
                                 std::chrono::steady_clock::now()
                                     .time_since_epoch())
                                 .count();
                         }),
      serial_fn_(std::move(serial_fn)),
      id_fn_(std::move(id_fn)),
      scheduler_(factory_), // scheduler_ 持 factory_ 引用（构造序已保证）
      driver_(scheduler_, cfg_.player_id) { // 驱动线程共享同一调度器
    // 【id 说明】driver_ 的 id 是构造时快照（仅诊断日志用）；选屏行为
    // 的 id 在 renderOnce 每帧经 effectiveId() 取——指派换 id 后渲染
    // 立即跟随（无需重建本线程/驱动线程）。

    // ---- 运行期默认装配：素材根目录非空 → 注册 FFmpeg 视频源 ----
    // 捕获 assets_root 值拷贝：creator 生命周期与工厂一致，避免悬垂。
    // 测试场景：assets_root 留空 → 此处不注册 → 测试用 factory() 注入 Stub。
    // 图片素材（PNG/JPG/BMP/GIF）复用同一解码器：kind 作工厂注册键区分，
    // 解码路径与视频一致（单帧媒体越界钳制末帧 = 静态图任意时长显示；
    // PNG 透明通道经 RGBA 直达 SoftRenderer 的 alpha 混合，遮挡关系由
    // TimelineEngine 的 clip z 排序保证）。
    if (!cfg_.assets_root.empty()) {
        const std::string root = cfg_.assets_root;
        const ms::HwAccelConfig hw = cfg_.hw_accel; // 硬解意图随源实例下发
        factory_.registerCreator(
            AssetKind::Video, [root, hw](const Asset&) {
                return std::make_unique<FfmpegMediaSource>(root,
                                                           AssetKind::Video,
                                                           hw);
            });
        factory_.registerCreator(
            AssetKind::Image, [root, hw](const Asset&) {
                return std::make_unique<FfmpegMediaSource>(root,
                                                           AssetKind::Image,
                                                           hw);
            });
    }

    // ---- 文本源注册【无条件】------------------------------------------------
    // 文本随场景 pb 分发（无文件、不依赖 assets_root）——裸启动也应渲染
    // 文本片段（与视频/图片"无根目录则占位"的降级语义不同：文本永远可渲染）
    factory_.registerCreator(AssetKind::Text, [](const Asset&) {
        return std::make_unique<TextMediaSource>();
    });

    // ---- 实时源注册【无条件】------------------------------------------------
    // Live 与 Text 同款"无文件素材"：随场景 pb 分发（AssetDeps 排除拉
    // 文件），不依赖 assets_root——裸启动也应尝试连接 NDI 源（源未上线
    // 时 produce 返回 false，占位语义与视频源降级一致）
    factory_.registerCreator(AssetKind::Live, [](const Asset&) {
        return std::make_unique<LiveMediaSource>();
    });
    // ---- 音频输出装配（P3；构造期装配——renderOnce 直调的确定性
    // 测试路径不经过 start()，音频拍必须已就位）----
    // 编辑器同款兑底链：工厂/AudioOut 打开失败 → Null（电平/混音链路
    // 照常，无声而已）；MS_AUDIO_OUT=null 强制 Null（对照测试用）。
    if (cfg_.audio_enable) {
        static const bool s_force_null = [] {
            const char* e = std::getenv("MS_AUDIO_OUT");
            return e && std::string_view(e) == "null";
        }();
        audio_engine_ = std::make_unique<AudioEngine>();
        if (s_force_null) {
            audio_engine_->attach(std::make_unique<NullAudioOutput>());
        } else if (cfg_.audio_output_factory) {
            audio_engine_->attach(cfg_.audio_output_factory());
        } else {
            audio_engine_->attach(
                std::make_unique<AudioOut>(cfg_.audio_device));
        }
        if (!audio_engine_->start(ms::AudioSpec{})) {
            // 真实输出打开失败（设备被独占/无设备）→ Null 兑底重试
            std::fprintf(stderr,
                         "render:%s: W-音频输出打开失败，回退静音输出"
                         "（电平链路照常）\n",
                         cfg_.player_id.c_str());
            audio_engine_->attach(std::make_unique<NullAudioOutput>());
            if (!audio_engine_->start(ms::AudioSpec{}))
                audio_engine_.reset(); // 连 Null 都失败 = 放弃音频
        } else {
            std::printf("render:%s: 音频输出: %s\n",
                        cfg_.player_id.c_str(),
                        audio_engine_->outputName());
        }
    }
}

RenderThread::~RenderThread() { stop(); }

// ---------------------------------------------------------------------------
// MPCDI 输出几何投递（HTTP 线程 → 渲染线程；锁内 shared_ptr 交接）
// ---------------------------------------------------------------------------
void RenderThread::setMpcdiWarp(std::shared_ptr<const WarpSpec> warp) {
    std::lock_guard<std::mutex> lock(mpcdi_mu_);
    mpcdi_warp_ = std::move(warp);
}

std::shared_ptr<const WarpSpec> RenderThread::mpcdiWarp() const {
    std::lock_guard<std::mutex> lock(mpcdi_mu_);
    return mpcdi_warp_;
}

// ---------------------------------------------------------------------------
// 生命周期
// ---------------------------------------------------------------------------
void RenderThread::start() {
    if (running_.load(std::memory_order_relaxed)) return; // 幂等

    // 捕获目录预先建好（含父目录；失败仅告警——写帧时再报错）
    if (!cfg_.capture_dir.empty()) {
        std::error_code ec;
        std::filesystem::create_directories(ms::u8Path(cfg_.capture_dir), ec);
        if (ec)
            std::fprintf(stderr, "render:%s: 捕获目录创建失败: %s (%s)\n",
                         cfg_.player_id.c_str(), cfg_.capture_dir.c_str(),
                         ec.message().c_str());
    }

    // ---- 解码驱动线程先行启动（渲染线程的"帧供给方"）----
    // 场景快照：与渲染线程同源 provider（Runtime 锁内拷贝）；
    // 会话时刻：渲染线程每帧写入的原子值（解码目标 = 渲染取样时刻）。
    driver_.start(
        [this] { return scene_fn_ ? scene_fn_() : nullptr; },
        [this] { return session_time_shared_.load(std::memory_order_relaxed); });

    running_.store(true, std::memory_order_relaxed);
    thread_ = std::thread(&RenderThread::loop, this);
}

void RenderThread::stop() {
    running_.store(false, std::memory_order_relaxed);
    driver_.stop(); // 先停解码驱动（最多等一个驱动节拍）再 join 渲染
    if (thread_.joinable()) thread_.join(); // 最坏等一个 present 周期
    // 音频拆除必须在【渲染线程 join 之后】：audioTick 持音源池裸指针
    // 在渲染线程混音——join 前清池 = 与在途混音拍并发 UAF（2026-11-10
    // 修复；原顺序"音频先停"实为渲染拍悬垂窗口）。AudioOut 自有 WASAPI
    // 渲染线程由 audio_engine_->stop() 内 close() 收（析构先停线程铁律
    // 不受本顺序影响——那是 AudioOut 自己的线程，与渲染线程无依赖）。
    if (audio_engine_) audio_engine_->stop();
    audio_sources_.clear();
}

// ---------------------------------------------------------------------------
// 渲染线程主循环
// ---------------------------------------------------------------------------
void RenderThread::loop() {
    // ---- 合成后端创建（渲染线程内：GL 上下文线程亲和——窗口/wglContext
    //      必须与 makeCurrent 同线程，故工厂调用锁死在 loop 开头而非
    //      start()/构造。工厂返回空指针/未配置 → backend_ 保持空，走
    //      renderer_ 软渲染基线，行为与旧版完全一致）----
    if (cfg_.backend_factory)
        backend_ = cfg_.backend_factory();
    if (backend_)
        std::printf("render:%s: 合成后端: %s\n", cfg_.player_id.c_str(),
                    typeid(*backend_).name());

    // ---- 启动即建窗（2026-11-04 回归修复）----------------------------------
    // 旧逻辑：输出窗口在首帧 beginFrame 内惰性创建——无 Editor 接入时渲染
    // 循环在"无场景"处早退，beginFrame 永不触发 → GL 后端（默认）下 display
    // 启动后只有终端、没有任何输出窗口（soft 的 GDI 预览窗在 main 无条件
    // 启动，故双后端改造前无此问题；本 backend_ 非空即 GL 后端——soft 模式
    // 不注入 factory，走 renderer_ 软渲染基线）。现改为：后端创建后立即
    // 就绪化（GL = 建窗+上下文+程序），黑屏待播，与 soft 部署体验对齐。
    // 失败仅留日志——后端内部 failed_ 让后续 beginFrame 走各自降级路径。
    if (backend_) {
        if (backend_->ensureReady())
            std::printf("render:%s: 输出窗口已就绪（等待 Editor 下发场景）\n",
                        cfg_.player_id.c_str());
        else
            std::fprintf(stderr, "render:%s: 合成后端就绪化失败（输出窗口"
                                 "未创建；可回退 --render-backend=soft）\n",
                         cfg_.player_id.c_str());
    }

    for (;;) {
        if (!running_.load(std::memory_order_relaxed)) break;

        const std::int64_t now = clock_fn_(); // 构造期已兜底非空（见构造函数）

        // ---- 【渲染帧间隔探针 2026-11-05】分离"渲染停顿 vs 解码停顿"----
        // 语义：本次循环起点距上次的间隔。正常节奏 = 栅格等待（16.7ms@60fps）
        // + renderOnce 耗时（<5ms）→ 稳态 ~16-17ms；间隔 >25ms = 渲染线程
        // 丢帧（renderOnce 内部阻塞 / OS 调度 / 锁等待 / SwapBuffers 排队）。
        // 与 DecodeDriver 的 [decode] 拍耗时、FfmpegMediaSource 的 [probe]
        // 跳变/补齐打点互证：渲染间隔大 + media_us 跳变 = 渲染层停顿
        // （解码被迫追帧，是果）；渲染间隔正常而 media_us 跳变 = 时钟/目标
        // 异常（解码补齐是果）。文件级 static（单实例进程足够；MS_CANVAS_DBG
        // 门控 + 500ms 节流——常态零开销）。
        {
            static const bool s_dbg =
                std::getenv("MS_CANVAS_DBG") != nullptr;
            if (s_dbg) {
                using namespace std::chrono;
                const std::int64_t steady_us = duration_cast<microseconds>(
                    steady_clock::now().time_since_epoch()).count();
                static std::int64_t s_last_loop_us = 0;
                static std::int64_t s_last_log_ms = 0;
                if (s_last_loop_us != 0) {
                    const std::int64_t gap = steady_us - s_last_loop_us;
                    if (gap > 25'000) { // 超过 1.5 帧（60fps 预算 16.7ms）
                        const std::int64_t now_ms = steady_us / 1000;
                        if (now_ms - s_last_log_ms > 500) {
                            s_last_log_ms = now_ms;
                            // 【归因】打点带出上一帧的合成/呈现耗时：
                            // 帧间隔大 + swap 大 → 呈现层停顿（vsync/DWM）；
                            // 帧间隔大 + compose 大 → 合成层慢；
                            // 两者都小 → 时间耗在 pump/调度/锁（待细分）。
                            std::fprintf(
                                stderr,
                                "render-probe: 帧间隔 %lld.%lldms"
                                "（>25ms 丢帧）compose=%lluus swap=%lluus\n",
                                static_cast<long long>(gap / 1000),
                                static_cast<long long>((gap / 100) % 10),
                                static_cast<unsigned long long>(
                                    last_compose_cost_us_.load(
                                        std::memory_order_relaxed)),
                                static_cast<unsigned long long>(
                                    last_swap_cost_us_.load(
                                        std::memory_order_relaxed)));
                            std::fflush(stderr);
                        }
                    }
                }
                s_last_loop_us = steady_us;
            }
        }

        const auto render_t0 = std::chrono::steady_clock::now();
        renderOnce(now);
        const auto render_us =
            std::chrono::duration_cast<std::chrono::microseconds>(
                std::chrono::steady_clock::now() - render_t0)
                .count();

        // ---- 输出窗口消息泵（每帧必调，2026-11-04"未响应"修复）----------
        // renderOnce 的无场景早退路径不经过 beginFrame/endFrame——启动即
        // 建窗后、场景到达前的空窗期若不泵消息，Windows 判定输出窗口
        // "未响应"（拖动/关闭无响应）。有帧路径 endFrame 内也泵（双保险）。
        if (backend_) backend_->pumpOsMessages();

        // renderOnce 后复查 running：stop 期间渲染完的最后一帧不白等
        if (!running_.load(std::memory_order_relaxed)) break;

        // ---- 渲染吞吐探针（MS_CANVAS_DBG 门控；5s 窗口）------------------
        // 回答"display 每秒到底能合成几帧、卡在哪一段"：frame=本窗口成功
        // 合成帧数（≈吞吐上限），拍耗时=renderOnce 全程（含解码取帧/合成/
        // 呈现），compose/swap=内部两段。判读：frame 明显低于编辑流频率 →
        // 中间态被合并（吞吐不足）；拍耗时高而 compose 低 → 卡在呈现/锁。
        {
            static const bool s_dbg =
                std::getenv("MS_CANVAS_DBG") != nullptr;
            if (s_dbg) {
                static std::int64_t s_t0_ms = 0;
                static long long s_cost_us = 0, s_n = 0;
                static std::uint64_t s_prev_frames = 0;
                const auto now_ms = std::chrono::duration_cast<
                                        std::chrono::milliseconds>(
                                        std::chrono::steady_clock::now()
                                            .time_since_epoch())
                                        .count();
                if (s_t0_ms == 0) {
                    s_t0_ms = now_ms;
                    s_prev_frames = frames_.load(std::memory_order_relaxed);
                }
                ++s_n;
                s_cost_us += render_us;
                // 出帧 = frames_ 计数增量（只数真正合成并呈现的帧）
                const std::uint64_t f_now = frames_.load(std::memory_order_relaxed);
                const long long s_frames = static_cast<long long>(f_now - s_prev_frames);
                if (now_ms - s_t0_ms >= 5000) {
                    std::fprintf(
                        stderr,
                        "render-probe: 5s 吞吐: 出帧=%lld 拍数=%lld"
                        " 平均拍耗时=%.1fms compose=%lluus swap=%lluus\n",
                        s_frames, s_n,
                        s_n ? static_cast<double>(s_cost_us) / s_n / 1000.0 : 0.0,
                        static_cast<unsigned long long>(
                            last_compose_cost_us_.load(std::memory_order_relaxed)),
                        static_cast<unsigned long long>(
                            last_swap_cost_us_.load(std::memory_order_relaxed)));
                    std::fflush(stderr);
                    s_t0_ms = now_ms;
                    s_prev_frames = f_now; // 下一窗口重新计数
                    s_cost_us = s_n = 0;
                }
            }
        }

        if (pacer_ && pacer_->epoch() != 0) {
            // 有会话：present 等到下一栅格点（各端同一物理时刻出帧，§5.6）
            pacer_->waitUntil(clock_fn_, pacer_->nextPresentTimeUs(now));
        } else {
            // 无会话/未开播：按 60fps 预算（16.6ms）扣掉本拍耗时后休眠。
            // 【2026-11-05 修正】原实现固定 sleep 16ms，实际周期 = 合成
            // 耗时(≈8ms) + 16ms ≈ 24ms → 空闲编辑吞吐被压到 ~41 次/秒；
            // 一旦编辑流（拖拽调参 ~50Hz）超过该上限，中间态就被合并丢弃，
            // 观感是"画面跟手但有点黏"。扣减耗时后周期回到 ~17ms。
            const long long budget_us = 16'600 - render_us;
            if (budget_us > 0)
                std::this_thread::sleep_for(std::chrono::microseconds(budget_us));
        }
    }

    // ---- 后端销毁（同线程逆操作：GL 上下文 wglMakeCurrent(NULL) 后删除，
    //      与创建严格的线程亲和配对——不得跨线程析构 GL 资源）----
    backend_.reset();
}

// ---------------------------------------------------------------------------
// 单帧渲染（确定性入口）
// ---------------------------------------------------------------------------
FrameStats RenderThread::renderOnce(std::int64_t now_us) {
    FrameStats stats;
    if (!scene_fn_ || !playhead_fn_) return stats; // 未装配 provider（防御）

    // ---- 快照输入（provider 内部加锁拷贝，渲染线程无共享可变状态）----
    // 【探针】快照锁耗时——PlayerRuntime::state_mutex_ 的持有者（tick 线程
    // / recv 线程）若持锁做慢操作，此处锁等待直接表现为渲染丢帧。
    // MS_CANVAS_DBG 门控；>5ms 打点（500ms 节流）。
    const auto snap_t0 = std::chrono::steady_clock::now();
    const std::shared_ptr<const Scene> scene = scene_fn_();
    const Playhead ph = playhead_fn_();
    // 【待机画面生命周期 2026-11-23 语义修正】"已播放"判定后移到合成之后
    // （本文件下方 composeFrame 返回处）：Playing 且 items>0（画面上真的
    // 有内容）才置位 ever_played_。原实现在此按播放状态置位——standalone
    // 开机自动播放"空时间轴"（有 clip 但无一可画，items=0）也会置位并
    // 每帧清卡 → 用户视角"从没播放过却永远黑屏"（模拟器实测复现，用户
    // 真机同因）。用户定案语义"第一次播放前显示"的判据是观众看到了内容，
    // 不是状态机进入了 Playing。
    // 【指派模型 2026-11-06】本帧选屏 id：id_fn_ 优先（Editor 指派换 id
    // 后下一帧立即跟随），未注入退回 cfg_.player_id（测试/旧调用方）。
    // 每帧取一次供全函数使用（含 ensureOutputSize/composeFrame/诊断）。
    const std::string my_id = effectiveId();

    // ---- 素材同步 OSD 采样（2026-11-16；三个呈现路径共用）----------------
    // 每帧采样一次 PlayerRuntime 原子量（零锁）→ setOsd 交给后端（内部
    // 内容比对去重，未变化的帧零成本）。active=false 时显式清 OSD——
    // 就绪瞬间起下一帧呈现即无叠字。静帧节流跳过的帧不重画：上一次的
    // OSD 留在呈现面上（DWM 保持），更新滞后 ≤500ms 无碍。
    if (backend_ && cfg_.asset_osd_provider) {
        const auto osd = cfg_.asset_osd_provider();
        // 【诊断 MS_OSD_DBG=1】OSD 状态变化打点（链路自证：active 判定/
        // done/total/percent 是否如预期推进）
        {
            static const bool s_osd_dbg =
                std::getenv("MS_OSD_DBG") != nullptr;
            static bool s_last_active = false;
            static int  s_last_done   = -1;
            if (s_osd_dbg &&
                (osd.active != s_last_active || osd.done != s_last_done)) {
                s_last_active = osd.active;
                s_last_done   = osd.done;
                std::fprintf(stderr,
                             "osd: state: active=%d done=%d total=%d "
                             "file%%=%d\n",
                             osd.active ? 1 : 0, osd.done, osd.total,
                             osd.file_percent);
                std::fflush(stderr);
            }
        }
        if (osd.active && osd.total > 0) {
            char txt[96];
            if (osd.file_percent >= 0)
                std::snprintf(txt, sizeof(txt),
                              "素材同步中 %d/%d · %d%%", osd.done,
                              osd.total, osd.file_percent);
            else
                std::snprintf(txt, sizeof(txt), "素材同步中 %d/%d",
                              osd.done, osd.total);
            backend_->setOsd(
                txt, static_cast<float>(osd.done) /
                         static_cast<float>(osd.total));
        } else {
            backend_->setOsd(nullptr, -1.0f); // 就绪/无素材：撤销 OSD
        }
    } else if (backend_) {
        backend_->setOsd(nullptr, -1.0f); // 未配 provider：确保无残留
    }
    // ---- 待机卡生命周期（2026-11-23 决策点统一重构）-----------------------
    // 清卡/画卡决策统一后移：!scene 早退分支（下方）按 ever_played_ 决定；
    // 有场景合成路径在 composeFrame 返回后按 items 判定（见下方）——本处
    // 不再"有场景即清卡"（旧实现把空时间轴 items=0 的待机卡也清掉了）。
    {
        const auto snap_us = std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - snap_t0).count();
        static const bool s_dbg = std::getenv("MS_CANVAS_DBG") != nullptr;
        static std::int64_t s_last_log_ms = 0;
        if (s_dbg && snap_us > 5'000) {
            const std::int64_t now_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch())
                    .count();
            if (now_ms - s_last_log_ms > 500) {
                s_last_log_ms = now_ms;
                std::fprintf(stderr,
                             "render-probe: 快照锁耗时 %lldus（>5ms——"
                             "state_mutex_ 被长持）\n",
                             static_cast<long long>(snap_us));
                std::fflush(stderr);
            }
        }
    }

    // 无场景 → 本帧无可画内容；但呈现链不能空窗（2026-11-04 MMC 对齐）：
    // MMC（MPV_Media_Console）render_loop 每轮无条件 draw+swap，输出表面
    // 持续提交 DWM。display 原实现无场景时完全跳过 beginFrame/endFrame
    // ——建窗首帧后再无任何 present，驱动/DWM 对"一次性呈现后永不更新"
    // 的窗口可能停止重合成（呈现冻结的成因之一）。现对齐：每轮空场景都
    // 持续 beginFrame + endFrame 上屏。
    // 【待机画面 2026-11-22】空场景上屏内容从恒黑改为居中待机卡（Flow
    // 图标 + standby_provider 注入的控制面地址）——现场不再面对一块分
    // 不清"待机还是故障"的黑屏。窗口尺寸每帧查客户区 → resize 自动跟随；
    // standby_provider 未装配/返回空 = 不画卡（回退清屏黑旧语义）。
    // 【生命周期定案 2026-11-22】待机卡只出现在"开机后第一次播放前"
    // （黑屏易被误读为设备未开）；ever_played_ 置位后永久回黑屏——
    // 显式 setStandby("") 清残留文本（内容比对使后续帧零成本）。
    // ---- 待机卡决策（!scene / 场景未绑定两条早退路径共用）----------------
    // 未播过真内容且 provider 已装配 → 画卡；否则清卡回黑（2026-11-22/23
    // 用户定案：待机卡只出现在"开机后第一次播放前"）。调用方须保证
    // backend_ 非空；呈现统一由 ensureReady 内部 beginFrame+endFrame 承担
    // （勿在外层叠加 begin/end——每帧双 swap 且首帧画的是上一帧的卡）。
    auto applyStandbyDecision = [this]() {
        if (cfg_.standby_provider &&
            !ever_played_.load(std::memory_order_relaxed)) {
            backend_->setStandby(cfg_.standby_provider().c_str());
        } else {
            backend_->setStandby("");
        }
    };

    if (!scene) {
        // 【停摆取证 2026-11-20】无场景早退静默——渲染线程"拍了不画"的
        // 隐形路径（拍照转、compose=0、出帧=0 的全零吞吐形态）。节流打点
        // 暴露该路径被走了（正常部署场景到达后不再进入）。
        {
            static std::int64_t s_last_warn_ms = 0;
            const std::int64_t now_ms =
                std::chrono::duration_cast<std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch())
                    .count();
            if (now_ms - s_last_warn_ms >= 3000) {
                s_last_warn_ms = now_ms;
                std::fprintf(stderr,
                             "render:%s: 早退: scene=null（无场景可画）\n",
                             cfg_.player_id.c_str());
                std::fflush(stderr);
            }
        }
        if (backend_) {
            applyStandbyDecision();
            backend_->ensureReady();
        }
        return stats;
    }

    // ---- 静帧预览（Idle：未开播/已停止）----
    // 【部署核对体验】此前 Idle 完全不渲染，下发场景后预览窗口永远停在
    // "等待输出画面"提示，用户误读为"显示端无画面"故障。现改为：收到
    // 场景即渲染播放头时刻静帧（Idle 时 current_us_=0 → 场景 0 时刻画面，
    // 素材未就绪的 clip 由渲染层占位），部署配置完即可肉眼核对"有没有画面"，
    // 播放后自动切实时渲染。Idle 画面静态：500ms 节流一帧足够（素材拉取
    // 完成后的下一个节流点自动换真画面，避免满帧率空转烧 CPU）。
    const bool idle_static =
        (ph.state != PlayState::Playing && ph.state != PlayState::Paused);
    // 【静帧节流撤销 2026-11-17】Idle/Paused 静帧不再 500ms 一拍——持续
    // 满帧率合成+呈现。撤销依据（用户实测实锤）：停止后画面滞留几百 ms
    // ~3s 随机，**窗口最大化/最小化一下立即切黑**——渲染/命令链日志全部
    // 同 ms 完成（状态沿帧已合成+swap），低频呈现（2fps）下 DWM 对 GL
    // 窗口的合成更新不可靠（呈现表面停在旧帧，强制重绘立即更新）。空
    // 场景/静帧合成成本实测 0.1~0.7ms（本文件 11-05 注释自证）——满帧率
    // 空转成本可忽略，换呈现链持续提交（与 2026-11-04"无场景也持续上屏
    // 黑帧"的 MMC 对齐原则同一逻辑：呈现不能空窗/降频）。
    // 【场景判新 2026-11-05】优先用 PlayerRuntime 的单调场景序号；未装配
    // 序号提供者（测试直构）时退回对象地址比较。地址回退不安全：上一帧的
    // shared_ptr 释放后 make_shared 常复用同一块内存，地址相等 ≠ 场景相同
    // ——实测表现为"每 100 次编辑有几次上屏探针漏打 + 静帧节流偶发重现"。
    const std::uint64_t scene_serial = serial_fn_ ? serial_fn_() : 0;
    const bool scene_changed =
        serial_fn_ ? (scene_serial != last_composed_serial_)
                   : (scene.get() != last_composed_scene_);
    // 【状态沿打点 2026-11-17】播放状态切换的第一帧标记（原"状态沿放行"
    // 节流豁免——节流已整体撤销，本标记仅保留诊断打点用途）。
    const bool state_edge = (ph.state != last_render_state_);
    const PlayState prev_render_state = last_render_state_;
    last_render_state_    = ph.state;
    if (state_edge) {
        std::fprintf(stderr,
                     "[%lld][render:%s] 状态沿 %d→%d：本帧放行合成"
                     "（idle_static=%d scene_changed=%d）\n",
                     tsMs(),
                     cfg_.player_id.c_str(),
                     static_cast<int>(prev_render_state),
                     static_cast<int>(ph.state),
                     idle_static ? 1 : 0,
                     scene_changed ? 1 : 0);
        std::fflush(stderr);
    }
    // 记录本帧消费的场景（变化检测基准）
    last_composed_serial_ = scene_serial;
    last_composed_scene_  = scene.get();

    // ---- 帧栅格：epoch 跟随 session_start（重锚/seek 后自动换轴）----
    // FramePacer 不可变对象 → epoch 变化时整体重建（极低频，播放会话切换）
    if (!pacer_ || pacer_->epoch() != ph.session_start_us)
        pacer_ = std::make_unique<FramePacer>(cfg_.fps, ph.session_start_us);

    // ---- 输出尺寸（每帧解析：cfg 显式 > 后端实际表面 > screen 兜底——
    //      GL 窗口客户区 WM_SIZE 实时维护，resize 下一帧即跟随）----
    if (!ensureOutputSize(*scene, my_id, now_us)) {
        // 【白屏诊断 2026-11-04】场景已到达但无绑定本机的 screen——旧实现
        // 静默早退，输出窗口保持未绘制背景（白），用户误读为渲染故障。
        // 指派模型（2026-11-06）下通常是：连接刚建立、指派 id 往返尚未
        // 完成（本机仍持旧标签）；重注册后 id 对上即自愈。醒目日志
        // **周期性重复**（15s 节流）：列全场景绑定关系，直接给对接方法
        // ——一次性输出会被后续心跳/重锚日志滚出可视区。id 对上
        // （场景带来匹配 screen）后本分支不再进入，告警自然静默。
        const std::int64_t now_ms = now_us / 1000;
        if (now_ms - last_no_screen_warn_ms_ >= 15000) {
            last_no_screen_warn_ms_ = now_ms;
            std::string bindings;
            for (const auto& s : scene->screens)
                bindings += " " + s.id + "->" + s.player_id;
            std::fprintf(stderr,
                         "\n[render:%s] W-场景中无本 display 的 screen，"
                         "白屏待播！（本提示每 15s 重复，直至 id 对上）\n"
                         "  本机生效 id=%s（--id=%s）；场景当前绑定:%s\n"
                         "  说明：连接后 Editor 会指派屏 id（自动采用），"
                         "无需改 --id 重启\n\n",
                         my_id.c_str(), my_id.c_str(),
                         cfg_.player_id.c_str(), bindings.c_str());
            std::fflush(stderr);
        }
        // 呈现链不能空窗（同无场景路径）：持续上屏。本机视角无内容 =
        // 待机——同款待机卡决策：未播过真内容画卡（指派往返期不再
        // 黑屏/残卡），已播过清卡回黑。
        if (backend_) {
            applyStandbyDecision();
            backend_->ensureReady();
        }
        return stats; // 场景未绑定本机 screen（待机卡/黑屏待播）
    }

    // ---- 会话时刻解算（§5.6 核心语义）----
    // Playing：从共享时钟推导栅格时刻——同一物理时刻各端解算出同一会话
    //          时刻 → 各端渲染同一画面（与本地 tick 状态机解耦）；
    // Paused：直接用播放头冻结值——暂停画面静止；
    // Idle：静帧时刻（未播放时播放头快照为 0 → 场景 0 时刻画面）。
    // 【最后防线 2026-11-20】Playing 且时钟落后锚点轴原点 2s+ = 跨机时钟
    // 域差未被校准（两端墙钟裸差；applyAnchorLocked 的本机化兜底未覆盖
    // 旧会话/异常链路时）——FramePacer 对 now<epoch 钳 0 会把画面永久
    // 冻在第一帧。改直通 tick 域播放头（钳 0 防负值黑屏），画面跟随播放
    // 头走（单端正确优先于端间同步），节流警告直接指认根因。
    std::int64_t session_t = ph.session_time_us;
    if (ph.state == PlayState::Playing) {
        if (pacer_->epoch() - now_us > 2'000'000) {
            session_t = std::max<std::int64_t>(0, ph.session_time_us);
            static std::int64_t s_last_domain_warn_ms = 0;
            const std::int64_t now_ms = now_us / 1000;
            if (now_ms - s_last_domain_warn_ms >= 3000) {
                s_last_domain_warn_ms = now_ms;
                std::fprintf(stderr,
                             "render:%s: W-时钟域差: 本机时钟落后锚点轴原点"
                             " %.3fs → 渲染兜底跟随本机播放头（锚点轴修复后"
                             "自动回归共享域）\n",
                             cfg_.player_id.c_str(),
                             static_cast<double>(pacer_->epoch() - now_us) /
                                 1e6);
                std::fflush(stderr);
            }
        } else {
            session_t = pacer_->frameSessionTimeUs(now_us);
        }
    }
    // 共享给解码驱动线程：解码目标帧 = 本帧渲染取样时刻（同源对齐）
    session_time_shared_.store(session_t, std::memory_order_relaxed);

    // ---- 音频拍（P3）：与画面同一 session_t 解算——同一拍顺手产出
    // 音频（方案 §3 P3"避免第二次解算分叉"）。音画同源时刻 = A/V
    // 同步的根基；窗长 = 会话时钟推进量（会话锁定产出）。
    if (audio_engine_)
        audioTick(*scene, session_t, ph.state == PlayState::Playing);

    // ---- 输出变形注入（2026-11-09 WATCHOUT 式手动网格变形）--------------
    // 从本机 screen 提取 warp 网格交后端（GL = FBO 两段式；soft/预览端
    // setWarp 默认空实现静默忽略）。每帧注入成本可忽略——renderer 内部
    // 比对去重，网格未变时零副作用。
    // 【M4 MPCDI 仲裁 2026-11-11】MPCDI 包激活时优先——同一输出变形
    // 二选一（手动网格与校准包叠加无定义，校准包是显式结果）；撤销
    // （nullptr）自动回落场景 screen 网格，语义自洽。
    if (backend_) {
        WarpSpec ws;
        if (auto mp = mpcdiWarp()) {
            ws = *mp;
        } else {
            for (const auto& s : scene->screens) {
                if (s.player_id == my_id) {
                    ws.mode = s.warp.mode;
                    ws.cols = s.warp.cols;
                    ws.rows = s.warp.rows;
                    ws.pts.assign(s.warp.pts.begin(), s.warp.pts.end());
                    break; // 与 ensureOutputSize 同一口径：首个匹配 screen
                }
            }
        }
        backend_->setWarp(ws);
    }

    // ---- 【下发延迟探针，2026-11-05】场景版本切换 → 本帧即用新场景合成 ----
    // 打印墙钟毫秒（与 display 侧 "[player] 场景已接收" 同钟域直接相减）：
    //   差值 = 从 recv 线程换指针到画面真正合成出来的间隔。该间隔由三段
    //   叠加：① recv 线程处理延迟 ② 等本渲染帧 ③ 静帧节流窗口（Idle/
    //   Paused 下 500ms 才画一帧——编辑期最常见的"display 慢半拍"来源）。
    // MS_CANVAS_DBG 门控（常态零开销）。
    {
        static const bool s_dbg = std::getenv("MS_CANVAS_DBG") != nullptr;
        // 用单调场景序号判"换过场景"：每次 MsgScene 都递增；version 字符串
        // 在编辑期往往不变（按 version 判会漏掉全部更新），对象地址又会
        // 被分配器复用（见 scene_changed 处注释）。
        static std::uint64_t s_last_serial = 0;
        if (s_dbg && scene_serial != s_last_serial) {
            std::fprintf(stderr,
                         "[%lld][render:%s] 场景上屏: version=%s"
                         "（静帧节流=%s t=%.3fs）\n",
                         std::chrono::duration_cast<std::chrono::milliseconds>(
                             std::chrono::steady_clock::now().time_since_epoch())
                             .count(),
                         cfg_.player_id.c_str(), scene->version.c_str(),
                         idle_static ? "是" : "否",
                         static_cast<double>(session_t) / 1e6);
            std::fflush(stderr);
            s_last_serial = scene_serial;
        }
    }

    // ---- 编排渲染（映射 → 读已发布帧 → 合成；零解码——解码在
    //      DecodeDriver 线程推进，渲染任何负载下不被解码阻塞）----
    // 【后端分发】loop 已建 backend_ → 经接口合成（GL 后端 SwapBuffers 直
    // 上窗口）；backend_ 空（软渲染基线/测试直调）→ 用 renderer_ 画布，
    // 与旧版逐位一致。composeFrame 是模板（鸭子类型），两分支分别实例化。
    // 【耗时打点】双打点包住 composeFrame 全程（映射+读帧+合成）——
    // 性能基线（软 vs GL vs 硬解）的统一数据源，ema 平滑稳态值。
    const auto cost_t0 = std::chrono::steady_clock::now();
    if (backend_) {
        stats = composeFrame(*scene, my_id, out_w_, out_h_, session_t,
                             scheduler_, *backend_);
        // ---- 待机卡合成路径决策（2026-11-23 语义修正；endFrame 前生效）--
        // items>0 = 画面有内容：清卡；且播放中才记"已播放"（此后撤下
        // 场景也永久回黑屏——用户定案"开始播放后不再显示"）。
        // items=0 = 空时间轴/无活跃 clip：用户视角没有内容 = 待机——
        // 未播过真内容则画待机卡（standby_provider 注入地址），已播过
        // 则清卡回黑。决策在 endFrame 前 → drawStandbyCard 本帧即生效。
        if (stats.items > 0) {
            if (ph.state == PlayState::Playing)
                ever_played_.store(true, std::memory_order_relaxed);
            backend_->setStandby("");
        } else if (cfg_.standby_provider &&
                   !ever_played_.load(std::memory_order_relaxed)) {
            backend_->setStandby(cfg_.standby_provider().c_str());
        } else {
            backend_->setStandby("");
        }
        // ---- 【修复 2026-11-05】诊断捕获必须在 endFrame（SwapBuffers）
        //      之前 ------------------------------------------------------
        // GL 后备缓冲在 SwapBuffers 后内容未定义（驱动普遍清空 BACK），
        // swap 后再 glReadPixels 读到的是黑帧——16:48 联测 display 端
        // "合成日志正常（t=2~8s drawn==items>0）但 12 帧 BMP 全黑"即
        // 此因。移到 swap 前读 BACK = 本帧绘制完毕的确定内容。
        maybeCapture(now_us);
        // ---- 【黑屏根因修复 2026-11-04】呈现调用缺失 -------------------
        // IComposeRenderer 契约是 beginFrame（清屏）→ draw…（写后备缓冲）
        // → endFrame（呈现 SwapBuffers + 泵消息）三段式；composeFrame 只
        // 负责前两段。旧渲染架构里 present 是 loop 内独立的
        // platformPresent 步骤（GL = SwapBuffers）——双线程改造时该调用
        // 丢失，播放路径每帧画满后备缓冲却从不 SwapBuffers：窗口恒黑
        // （DWM 表面冻结在最后一次 swap 的内容——idle ensureReady 成对
        // 调用能上屏、播放恒黑、readPixels 导帧内容完好，三大悖论同源）。
        // 修复：composeFrame 返回即本帧绘制完毕，立即呈现。
        // 【探针】SwapBuffers 耗时（vsync 排队/DWM/驱动层停顿的直接观测；
        // >25ms = 呈现层丢帧，MS_CANVAS_DBG 门控 + 500ms 节流）
        {
            const auto sw_t0 = std::chrono::steady_clock::now();
            backend_->endFrame();
            const auto sw_us = static_cast<std::uint64_t>(
                std::chrono::duration_cast<
                    std::chrono::microseconds>(
                    std::chrono::steady_clock::now() - sw_t0)
                    .count());
            last_swap_cost_us_.store(sw_us, std::memory_order_relaxed);
            static const bool s_dbg = std::getenv("MS_CANVAS_DBG") != nullptr;
            static std::int64_t s_last_log_ms = 0;
            if (s_dbg && sw_us > 25'000) {
                const std::int64_t now_ms =
                    std::chrono::duration_cast<std::chrono::milliseconds>(
                        std::chrono::steady_clock::now().time_since_epoch())
                        .count();
                if (now_ms - s_last_log_ms > 500) {
                    s_last_log_ms = now_ms;
                    std::fprintf(stderr,
                                 "render-probe: SwapBuffers 耗时 %lldus"
                                 "（>25ms 呈现层丢帧）\n",
                                 static_cast<long long>(sw_us));
                    std::fflush(stderr);
                }
            }
        }
    } else {
        stats = composeFrame(*scene, my_id, out_w_, out_h_, session_t,
                             scheduler_, renderer_);
    }
    const auto cost_us = static_cast<std::uint64_t>(
        std::chrono::duration_cast<std::chrono::microseconds>(
            std::chrono::steady_clock::now() - cost_t0)
            .count());
    last_compose_cost_us_.store(cost_us, std::memory_order_relaxed);
    // 【状态沿帧完成打点 2026-11-17】与"状态沿…本帧放行合成"配对：两条
    // 时戳一减 = 该帧从放行到合成+呈现完成的全程耗时；stats 直接可见
    // （items=0 应黑屏 / items>0 画了什么）。配合末段 swap 耗时定位
    // "状态沿放行了但画面迟迟不切"的卡点（合成 vs 呈现 vs 显示设备）。
    if (state_edge) {
        std::fprintf(stderr,
                     "[%lld][render:%s] 状态沿帧完成: compose=%lluus "
                     "swap=%lluus items=%zu drawn=%zu no_frame=%zu t=%.3fs\n",
                     tsMs(),
                     cfg_.player_id.c_str(),
                     static_cast<unsigned long long>(cost_us),
                     static_cast<unsigned long long>(
                         last_swap_cost_us_.load(std::memory_order_relaxed)),
                     stats.items, stats.drawn, stats.no_frame,
                     static_cast<double>(session_t) / 1e6);
        std::fflush(stderr);
    }
    // EMA(α=1/16)：ema += (cost - ema)/16（首样本直接取值——避免从 0 缓升）
    const std::uint64_t prev = ema_compose_cost_us_.load(std::memory_order_relaxed);
    ema_compose_cost_us_.store(
        prev == 0 ? cost_us : prev - prev / 16 + cost_us / 16,
        std::memory_order_relaxed);
    frames_.fetch_add(1, std::memory_order_relaxed);
    // 【停摆取证 2026-11-20】正常帧心跳（5s 节流）——渲染线程活着且到过
    // 函数末段的直接证据（异常态另有 2s 重复提醒，此处补"正常也在转"）。
    {
        static std::int64_t s_last_hb_ms = 0;
        const std::int64_t now_ms = now_us / 1000;
        if (now_ms - s_last_hb_ms >= 5000) {
            s_last_hb_ms = now_ms;
            // 【OOM 取证 2026-11-24】心跳带内存水位（5s 一拍足够看趋势）
            const long rss_kb = procStatusKB("VmRSS:");
            const long hwm_kb = procStatusKB("VmHWM:");
            std::fprintf(stderr,
                         "render:%s: 帧心跳: t=%.3fs items=%zu drawn=%zu"
                         " no_frame=%zu state=%d rss=%ldMB hwm=%ldMB\n",
                         cfg_.player_id.c_str(),
                         static_cast<double>(session_t) / 1e6,
                         stats.items, stats.drawn, stats.no_frame,
                         static_cast<int>(ph.state),
                         rss_kb > 0 ? rss_kb / 1024 : -1L,
                         hwm_kb > 0 ? hwm_kb / 1024 : -1L);
            std::fflush(stderr);
        }
    }

    // ---- 合成诊断（2026-11-04 黑屏排查定位器）------------------------------
    // FrameStats 四计数直接区分黑屏层级（此前 stats 被静默丢弃，排查全靠猜）：
    //   items==0            → 本机无绘制项：clip 时间域不活跃 / screen 与
    //                         clip 不相交 / screen 绑定过滤没命中（映射层）
    //   items>0, no_frame>0 → 解码未发布帧：DecodeDriver 未跑/prepare 失败/
    //                         解码慢于渲染（驱动层——自愈型，画面"迟到"）
    //   items>0, invalid>0  → 后端 draw 返回 false（纹理/域不匹配等绘制层）
    //   drawn==items>0 但窗口黑 → 上屏/交换链问题（显示层）
    // 正常（drawn==items>0）静默零开销；异常态 2s 节流重复提醒（防刷屏）。
    // items==0 时附挂解算要素（活跃 clip 数/播放头/本机屏矩形）——定位
    // 过滤层问题所需的最小信息集。
    {
        const bool abnormal = (stats.items == 0) || stats.no_frame > 0 ||
                              stats.invalid > 0;
        const std::int64_t now_ms = now_us / 1000;
        if (abnormal && now_ms - last_stats_warn_ms_ >= 2000) {
            last_stats_warn_ms_ = now_ms;
            std::fprintf(stderr,
                         "\n[render:%s] S-合成异常: items=%zu drawn=%zu "
                         "no_frame=%zu invalid=%zu t=%.3fs\n",
                         my_id.c_str(), stats.items, stats.drawn,
                         stats.no_frame, stats.invalid,
                         static_cast<double>(session_t) / 1e6);
            if (stats.items == 0) {
                // 映射层异常：列出解算要素（屏绑定/矩形 + clip 时间域）
                for (const auto& s : scene->screens)
                    if (s.player_id == my_id)
                        std::fprintf(stderr,
                                     "  本机屏 %s: 画布矩形 (%.0f,%.0f) "
                                     "%.0fx%.0f\n",
                                     s.id.c_str(), s.x, s.y, s.w, s.h);
                std::size_t total_clips = 0;
                for (const auto& tk : scene->timeline.tracks)
                    total_clips += tk.clips.size();
                std::fprintf(stderr,
                             "  本机无绘制项→核对: 片段时间域是否覆盖"
                             " t（当前轨道共 %zu 个 clip）、screen 矩形"
                             "与 clip 矩形是否相交\n\n",
                             total_clips);
            }
            std::fflush(stderr);
        }
    }

    // ---- 播放冻结兜底日志（2026-11-20 display 视频不刷新现场诊断）--------
    // 现场实锤症状："很多电脑上 display 视频不刷新但音频正常"。正常播放
    // 应每帧 drawn==items>0；Playing 且 items>0 但连续 5s 零绘制 = 视频
    // 链停摆（解码未发布帧 / prepare 持续失败 / 硬解持续失败 / 未来帧
    // 拒绝全拦）。此前该状态只有 MS_CANVAS_DBG=1 才有细分证据——现场
    // 部署多不开调试开关，无从下手。本日志无门控自释（60s 限频周期提醒
    // 防滚出可视区），直接告知停摆事实 + 归档取证路径。
    {
        const bool playing_now = (ph.state == PlayState::Playing);
        const bool frozen =
            playing_now && stats.items > 0 && stats.drawn == 0;
        const std::int64_t now_ms = now_us / 1000;
        if (frozen) {
            if (frozen_since_ms_ == 0) frozen_since_ms_ = now_ms;
            if (now_ms - frozen_since_ms_ >= 5'000 &&
                now_ms - last_frozen_warn_ms_ >= 60'000) {
                last_frozen_warn_ms_ = now_ms;
                std::fprintf(
                    stderr,
                    "\n[render:%s] F-播放停摆: 已 %llds 无一帧绘制"
                    "（呈现链正常，视频链停摆）items=%zu drawn=%zu "
                    "no_frame=%zu invalid=%zu t=%.3fs\n"
                    "  常见成因：视频解码未发布帧（素材未同步/prepare 失败/"
                    "解码吞吐追不上/硬解持续失败/未来帧拒绝）。"
                    "请设 MS_CANVAS_DBG=1 重启收集 [probe]/[decode]/[sched]/"
                    "[media]/[sched] 日志反馈定位。\n\n",
                    my_id.c_str(),
                    static_cast<long long>((now_ms - frozen_since_ms_) / 1000),
                    stats.items, stats.drawn, stats.no_frame, stats.invalid,
                    static_cast<double>(session_t) / 1e6);
                std::fflush(stderr);
            }
        } else {
            frozen_since_ms_ = 0; // 恢复绘制（或非播放态）：检测复位
        }
    }

    // ---- 帧输出回调（显示端预览窗口/平台 present 钩子）----
    // 仅软渲染基线路径回调：GL 后端画面已在 endFrame（SwapBuffers）直上
    // 窗口，再拷贝一份走 GDI 反而多一次全帧拷贝（性能反优化）。
    // 位置约定：composeFrame 返回后画布必为完整帧（beginFrame 清屏 + 逐项
    // 绘制已结束）——早退路径（无场景/未绑定 screen/Idle 节流跳过）不会
    // 走到这里。同步调用：回调实现须只做快照拷贝（见 Config.frame_sink 注释）。
    if (!backend_ && cfg_.frame_sink) {
        const int w = renderer_.width();
        const int h = renderer_.height();
        if (w > 0 && h > 0) {
            const auto& px = renderer_.pixels();
            cfg_.frame_sink(w, h, px.data(),
                            static_cast<std::uint32_t>(w) * 4);
        }
    }

    // ---- 诊断帧导出（联测核对各端画面内容的证据链）-----------------------
    // GL 后端已在 endFrame 前 capture（swap 后 BACK 未定义，见上）；
    // 软渲染基线无 swap 概念，画布内容常驻，在此处导出。
    if (!backend_) maybeCapture(now_us);
    return stats;
}

// ---------------------------------------------------------------------------
// audioTick（P3）：渲染拍顺路的音频混音
//
// 时间语义（与编辑器 audioTick 同口径，A/V 同步的锚）：
//   * 活跃判定 = TimelineEngine::isActive(c, session_t)（与视频取帧同一
//     纯函数，前闭后开同口径）；
//   * 增益 = TweenEngine::resolveGain(c, session_t)（通道整条接管/静态
//     基准一个出口，钳 [0,2]）；
//   * 素材内时间 = in_us + (t - start)（与视频取帧同口径——裁剪片段
//     音画同点位；AudioMediaSource::read 对前后跳自消化 drop/seek）；
//   * 产出窗长 = 会话时钟推进量（session-locked，方案 P3 同拍产出）——
//     音频样本数与画面时间轴严格同比例（48 样本/会话毫秒），A/V 无
//     累积漂移；与设备时钟的速率差由 AudioOut 环水位裁齐消化（P4）。
//   * 非 Playing：混空声源（静音窗，电平归零）——暂停 = 播放头冻结 ≠
//     画面/声音冻结语义下的自然静音；恢复后声源游标连续，无爆音。
// 音源选取（编辑器同口径，实现在 audio/AudioCasting.hpp 单一出口）：
// 真实素材音频优先（池化跨拍复用）；无音轨/打开失败 = 无声（不建
// voice，文件到达后下拍自动接管）；MS_AUDIO_SINE=1 强制正弦（诊断）。
// ---------------------------------------------------------------------------
void RenderThread::audioTick(const Scene& scene, std::int64_t session_t,
                             bool playing) {
    // 活跃片段筛选/音源池化/voices 组装全部在选源出口（与编辑器同源，
    // A/V 同步锚点逻辑单源化）；非 Playing 清空 voices = 静音窗。
    if (playing)
        ms::audio::buildVoices(scene, session_t, cfg_.assets_root,
                               audio_sources_, audio_voices_);
    else
        audio_voices_.clear();

    // 窗长 = 会话时钟推进量（48 样本/会话毫秒；钳制在引擎内）
    std::uint32_t frames = 1'600; // 标准窗（首拍/回退路径）
    if (last_audio_t_us_ != std::numeric_limits<std::int64_t>::min()) {
        const std::int64_t dt = session_t - last_audio_t_us_;
        if (dt > 0) {
            if (dt > 500'000) { // 长停顿后续播：旧位置音频已失配 → 清环
                audio_engine_->flushOutput();
            }
            // 窗长换算单一出口（framesForWindowUs：四舍五入 + 钳窗，
            // 与 editor echo 拍同源——【S2 修复 2026-11-11】原
            // `dt*48/1'000` 整数截断致供给欠 604ppm → 周期 underrun）
            frames = framesForWindowUs(dt);
        } else if (dt == 0) {
            frames = 0; // 同栅格重复渲染（Paused/静帧节流拍）：零产出——
                        // 会话时钟未推进，产出即过产（环水位虚高）
        } else {
            // dt < 0（重锚回跳/seek 重灌）：丢弃输出环内旧位置样本
            //（不清会按旧内容继续出声 ~1-2 窗 = 跳转后串音），
            // 再按新位置重灌一标准窗——声源 read 的回退 seek 自然对位
            audio_engine_->flushOutput();
        }
    }
    if (frames > 0) audio_engine_->mixFrames(audio_voices_, session_t, frames);
    last_audio_t_us_ = session_t;

    // ---- 观测（2026-11-12）：voice 变化即打 + 电平 1s 一行 ----
    // session_t 为会话时间（us）——素材内秒 = (session_t - start)/1e6，
    // 这里只打会话秒与电平，够定位"出没出声/何时开始"。
    if (audio_voices_.size() != last_voice_cnt_) {
        last_voice_cnt_ = audio_voices_.size();
        std::fprintf(stderr,
                     "render: audio voices=%zu t=%.2fs playing=%d\n",
                     audio_voices_.size(),
                     static_cast<double>(session_t) / 1e6,
                     playing ? 1 : 0);
        std::fflush(stderr);
        audio_log_div_ = 0;
    } else if (++audio_log_div_ >= 60) { // ≈1s@60fps：电平心跳
        audio_log_div_ = 0;
        std::fprintf(stderr, "render: audio lvl=%.4f t=%.2fs\n",
                     audio_engine_ ? audio_engine_->level() : 0.0f,
                     static_cast<double>(session_t) / 1e6);
        std::fflush(stderr);
    }
}

// ---------------------------------------------------------------------------
// 输出尺寸解析（三级优先：cfg 显式 > 后端实际表面 > screen 兜底）
// ---------------------------------------------------------------------------
bool RenderThread::ensureOutputSize(const Scene& scene,
                                    const std::string& id,
                                    std::int64_t now_us) {
    // ---- 优先级 1：显式指定（Editor 预览窗/NDI 虚拟输出/测试固定分辨率）
    if (cfg_.out_w > 0 && cfg_.out_h > 0) {
        out_w_ = cfg_.out_w;
        out_h_ = cfg_.out_h;
        return true;
    }

    // ---- 优先级 2【显示分辨率模型 2026-11-10】：后端实际输出表面 -------
    // GL 窗口客户区由 WM_SIZE 在渲染线程内实时维护（全屏铺满 = 设备物理
    // 分辨率）——渲染光栅恒 = 输出设备实际尺寸，SwapBuffers 恒点对点 1:1，
    // 取景矩形（Screen.w/h）只决定画布内容映射进该光栅（比例失配被吸收
    // 进取景映射段——Stretch 铺满为默认语义）。窗口未创建（首帧前）/
    // 已销毁 / Soft 后端（无窗口）→ 0 → 落入兜底，与旧行为一致。
    if (backend_) {
        const int bw = backend_->outputWidth();
        const int bh = backend_->outputHeight();
        if (bw > 0 && bh > 0) {
            out_w_ = bw;
            out_h_ = bh;

            // ---- 比例失配提示（预期行为的醒目知会，10s 限频）------------
            // 取景矩形（Screen.w/h）比例 vs 输出光栅比例差 >2% 时提示：
            // 内容将被 Stretch 拉伸铺满（显示分辨率模型下无 letterbox）。
            // 调整取景比例即可消除——只是知会，非故障。
            const std::int64_t now_ms = now_us / 1000;
            for (const auto& s : scene.screens) {
                if (s.player_id != id || s.w <= 0.0 || s.h <= 0.0) continue;
                const double scr_ar = s.w / s.h;
                const double out_ar = static_cast<double>(bw) /
                                      static_cast<double>(bh);
                if (std::abs(scr_ar - out_ar) / out_ar > 0.02 &&
                    now_ms - last_ratio_warn_ms_ >= 10'000) {
                    last_ratio_warn_ms_ = now_ms;
                    std::fprintf(stderr,
                                 "render:%s: 提示: 取景矩形 %.0fx%.0f"
                                 "（比例 %.2f）与输出光栅 %dx%d（比例 "
                                 "%.2f）不一致——内容将拉伸铺满（取景只"
                                 "管内容，输出恒 1:1；等比请调整取景矩"
                                 "形比例）\n",
                                 id.c_str(), s.w, s.h, scr_ar, bw, bh,
                                 out_ar);
                    std::fflush(stderr);
                }
                break; // 与输出尺寸解析同口径：首个匹配 screen
            }
            return true;
        }
    }

    // ---- 优先级 3：兜底（软渲染基线/headless/测试——GL 窗口未建首帧）---
    // 自动：首个绑定本机（生效 id，指派模型下 = Editor 指派值）的 screen，
    // 画布尺寸 1:1 输出（拉伸 scale=1）
    for (const auto& s : scene.screens) {
        if (s.player_id == id && s.w > 0.0 && s.h > 0.0) {
            out_w_ = static_cast<int>(s.w);
            out_h_ = static_cast<int>(s.h);
            return true;
        }
    }
    return false; // 场景未绑定本机（尚未注册/绑屏错误）
}

// ---------------------------------------------------------------------------
// 诊断帧导出（节流：间隔 + 上限双保护）
// ---------------------------------------------------------------------------
void RenderThread::maybeCapture(std::int64_t now_us) {
    if (cfg_.capture_dir.empty()) return;
    if (captured_.load(std::memory_order_relaxed) >= cfg_.capture_max) return;
    if (last_capture_us_ != 0 &&
        now_us - last_capture_us_ < cfg_.capture_interval_us)
        return;
    last_capture_us_ = now_us;

    // 目录保障：renderOnce 确定性测试不经过 start()，此处幂等兜底
    std::error_code ec;
    std::filesystem::create_directories(ms::u8Path(cfg_.capture_dir), ec);

    char name[64];
    std::snprintf(name, sizeof(name), "frame_%06llu.bmp",
                  static_cast<unsigned long long>(
                      frames_.load(std::memory_order_relaxed)));
    const std::string path = cfg_.capture_dir + "/" + name;

    // ---- 取像素：统一经活动后端回读（接口化）----
    //   * GL 后端：glReadPixels（bottom-up）→ 后端内翻转为 top-down；
    //     尺寸 = 窗口客户区（可能与画布不同，BMP 按实际值写）；
    //   * 软渲染基线（backend_ 空）：直读画布（零转换；测试同路径）。
    std::vector<std::uint8_t> px;
    int cap_w = 0, cap_h = 0;
    std::uint32_t stride = 0;
    bool have_px = false;
    if (backend_) {
        have_px = backend_->readPixels(px, cap_w, cap_h, stride);
    } else if (renderer_.width() > 0 && renderer_.height() > 0) {
        px      = renderer_.pixels(); // 拷贝快照（导帧 1s/次，开销可忽略）
        cap_w   = renderer_.width();
        cap_h   = renderer_.height();
        stride  = static_cast<std::uint32_t>(cap_w) * 4;
        have_px = true;
    }
    if (have_px && writeBmp(path, cap_w, cap_h, px.data(), stride)) {
        captured_.fetch_add(1, std::memory_order_relaxed);
        std::printf("render:%s: 诊断帧: %s\n", cfg_.player_id.c_str(),
                    path.c_str());
    } else {
        std::fprintf(stderr, "render:%s: 诊断帧写入失败: %s\n",
                     cfg_.player_id.c_str(), path.c_str());
    }
}

// ---------------------------------------------------------------------------
// BMP 导出（RGBA 画布 → 32bpp BGRX 底行优先）
// ---------------------------------------------------------------------------
bool RenderThread::writeBmp(const std::string& path, int w, int h,
                            const std::uint8_t* rgba, std::uint32_t stride) {
    if (w <= 0 || h <= 0 || rgba == nullptr) return false;
    const std::size_t row_bytes = static_cast<std::size_t>(w) * 4;
    const std::uint32_t img_bytes =
        static_cast<std::uint32_t>(row_bytes) * static_cast<std::uint32_t>(h);

    // ---- 54 字节头（BITMAPFILEHEADER 14B + BITMAPINFOHEADER 40B）----
    std::vector<std::uint8_t> hdr;
    hdr.reserve(54);
    hdr.push_back('B'); hdr.push_back('M');       // bfType
    put32(hdr, 54 + img_bytes);                   // bfSize（文件总长）
    put16(hdr, 0); put16(hdr, 0);                 // bfReserved1/2
    put32(hdr, 54);                               // bfOffBits（像素数据偏移）
    put32(hdr, 40);                               // biSize（INFOHEADER 大小）
    put32(hdr, static_cast<std::uint32_t>(w));    // biWidth
    put32(hdr, static_cast<std::uint32_t>(h));    // biHeight（正 = 底行优先）
    put16(hdr, 1);                                // biPlanes
    put16(hdr, 32);                               // biBitCount
    put32(hdr, 0);                                // biCompression = BI_RGB
    put32(hdr, img_bytes);                        // biSizeImage
    put32(hdr, 0); put32(hdr, 0);                 // biXPelsPerMeter/Y
    put32(hdr, 0);                                // biClrUsed
    put32(hdr, 0);                                // biClrImportant

    std::ofstream out(ms::u8Path(path), std::ios::binary);
    if (!out) return false;
    out.write(reinterpret_cast<const char*>(hdr.data()),
              static_cast<std::streamsize>(hdr.size()));

    // ---- 像素：RGBA → BGRX，底行优先（BMP 自下而上存储）----
    std::vector<std::uint8_t> row(row_bytes);
    for (int y = h - 1; y >= 0; --y) {
        const std::uint8_t* src =
            rgba + static_cast<std::size_t>(y) * stride;
        for (int x = 0; x < w; ++x) {
            row[x * 4 + 0] = src[x * 4 + 2];      // B ← R 通道互换
            row[x * 4 + 1] = src[x * 4 + 1];      // G
            row[x * 4 + 2] = src[x * 4 + 0];      // R ← B 通道互换
            row[x * 4 + 3] = 0xFF;                // X（BMP 无 alpha，填满）
        }
        out.write(reinterpret_cast<const char*>(row.data()),
                  static_cast<std::streamsize>(row_bytes));
    }
    return out.good();
}

} // namespace ms
