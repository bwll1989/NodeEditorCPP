//
// Created by WuBin on 2025/8/29.
//

#include "TimestampRingQueueLockFree.hpp"
#include "AudioTimestampRingQueue.h"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

AudioTimestampRingQueue::AudioTimestampRingQueue(int maxSize, QObject* parent)
    : QObject(parent), maxSize_(maxSize)
{
    slots_.resize(static_cast<size_t>(maxSize_ > 0 ? maxSize_ : 0));
}

AudioTimestampRingQueue::~AudioTimestampRingQueue()
{
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
