// ============================================================================
//  TimelineAudioBusHost.cpp —— 编辑器侧 timeline 混音写入通道实现（T1）
//
//  每 tick 流程（方案 §T1；单通道端口语义 2026-11-13 定案修正）：
//    1. 取全局帧计数 clock（TimestampGenerator 单例，50fps 钟线程已唤醒）；
//    2. 锁内快照命令面：playing / 锚点 / 场景 shared_ptr；
//    3. 非播放态：不写（pause 语义），回到等待；
//    4. t_us = anchor_session_us + (clock - anchor_clock) × 帧间隔(us)
//       （64 位；帧间隔 = 1e6 / fps，与节点读侧同钟同拍）；
//    5. 全场景 buildVoices（选源单一出口）→ 单引擎 mixFrames 一次 →
//       拆分输出把交错立体声按通道 deinterleave 成单声道帧：
//       "BUS N" ↔ 混音输出通道 N-1（BUS 1=FL、BUS 2=FR）；
//    6. 超过混音域通道数的 bus 推静音帧（时间戳链无洞）；非 external
//       的 bus 跳过（单写者协调）。
// ============================================================================

#include "TimelineAudioBusHost.hpp"

#include <cmath>
#include <cstdio>
#include <utility>

#include <QTimer>
#include <QObject>

#include "Common/AppConfig/ConstantDefines.h" // AppConfigs::FIXED_DELAY_FRAMES
#include "TimestampGenerator/AudioThreadRealtime.hpp"

