// ============================================================================
//  SendScheduler.cpp —— 时敏指令发送调度线程实现
// ============================================================================
// 语义移植对照（旧实现 → 本线程，逐位同源）：
//   * Mapping 求值链：MappingClipModel::emitForFrame/evaluateAndSend/evalList
//    （9df35f3e 补帧迭代后版本）——区间 [st,en] 双闭、绝对帧号判等去重、
//     $percent/$frame 注入、with(Math) 求值、List/Int/Float/String 四态、
//     补帧代表时刻 = max(framesToUs(f), st)、出区间复位去重；
//   * Artnet 解码发送链：ArtnetClipModel::onPlayheadAdvanced/emitVideoFrame/
//     ensureVideoOpened/readFrameAt/extractDmx/sendFrames——区间 [st,en)
//     前闭后开、相对视频帧号去重、离开不清零（状态表保持末帧 50Hz 重发）、
//     FFV1 512×N 校验、GRAY8 直读 / RGBA 取 R、owner 桶状态表；
//   * 补帧 cap：Mapping 120 / Artnet 8（每帧 seek+decode 成本高）。
// 差异点（治本带来的唯一行为变化）：
//   * 驱动源从"GUI 33ms tick 采样（适配器 onPlayheadAdvanced 虚钩子）"换成
//     "调度线程按主时钟帧格 deadline 醒"——发送不再受 GUI 事件循环抖动影响
//    （补帧从常态机制退化为时钟读数跳变时的兜底，如 seek/卡顿恢复）；
//   * seek poke 保留暂停态拖动求值（旧 updateControlTriggers 同语义）。
// ============================================================================

#include "SendScheduler.hpp"

#include <algorithm>
#include <chrono>
#include <cmath> // llround
#include <cstdio>
#include <cstdlib>
#include <unordered_map>

#include <QDateTime>
#include <QJSEngine>
#include <QJSValue>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QtMath> // M_PI/M_E（QJSEngine 全局常量，MappingClipModel 同源）

#include "Common/AppConfig/ConstantDefines.h"  // AppConstants::MEDIA_LIBRARY_STORAGE_DIR
#include "Common/AppConfig/OSCMessage.h"      // OSCMessage（parseOSC 载荷）
#include "Common/BaseClass/TimeLineDefines.h" // QtTimeline::usToFrames/framesToUs
#include "Common/Devices/ArtnetSender/ArtnetService.h"
#include "Common/Devices/ArtnetSender/ArtnetFrame.h"
#include "Common/Devices/StatusContainer/StatusContainer.h" // parseOSC 发送入口

#ifdef _WIN32
#ifndef NOMINMAX
#define NOMINMAX // 防 min/max 宏污染 std::min/std::max（本项目约定，同全局先例）
#endif
#include <windows.h> // timeBeginPeriod/timeEndPeriod（winmm）
#pragma comment(lib, "winmm.lib")
#endif

