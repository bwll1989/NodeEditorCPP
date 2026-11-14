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

} // namespace NodeDataTypes
