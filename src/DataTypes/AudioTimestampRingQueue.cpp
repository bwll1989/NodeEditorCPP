//
// Created by WuBin on 2025/8/29.
//

#include "AudioTimestampRingQueue.h"

using QtNodes::NodeData;
using QtNodes::NodeDataType;

AudioTimestampRingQueue::AudioTimestampRingQueue(int maxSize, QObject* parent)
    : QObject(parent),maxSize_(maxSize), isActive_(true), writeIndex_(0) {
    frames_.resize(maxSize_);  // 预分配固定大小的缓冲区
}


AudioTimestampRingQueue::~AudioTimestampRingQueue() {
    clear();
}

bool AudioTimestampRingQueue::pushFrame(const AudioFrame& frame) {
    int emittedWriteIndex = -1;
    AudioFrame emittedFrame;
    {
        QMutexLocker locker(&mutex_);
        if (!isActive_) {
            return false;
        }

        // 准备覆盖前的旧时间戳
        const qint64 oldTs = frames_[writeIndex_].timestamp;
        const bool wasValid = oldTs > 0;

        // 覆盖槽位
        frames_[writeIndex_] = frame;
        const bool nowValid = frame.timestamp > 0;

        // 维护有效帧计数
        if (!wasValid && nowValid) {
            if (validFrames_ < maxSize_) validFrames_++;
        } else if (wasValid && !nowValid) {
            if (validFrames_ > 0) validFrames_--;
        }

        // 先移除旧映射，避免哈希膨胀与指向过期槽位
        if (wasValid && oldTs != frame.timestamp) {
            timestampToIndex_.remove(oldTs);
        }
        // 再写入新映射
        if (nowValid) {
            timestampToIndex_.insert(frame.timestamp, writeIndex_);
        }

        // 记录要发射的参数，锁外发射信号
        emittedWriteIndex = writeIndex_;
        emittedFrame = frame;

        // 更新写索引（环形递增）
        writeIndex_ = (writeIndex_ + 1) % maxSize_;
    }

    // 锁外发射，避免长时间持锁导致卡顿
    if (emittedWriteIndex >= 0) {
        emit frameWritten(emittedWriteIndex);
        emit newFrameWritten(emittedFrame);
    }

    return true;
}

double AudioTimestampRingQueue::getUsedRatio() const {
    QMutexLocker locker(&mutex_);
    if (maxSize_ <= 0) return 0.0;
    return static_cast<double>(validFrames_) / static_cast<double>(maxSize_);
}

bool AudioTimestampRingQueue::getFrameByTimestamp(qint64 targetFrameCount, AudioFrame& frame) {
    QMutexLocker locker(&mutex_);
    if (!isActive_) {
        return false;
    }

    // 先用小缓存（连续访问很常见）
    if (lastHitIndex_ >= 0) {
        const AudioFrame& cached = frames_[lastHitIndex_];
        if (cached.timestamp == targetFrameCount) {
            frame = cached;
            return true;
        }
        if (cached.timestamp + 1 == targetFrameCount) {
            // 直接使用相邻槽（避免二次哈希）：假设时间戳连续写入映射到同一槽也安全
            auto it = timestampToIndex_.find(targetFrameCount);
            if (it != timestampToIndex_.end()) {
                int idx = it.value();
                const AudioFrame& hit = frames_[idx];
                if (hit.timestamp == targetFrameCount) {
                    frame = hit;
                    lastHitTimestamp_ = hit.timestamp;
                    lastHitIndex_ = idx;
                    return true;
                }
            }
        }
        if (cached.timestamp == targetFrameCount + 1) {
            frame = cached;
            return true;
        }
    }

    // 单次哈希查找：先 target
    auto it = timestampToIndex_.find(targetFrameCount);
    if (it != timestampToIndex_.end()) {
        int idx = it.value();
        const AudioFrame& hit = frames_[idx];
        if (hit.timestamp == targetFrameCount) {
            frame = hit;
            lastHitTimestamp_ = hit.timestamp;
            lastHitIndex_ = idx;
            return true;
        }
    }

    // // 再 target+1（最多一次额外查找）
    // auto it2 = timestampToIndex_.find(targetFrameCount + 1);
    // qDebug()<<"can not find targetFrameCount"<<targetFrameCount;
    // if (it2 != timestampToIndex_.end()) {
    //     int idx2 = it2.value();
    //     const AudioFrame& hit2 = frames_[idx2];
    //     if (hit2.timestamp == targetFrameCount + 1) {
    //         frame = hit2;
    //         lastHitTimestamp_ = hit2.timestamp;
    //         lastHitIndex_ = idx2;
    //         return true;
    //     }
    // }

    return false;
}

void AudioTimestampRingQueue::clear() {
    QMutexLocker locker(&mutex_);
    writeIndex_ = 0;
    timestampToIndex_.clear();
    validFrames_ = 0;
    lastHitTimestamp_ = 0;
    lastHitIndex_ = -1;
    for (int i = 0; i < frames_.size(); ++i) {
        frames_[i] = AudioFrame();
    }
}

void AudioTimestampRingQueue::setActive(bool active) {
    QMutexLocker locker(&mutex_);
    isActive_ = active;
}

bool AudioTimestampRingQueue::isActive() const {
    QMutexLocker locker(&mutex_);
    return isActive_;
}

