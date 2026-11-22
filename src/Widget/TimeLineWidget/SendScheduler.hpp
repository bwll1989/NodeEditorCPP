// ============================================================================
//  SendScheduler.hpp —— 时敏指令发送调度线程（2026-11-22 治本落地）
// ============================================================================
// 背景（GUI 饥饿丢发 bug 的治本）：Mapping（逐帧表达式求值发 OSC）与
// Artnet（逐帧解码 DMX 视频写状态表）是"每帧一次"语义的时敏发送——旧
// 实现挂在适配器 onPlayheadAdvanced 虚钩子（GUI 33ms tick 采样驱动），
// GUI 忙时 tick 派发推迟 → 发送"晚"（9df35f3e 补帧只保"不丢"）；且发送
// 合约分散在片段模型（QJSEngine/FFmpeg 与界面耦合）。
//
// 本类把发送执行独立成专用线程（快照交接，GUI 零阻塞）：
//   * 时钟：构造注入 ClockFn（controller 装配：会话播放优先 sessionTimeUs，
//     否则预览 playheadUs——两者均线程安全；非播放返回 -1）。调度循环每拍
//     现算，无 play/pause/stop 命令协议（时钟有无读数即播放态；播放状态机
//     仍在 GUI controller）；
//   * 节奏：wait_until(min(下一帧格边界, now+5ms))——帧格边界（PAL 25fps
//     = 20ms + k·40ms 网格）醒 = 帧时距内零误差；命令响应（快照/seek poke）
//     ≤5ms。播放期 timeBeginPeriod(1) 压 Windows 15.6ms 时钟中断，停止期
//     timeEndPeriod(1) 归还；
//   * 补帧：上帧→当前帧跨过的每帧补发（cap：Mapping 120 / Artnet 8——
//     每帧 seek+decode 成本高），语义与 9df35f3e 逐位同源；
//   * seek poke：onSeek(us)（视图拖播放头/OSC 跳转，controller 装配）——
//     暂停态即时求值一帧（旧 updateControlTriggers 同语义），播放态作
//     下一拍提示；
//   * 快照：installSnapshot（GUI 线程全量换血，纯值数据）——worker 采纳
//     时撤销旧 Artnet 状态表桶（新配置下一拍重推，旧口径）；DMX 解码器按
//     文件路径缓存跨快照复用；
//   * QJSEngine：worker 线程私有一份（run() 内创建——QObject 亲和当前
//     线程；PI/E 全局注入一次，$percent/$frame 每帧注入）——多序列共享，
//     快照换血不重建。
//
// 线程安全前提（已逐一验证）：ClockFn 读数（MsRuntimeBridge 纯计算 /
// PreviewRenderer playhead_mutex_ 纯函数）、StatusContainer::parseOSC
//（0140db0a 修复：锁内定稿锁外 emit）、ArtnetService 状态表（mInputMutex，
// 50Hz 发送泵独立线程）。Control 片段上升沿不走本线程（GUI 保留——跳变
// 语义无逐帧要求，见适配器 updateControlTriggers）。
// ============================================================================

#pragma once

#include <cstdint>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <QString>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
} // FFmpeg include 路径 = 根 CMakeLists 全局 include_directories（插件同源）

namespace ms {

class PlaybackSendScheduler {
public:
    // ---- 快照数据（GUI 线程构建；worker 只读；纯值自持）-------------------
    /// Mapping 单条消息（clip_data messages 表条目镜像；value 持久化即
    /// 表达式原文，求值在 worker）
    struct MappingEntry {
        QString address;     ///< OSC 地址
        QString type;        ///< 控件类型（"Int"/"Float"/"String"）
        QString expression;  ///< 表达式原文
    };
    /// Mapping 序列：区间 [start, end] 双闭（旧 MappingClipModel 口径）
    struct MappingSeq {
        std::int64_t start_us = 0;
        std::int64_t end_us   = 0;
        std::vector<MappingEntry> entries;
    };
    /// Artnet 序列：区间 [start, end) 前闭后开（库时间约定）；owner 为
    /// 状态表桶令牌（ArtnetService::updateFrame/releaseOwner 的 void* 键
    /// ——快照持有 shared_ptr 保活，地址唯一稳定）
    struct ArtnetSeq {
        std::int64_t start_us = 0;
        std::int64_t end_us   = 0;
        QString file;             ///< DMX 视频媒体库相对路径
        QString host;             ///< 目标主机（广播）
        int start_universe  = 0;  ///< 回放起始域
        std::shared_ptr<void> owner; ///< 状态表桶令牌（非空才发送）
    };
    /// 全量快照（适配器在装载/编辑漏斗构建；installSnapshot 换血）
    struct Snapshot {
        std::vector<MappingSeq> mappings;
        std::vector<ArtnetSeq>  artnets;
    };

