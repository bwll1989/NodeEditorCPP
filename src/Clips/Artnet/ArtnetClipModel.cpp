// ============================================================================
//  ArtnetClipModel.cpp —— Artnet 灯光片段模型实现（μs 域移植自节点体系）
// ============================================================================

#include "ArtnetClipModel.hpp"

#include <QDebug>
#include <QDateTime>
#include <QJsonDocument>
#include <QJsonObject>

#include "Common/AppConfig/ConstantDefines.h" // AppConstants::MEDIA_LIBRARY_STORAGE_DIR
#include "Common/Devices/ArtnetSender/ArtnetService.h"
#include "Common/Devices/ArtnetSender/ArtnetFrame.h"

// ---------------------------------------------------------------------------
// 构造/析构
// ---------------------------------------------------------------------------
ArtnetClipModel::ArtnetClipModel(qint64 startUs, QObject* parent)
    : SceneClipModel(startUs, QStringLiteral("Artnet"), {}, parent) {
    RESIZEABLE  = true;   // 可拖拽调长（时长亦由 DMX 视频自适应回填）
    EMBEDWIDGET = false;
    SHOWBORDER  = false;  // 【2026-11-27 用户定案】所有片段无边框
    setColor(QColor(0x66, 0xCC, 0xCC)); // 青（节点体系 Artnet 类型色 #66cccc）
}

ArtnetClipModel::~ArtnetClipModel() {
    // 从状态表撤销本片段名下全部输出桶（防悬空属主标签残留——旧版同口径）
    ArtnetService::getInstance()->releaseOwner(this);
    resetDecoder();
}

// ---------------------------------------------------------------------------
// 媒体库相对路径 → 绝对路径（窄串入 FFmpeg 前的 UTF-8 转换在调用点完成——
// 旧口径 path.toUtf8().constData()；媒体库目录全 ASCII 工程路径无碍，
// 文件名含中文由 toUtf8 保证字节正确——FFmpeg 按 UTF-8 处理）
// ---------------------------------------------------------------------------
QString ArtnetClipModel::absolutePath() const {
    if (file_.isEmpty()) return {};
    return AppConstants::MEDIA_LIBRARY_STORAGE_DIR + "/" + file_;
}

// ---------------------------------------------------------------------------
// 业务数据镜像：解析 + 差异应用（幂等）
// ---------------------------------------------------------------------------
void ArtnetClipModel::setClipPayload(const QString& json) {
    if (json.isEmpty() && file_.isEmpty() &&
        targetHost_ == QStringLiteral("192.168.1.255") && startUniverse_ == 0)
        return; // 空 payload 且从未配置过：零开销（Add Item 占位高频路径）
    applyPayload(json);
}

void ArtnetClipModel::applyPayload(const QString& json) {
    // ---- 解析（坏 JSON = 保守全默认，不抛异常——播放路径不容异常逃逸）----
    QString file;
    QString host = QStringLiteral("192.168.1.255");
    int     universe = 0;
    if (!json.isEmpty()) {
        const QJsonDocument doc = QJsonDocument::fromJson(json.toUtf8());
        if (doc.isObject()) {
            const QJsonObject o = doc.object();
            file     = o[QStringLiteral("file")].toString();
            host     = o[QStringLiteral("targetHost")].toString(
                           QStringLiteral("192.168.1.255"));
            universe = o[QStringLiteral("startUniverse")].toInt(0);
        }
    }
    if (host.trimmed().isEmpty()) host = QStringLiteral("192.168.1.255");

    // ---- 幂等守卫：三字段全同 = 无变化（on_edited 高频回推防抖）----------
    const bool fileChanged     = (file != file_);
    const bool hostChanged     = (host != targetHost_);
    const bool universeChanged = (universe != startUniverse_);
    if (!fileChanged && !hostChanged && !universeChanged) return;

    file_         = file;
    targetHost_   = host;
    startUniverse_= universe;

    // ---- 差异应用（旧版 onUniverseChanged/onHostChanged/onFileChanged 口径：
    //      配置变更 = 撤销旧桶 + 复位去重；文件变更再加重置解码器 + 自适应）--
    releaseOutputs();
    if (fileChanged) {
        resetDecoder();
        if (!file_.isEmpty()) probeAndFitDuration(); // 时长自适应（含重开探测）
    }
}

