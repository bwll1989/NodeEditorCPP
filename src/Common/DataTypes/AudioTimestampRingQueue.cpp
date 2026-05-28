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

/**
 * 函数级注释：写入一帧音频到环形缓冲区（单写者场景），并在写入完成后发出通知信号
 * - 覆盖策略：缓冲区固定长度，写入总是覆盖当前 writeIndex_ 指向的槽位
 * - 有效帧统计：通过 timestamp > 0 判断槽位是否“有效”，用于 getUsedRatio() 的使用率估算
 * - 读侧定位锚点：latestTimestamp_/latestIndex_ 记录“最近一次写入的有效帧”，用于读侧按时间戳 O(1) 反推定位
 * - 信号发射：在锁外 emit，避免信号槽执行耗时导致写入线程长时间持锁
 */
bool AudioTimestampRingQueue::pushFrame(const AudioFrame& frame) {
    int emittedWriteIndex = -1;
    AudioFrame emittedFrame;
    {
        QMutexLocker locker(&mutex_);
        if (!isActive_) {
            return false;
        }

        // 读取将被覆盖的槽位信息：用于维护有效帧计数（validFrames_）
        const qint64 oldTs = frames_[writeIndex_].timestamp;
        const bool wasValid = oldTs > 0;

        // 覆盖槽位：环形缓冲区写入，固定写到 writeIndex_，不做 push_back 扩容
        frames_[writeIndex_] = frame;
        const bool nowValid = frame.timestamp > 0;

        // 维护有效帧计数：仅统计 timestamp>0 的槽位数量
        // - 从无效->有效：+1
        // - 从有效->无效：-1
        if (!wasValid && nowValid) {
            if (validFrames_ < maxSize_) validFrames_++;
        } else if (wasValid && !nowValid) {
            if (validFrames_ > 0) validFrames_--;
        }

        // 更新读侧定位锚点：记录“最近一次写入的有效帧”所在槽位
        // 读侧会通过 delta = latestTimestamp_ - target 反推 idx = latestIndex_ - delta（取模）来定位目标帧
        if (nowValid) {
            latestTimestamp_ = frame.timestamp;
            latestIndex_ = writeIndex_;
        }

        // 记录要发射的参数（写入索引与帧内容），确保锁外 emit 时不再访问共享数据结构
        emittedWriteIndex = writeIndex_;
        emittedFrame = frame;

        // 更新写索引（环形递增）：下一次写入将覆盖下一个槽位
        writeIndex_ = (writeIndex_ + 1) % maxSize_;
    }

    // 锁外发射：避免信号槽执行耗时导致长时间持锁，影响音频实时性
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

/**
 * 函数级注释：根据目标时间戳获取对应音频帧（环形缓冲区，读侧优化）
 * - 线程安全：内部使用 QMutexLocker 保证并发读写安全，锁外不持有资源
 * - 访问策略（由快到慢）：
 *   1) 小缓存命中：优先检查上次命中的槽位（lastHitIndex_），若 timestamp 相等直接返回
 *   2) 相邻槽命中：若上次命中时间戳 + 1 等于目标时间戳，尝试读取相邻槽位（顺序访问高概率命中）
 *   3) 基于最新写入锚点的 O(1) 反推定位：
 *      - latestTimestamp_ / latestIndex_ 在写入有效帧时更新
 *      - 通过 delta = latestTimestamp_ - targetFrameCount 反推目标槽位 idx = (latestIndex_ - delta) mod maxSize_
 *      - 校验 frames_[idx].timestamp 是否匹配，匹配则命中
 *   4) 小范围容错扫描：若未命中，围绕 idx 进行左右各不超过 4 个槽位的扫描，处理偶发的丢帧/非严格连续情况
 * - 返回值：
 *   - true：找到匹配的帧并写入到参数 frame
 *   - false：队列未激活、队列为空、目标已被覆盖或无法命中
 * - 适用场景：单生产者/单消费者、写入时间戳单调递增、读侧以顺序访问为主（偶发跳转可由扫描容错）
 * - 复杂度与稳定性：避免哈希查找与大范围线性扫描，绝大多数情况下为 O(1) 命中；在非连续情况下退化为常数半径扫描
 */
bool AudioTimestampRingQueue::getFrameByTimestamp(qint64 targetFrameCount, AudioFrame& frame) {
    QMutexLocker locker(&mutex_);
    if (!isActive_) {
        return false;
    }

    // 先用小缓存（连续访问很常见）
    if (lastHitIndex_ >= 0 && lastHitIndex_ < maxSize_) {
        const AudioFrame& cached = frames_[lastHitIndex_];
        if (cached.timestamp == targetFrameCount) {
            frame = cached;
            return true;
        }
        
        // 真正实现"直接使用相邻槽"，不查哈希表，大幅降低锁竞争
        if (cached.timestamp + 1 == targetFrameCount) {
            int nextIdx = (lastHitIndex_ + 1) % maxSize_;
            const AudioFrame& nextFrame = frames_[nextIdx];
            if (nextFrame.timestamp == targetFrameCount) {
                frame = nextFrame;
                lastHitIndex_ = nextIdx;
                lastHitTimestamp_ = nextFrame.timestamp;
                return true;
            }
        }
        
        // 容错：如果缓存的时间戳已经不匹配（比如被覆盖），走下面的基于最新写入的定位
    }

    if (latestTimestamp_ <= 0 || latestIndex_ < 0 || maxSize_ <= 0) {
        return false;
    }

    const qint64 delta = latestTimestamp_ - targetFrameCount;
    if (delta < 0 || delta >= maxSize_) {
        return false;
    }

    int idx = latestIndex_ - static_cast<int>(delta);
    idx %= maxSize_;
    if (idx < 0) idx += maxSize_;

    const AudioFrame& hit = frames_[idx];
    if (hit.timestamp == targetFrameCount) {
        frame = hit;
        lastHitTimestamp_ = hit.timestamp;
        lastHitIndex_ = idx;
        return true;
    }

    constexpr int scanRadius = 4;
    for (int offset = 1; offset <= scanRadius; ++offset) {
        int idxR = (idx + offset) % maxSize_;
        const AudioFrame& r = frames_[idxR];
        if (r.timestamp == targetFrameCount) {
            frame = r;
            lastHitTimestamp_ = r.timestamp;
            lastHitIndex_ = idxR;
            return true;
        }

        int idxL = (idx - offset) % maxSize_;
        if (idxL < 0) idxL += maxSize_;
        const AudioFrame& l = frames_[idxL];
        if (l.timestamp == targetFrameCount) {
            frame = l;
            lastHitTimestamp_ = l.timestamp;
            lastHitIndex_ = idxL;
            return true;
        }
    }

    return false;
}

void AudioTimestampRingQueue::clear() {
    QMutexLocker locker(&mutex_);
    writeIndex_ = 0;
    validFrames_ = 0;
    lastHitTimestamp_ = 0;
    lastHitIndex_ = -1;
    latestTimestamp_ = 0;
    latestIndex_ = -1;
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

