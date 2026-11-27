// ============================================================================
//  ArtnetClipModel.cpp —— Artnet 灯光片段模型实现（配置镜像 + 时长自适应）
//
//  【2026-11-27 死代码清理】FFmpeg 逐帧解码发送链（onPlayheadAdvanced/
//  emitVideoFrame/ensureVideoOpened/readFrameAt/extractDmx/sendFrames/
//  resetDecoder/releaseOutputs）退役——PlaybackSendScheduler 独立线程
//  承担（DmxDecoder 逐行移植自本实现旧版，语义同源；快照链直读工作副本
//  clip_data，模型镜像不再是播放数据源）。本文件只留编辑期逻辑。
// ============================================================================

#include "ArtnetClipModel.hpp"

#include <QJsonDocument>
#include <QJsonObject>

#include "Common/AppConfig/ConstantDefines.h" // AppConstants::MEDIA_LIBRARY_STORAGE_DIR

// ---------------------------------------------------------------------------
// 构造（析构默认——状态表桶 owner 令牌由 PlaybackSendScheduler 快照自管，
// 模型不再持有任何输出）
// ---------------------------------------------------------------------------
ArtnetClipModel::ArtnetClipModel(qint64 startUs, QObject* parent)
    : SceneClipModel(startUs, QStringLiteral("Artnet"), {}, parent) {
    RESIZEABLE  = true;   // 可拖拽调长（时长亦由 DMX 视频自适应回填）
    EMBEDWIDGET = false;
    SHOWBORDER  = false;  // 【2026-11-27 用户定案】所有片段无边框
    setColor(QColor(0x66, 0xCC, 0xCC)); // 青（节点体系 Artnet 类型色 #66cccc）
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
    // ---- 解析（坏 JSON = 保守全默认，不抛异常——编辑路径不容异常逃逸）----
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
    const bool fileChanged = (file != file_);
    if (!fileChanged && host == targetHost_ && universe == startUniverse_)
        return;

    file_          = file;
    targetHost_    = host;
    startUniverse_ = universe;

    // ---- 文件变更 → 时长自适应（播放配置由调度器快照直读工作副本，此处
    //      只服务编辑体验：拖入/换绑后片段长度立即贴合素材）----------------
    if (fileChanged)
        if (!file_.isEmpty()) probeAndFitDuration();
}

// ---------------------------------------------------------------------------
// 媒体库相对路径 → 绝对路径（窄串入 FFmpeg 前的 UTF-8 转换在调用点完成——
// 旧口径 path.toUtf8().constData()；文件名含中文由 toUtf8 保证字节正确
// ——FFmpeg 按 UTF-8 处理）
// ---------------------------------------------------------------------------
QString ArtnetClipModel::absolutePath() const {
    if (file_.isEmpty()) return {};
    return AppConstants::MEDIA_LIBRARY_STORAGE_DIR + "/" + file_;
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
