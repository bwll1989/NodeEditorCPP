#include "TimelineAudioBusRegistry.h"

namespace NodeDataTypes
{

TimelineAudioBusRegistry* TimelineAudioBusRegistry::instance()
{
    static TimelineAudioBusRegistry s_instance;
    return &s_instance;
}

TimelineAudioBusRegistry::TimelineAudioBusRegistry() = default;

void TimelineAudioBusRegistry::registerBus(const QString& busName,
                                           std::shared_ptr<AudioTimestampRingQueue> queue)
{
    if (busName.isEmpty() || !queue) {
        return;
    }
    QMutexLocker locker(&mutex_);
    buses_[busName] = std::move(queue);
}

void TimelineAudioBusRegistry::unregisterBus(const QString& busName)
{
    QMutexLocker locker(&mutex_);
    buses_.erase(busName);
    driveModes_.erase(busName); // 驱动权登记随队列一并清（端口重建后默认 external）
}

std::shared_ptr<AudioTimestampRingQueue> TimelineAudioBusRegistry::acquire(const QString& busName) const
{
    QMutexLocker locker(&mutex_);
    const auto it = buses_.find(busName);
    return (it != buses_.end()) ? it->second : nullptr;
}

QStringList TimelineAudioBusRegistry::buses() const
{
    QMutexLocker locker(&mutex_);
    QStringList names;
    names.reserve(static_cast<int>(buses_.size()));
    for (const auto& kv : buses_) {
        names << kv.first;
    }
    return names;
}

void TimelineAudioBusRegistry::setBusDriveMode(const QString& busName,
                                               const QString& mode)
{
    if (busName.isEmpty()) {
        return;
    }
    QMutexLocker locker(&mutex_);
    if (mode == QStringLiteral("external")) {
        driveModes_.erase(busName); // 默认态不入表（未登记 = external）
    } else {
        driveModes_[busName] = mode;
    }
}

QString TimelineAudioBusRegistry::busDriveMode(const QString& busName) const
{
    QMutexLocker locker(&mutex_);
    const auto it = driveModes_.find(busName);
    return (it != driveModes_.end()) ? it->second
                                     : QStringLiteral("external");
}

// ---------------------------------------------------------------------------
// 播放态发布面（Host 写 / 节点 worker 读；免锁原子——单标量无复合不变量）
// ---------------------------------------------------------------------------
void TimelineAudioBusRegistry::setPlaybackActive(bool active)
{
    // release：保证 Host 先写的 position/playing 对读侧 acquire 可见
    playback_active_.store(active, std::memory_order_release);
}

void TimelineAudioBusRegistry::setPlaybackPlaying(bool playing)
{
    playback_playing_.store(playing, std::memory_order_release);
}

void TimelineAudioBusRegistry::setPlaybackPosition(double positionSec,
                                                   qint64 anchorFrame)
{
    // 先锚点后位置：读侧即便撕裂到中间态，误差也 ≤1 拍（外推口径自愈）
    playback_anchor_frame_.store(anchorFrame, std::memory_order_relaxed);
    playback_position_sec_.store(positionSec, std::memory_order_release);
}

bool TimelineAudioBusRegistry::playbackActive() const
{
    return playback_active_.load(std::memory_order_acquire);
}

bool TimelineAudioBusRegistry::playbackPlaying() const
{
    return playback_playing_.load(std::memory_order_acquire);
}

double TimelineAudioBusRegistry::playbackPositionSec() const
{
    return playback_position_sec_.load(std::memory_order_acquire);
}

qint64 TimelineAudioBusRegistry::playbackAnchorFrame() const
{
    return playback_anchor_frame_.load(std::memory_order_relaxed);
}

} // namespace NodeDataTypes
