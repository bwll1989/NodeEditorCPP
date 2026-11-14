#pragma once

#include "DataTypesExport.h"
#include "AudioTimestampRingQueue.h"

#include <QMutex>
#include <QObject>
#include <QString>
#include <QStringList>
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

private:
    explicit TimelineAudioBusRegistry();

    mutable QMutex mutex_;
    std::map<QString, std::shared_ptr<AudioTimestampRingQueue>> buses_;
    /// bus 名 → 驱动模式（"external"/"test"/"silence"；节点 setDriveMode
    /// 时同步写——host 写侧据此跳过非 external 的队列，单写者不破）
    std::map<QString, QString> driveModes_;
};

} // namespace NodeDataTypes
