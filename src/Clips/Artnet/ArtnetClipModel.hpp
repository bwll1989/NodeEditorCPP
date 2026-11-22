// ============================================================================
//  ArtnetClipModel —— Artnet 灯光片段模型（src/Clips/Artnet/）
//
//  【2026-11-15 片段移植】自节点体系 artnetclipmodel.hpp/.cpp（643 行，
//  NodeClipDelegateModel 帧域）移植为 SceneClipModel（μs 域）子类。移植
//  对照表（节点体系 → 本插件）：
//    * currentData(currentFrame) 拉取驱动 → onPlayheadAdvanced(prev,now)
//      推送驱动（适配器播放头两落点统一过钩子，见 AbstractClipModel）；
//    * 时间轴帧号（getTimeCodeType 制式）→ μs 直算（rel_us/1e6 = 视频秒），
//      帧去重键 = 视频自身帧号（llround(秒×视频fps)）——比旧版按时间轴帧
//      去重更精确（视频 fps ≠ 时间轴 fps 时不再重复解码/漏帧）；
//    * save()/load() QJsonObject 载荷 → ms::Clip.clip_data（pb field 13，
//      随场景分发）经 setClipPayload 灌入模型（适配器装载/面板写回两路）；
//    * loadArtnetInfo 时长自适应 → payload 文件变更路径 probeAndFitDuration
//      （setEnd 自适应 → lengthChanged → 适配器 onClipGeometryChanged 写回
//      工作副本——链路闭环，面板无需感知模型）；
//    * "/file" "/stop" 标记：节点体系数据流的 OSC 习惯标记，本体系无消费者
//      （适配器不消费 currentData）——不再产生；
//    * GlobalEventBus /universe /host /file 外部控制订阅：本体系属性绑定
//      体系未覆盖片段业务字段——暂缓（见 Doc.md 遗留）。
//
//  回放语义（与节点体系同源）：播放头 ∈ [start, end) 时逐帧解码 DMX 视频
//  （宽 512 × 高 = 域数，FFV1；GRAY8 直读 / RGBA 取 R），每个 universe 的
//  512 字节写入 ArtnetService 状态表（owner = this，多源 HTP/LTP 合并、
//  releaseOwner 撤销），服务内置 50Hz 节拍合成广播。离开区间不清零（状态
//  表保持最后一帧持续重发——旧口径）。subnet/net 恒 0（Timeline 简化）。
//  解码在 GUI 线程（与节点体系同口径；FFV1 512×N 帧解码开销可忽略）。
// ============================================================================

#pragma once

#include <QByteArray>
#include <QColor>
#include <QString>
#include <QVector>

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libavutil/imgutils.h>
#include <libavutil/time.h>
}

#include "SceneClipModel.hpp"

class ArtnetService;

/// Artnet 灯光片段模型：配置镜像（clip_data）+ FFmpeg 逐帧 DMX 解码 +
/// ArtnetService 状态表写入。几何/绘制走基类（青色块 #66CCCC，可拖拽调长
/// ——时长亦可由 DMX 视频自适应回填）。
class ArtnetClipModel : public SceneClipModel {
    Q_OBJECT
public:
    explicit ArtnetClipModel(qint64 startUs, QObject* parent = nullptr);
    ~ArtnetClipModel() override;

    /// 【AbstractClipModel】工作副本业务数据镜像：{"file","targetHost",
    /// "startUniverse"} → 字段解析 + 差异应用（幂等：内容未变直接返回）。
    /// 文件变更 = 释放旧输出 + 重置解码器 + 时长自适应；主机/起始域变更 =
    /// 释放旧输出（新配置下一拍重推）。
    void setClipPayload(const QString& json) override;

    /// 【AbstractClipModel】播放头推进（适配器每拍调用）：区间内逐帧解码
    /// → 写状态表；同视频帧去重（50Hz 泵持续重发状态表，无需重复写）。
    /// 【2026-11-20】prev→now 跨帧补发（GUI 饥饿修复，上限 8 帧——每帧
    /// seek+decode 成本高；详见 cpp 注释）。
    /// 【2026-11-22 退役标注】适配器已停止调用本钩子——逐帧解码发送由
    /// PlaybackSendScheduler 独立线程承担（解码栈移植于本实现，按文件
    /// 路径缓存跨快照复用；见 src/Widget/TimeLineWidget/SendScheduler.cpp）。
    /// 本实现保留一版作回滚路径，验证稳定后清理。
    void onPlayheadAdvanced(qint64 prevUs, qint64 nowUs) override;

private:
    // ---- 业务字段（clip_data 镜像；权威在 ms::Clip.clip_data）--------------
    QString file_;         ///< DMX 视频媒体库相对路径（Documents/Flow/Medias 下）
    QString targetHost_ = QStringLiteral("192.168.1.255"); ///< 目标主机（广播）
    int     startUniverse_ = 0;                            ///< 回放起始域

    // ---- FFmpeg 解码状态（懒打开；路径/文件变更时重置）---------------------
    AVFormatContext* fmt_        = nullptr;
    AVCodecContext*  codec_      = nullptr;
    AVFrame*         frame_      = nullptr;
    AVPacket*        packet_     = nullptr;
    QString          openedPath_;        ///< 已打开绝对路径（变更检测）
    int              videoStream_ = -1;
    double           timeBase_    = 0.0;
    double           videoFps_    = 0.0;
    int              videoW_ = 0, videoH_ = 0;
    int              universeCount_ = 0; ///< = videoH_（一行 = 一个 universe）
    QVector<QByteArray> dmx_;            ///< 每 universe 512 字节缓存
    qint64           lastVideoFrame_ = -1; ///< 去重：上次已发送的视频帧号

    // ---- 内部流水（实现在 ArtnetClipModel.cpp）-----------------------------
    /// 补帧上限（GUI 饥饿补发窗口；readFrameAt 每帧 seek+decode，成本高
    /// ——比 Mapping 的 120 收紧；超过视为人为 seek 不补帧）。
    static constexpr qint64 kMaxCatchupFrames = 8;
    void   applyPayload(const QString& json);  ///< 解析 + 差异应用
    void   resetDecoder();                     ///< 关闭并释放解码器全链
    QString absolutePath() const;              ///< 媒体库相对路径 → 绝对路径
    bool   ensureVideoOpened(const QString& absPath); ///< 懒打开 + 校验
    bool   readFrameAt(double seconds);        ///< 定位 + 解码一帧
    void   emitVideoFrame(double seconds, qint64 vidFrame); ///< 解码→提取→发送→去重
    void   extractDmx();                       ///< 当前帧 → dmx_（512B/域）
    void   sendFrames();                       ///< dmx_ → 状态表（owner=this）
    void   releaseOutputs();                   ///< 撤销状态表属主桶 + 复位去重
    void   probeAndFitDuration();              ///< 时长自适应（旧 loadArtnetInfo）
};
