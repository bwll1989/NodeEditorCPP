// ============================================================================
//  TimelineAudioBusHost.hpp —— 编辑器侧 timeline 混音写入通道（T1，2026-11-13）
//
//  设计依据：《docs/TimeLineAudioBus实施方案.md》T1 + 2026-11-13 用户定案：
//    * timeline 从"播放器"降级为"音频源"：runtime 混音后经命名环形队列
//      （TimelineAudioBusRegistry）进入节点图，TimeLineAudioBus 节点按
//      同一全局帧计数读取——音频样本不跨 timeline/图边界，跨界只传控制面；
//    * 【bus 分配语义（2026-11-13 用户定案修正）】**单通道端口**：全场景
//      混音一次（buildVoices track_index=-1），混音输出按通道拆分——一个
//      端口对应混音输出一个通道："BUS N" ↔ 通道 N-1（BUS 1=FL、BUS 2=FR，
//      混音域恒立体声；节点增删端口=通道增删）。无对应混音通道的 bus 写
//      静音帧（时间戳链无洞）；
//    * 【单写者铁律】每队列一个写者：节点 driveMode=external（默认）时
//      host 是唯一写者；非 external（test/silence）的 bus host 跳过
//     （经 TimelineAudioBusRegistry 的 driveMode 登记协调）；
//    * 【时间自锚定推进】GUI 只发命令事件（play/seek/pause/stop），host
//      线程以全局帧计数自推会话时间 t_us = anchor + Δclock×帧间隔——与
//      节点读侧同钟（音画同拍锚点），不回读 GUI 33ms 台阶播放头；
//    * 混音口径与 display 渲染拍同源：ms::audio::buildVoices（选源单一
//      出口）+ ms::AudioEngine::mixFrames（位置直通，立体声输出）。
//
//  线程模型：
//    * 命令面（GUI 线程）：commandPlay/Pause/Seek/Stop/updateScene——
//      锁内写状态（mutex 保护，微秒级）；
//    * worker 线程（AudioTickWaiter 驱动，TimestampGenerator 50fps 唤醒）：
//      每 tick 读状态快照 → 解 voices → 混音 → 拆通道写队列。
//
//  时间戳口径（全系统统一）：块 timestamp = 全局帧计数 + FIXED_DELAY_FRAMES
//  （与 TimeLineAudioBus 节点读侧/AudioDecoder 等源节点同口径）；块长 =
//  getSamplesPerFrame(48000)（50fps → 960 恰整除，与节点同款换算）。
// ============================================================================

#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <QByteArray>
#include <QStringList>
#include <QTimer>   // 电平观测泵（完整类型——inline 析构在含方实例化成员析构，unique_ptr 需完整 deleter）

#include "audio/AudioCasting.hpp"      // buildVoices（选源单一出口）
#include "audio/AudioEngine.hpp"       // AudioEngine / AudioVoice
#include "audio/AudioTypes.hpp"
#include "audio/IAudioOutput.hpp"
#include "Common/DataTypes/AudioTimestampRingQueue.h"
#include "Common/DataTypes/TimelineAudioBusRegistry.h"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include "model/SceneModel.hpp"

namespace ms {

// ---------------------------------------------------------------------------
// BusChannelSplitOutput —— IAudioOutput → 注册表环形队列（按通道拆分）
//
// 混音域规格（AudioSpec.channels，当前恒 2 = FL/FR）：mixFrames 每窗恰好
// write 一次交错流；本实现把交错样本 deinterleave 成单声道流，通道 c 打包
// AudioFrame（48k/1ch/f32、timestamp=pendingTimestamp_）push 进第 c 个队列。
// 队列清单每 tick 由 host 从注册表刷新（weak_ptr；节点增删端口动态跟随）。
// ---------------------------------------------------------------------------
class BusChannelSplitOutput final : public IAudioOutput {
public:
    bool open(const AudioSpec& spec) override {
        spec_ = spec;
        return true;
    }
    void write(const float* interleaved, std::uint32_t frames) override {
        if (frames == 0) return;
        const int ch = spec_.channels;
        for (int c = 0; c < ch; ++c) {
            auto q = (static_cast<std::size_t>(c) < channelQueues_.size())
                         ? channelQueues_[static_cast<std::size_t>(c)].lock()
                         : nullptr;
            if (!q) continue;
            AudioFrame f;
            f.sampleRate    = spec_.sample_rate;
            f.channels      = 1;          // 单通道端口语义（一端口一通道）
            f.bitsPerSample = 32;
            f.timestamp     = pendingTimestamp_;
            // 抽取通道 c：交错流第 c 列 → 连续单声道缓冲
            QByteArray data;
            data.resize(static_cast<qsizetype>(frames) *
                        static_cast<int>(sizeof(float)));
            auto* dst = reinterpret_cast<float*>(data.data());
            for (std::uint32_t fi = 0; fi < frames; ++fi)
                dst[fi] = interleaved[static_cast<std::size_t>(fi) * ch + c];
            f.data = std::move(data);
            q->pushFrame(f);
        }
    }
    void close() override { channelQueues_.clear(); }
    const char* name() const override { return "TimelineBusRingQueue"; }

