//
// Created by WuBin on 2025/8/29.
//

#include "TimestampRingQueueLockFree.hpp"
#include "AudioTimestampRingQueue.h"

#include <algorithm>

using QtNodes::NodeData;
using QtNodes::NodeDataType;

AudioTimestampRingQueue::AudioTimestampRingQueue(int maxSize, QObject* parent)
    : QObject(parent), maxSize_(maxSize)
{
    slots_.resize(static_cast<size_t>(maxSize_ > 0 ? maxSize_ : 0));
}

AudioTimestampRingQueue::~AudioTimestampRingQueue()
{
    {
        QMutexLocker locker(&waitersMutex_);
        frameWaiters_.clear();
    }
    clear();
}

bool AudioTimestampRingQueue::pushFrame(const AudioFrame& frame)
{
    if (!isActive_.load(std::memory_order_acquire)) {
        return false;
    }

    const int wi = writeIndex_.load(std::memory_order_relaxed);
    if (maxSize_ <= 0 || wi < 0 || wi >= maxSize_) {
        return false;
    }

    const qint64 oldTs = slots_[static_cast<size_t>(wi)].frame.timestamp;
    const bool wasValid = oldTs > 0;
    const bool nowValid = frame.timestamp > 0;

    TimestampRingQueueDetail::writeRingEntry(slots_[static_cast<size_t>(wi)], frame);

    if (!wasValid && nowValid) {
        int expected = validFrames_.load(std::memory_order_relaxed);
        while (expected < maxSize_ &&
               !validFrames_.compare_exchange_weak(expected, expected + 1, std::memory_order_relaxed)) {
        }
    } else if (wasValid && !nowValid) {
        int expected = validFrames_.load(std::memory_order_relaxed);
        while (expected > 0 &&
               !validFrames_.compare_exchange_weak(expected, expected - 1, std::memory_order_relaxed)) {
        }
    }

    if (nowValid) {
        latestTimestamp_.store(frame.timestamp, std::memory_order_release);
        latestIndex_.store(wi, std::memory_order_release);
    }

    writeIndex_.store((wi + 1) % maxSize_, std::memory_order_release);

    notifyFrameWaiters();

    emit frameWritten(wi);
    emit newFrameWritten(frame);
    return true;
}

double AudioTimestampRingQueue::getUsedRatio() const
{
    if (maxSize_ <= 0) {
        return 0.0;
    }
    return static_cast<double>(validFrames_.load(std::memory_order_relaxed)) /
           static_cast<double>(maxSize_);
}

bool AudioTimestampRingQueue::getFrameByTimestamp(qint64 targetFrameCount, AudioFrame& frame)
{
    if (!isActive_.load(std::memory_order_acquire) || maxSize_ <= 0) {
        return false;
    }

    return TimestampRingQueueDetail::lookupFrameByTimestamp(slots_,
                                                          maxSize_,
                                                          latestTimestamp_.load(std::memory_order_acquire),
                                                          latestIndex_.load(std::memory_order_acquire),
                                                          lastHitTimestamp_,
                                                          lastHitIndex_,
                                                          targetFrameCount,
                                                          frame);
}

void AudioTimestampRingQueue::clear()
{
    for (auto& ringEntry : slots_) {
        TimestampRingQueueDetail::writeRingEntry(ringEntry, AudioFrame());
    }

    writeIndex_.store(0, std::memory_order_relaxed);
    validFrames_.store(0, std::memory_order_relaxed);
    lastHitTimestamp_ = 0;
    lastHitIndex_ = -1;
    latestTimestamp_.store(0, std::memory_order_relaxed);
    latestIndex_.store(-1, std::memory_order_relaxed);
}

void AudioTimestampRingQueue::setActive(bool active)
{
    isActive_.store(active, std::memory_order_release);
}

bool AudioTimestampRingQueue::isActive() const
{
    return isActive_.load(std::memory_order_acquire);
}

qint64 AudioTimestampRingQueue::latestTimestamp() const
{
    return latestTimestamp_.load(std::memory_order_acquire);
}

bool AudioTimestampRingQueue::peekNextTimestampAfter(qint64 afterTs, qint64 &outTs) const
{
    if (!isActive_.load(std::memory_order_acquire) || maxSize_ <= 0) {
        return false;
    }

    const qint64 latest = latestTimestamp_.load(std::memory_order_acquire);
    const int latestIdx = latestIndex_.load(std::memory_order_acquire);
    if (latest <= afterTs || latestIdx < 0 || latestIdx >= maxSize_) {
        return false;
    }

    // 连续常见路径：afterTs+1 相对 latest 的槽位直接验戳（不拷贝 PCM）
    const qint64 target = afterTs + 1;
    if (target > 0 && target <= latest) {
        const qint64 delta = latest - target;
        if (delta >= 0 && delta < maxSize_) {
            int idx = latestIdx - static_cast<int>(delta);
            idx %= maxSize_;
            if (idx < 0) {
                idx += maxSize_;
            }
            qint64 ts = 0;
            if (TimestampRingQueueDetail::readRingEntryTimestamp(slots_[static_cast<size_t>(idx)], ts)
                && ts == target) {
                outTs = target;
                return true;
            }
        }
    }

    // 从最新槽沿写方向回溯：时间戳递减，找到仍 > afterTs 的最旧一帧
    qint64 bestTs = -1;
    int idx = latestIdx;
    for (int step = 0; step < maxSize_; ++step) {
        qint64 ts = 0;
        if (TimestampRingQueueDetail::readRingEntryTimestamp(slots_[static_cast<size_t>(idx)], ts)
            && ts > 0) {
            if (ts <= afterTs) {
                break; // 更旧的只会更小，可以停
            }
            bestTs = ts;
        }
        idx -= 1;
        if (idx < 0) {
            idx += maxSize_;
        }
    }

    if (bestTs < 0) {
        return false;
    }
    outTs = bestTs;
    return true;
}

bool AudioTimestampRingQueue::getNextFrameAfter(qint64 afterTs, AudioFrame &frame)
{
    qint64 nextTs = 0;
    if (!peekNextTimestampAfter(afterTs, nextTs)) {
        return false;
    }
    if (!getFrameByTimestamp(nextTs, frame) || frame.timestamp != nextTs) {
        return false;
    }
    return true;
}

void AudioTimestampRingQueue::registerFrameWaiter(AudioTickWaiter *waiter)
{
    if (!waiter) {
        return;
    }
    QMutexLocker locker(&waitersMutex_);
    if (std::find(frameWaiters_.begin(), frameWaiters_.end(), waiter) == frameWaiters_.end()) {
        frameWaiters_.push_back(waiter);
    }
}

void AudioTimestampRingQueue::unregisterFrameWaiter(AudioTickWaiter *waiter)
{
    if (!waiter) {
        return;
    }
    QMutexLocker locker(&waitersMutex_);
    frameWaiters_.erase(std::remove(frameWaiters_.begin(), frameWaiters_.end(), waiter),
                        frameWaiters_.end());
}

void AudioTimestampRingQueue::notifyFrameWaiters()
{
    std::vector<AudioTickWaiter *> waiters;
    {
        QMutexLocker locker(&waitersMutex_);
        waiters = frameWaiters_;
    }
    for (AudioTickWaiter *waiter : waiters) {
        if (waiter) {
            waiter->notifyFromClock();
        }
    }
}
