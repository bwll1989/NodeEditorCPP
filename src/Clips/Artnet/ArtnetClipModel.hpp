// ============================================================================
//  ArtnetClipModel —— Artnet 灯光片段模型（src/Clips/Artnet/）
//
//  【2026-11-15 片段移植】自节点体系 artnetclipmodel.hpp/.cpp（643 行，
//  NodeClipDelegateModel 帧域）移植为 SceneClipModel（μs 域）子类。
//  【2026-11-27 死代码清理】播放期逐帧解码发送链（onPlayheadAdvanced →
//  emitVideoFrame → ensureVideoOpened/readFrameAt/extractDmx/sendFrames →
//  ArtnetService 状态表）整体退役——【2026-11-22 起】PlaybackSendScheduler
//  独立线程承担（解码栈逐行移植，快照从工作副本 clip_data 直读，owner
//  令牌由调度器自管；见 src/Widget/TimeLineWidget/SendScheduler.cpp）。
//  本模型收敛为【配置镜像 + 编辑期服务】：
//    * setClipPayload —— {"file","targetHost","startUniverse"} 字段镜像
//      （ms::Clip.clip_data 权威；解析容错 + 幂等）；
//    * probeAndFitDuration —— 文件变更时长自适应（setEnd → lengthChanged
//      → 适配器 onClipGeometryChanged 写回工作副本，链路闭环）。
//  几何/绘制走基类（青色块 #66CCCC，可拖拽调长）。subnet/net 恒 0。
// ============================================================================

#pragma once

#include <QColor>
#include <QString>

extern "C" {
#include <libavformat/avformat.h>  // probeAndFitDuration 时长探测
#include <libavutil/avutil.h>
}

#include "SceneClipModel.hpp"

/// Artnet 灯光片段模型：clip_data 配置镜像 + 文件时长自适应。播放期
/// 逐帧解码发送不在本类（PlaybackSendScheduler 独立线程，快照直读
/// 工作副本）——本类只服务编辑期（payload 灌入 + 时长回填）。
class ArtnetClipModel : public SceneClipModel {
    Q_OBJECT
public:
    explicit ArtnetClipModel(qint64 startUs, QObject* parent = nullptr);

    /// 【AbstractClipModel】工作副本业务数据镜像：{"file","targetHost",
    /// "startUniverse"} → 字段解析 + 差异应用（幂等：内容未变直接返回）。
    /// 文件变更 = 时长自适应（probeAndFitDuration）。播放期消费方为
    /// PlaybackSendScheduler（快照直读工作副本，不经模型）。
    void setClipPayload(const QString& json) override;

private:
    // ---- 业务字段（clip_data 镜像；权威在 ms::Clip.clip_data）--------------
    QString file_;         ///< DMX 视频媒体库相对路径（Documents/Flow/Medias 下）
    QString targetHost_ = QStringLiteral("192.168.1.255"); ///< 目标主机（广播）
    int     startUniverse_ = 0;                            ///< 回放起始域

    // ---- 内部流水 ----------------------------------------------------------
    void   applyPayload(const QString& json);  ///< 解析 + 差异应用
    QString absolutePath() const;              ///< 媒体库相对路径 → 绝对路径
    void   probeAndFitDuration();              ///< 时长自适应（旧 loadArtnetInfo）
};