    AudioSpec spec_{};
    qint64 pendingTimestamp_ = 0;   ///< 本拍块时间戳（host 盖章）
    /// 混音输出通道 c → 目标队列（"BUS c+1"；weak 引用，host 每 tick 刷新）
    std::vector<std::weak_ptr<AudioTimestampRingQueue>> channelQueues_;
};

// ---------------------------------------------------------------------------
// TimelineAudioBusHost —— runtime 侧混音写入宿主
// ---------------------------------------------------------------------------
class TimelineAudioBusHost {
public:
    TimelineAudioBusHost() = default;
    ~TimelineAudioBusHost() { stop(); }
    TimelineAudioBusHost(const TimelineAudioBusHost&) = delete;
    TimelineAudioBusHost& operator=(const TimelineAudioBusHost&) = delete;

    // ---- 生命周期（MainWindow 构造 start / 析构 stop）-----------------------
    void start();
    void stop();

    // ---- 播放命令面（GUI 线程；对齐 msHandlePlay/Pause/Stop 与 seek）--------
    /// 起播：锚定会话起点（start_us = 当前播放头）+ 清全部 bus 队列残留
    void commandPlay(std::int64_t start_us);
    /// 暂停：停止写队列（下游消费完自然静音，方案 §T1）
    void commandPause();
    /// 跳转：播放中 → 重锚（worker 检测代际变化 flush）；暂停/停止态 →
    /// 忽略（起播时 commandPlay 携带定位点）
    void commandSeek(std::int64_t session_us);
    /// 停止：回非播放态（定位归零交 commandPlay 携带）
    void commandStop();

    // ---- 场景快照刷新（GUI 线程；自动追平/手动下发/装载点调用）--------------
    /// 锁内深拷贝工作场景——worker 每拍锁内取 shared_ptr（无深拷贝）
    void updateScene(const Scene& scene);

    // ---- 观测（GUI 任意拍轮询）---------------------------------------------
    /// 混音 RMS 电平（整体混音输出，engine.level() 原子快照——worker 每拍
    /// 混音后写入；非播放拍归零。对齐合并前 EditorWindow 电平表数据源语义）
    float busLevel(int busIndex) const;
    bool  running() const noexcept { return running_.load(std::memory_order_acquire); }

    // ---- 电平观测泵（2026-11-14 内置；电平表数据泵随数据源走）--------------
    /// 注入电平消费端并启动 33ms 采样拍（host 自持定时器——采样节奏归音频
    /// 宿主管，宿主外零泵代码；传空 function = 停泵）。sink 在 GUI 线程回调
    /// （timer 直连，本类与 timer 同在主线程），实参 = busLevel(0) 总输出
    /// RMS 快照（非播放拍已归零，消费端无需判播放态）。
    void setLevelPumpSink(std::function<void(float)> sink);

private:
    /// 混音核心：单引擎 + 拆分输出 + 全场景音源池（跨拍复用解码器）
    struct MixCore {
        std::unique_ptr<AudioEngine> engine;
        BusChannelSplitOutput*       out = nullptr;  ///< engine 拥有（裸别名）
        std::map<std::string, AudioPoolEntry> pool;  ///< 选源池（clip id → 条目）
        bool started = false;                        ///< engine.start 已调用
    };

    void loop();
    /// flush 全部 bus 队列（seek/起播重对位——环内旧位置音频清掉防串音）
    void flushAllQueues();
    /// 向队列推一拍静音帧（单声道；时间戳链无洞——超过混音域通道数的
    /// 端口由此补齐）
    static void pushSilence(const std::shared_ptr<AudioTimestampRingQueue>& q,
                            qint64 timestamp, int frames);

    // ---- 状态（cmd_mutex_ 保护；worker 每拍快照）---------------------------
    mutable std::mutex cmd_mutex_;
    bool         playing_         = false;   ///< 播放态
    std::int64_t anchor_session_us_ = 0;    ///< 锚定会话时间（起播/seek 定位）
    qint64       anchor_clock_    = 0;      ///< 锚定全局帧计数
    qint64       anchor_epoch_    = 0;      ///< 锚点代际（seek 重锚 → worker flush 判据）
    std::shared_ptr<const Scene> scene_;    ///< 工作场景快照（GUI 线程换发）
    std::vector<AudioVoice> voices_;        ///< 本拍混音输入缓冲（worker 线程复用）

    // ---- worker 侧（仅 worker 线程访问）------------------------------------
    MixCore core_;                            ///< 单混音核心（全场景一次混音）
    qint64  last_diag_clock_ = -1'000'000;    ///< 诊断心跳节流（上次打点帧计数）
    qint64  last_voice_diag_clock_ = -1'000'000; ///< 播放态 voices/lvl 心跳独立节流（2026-11-13 修：与 tick 心跳共用变量会互相吞噬）

    // ---- 电平观测（worker 写 / GUI 读；对齐 AudioEngine::level 原子口径）--
    //  engine 指针本身在 worker 侧 core_，GUI 不直接持有——worker 每拍把
    //  engine.level() 快照到此原子，busLevel() 读它（零锁零竞态）
    std::atomic<float> mix_level_{0.0f};

    // ---- 电平观测泵（GUI 线程；setLevelPumpSink 注入即启）------------------
    //  unique_ptr 析构自动停泵（stop() 有 !running early-return，清理不依赖它）
    std::unique_ptr<QTimer>    level_pump_;   ///< 33ms 采样拍（主线程）
    std::function<void(float)> level_sink_;   ///< 电平消费端（宿主注入转发）

    AudioTickWaiter  tickWaiter_;
    std::atomic<bool> running_{false};
    std::atomic<bool> stopRequested_{false};
    std::thread      thread_;
};

} // namespace ms
