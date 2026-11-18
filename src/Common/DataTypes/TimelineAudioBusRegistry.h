#pragma once

#include "DataTypesExport.h"
#include "AudioTimestampRingQueue.h"

#include <QMutex>
#include <QObject>
#include <QString>
#include <QStringList>
#include <atomic>
#include <map>
#include <memory>

namespace NodeDataTypes
{

/**
 * @file TimelineAudioBusRegistry.h
 * @brief timeline 音频 bus 环形队列注册表（TimeLineAudioBus 节点 ↔ runtime 的会合点）
 *
 * 归属约定（2026-11-13 TimeLineAudioBus 实施方案定稿）：
 *   - 队列唯一出生点 = TimeLineAudioBus 节点（每个 Audio 输出端口一个队列）；
 *   - 节点在创建/增删端口时以端口名为 bus 名 register/unregister；
 *   - runtime（timeline 混音写入侧，后续接入）按名 acquire 后以单写者身份
 *     每 tick pushFrame（时间戳 = 全局帧计数，块长 = getSamplesPerFrame(48000)）；
 *   - 驱动权协调：节点 driveMode=external（默认）时节点不写队列，runtime 是
 *     唯一写者；driveMode=test/silence 时节点是写者（联调用），runtime 侧
 *     接入前应跳过处于非 external 模式的 bus。
 *
 * 单写者铁律：每个队列任一时刻只允许一个写者（节点或 runtime 二选一）。
 */
class DATATYPES_EXPORT TimelineAudioBusRegistry : public QObject
{
    Q_OBJECT

public:
    static TimelineAudioBusRegistry* instance();

    /// 节点侧：注册/更换某 bus 的队列（同名覆盖旧队列指针）
    void registerBus(const QString& busName, std::shared_ptr<AudioTimestampRingQueue> queue);

    /// 节点侧：注销（端口删除/节点析构）；不存在时静默
    void unregisterBus(const QString& busName);

    /// runtime 侧：按名取队列；无此 bus 返回 nullptr
    std::shared_ptr<AudioTimestampRingQueue> acquire(const QString& busName) const;

    /// 当前全部 bus 名（诊断/属性面板展示）
    QStringList buses() const;

    /// -----------------------------------------------------------------------
    /// 驱动权协调（单写者铁律的裁决面，2026-11-13 T1）：
    ///   * external（默认）= runtime（TimelineAudioBusHost）是唯一写者；
    ///   * 非 external（test/silence）= 节点 worker 自写，host 侧跳过。
    /// 未登记的 bus 默认 external（host 接管写入的预期默认态）。
    /// -----------------------------------------------------------------------
    void setBusDriveMode(const QString& busName, const QString& mode);
    /// bus 驱动模式（未登记 = "external"）
    QString busDriveMode(const QString& busName) const;

    /// -----------------------------------------------------------------------
    /// 播放态发布面（2026-11-17 T3 落地，原诊断根因修复）
    ///   * 背景：runtime host（TimelineAudioBusHost）此前只写音频队列，
    ///     节点 publishSync 的 playing 恒 false → 下游 LTCGenerator 等
    ///     SyncData 消费端判"源暂停"进入定格静音（hold）。
    ///   * 写侧 = Host：命令面（commandPlay/Pause/Seek/Stop，GUI 线程）
    ///     即时写播放态/定位点；worker 每播放拍连续写播放头（会话秒）
    ///     + 写入时刻全局帧计数锚点（与 SyncSharedState 外推口径一致）。
    ///   * 读侧 = TimeLineAudioBus 节点 worker：publishSync 每拍轮询。
    ///   * playbackActive：Host start/stop 置位——节点据此裁决"听 Host
    ///     还是回退本地属性兜底"（节点单独联调/无编辑器宿主场景）。
    ///   * 原子量免锁：单标量无复合不变量；读侧 50fps 轮询开销可忽略。
    /// -----------------------------------------------------------------------
    void setPlaybackActive(bool active);
    void setPlaybackPlaying(bool playing);
    /// 写播放头：positionSec = 会话时间（秒）；anchorFrame = 写入时刻全局帧计数
    void setPlaybackPosition(double positionSec, qint64 anchorFrame);

    /// Host 存活标志（决定节点消费路径：Host 发布 vs 本地属性兜底）
    bool playbackActive() const;
    bool playbackPlaying() const;
    double playbackPositionSec() const;
    qint64 playbackAnchorFrame() const;

private:
    explicit TimelineAudioBusRegistry();

    mutable QMutex mutex_;
    std::map<QString, std::shared_ptr<AudioTimestampRingQueue>> buses_;
    /// bus 名 → 驱动模式（"external"/"test"/"silence"；节点 setDriveMode
    /// 时同步写——host 写侧据此跳过非 external 的队列，单写者不破）
    std::map<QString, QString> driveModes_;

    /// 播放态发布面（Host 写 / 节点读；语义见上方方法注释）
    std::atomic<bool>    playback_active_{false};
    std::atomic<bool>    playback_playing_{false};
    std::atomic<double>  playback_position_sec_{0.0};
    std::atomic<qint64>  playback_anchor_frame_{0};
};

} // namespace NodeDataTypes