    /// 时钟源：返回当前播放头 μs；会话与预览都不在播返回 -1（= 空闲）。
    /// 线程安全（controller 注入双域：会话 playing → sessionTimeUs()，
    /// 否则预览 playing → playheadUs()——纯函数）。
    using ClockFn = std::function<std::int64_t()>;

    explicit PlaybackSendScheduler(ClockFn clock);
    /// 停线程（置退出 + 唤醒 + join）；join 后撤销本调度器名下全部
    /// Artnet 状态表桶（快照 owner 令牌集中释放）+ 释放解码器缓存。
    ~PlaybackSendScheduler();

    PlaybackSendScheduler(const PlaybackSendScheduler&)            = delete;
    PlaybackSendScheduler& operator=(const PlaybackSendScheduler&) = delete;

    /// 【GUI 线程】换血快照（全量替换）。worker 下一拍（≤5ms）采纳：
    /// 撤销旧快照全部 Artnet 状态表桶（新配置下一拍重推——旧 applyPayload
    /// releaseOutputs 同口径）；解码器按绝对路径缓存跨快照复用。传空
    /// shared_ptr = 清空全部序列。
    void installSnapshot(std::shared_ptr<const Snapshot> snap);

    /// 【GUI 线程】seek 通知（视图拖播放头 flush / OSC /currentFrame 命令，
    /// controller seekRequestedUs 装配）：worker 立即唤醒，暂停态对该时刻
    /// 求值一帧（跨帧补发 cap 内——旧 updateControlTriggers 拖动小步补发
    /// 同语义），播放态作下一拍提示。
    void onSeek(std::int64_t us);

private:
    void run(); // 线程主循环

    // ---- worker 私有运行态（仅 run() 线程触碰；快照经 mutex 交接）--------
    struct MappingRun {
        std::int64_t last_frame = -1; ///< 上次已发绝对帧号（usToFrames 域；
                                      ///  -1 = 未初始化只发当前帧；出区间复位）
    };
    /// DMX 视频解码器（ArtnetClipModel 解码栈移植；按绝对路径缓存跨快照
    /// 复用——懒打开/校验/定位解码/提取，全部 worker 线程私有状态）
    struct DmxDecoder {
        AVFormatContext* fmt    = nullptr;
        AVCodecContext*  codec  = nullptr;
        AVFrame*         frame  = nullptr;
        AVPacket*        packet = nullptr;
        QString opened_path;    ///< 已打开绝对路径（复用判定）
        int video_stream = -1;
        double time_base = 0.0;
        double video_fps = 0.0;
        int w = 0, h = 0;       ///< 宽恒 512（DMX 通道数）；高 = universe 数
        std::vector<QByteArray> dmx; ///< 每 universe 512 字节缓存
        bool ensureOpened(const QString& abs_path);  ///< 懒打开 + 校验
        bool readFrameAt(double seconds);            ///< 定位 + 解码一帧
        void extractDmx();                           ///< 当前帧 → dmx
        void reset();                                ///< 释放解码链
        ~DmxDecoder() { reset(); }
    };
    struct ArtnetRun {
        std::shared_ptr<DmxDecoder> dec; ///< 按路径缓存（跨快照复用）
        std::int64_t last_video_frame = -1; ///< 去重键 = 视频自身相对帧号
    };

    std::mutex              mutex_;
    std::condition_variable cv_;
    bool    quit_             = false;  ///< 退出标志
    bool    snapshot_pending_ = false;  ///< 待采纳快照（醒发条件之一）
    std::int64_t seek_us_     = -1;     ///< 待消费 poke（-1 = 无）
    std::shared_ptr<const Snapshot> pending_;   ///< 待采纳快照（锁内交接）
    std::shared_ptr<const Snapshot> current_;   ///< 生效快照（worker 只读）
    ClockFn clock_;                     ///< 时钟源（构造注入，只读）

    std::thread worker_;    ///< 调度线程（最后声明：构造完毕再启动）
};

} // namespace ms