namespace ms {

// ===========================================================================
// 诊断探针（MS_SCHED_DBG=1）：调度拍/补帧/采纳事件 stderr 限速打印
// ===========================================================================
namespace {
bool schedDbg() {
    static const bool on = std::getenv("MS_SCHED_DBG") != nullptr;
    return on;
}
} // namespace

// ===========================================================================
// 构造 / 析构
// ===========================================================================
PlaybackSendScheduler::PlaybackSendScheduler(ClockFn clock)
    : clock_(std::move(clock)) {
    worker_ = std::thread([this] { run(); });
}

PlaybackSendScheduler::~PlaybackSendScheduler() {
    {
        std::lock_guard<std::mutex> lk(mutex_);
        quit_ = true;
    }
    cv_.notify_all();
    if (worker_.joinable()) worker_.join();
    // join 后撤销本调度器名下全部 Artnet 状态表桶（最后快照的 owner 令牌
    // ——50Hz 发送泵还在跑，残留桶会持续广播已停止片段的末帧）
    if (current_) {
        for (const auto& s : current_->artnets)
            if (s.owner)
                ArtnetService::getInstance()->releaseOwner(s.owner.get());
    }
}

// ===========================================================================
// GUI 线程入口：快照换血 / seek poke
// ===========================================================================
void PlaybackSendScheduler::installSnapshot(
        std::shared_ptr<const Snapshot> snap) {
    {
        std::lock_guard<std::mutex> lk(mutex_);
        pending_ = std::move(snap);
        snapshot_pending_ = true;
    }
    cv_.notify_all();
}

void PlaybackSendScheduler::onSeek(std::int64_t us) {
    {
        std::lock_guard<std::mutex> lk(mutex_);
        seek_us_ = us;
    }
    cv_.notify_all();
}

// ===========================================================================
// DmxDecoder —— DMX 视频解码器（ArtnetClipModel 解码栈移植，FFmpeg 8.1 API）
// ===========================================================================
void PlaybackSendScheduler::DmxDecoder::reset() {
    if (frame)  { av_frame_free(&frame);     frame  = nullptr; }
    if (packet) { av_packet_free(&packet);   packet = nullptr; }
    if (codec)  { avcodec_free_context(&codec); codec = nullptr; }
    if (fmt)    { avformat_close_input(&fmt); fmt = nullptr; }
    opened_path.clear();
    video_stream = -1;
    time_base = video_fps = 0.0;
    w = h = 0;
    dmx.clear();
}

/// 懒打开 + 校验（ArtnetClipModel::ensureVideoOpened 逐行移植）：宽必须
/// 512（DMX 通道数），高 ≥1（= universe 数），解码器必须是 FFV1。
bool PlaybackSendScheduler::DmxDecoder::ensureOpened(const QString& abs_path) {
    if (opened_path == abs_path && fmt) return true;
    reset();

    const QByteArray path_utf8 = abs_path.toUtf8();
    if (avformat_open_input(&fmt, path_utf8.constData(), nullptr, nullptr) < 0) {
        fmt = nullptr;
        return false;
    }
    if (avformat_find_stream_info(fmt, nullptr) < 0) {
        avformat_close_input(&fmt);
        fmt = nullptr;
        return false;
    }
    for (unsigned i = 0; i < fmt->nb_streams; ++i) {
        if (fmt->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            video_stream = static_cast<int>(i);
            break;
        }
    }
    if (video_stream < 0) { reset(); return false; }

    const AVCodecParameters* par = fmt->streams[video_stream]->codecpar;
    w = par->width;
    h = par->height;
    if (w != 512 || h < 1) { reset(); return false; }

    const AVCodec* codec_desc = avcodec_find_decoder(par->codec_id);
    if (!codec_desc || codec_desc->id != AV_CODEC_ID_FFV1) { reset(); return false; }

    codec = avcodec_alloc_context3(codec_desc);
    if (!codec) { reset(); return false; }
    if (avcodec_parameters_to_context(codec, par) < 0 ||
        avcodec_open2(codec, codec_desc, nullptr) < 0) {
        reset();
        return false;
    }
    frame  = av_frame_alloc();
    packet = av_packet_alloc();
    if (!frame || !packet) { reset(); return false; }

    const AVStream* vs = fmt->streams[video_stream];
    time_base = av_q2d(vs->time_base);
    video_fps = av_q2d(vs->r_frame_rate);
    if (video_fps <= 0) video_fps = 25.0; // 异常流兜底（与时间轴基准一致）

    // Universe 缓冲初始化（一行 = 一个 universe，各 512 字节清零）
    dmx.clear();
    dmx.reserve(static_cast<size_t>(h));
    for (int i = 0; i < h; ++i) {
        QByteArray u;
        u.resize(512);
        u.fill(char(0x00));
        dmx.push_back(u);
    }
    opened_path = abs_path;
    return true;
}

/// 定位 + 解码一帧（ArtnetClipModel::readFrameAt 逐行移植）：秒 → 流时间戳
/// → av_seek_frame(BACKWARD) + flush → 读解。
bool PlaybackSendScheduler::DmxDecoder::readFrameAt(double seconds) {
    if (!fmt || !codec || time_base <= 0) return false;
    const int64_t target_ts = static_cast<int64_t>(seconds / time_base);
    if (av_seek_frame(fmt, video_stream, target_ts, AVSEEK_FLAG_BACKWARD) < 0)
        return false;
    avcodec_flush_buffers(codec);

    while (av_read_frame(fmt, packet) >= 0) {
        if (packet->stream_index == video_stream) {
            if (avcodec_send_packet(codec, packet) < 0) {
                av_packet_unref(packet);
                continue;
            }
            const int ret = avcodec_receive_frame(codec, frame);
            av_packet_unref(packet);
            if (ret == 0) return true;
            if (ret == AVERROR(EAGAIN)) continue;
            return false;
        }
        av_packet_unref(packet);
    }
    return false;
}

/// 当前解码帧 → dmx（ArtnetClipModel::extractDmx 逐行移植）：灰度（GRAY8）
/// 直读每列像素；非灰度（兼容旧 RGBA 布局）取 R 通道。
/// 【2026-11-27 性能】复用 dmx[universe] 已分配 QByteArray（ensureOpened
/// 路径 reserve(h)+push_back 已就位）——原每帧每 universe 新建 512B 满载
/// 分配，播放期 50Hz×universe 数次堆分配白付。
void PlaybackSendScheduler::DmxDecoder::extractDmx() {
    if (!frame || !frame->data[0]) return;
    const int usable_width = std::min(w, 512);
    for (int universe = 0; universe < h; ++universe) {
        QByteArray& out = dmx[static_cast<size_t>(universe)]; // 复用缓冲
        out.fill(char(0x00));
        const uint8_t* row = frame->data[0] + universe * frame->linesize[0];
        if (frame->format == AV_PIX_FMT_GRAY8) {
            for (int col = 0; col < usable_width; ++col)
                out[col] = char(row[col]);
        } else {
            for (int col = 0; col < usable_width; ++col)
                out[col] = char(row[col * 4]); // RGBA 步长 4，取 R 通道
        }
    }
}

// ===========================================================================
// 线程主循环
// ===========================================================================
void PlaybackSendScheduler::run() {
    // ---- worker 线程私有 QJSEngine（QObject 亲和当前线程；PI/E 全局一次
    // ——MappingClipModel ctor 同源；多序列顺序求值共享；快照换血不重建）----
    QJSEngine engine;
    engine.globalObject().setProperty(QStringLiteral("PI"), M_PI);
    engine.globalObject().setProperty(QStringLiteral("E"),  M_E);

    // ---- 运行态（worker 局部；快照采纳时与快照下标对齐重建）--------------
    std::vector<MappingRun> mruns;
    std::vector<ArtnetRun>  aruns;
    // 解码器按绝对路径缓存（跨快照复用——同路径视频不因编辑重开）
    std::unordered_map<QString, std::shared_ptr<DmxDecoder>> dec_cache;

    bool hr_timer = false; // timeBeginPeriod(1) 生效标志

    // ---- Mapping 单帧求值 + 发送（emitForFrame/evaluateAndSend/evalList
    // 逐行复刻；tUs = 帧代表时刻）------------------------------------------
    auto emitMappingFrame = [&](const MappingSeq& seq, MappingRun& run,
                                std::int64_t t_us) {
        const std::int64_t frame = QtTimeline::usToFrames(t_us);
        if (frame == run.last_frame) return; // 同帧去重（帧域语义）
        run.last_frame = frame;

        const std::int64_t st  = seq.start_us;
        const std::int64_t len = seq.end_us - seq.start_us;
        const double percent = len > 0 ? double(t_us - st) / double(len) : 0.0;
        const int rel_frame =
            static_cast<int>(QtTimeline::usToFrames(t_us - st));
        engine.globalObject().setProperty(QStringLiteral("$percent"), percent);
        engine.globalObject().setProperty(QStringLiteral("$frame"), rel_frame);

        for (const auto& e : seq.entries) {
            OSCMessage m;
            m.address = e.address;
            const QString trimmed = e.expression.trimmed();
            if (trimmed.startsWith(QLatin1Char('['))) {
                // 列表：with(Math) 求值（报错/非数组回退 JSON 直解→空列表）
                m.type  = QStringLiteral("List");
                const QJSValue result = engine.evaluate(
                    QStringLiteral("with(Math) { (%1) }").arg(trimmed));
                QVariantList list;
                if (!result.isError() && result.isArray()) {
                    const int n =
                        result.property(QStringLiteral("length")).toInt();
                    list.reserve(n);
                    for (int i = 0; i < n; ++i) {
                        const QJSValue el = result.property(i);
                        if (el.isNumber())      list.append(el.toNumber());
                        else if (el.isBool())   list.append(el.toBool());
                        else                    list.append(el.toString());
                    }
                } else {
                    const QJsonDocument doc =
                        QJsonDocument::fromJson(trimmed.toUtf8());
                    if (doc.isArray()) list = doc.array().toVariantList();
                }
                m.value = list;
            } else if (e.type == QLatin1String("Int")) {
                const QJSValue r = engine.evaluate(
                    QStringLiteral("with(Math) { %1 }").arg(e.expression));
                m.type  = QStringLiteral("Int");
                m.value = r.isError() ? 0 : r.toInt();
            } else if (e.type == QLatin1String("Float")) {
                const QJSValue r = engine.evaluate(
                    QStringLiteral("with(Math) { %1 }").arg(e.expression));
                m.type  = QStringLiteral("Float");
                m.value = r.isError() ? 0.0 : r.toNumber();
            } else {
                // String：表达式原文直发（不修剪——控件同口径）
                m.type  = QStringLiteral("String");
                m.value = e.expression;
            }
            StatusContainer::instance()->parseOSC(m);
        }
    };

    // ---- Artnet 单视频帧解码 + 发送（emitVideoFrame/sendFrames 逐行复刻；
    // vid_frame = 相对视频帧号去重键）--------------------------------------
    auto emitArtnetVideoFrame = [&](const ArtnetSeq& seq, ArtnetRun& run,
                                    double seconds, std::int64_t vid_frame) {
        if (vid_frame == run.last_video_frame) return; // 同视频帧去重
        if (!run.dec) return;
        const QString abs_path = AppConstants::MEDIA_LIBRARY_STORAGE_DIR +
                                 QStringLiteral("/") + seq.file;
        if (!run.dec->ensureOpened(abs_path)) return;
        if (!run.dec->readFrameAt(seconds)) return;
        run.dec->extractDmx();

        ArtnetService* svc = ArtnetService::getInstance();
        const int universe_count = std::min<int>(
            static_cast<int>(run.dec->dmx.size()), run.dec->h);
        // 【2026-11-27 性能】同拍同帧时间戳一次取值——原每 universe 调一次
        // QDateTime::currentMSecsSinceEpoch（系统时间调用 ×universe 数）
        const qint64 now_ms = QDateTime::currentMSecsSinceEpoch();
        for (int idx = 0; idx < universe_count; ++idx) {
            ArtnetFrame f;
            f.host      = seq.host;
            f.sequence  = 0; // 占位——发送侧自增
            f.timestamp = now_ms;
            f.universe  = static_cast<quint16>(seq.start_universe + idx);
            f.setDmxData(run.dec->dmx[static_cast<size_t>(idx)]);
            svc->updateFrame(f, seq.owner.get());
        }
        run.last_video_frame = vid_frame;
    };

    // ---- Artnet 序列一拍推进（onPlayheadAdvanced 补帧结构逐位同源；
    // 出区间不复位——状态表保持末帧 50Hz 重发）------------------------------
    auto tickArtnetSeq = [&](const ArtnetSeq& seq, ArtnetRun& run,
                             std::int64_t now_us) {
        if (now_us < seq.start_us || now_us >= seq.end_us) return; // [st,en)
        if (seq.file.isEmpty() || !seq.owner) return;
        if (!run.dec) return; // 快照采纳时未建（文件为空等）
        const double fps = run.dec->video_fps;
        if (run.last_video_frame >= 0 && fps > 0) {
            const std::int64_t f_now =
                llround(double(now_us - seq.start_us) / 1e6 * fps);
            const std::int64_t gap = f_now - run.last_video_frame;
            if (gap > 1 && gap <= 8) { // kMaxCatchupFrames（Artnet 同族）
                for (std::int64_t f = run.last_video_frame + 1; f < f_now; ++f)
                    emitArtnetVideoFrame(seq, run, double(f) / fps, f);
            }
        }
        const double seconds = double(now_us - seq.start_us) / 1e6;
        emitArtnetVideoFrame(seq, run, seconds,
                             llround(seconds * std::max(fps, 0.0)));
    };

    // ---- Mapping 序列一拍推进（onPlayheadAdvanced 补帧结构逐位同源；
    // 出区间复位去重）------------------------------------------------------
    auto tickMappingSeq = [&](const MappingSeq& seq, MappingRun& run,
                              std::int64_t now_us) {
        if (now_us < seq.start_us || now_us > seq.end_us) { // [st,en] 双闭
            run.last_frame = -1; // 出区间复位（再进 = 重新触发）
            return;
        }
        const std::int64_t f_now = QtTimeline::usToFrames(now_us);
        if (run.last_frame >= 0 && run.last_frame < f_now) {
            // 补帧：上帧之后第一个不早于区间起点的帧；代表时刻 clamp 到 st
            const std::int64_t f_from = std::max(
                run.last_frame + 1, QtTimeline::usToFrames(seq.start_us));
            if (f_now - f_from < 120) { // kMaxCatchupFrames（Mapping）
                for (std::int64_t f = f_from; f < f_now; ++f)
                    emitMappingFrame(
                        seq, run,
                        std::max(QtTimeline::framesToUs(static_cast<int>(f)),
                                 seq.start_us));
            }
        }
        emitMappingFrame(seq, run, now_us); // 末帧（now 本尊）
    };

    // ---- 主循环 -----------------------------------------------------------
    std::unique_lock<std::mutex> lk(mutex_);
    while (!quit_) {
        // ---- 决定唤醒时刻：min(下一帧格边界, now+5ms)；空闲 20ms 轮询；
        // 命令待办立即醒。锁内读时钟：clock lambda 只触 runtime/preview 各自
        // 内部锁（与本锁无环；preview playheadUs 锁外调时钟已防自锁）------
        const std::int64_t ph = clock_();
        const auto now_tp = std::chrono::steady_clock::now();
        std::chrono::steady_clock::time_point wake_tp;
        if (seek_us_ >= 0 || snapshot_pending_) {
            wake_tp = now_tp; // 命令待办：不睡
        } else if (ph < 0) {
            wake_tp = now_tp + std::chrono::milliseconds(20);
            if (hr_timer) { timeEndPeriod(1); hr_timer = false; }
            // 空闲复位：Mapping 出区间语义（下次播放从当前帧重新触发）；
            // Artnet 不复位（离开不清零——状态表保持末帧重发）
            for (auto& r : mruns) r.last_frame = -1;
        } else {
            if (!hr_timer) { timeBeginPeriod(1); hr_timer = true; }
            // 下一帧格边界（PAL 25fps：帧 k 占 [framesToUs(k), framesToUs(k+1))
            // ——帧边界醒 = 帧时距内零误差；钳 5ms 保命令响应上限）
            std::int64_t dt_us =
                QtTimeline::framesToUs(
                    static_cast<int>(QtTimeline::usToFrames(ph) + 1)) - ph;
            if (dt_us < 1)    dt_us = 1;
            if (dt_us > 5000) dt_us = 5000;
            wake_tp = now_tp + std::chrono::microseconds(dt_us);
        }
        cv_.wait_until(lk, wake_tp, [this] {
            return quit_ || seek_us_ >= 0 || snapshot_pending_;
        });
        if (quit_) break;

        // ---- 采纳快照（换血）：旧 Artnet 桶撤销 → 新快照生效 → 运行态重建
        // （last 基线丢弃 = 下拍按当前帧重发，旧 applyPayload 复位同口径）--
        if (snapshot_pending_) {
            snapshot_pending_ = false;
            std::shared_ptr<const Snapshot> old = current_;
            current_ = pending_;
            pending_.reset();
            if (old) {
                for (const auto& s : old->artnets)
                    if (s.owner)
                        ArtnetService::getInstance()->releaseOwner(
                            s.owner.get());
            }
            const size_t nm = current_ ? current_->mappings.size() : 0;
            const size_t na = current_ ? current_->artnets.size() : 0;
            mruns.assign(nm, MappingRun{});
            aruns.assign(na, ArtnetRun{});
            if (current_) {
                for (size_t i = 0; i < na; ++i) {
                    const auto& seq = current_->artnets[i];
                    if (seq.file.isEmpty()) continue;
                    const QString abs_path =
                        AppConstants::MEDIA_LIBRARY_STORAGE_DIR +
                        QStringLiteral("/") + seq.file;
                    auto it = dec_cache.find(abs_path);
                    if (it == dec_cache.end()) {
                        it = dec_cache
                                 .emplace(abs_path,
                                          std::make_shared<DmxDecoder>())
                                 .first;
                    }
                    aruns[i].dec = it->second;
                }
                // 缓存清理：新快照未引用的路径出缓存（防文件删除后滞留）
                for (auto it = dec_cache.begin(); it != dec_cache.end();) {
                    bool used = false;
                    for (const auto& s : current_->artnets) {
                        if (!s.file.isEmpty() &&
                            AppConstants::MEDIA_LIBRARY_STORAGE_DIR +
                                    QStringLiteral("/") + s.file ==
                                it->first) {
                            used = true;
                            break;
                        }
                    }
                    it = used ? std::next(it) : dec_cache.erase(it);
                }
            }
            if (schedDbg()) {
                std::fprintf(stderr,
                             "sched-dbg: snapshot adopt mappings=%zu "
                             "artnets=%zu\n",
                             nm, na);
                std::fflush(stderr);
            }
        }

        // ---- 本拍时刻：seek poke 优先（暂停态求值一帧 / 播放态作提示），
        // 否则现读主时钟（一切以时钟为准——不积分不预测）--------------------
        std::int64_t now_us = -1;
        if (seek_us_ >= 0) {
            now_us = seek_us_;
            seek_us_ = -1;
            if (schedDbg()) {
                std::fprintf(stderr, "sched-dbg: seek poke us=%lld\n",
                             (long long)now_us);
                std::fflush(stderr);
            }
        } else {
            now_us = clock_();
        }
        if (now_us < 0 || !current_) continue; // 空闲拍 / 无快照

        // ---- 执行（锁外：求值/解码/发送都是慢活，不挡 GUI 快照交接）-------
        lk.unlock();
        const size_t nm = std::min(mruns.size(), current_->mappings.size());
        for (size_t i = 0; i < nm; ++i)
            tickMappingSeq(current_->mappings[i], mruns[i], now_us);
        const size_t na = std::min(aruns.size(), current_->artnets.size());
        for (size_t i = 0; i < na; ++i)
            tickArtnetSeq(current_->artnets[i], aruns[i], now_us);
        lk.lock();
    }
    // 高分辨率时钟归还兜底（正常路径在空闲分支已归还）
    if (hr_timer) timeEndPeriod(1);
}

} // namespace ms