// ---------------------------------------------------------------------------
// 时长自适应（旧 loadArtnetInfo μs 化）：探测 DMX 视频 → setEnd(start+时长)
// 。lengthChanged → 适配器 onClipGeometryChanged 写回工作副本（装载期由
// loading_ 抑制，持久化时长随后覆盖——装载以工作副本为准）。
// 顺带校验文件（打不开 = 静默保持现几何——编辑区信息标签另有独立探测提示）
// ---------------------------------------------------------------------------
void ArtnetClipModel::probeAndFitDuration() {
    AVFormatContext* probe = nullptr;
    const QByteArray pathUtf8 = absolutePath().toUtf8();
    if (avformat_open_input(&probe, pathUtf8.constData(), nullptr, nullptr) < 0)
        return;
    if (avformat_find_stream_info(probe, nullptr) < 0) {
        avformat_close_input(&probe);
        return;
    }
    double seconds = 0.0;
    for (unsigned i = 0; i < probe->nb_streams; ++i) {
        AVStream* st = probe->streams[i];
        if (st->codecpar->codec_type != AVMEDIA_TYPE_VIDEO) continue;
        seconds = st->duration > 0
                      ? st->duration * av_q2d(st->time_base)
                      : probe->duration / (double)AV_TIME_BASE;
        break;
    }
    avformat_close_input(&probe);
    if (seconds <= 0) return;
    const qint64 durUs = static_cast<qint64>(llround(seconds * 1e6));
    if (durUs > 0 && durUs != length())
        setEnd(start() + durUs); // → lengthChanged → 工作副本回写链
}

// ---------------------------------------------------------------------------
// 解码器生命周期
// ---------------------------------------------------------------------------
void ArtnetClipModel::resetDecoder() {
    if (frame_)  { av_frame_free(&frame_);   frame_  = nullptr; }
    if (packet_) { av_packet_free(&packet_); packet_ = nullptr; }
    if (codec_)  { avcodec_free_context(&codec_); codec_ = nullptr; }
    if (fmt_)    { avformat_close_input(&fmt_); fmt_ = nullptr; }
    openedPath_.clear();
    videoStream_ = -1;
    timeBase_ = videoFps_ = 0.0;
    videoW_ = videoH_ = 0;
    universeCount_ = 0;
    dmx_.clear();
    lastVideoFrame_ = -1;
}

/**
 * 懒打开 + 校验（旧 ensureVideoOpened 移植，FFmpeg 8.1 API 同形）：
 * 宽必须 512（DMX 通道数），高 ≥1（= universe 数），解码器必须是 FFV1
 * （DMX 视频的约定编码——非 FFV1 拒绝，与旧版同口径）。
 */
bool ArtnetClipModel::ensureVideoOpened(const QString& absPath) {
    if (openedPath_ == absPath && fmt_) return true;

    resetDecoder();

    const QByteArray pathUtf8 = absPath.toUtf8();
    if (avformat_open_input(&fmt_, pathUtf8.constData(), nullptr, nullptr) < 0) {
        fmt_ = nullptr;
        return false;
    }
    if (avformat_find_stream_info(fmt_, nullptr) < 0) {
        avformat_close_input(&fmt_);
        fmt_ = nullptr;
        return false;
    }
    for (unsigned i = 0; i < fmt_->nb_streams; ++i) {
        if (fmt_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_VIDEO) {
            videoStream_ = static_cast<int>(i);
            break;
        }
    }
    if (videoStream_ < 0) { resetDecoder(); return false; }

    const AVCodecParameters* par = fmt_->streams[videoStream_]->codecpar;
    videoW_ = par->width;
    videoH_ = par->height;
    if (videoW_ != 512 || videoH_ < 1) { resetDecoder(); return false; }

    const AVCodec* codec = avcodec_find_decoder(par->codec_id);
    if (!codec || codec->id != AV_CODEC_ID_FFV1) { resetDecoder(); return false; }

    codec_ = avcodec_alloc_context3(codec);
    if (!codec_) { resetDecoder(); return false; }
    if (avcodec_parameters_to_context(codec_, par) < 0 ||
        avcodec_open2(codec_, codec, nullptr) < 0) {
        resetDecoder();
        return false;
    }
    frame_  = av_frame_alloc();
    packet_ = av_packet_alloc();
    if (!frame_ || !packet_) { resetDecoder(); return false; }

    const AVStream* vs = fmt_->streams[videoStream_];
    timeBase_ = av_q2d(vs->time_base);
    videoFps_ = av_q2d(vs->r_frame_rate);
    if (videoFps_ <= 0) videoFps_ = 25.0; // 异常流兜底（与时间轴基准一致）

    // Universe 缓冲初始化（一行 = 一个 universe，各 512 字节清零）
    universeCount_ = videoH_;
    dmx_.clear();
    dmx_.reserve(universeCount_);
    for (int i = 0; i < universeCount_; ++i) {
        QByteArray u;
        u.resize(512);
        u.fill(char(0x00));
        dmx_.append(u);
    }
    lastVideoFrame_ = -1;
    openedPath_ = absPath;
    return true;
}

/**
 * 定位 + 解码一帧（旧 readFrameAt 移植，帧号 → 秒换算改 μs 直算）：
 * 秒 → 流时间戳（/time_base）→ av_seek_frame(BACKWARD) + flush → 读解。
 */
bool ArtnetClipModel::readFrameAt(double seconds) {
    if (!fmt_ || !codec_ || timeBase_ <= 0) return false;
    const int64_t targetTs = static_cast<int64_t>(seconds / timeBase_);
    if (av_seek_frame(fmt_, videoStream_, targetTs, AVSEEK_FLAG_BACKWARD) < 0)
        return false;
    avcodec_flush_buffers(codec_);

    while (av_read_frame(fmt_, packet_) >= 0) {
        if (packet_->stream_index == videoStream_) {
            if (avcodec_send_packet(codec_, packet_) < 0) {
                av_packet_unref(packet_);
                continue;
            }
            const int ret = avcodec_receive_frame(codec_, frame_);
            av_packet_unref(packet_);
            if (ret == 0) return true;
            if (ret == AVERROR(EAGAIN)) continue;
            return false;
        }
        av_packet_unref(packet_);
    }
    return false;
}