namespace ms {

// ---------------------------------------------------------------------------
// 生命周期
// ---------------------------------------------------------------------------
void TimelineAudioBusHost::start() {
    if (running_.load(std::memory_order_acquire)) return;
    stopRequested_.store(false, std::memory_order_relaxed);
    running_.store(true, std::memory_order_release);
    // T3（2026-11-17）：Host 存活即接管播放态发布面——节点 worker 据此
    // 从"本地属性兜底"切到"听 Host"（修复 SyncData playing 恒 false）
    NodeDataTypes::TimelineAudioBusRegistry::instance()->setPlaybackActive(true);
    thread_ = std::thread([this] { loop(); });
}

void TimelineAudioBusHost::stop() {
    if (!running_.load(std::memory_order_acquire)) return;
    stopRequested_.store(true, std::memory_order_relaxed);
    tickWaiter_.requestStop();
    if (thread_.joinable()) thread_.join();
    running_.store(false, std::memory_order_release);
    // 退出发布面：节点回退本地属性兜底；播放态清 false（下游定格而非失联误判）
    auto* reg = NodeDataTypes::TimelineAudioBusRegistry::instance();
    reg->setPlaybackPlaying(false);
    reg->setPlaybackActive(false);
}

// ---------------------------------------------------------------------------
// 电平观测泵（GUI 线程；2026-11-14 从 MainWindow 收编内置——采样节奏归
// 音频宿主管，宿主外零泵代码。33ms≈30fps 显示惯例，与混音 50fps 拍率解耦；
// 数据面仍走 mix_level_ 原子快照，泵只是 GUI 侧读拍，不跨线程直推 UI）
// ---------------------------------------------------------------------------
void TimelineAudioBusHost::setLevelPumpSink(std::function<void(float)> sink) {
    level_sink_ = std::move(sink);
    if (level_sink_ && !level_pump_) {
        level_pump_ = std::make_unique<QTimer>();
        level_pump_->setInterval(33);
        // 无上下文对象重载：functor 生命周期绑定 timer 本身（timer 销毁即断）
        QObject::connect(level_pump_.get(), &QTimer::timeout, [this] {
            if (level_sink_) level_sink_(busLevel(0));   // 0=总输出
        });
        level_pump_->start();
    } else if (!level_sink_ && level_pump_) {
        level_pump_->stop();
    }
}

// ---------------------------------------------------------------------------
// 命令面（GUI 线程）
// ---------------------------------------------------------------------------
void TimelineAudioBusHost::commandPlay(std::int64_t start_us) {
    std::lock_guard<std::mutex> lk(cmd_mutex_);
    anchor_session_us_ = std::max<std::int64_t>(0, start_us);
    anchor_clock_      = TimestampGenerator::getInstance()->getCurrentFrameCount();
    ++anchor_epoch_;   // 起播代际推进：worker 检测到即 flush（清旧会话残留）
    playing_           = true;
    // T3：播放态即时发布（不等下一拍）——先定位点后 playing，下游读侧
    // 即便读到中间态也只是晚一拍起播
    auto* reg = NodeDataTypes::TimelineAudioBusRegistry::instance();
    reg->setPlaybackPosition(anchor_session_us_ / 1e6, anchor_clock_);
    reg->setPlaybackPlaying(true);
}

void TimelineAudioBusHost::commandPause() {
    std::lock_guard<std::mutex> lk(cmd_mutex_);
    playing_ = false;
    // T3：立即失播——播放头原子保留最后一拍值（节点外推随 playing=false 定格）
    NodeDataTypes::TimelineAudioBusRegistry::instance()->setPlaybackPlaying(false);
}

void TimelineAudioBusHost::commandSeek(std::int64_t session_us) {
    std::lock_guard<std::mutex> lk(cmd_mutex_);
    if (!playing_) return; // 暂停/停止态：定位点由 commandPlay 携带
    // 播放中跳转：重锚（时间戳链由 worker 检测锚点代际后 flush 统一做）
    anchor_session_us_ = std::max<std::int64_t>(0, session_us);
    anchor_clock_      = TimestampGenerator::getInstance()->getCurrentFrameCount();
    ++anchor_epoch_;
    // T3：同步发布面重锚——否则下游 SyncData 外推继续用旧定位点滑向错位
    NodeDataTypes::TimelineAudioBusRegistry::instance()->setPlaybackPosition(
        anchor_session_us_ / 1e6, anchor_clock_);
}

void TimelineAudioBusHost::commandStop() {
    std::lock_guard<std::mutex> lk(cmd_mutex_);
    playing_ = false;
    // T3：同 pause——播放头保留、播放态清 false（定位归零交 commandPlay 携带）
    NodeDataTypes::TimelineAudioBusRegistry::instance()->setPlaybackPlaying(false);
}

void TimelineAudioBusHost::updateScene(const Scene& scene) {
    auto snap = std::make_shared<const Scene>(scene); // GUI 线程深拷贝一次
    std::lock_guard<std::mutex> lk(cmd_mutex_);
    scene_ = std::move(snap);
}

float TimelineAudioBusHost::busLevel(int busIndex) const {
    Q_UNUSED(busIndex); // 总输出电平语义（对齐合并前 EditorWindow 电平表）
    return mix_level_.load(std::memory_order_relaxed);
}

// ---------------------------------------------------------------------------
// worker 主循环
// ---------------------------------------------------------------------------
void TimelineAudioBusHost::loop() {
    AudioThreadRealtimeGuard realtimeGuard(L"Pro Audio");
    auto* gen = TimestampGenerator::getInstance();
    gen->registerAudioTickWaiter(&tickWaiter_);

    auto* registry = NodeDataTypes::TimelineAudioBusRegistry::instance();
    qint64 last_anchor_epoch = -1;   ///< 锚点代际（seek 检测 → flush）
    qint64 last_written_clock = -1;  ///< 上一拍帧计数（回退检测）

    while (!stopRequested_.load(std::memory_order_relaxed)) {
        if (!tickWaiter_.wait(200)) {
            if (stopRequested_.load(std::memory_order_relaxed)) break;
            continue; // 超时兜底（钟线程正常时不会走到）
        }
        if (stopRequested_.load(std::memory_order_relaxed)) break;

        const qint64 clock = gen->getCurrentFrameCount();

        // ---- 命令面快照（锁内值拷贝，微秒级）--------------------------
        bool         playing = false;
        std::int64_t anchor_us = 0;
        qint64       anchor_clock = 0;
        qint64       anchor_epoch = 0;
        std::shared_ptr<const Scene> scene;
        {
            std::lock_guard<std::mutex> lk(cmd_mutex_);
            playing      = playing_;
            anchor_us    = anchor_session_us_;
            anchor_clock = anchor_clock_;
            anchor_epoch = anchor_epoch_;
            scene        = scene_;
        }
        // seek/起播重锚检测（锚点代际）+ 时钟回退（resetFrameCount）：
        // 旧位置音频作废——flush 环即可。【2026-11-15 不再清音源池】解码器
        // read(media_us) 自带回退/前向大跳 doSeek（AudioMediaSource 401/411
        // 行）完全自定位，复用条目 = 快路径（与 display 端同款行为）；此前
        // 每次重锚 pool.clear() = 全源重新 avformat_open_input（每源数十至
        // 数百 ms 串行）——display 端跳转后音频很快输出、编辑端"要等一下
        // 才出声"的根源（display 复用源 doSeek，编辑端重建源）。池失效由
        // buildVoices 逐条目素材路径指纹自愈（换绑/删除素材按条目重建）。
        if (anchor_epoch != last_anchor_epoch || clock <= last_written_clock) {
            flushAllQueues();
            last_anchor_epoch = anchor_epoch;
        }
        last_written_clock = clock;
        // 场景换发（updateScene 深拷贝换指针）不整池清空——编辑期每次发布
        // （含纯几何改动）都会换发快照，整池 clear = 全源重新
        // avformat_open_input（每源数十至数百 ms 串行）→ voices 空窗，
        // 听感"改属性时播放声音暂停"。失效判定交给 buildVoices 逐条目
        // 指纹（池键=clip id + 条目记素材路径，不匹配才 erase 重建）：
        // 换绑/删除素材按条目自愈，与 display 端（RenderThread 池仅 stop
        // 时清）及重锚路径同款行为；残留已删 clip 条目由池上限兜底。

        // 【诊断期】1s 节流恒打心跳（含非播放态）：playing/scene/buses
        // 状态可见化——定位无声问题必看；定版后移回播放态内
        if (clock - last_diag_clock_ >=
            static_cast<qint64>(std::llround(gen->getFrameRate() > 0 ? gen->getFrameRate() : 50))) {
            last_diag_clock_ = clock;
            std::size_t nb = 0;
            for (const QString& n : registry->buses())
                if (registry->busDriveMode(n) == QStringLiteral("external")) ++nb;
            std::fprintf(stderr,
                         "tl-audio-bus: tick playing=%d scene=%d buses=%zu "
                         "clock=%lld\n",
                         playing ? 1 : 0, scene ? 1 : 0, nb,
                         static_cast<long long>(clock));
            std::fflush(stderr);
        }

        if (!playing || !scene) {
            // 非播放拍电平归零（对齐合并前 EditorWindow「非 Playing 恒混
            // 空声源 → 电平表归零」——GUI 轮询侧不需自判播放态）
            mix_level_.store(0.0f, std::memory_order_relaxed);
            continue;
        }

        // ---- 时间推进（自锚定：与节点读侧同钟同拍）---------------------
        const double fps = gen->getFrameRate();
        if (fps <= 0.0) continue;
        const std::int64_t frame_interval_us =
            static_cast<std::int64_t>(std::llround(1'000'000.0 / fps));
        const std::int64_t t_us = anchor_us +
            (clock - anchor_clock) * frame_interval_us;
        // T3：播放头连续发布（会话秒 + 本拍全局帧锚点）——节点 worker
        // 每拍轮询读出，经 SyncData 外推供 LTCGenerator 等下游跟随
        registry->setPlaybackPosition(t_us / 1e6, clock);
        const qint64 timestamp =
            clock + AppConfigs::FIXED_DELAY_FRAMES;
        const int frames = gen->getSamplesPerFrame(48000);
        if (frames <= 0) continue;

        // ---- 引擎懒装配+启动（混音域恒 48k/2ch；块长与节点同款换算）----
        // 【惰性装配铁律】MixCore 的 engine/out 默认空——首次拍创建 +
        // attach 拆分输出。缺这步 = 空指针解引用；即使 engine 存在而
        // 未 attach 输出，mixFrames 也会静默丢弃样本（"无音频输出"根因）。
        constexpr int kMixChannels = 2; // 混音域通道数（立体声 FL/FR）
        if (!core_.engine) {
            auto out = std::make_unique<BusChannelSplitOutput>();
            core_.out    = out.get();
            core_.engine = std::make_unique<AudioEngine>();
            core_.engine->attach(std::move(out));
        }
        if (!core_.started) {
            AudioSpec spec;
            spec.sample_rate     = 48'000;
            spec.channels        = kMixChannels;
            spec.frames_per_tick = static_cast<std::uint32_t>(frames);
            core_.engine->start(spec);
            core_.started = true;
            std::fprintf(stderr,
                         "tl-audio-bus: 混音引擎启动：48k/%dch/%d帧/拍\n",
                         kMixChannels, frames);
            std::fflush(stderr);
        }

        // ---- 通道→队列映射（每 tick 刷新；节点增删端口动态跟随）--------
        // "BUS N" ↔ 混音输出通道 N-1（单通道端口语义）；超过混音域
        // 通道数（2）的 bus 拿到静音帧（时间戳链无洞）
        std::vector<std::weak_ptr<AudioTimestampRingQueue>> channelQueues;
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> silentQueues;
        for (const QString& name : registry->buses()) {
            // bus 名 → 通道号："BUS N" ↔ 通道 N-1（portCaption 硬编码格式）
            bool ok = false;
            const int n = QStringView(name).mid(4).toInt(&ok);
            if (!ok || n < 1) continue;
            // 单写者协调：节点自驱（test/silence）的 bus 不碰
            if (registry->busDriveMode(name) != QStringLiteral("external"))
                continue;
            auto q = registry->acquire(name);
            if (!q) continue;
            if (n - 1 < kMixChannels) {
                if (static_cast<int>(channelQueues.size()) <= n - 1)
                    channelQueues.resize(static_cast<std::size_t>(n));
                channelQueues[static_cast<std::size_t>(n - 1)] = q;
            } else {
                silentQueues.push_back(std::move(q));
            }
        }

        // ---- 混音 → 拆通道写队列 --------------------------------------
        voices_.clear();
        if (scene) ms::audio::buildVoices(*scene, t_us, std::string(),
                                          core_.pool, voices_);
        // 【无声定位器 2026-11-13】播放中 voices=0 且场景有轨道——打印
        // 场景内片段真实时间字段与播放头，鉴别「快照无片段」vs「时间
        // 字段错位（start/duration 反序列化口径）」vs「素材缺失」
        static int s_empty_diag = 0;
        if (voices_.empty() && scene && !scene->timeline.tracks.empty()
            && s_empty_diag < 3) {
            ++s_empty_diag;
            std::fprintf(stderr,
                         "tl-audio-bus:[诊断] voices=0 但场景有轨道："
                         "t_us=%lld tracks=%zu\n",
                         static_cast<long long>(t_us),
                         scene->timeline.tracks.size());
            for (const auto& tr : scene->timeline.tracks) {
                std::fprintf(stderr, "  track kind=%d clips=%zu\n",
                             static_cast<int>(tr.kind), tr.clips.size());
                for (const auto& c : tr.clips) {
                    std::fprintf(stderr,
                                 "    clip id=%s start=%lld dur=%lld "
                                 "in=%lld asset='%s'\n",
                                 c.id.c_str(),
                                 static_cast<long long>(c.start_us),
                                 static_cast<long long>(c.duration_us),
                                 static_cast<long long>(c.in_us),
                                 c.asset_id.c_str());
                }
            }
            for (const auto& a : scene->assets) {
                std::fprintf(stderr, "    asset id=%s path='%s'\n",
                             a.id.c_str(), a.path.c_str());
            }
            std::fflush(stderr);
        }
        core_.out->channelQueues_ = channelQueues;
        core_.out->pendingTimestamp_ = timestamp;
        core_.engine->mixFrames(voices_, t_us,
                                static_cast<std::uint32_t>(frames));
        // 电平快照（worker 独占 engine——GUI 经 busLevel() 读原子副本）
        mix_level_.store(core_.engine->level(), std::memory_order_relaxed);
        // 混音域之外的通道：静音帧补时间戳链（无对应混音输出）
        for (auto& q : silentQueues) pushSilence(q, timestamp, frames);

        // 诊断心跳（1s 节流）：写入侧存活性 + voices/bus 映射可见化
        // 【2026-11-13 修】独立节流变量——此前与 tick 心跳共用
        // last_diag_clock_，tick 先更新后本心跳永不满足（被吞）
        if (clock - last_voice_diag_clock_ >=
            static_cast<qint64>(std::llround(fps))) {
            last_voice_diag_clock_ = clock;
            std::fprintf(stderr,
                         "tl-audio-bus: t=%.2fs voices=%zu buses=%d "
                         "ts=%lld lvl=%.3f\n",
                         static_cast<double>(t_us) / 1e6,
                         voices_.size(),
                         static_cast<int>(channelQueues.size()) +
                             static_cast<int>(silentQueues.size()),
                         static_cast<long long>(timestamp),
                         core_.engine->level());
            std::fflush(stderr);
        }
    }

    gen->unregisterAudioTickWaiter(&tickWaiter_);
}

// ---------------------------------------------------------------------------
// 静音帧（单声道；块长同款）
// ---------------------------------------------------------------------------
void TimelineAudioBusHost::pushSilence(
        const std::shared_ptr<AudioTimestampRingQueue>& q,
        qint64 timestamp, int frames) {
    if (!q || frames <= 0) return;
    AudioFrame f;
    f.sampleRate    = 48'000;
    f.channels      = 1;
    f.bitsPerSample = 32;
    f.timestamp     = timestamp;
    f.data.assign(static_cast<qsizetype>(frames) *
                  static_cast<int>(sizeof(float)), char(0));
    q->pushFrame(f);
}

void TimelineAudioBusHost::flushAllQueues() {
    auto* registry = NodeDataTypes::TimelineAudioBusRegistry::instance();
    for (const QString& name : registry->buses()) {
        if (registry->busDriveMode(name) != QStringLiteral("external")) continue;
        if (auto q = registry->acquire(name)) q->clear();
    }
}

} // namespace ms
