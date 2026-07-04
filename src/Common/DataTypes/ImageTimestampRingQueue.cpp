#include "ImageTimestampRingQueue.h"

namespace NodeDataTypes
{

ImageTimestampRingQueue::ImageTimestampRingQueue(int maxSize, QObject* parent)
    : QObject(parent)
    , maxSize_(maxSize)
{
    slots_.resize(static_cast<size_t>(maxSize_ > 0 ? maxSize_ : 0));
}

ImageTimestampRingQueue::~ImageTimestampRingQueue()
{
    clear();
}

bool ImageTimestampRingQueue::pushFrame(const ImageFrame& frame)
{
    return pushFrame(ImageFrame(frame));
}

bool ImageTimestampRingQueue::pushFrame(ImageFrame&& frame)
{
    if (!isActive_.load(std::memory_order_acquire)) {
        return false;
    }

    const int wi = writeIndex_.load(std::memory_order_relaxed);
    if (maxSize_ <= 0 || wi < 0 || wi >= maxSize_) {
        return false;
    }

    const qint64 newTimestamp = frame.timestamp;
    const bool nowValid = newTimestamp >= 0;

    // 覆盖写入前记录旧槽位有效性，用于维护 validFrames_ 计数
    const qint64 oldTs = slots_[static_cast<size_t>(wi)].frame.timestamp;
    const bool wasValid = oldTs >= 0;

    TimestampRingQueueDetail::writeRingEntry(slots_[static_cast<size_t>(wi)], std::move(frame));

    // 槽位从 无效→有效 或 有效→无效 时增减 validFrames_
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
        latestTimestamp_.store(newTimestamp, std::memory_order_release);
        latestIndex_.store(wi, std::memory_order_release);
    }

    // 写入完成后推进 writeIndex_，形成环形覆盖
    writeIndex_.store((wi + 1) % maxSize_, std::memory_order_release);

    emit frameWritten(wi);

    // 信号 payload 从槽位再读一次，确保订阅方拿到已提交的帧
    ImageFrame signalFrame;
    if (TimestampRingQueueDetail::readRingEntry(slots_[static_cast<size_t>(wi)], signalFrame)) {
        emit newFrameWritten(signalFrame);
    }
    return true;
}

double ImageTimestampRingQueue::getUsedRatio() const
{
    if (maxSize_ <= 0) {
        return 0.0;
    }
    return static_cast<double>(validFrames_.load(std::memory_order_relaxed)) /
           static_cast<double>(maxSize_);
}

bool ImageTimestampRingQueue::getFrameByTimestamp(qint64 targetFrameCount, ImageFrame& frame)
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

bool ImageTimestampRingQueue::getLatestFrame(ImageFrame& frame) const
{
    if (!isActive_.load(std::memory_order_acquire) || maxSize_ <= 0) {
        return false;
    }

    const int idx = latestIndex_.load(std::memory_order_acquire);
    const qint64 ts = latestTimestamp_.load(std::memory_order_acquire);
    if (idx < 0 || idx >= maxSize_ || ts < 0) {
        return false;
    }

    ImageFrame latest;
    if (!TimestampRingQueueDetail::readRingEntry(slots_[static_cast<size_t>(idx)], latest)) {
        return false;
    }
    if (latest.timestamp < 0 || latest.empty()) {
        return false;
    }

    frame = std::move(latest);
    return true;
}

void ImageTimestampRingQueue::clear()
{
    for (auto& ringEntry : slots_) {
        TimestampRingQueueDetail::writeRingEntry(ringEntry, ImageFrame());
    }

    writeIndex_.store(0, std::memory_order_relaxed);
    validFrames_.store(0, std::memory_order_relaxed);
    lastHitTimestamp_ = 0;
    lastHitIndex_ = -1;
    latestTimestamp_.store(0, std::memory_order_release);
    latestIndex_.store(-1, std::memory_order_release);
}

void ImageTimestampRingQueue::setActive(bool active)
{
    isActive_.store(active, std::memory_order_release);
}

bool ImageTimestampRingQueue::isActive() const
{
    return isActive_.load(std::memory_order_acquire);
}

} // namespace NodeDataTypes