/**
 * 当前解码帧 → dmx_（旧 extractDMXFromCurrentFrame 逐行移植）：
 * 灰度（GRAY8）直读每列像素；非灰度（兼容旧 RGBA 布局）取 R 通道。
 */
void ArtnetClipModel::extractDmx() {
    if (!frame_ || !frame_->data[0]) return;
    const int usableWidth = qMin(videoW_, 512);
    for (int universe = 0; universe < universeCount_; ++universe) {
        QByteArray out;
        out.resize(512);
        out.fill(char(0x00));
        const uint8_t* row = frame_->data[0] +
                             universe * frame_->linesize[0];
        if (frame_->format == AV_PIX_FMT_GRAY8) {
            for (int col = 0; col < usableWidth; ++col)
                out[col] = char(row[col]);
        } else {
            for (int col = 0; col < usableWidth; ++col)
                out[col] = char(row[col * 4]); // RGBA 步长 4，取 R 通道
        }
        dmx_[universe] = out;
    }
}

/**
 * dmx_ → 状态表（旧 sendArtnetFrames 移植；owner = this 供多源合并与
 * releaseOwner 撤销。sequence=0 占位——发送侧自增，时间轴不关心）。
 */
void ArtnetClipModel::sendFrames() {
    ArtnetService* svc = ArtnetService::getInstance();
    for (int idx = 0; idx < universeCount_ && idx < dmx_.size(); ++idx) {
        ArtnetFrame f;
        f.host      = targetHost_;
        f.sequence  = 0;
        f.timestamp = QDateTime::currentMSecsSinceEpoch();
        f.universe  = static_cast<quint16>(startUniverse_ + idx); // subnet/net 恒 0
        f.setDmxData(dmx_[idx]);
        svc->updateFrame(f, this);
    }
}

/// 撤销本片段状态表输出桶 + 复位去重（配置变更路径；zeroFirst=false——
/// 旧口径：配置变更立即按新配置重推，无需补发全零）
void ArtnetClipModel::releaseOutputs() {
    ArtnetService::getInstance()->releaseOwner(this);
    lastVideoFrame_ = -1;
}

// ---------------------------------------------------------------------------
// 播放头推进（适配器每拍调用；μs 直算——rel_us/1e6 = 视频秒，去重键 =
// 视频自身帧号）。区间外不动作（离开不清零：状态表保持最后一帧持续 50Hz
// 重发——旧口径"时间轴暂停后保持该帧状态"）。
//
// 【2026-11-20 补帧修复（GUI 饥饿，同 MappingClipModel）】播放权威时钟是
// wall-clock，33ms GUI tick 只是采样——界面操作推迟 tick 派发时一次调用
// 跨多视频帧，旧 Q_UNUSED 只发当前帧 → 中间帧 DMX 状态丢失（灯效跳变）。
// 补发 prev→now 跨过的帧；readFrameAt 每帧一次 seek+flush+decode，成本
// 高于表达式求值，上限收紧到 kMaxCatchupFrames=8（≈267ms@30fps 播放期
// 饥饿足够覆盖；更大跳变视为人为 seek，跳到当前帧即可）。
// ---------------------------------------------------------------------------
void ArtnetClipModel::onPlayheadAdvanced(qint64 prevUs, qint64 nowUs) {
    const qint64 st = start();
    const qint64 en = end();
    if (nowUs < st || nowUs >= en) return;   // 前闭后开（库时间约定）
    if (file_.isEmpty()) return;

    if (prevUs >= 0 && prevUs < nowUs && videoFps_ > 0) {
        const double fps    = videoFps_;
        const qint64 fPrev  = llround(double(prevUs - st) / 1e6 * fps);
        const qint64 fNow   = llround(double(nowUs  - st) / 1e6 * fps);
        if (fNow - fPrev > 1 && fNow - fPrev <= kMaxCatchupFrames)
            for (qint64 f = fPrev + 1; f < fNow; ++f)
                emitVideoFrame(double(f) / fps, f);
    }
    const double seconds = double(nowUs - st) / 1e6;
    emitVideoFrame(seconds, llround(seconds * videoFps_));
}

/// 单视频帧解码 → 提取 → 发送 → 记去重（onPlayheadAdvanced 末帧与补帧
/// 共用流水；vidFrame 为去重键 = 视频自身帧号）。
void ArtnetClipModel::emitVideoFrame(double seconds, qint64 vidFrame) {
    if (vidFrame == lastVideoFrame_) return; // 同视频帧去重（泵持续重发）
    if (!ensureVideoOpened(absolutePath())) return;
    if (!readFrameAt(seconds)) return;
    extractDmx();
    sendFrames();
    lastVideoFrame_ = vidFrame;
}
