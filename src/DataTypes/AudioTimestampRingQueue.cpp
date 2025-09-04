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
    QMutexLocker locker(&mutex_);
    if (!isActive_) {
        return false;
    }

    // 将帧写入当前写索引位置
    frames_[writeIndex_] = frame;
    // 发送信号通知数据已写入
    emit frameWritten(writeIndex_);
    emit newFrameWritten(frame);
    // 更新写索引（环形递增）
    writeIndex_ = (writeIndex_ + 1) % maxSize_;

    return true;
}

int AudioTimestampRingQueue::getAvailableFrameCount(int consumerReadIndex) const {
    if (consumerReadIndex == writeIndex_) {
        return 0; // 没有新数据
    }

    if (writeIndex_ > consumerReadIndex) {
        return writeIndex_ - consumerReadIndex;
    } else {
        return maxSize_ - consumerReadIndex + writeIndex_;
    }
}

double AudioTimestampRingQueue::getUsedRatio() const {
    QMutexLocker locker(&mutex_);

    if (consumerStatus_.isEmpty()||frames_.isEmpty()) {
        return -1;
    }

    int maxAvailableFrames = 0;
    for (auto it = consumerStatus_.begin(); it != consumerStatus_.end(); ++it) {
        int available = getAvailableFrameCount(it.value().readIndex);
        maxAvailableFrames = std::max(maxAvailableFrames, available);
    }

    return static_cast<double>(maxAvailableFrames) / maxSize_;
}

bool AudioTimestampRingQueue::getFrameByTimestamp(int consumerId, qint64 targetTimestamp, qint64 tolerance, AudioFrame& frame) {
    QMutexLocker locker(&mutex_);

    if (!isActive_ ) {
        return false;
    }

    // 确保消费者已注册
    if (!consumerStatus_.contains(consumerId)) {
        return false;
    }

    ConsumerInfo& consumer = consumerStatus_[consumerId];
    int bestIndex = -1;
    qint64 bestTimeDiff = LLONG_MAX;

    // 从消费者当前读索引开始搜索
    int searchCount = 0;
    int currentIndex = consumer.readIndex;

    // 搜索可用的帧（最多搜索整个队列）
    while (searchCount < maxSize_) {
        const AudioFrame& currentFrame = frames_[currentIndex];
        qint64 timeDiff = currentFrame.timestamp - targetTimestamp;

        // 优先选择时间戳小于等于目标时间戳的帧
        if (timeDiff <= 0 && std::abs(timeDiff) <= tolerance) {
            if (std::abs(timeDiff) < bestTimeDiff) {
                bestTimeDiff = std::abs(timeDiff);
                bestIndex = currentIndex;
            }
        }
        // 如果没有找到过去的帧，考虑未来的帧（更严格的容差）
        else if (bestIndex == -1 && timeDiff > 0 && timeDiff <= tolerance / 2) {
            if (timeDiff < bestTimeDiff) {
                bestTimeDiff = timeDiff;
                bestIndex = currentIndex;
            }
        }

        // 移动到下一个索引
        currentIndex = (currentIndex + 1) % maxSize_;
        searchCount++;

        // 如果到达写索引，停止搜索
        if (currentIndex == writeIndex_) {
            break;
        }
    }

    if (bestIndex != -1) {
        frame = frames_[bestIndex];
        // 更新消费者读索引到找到的帧的下一位
        consumer.readIndex = (bestIndex + 1) % maxSize_;
        consumer.readTimestamp = frame.timestamp;
        return true;
    }
    return false;
}

void AudioTimestampRingQueue::clear() {
    QMutexLocker locker(&mutex_);
    writeIndex_ = 0;
    // 重置所有消费者的读索引
    for (auto it = consumerStatus_.begin(); it != consumerStatus_.end(); ++it) {
        it.value().readIndex = 0;
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

int AudioTimestampRingQueue::registerNewConsumer(bool syncToWriteIndex) {
    static QAtomicInt nextId(1);
    int consumerId = nextId.fetchAndAddAcquire(1);

    if (registerConsumer(consumerId,syncToWriteIndex)) {
        return consumerId;
    }
    return -1;
}

bool AudioTimestampRingQueue::registerConsumer(int consumerId,bool syncToWriteIndex) {
    QMutexLocker locker(&mutex_);

    if (consumerStatus_.contains(consumerId)) {
        return false; // 消费者已存在
    }

    ConsumerInfo info;
    info.consumerId = consumerId;

    info.readTimestamp = 0;

    // 根据系统时钟确定初始读索引
    qint64 currentSystemTime = QDateTime::currentMSecsSinceEpoch();
    int bestIndex = findBestIndexByTimestamp(currentSystemTime);

    if (bestIndex != -1) {
        info.readIndex = bestIndex;
        info.readTimestamp = frames_[bestIndex].timestamp;
        qDebug() << "Consumer" << consumerId << "registered with readIndex" << bestIndex
                 << "timestamp" << frames_[bestIndex].timestamp;
        consumerStatus_[consumerId] = info;
        return true;
    }
        if (bestIndex==-1||syncToWriteIndex){
        // 如果找不到合适的帧或指定与写索引同步，则使用当前写索引
        info.readIndex = writeIndex_;
        qDebug() << "Consumer" << consumerId << "registered with writeIndex" << writeIndex_
                 << "(no suitable frame found)";
        consumerStatus_[consumerId] = info;
        return true;
    }

    consumerStatus_[consumerId] = info;
    return true;
}

int AudioTimestampRingQueue::findBestIndexByTimestamp(qint64 targetTimestamp) const {
    int bestIndex = -1;
    qint64 bestTimeDiff = LLONG_MAX;

    // 计算队列中有效帧的数量
    int availableFrames = getAvailableFrameCount(0); // 从索引0开始计算总帧数
    if (availableFrames == 0) {
        return -1;
    }

    // 搜索所有有效帧
    for (int i = 0; i < maxSize_; ++i) {
        const AudioFrame& frame = frames_[i];

        // 跳过无效帧（时间戳为0或负数）
        if (frame.timestamp <= 0) {
            continue;
        }

        qint64 timeDiff = std::abs(frame.timestamp - targetTimestamp);

        // 优先选择时间戳小于等于目标时间戳的帧
        if (frame.timestamp <= targetTimestamp) {
            if (timeDiff < bestTimeDiff) {
                bestTimeDiff = timeDiff;
                bestIndex = i;
            }
        }
        // 如果没有找到过去的帧，考虑未来的帧
        else if (bestIndex == -1) {
            if (timeDiff < bestTimeDiff) {
                bestTimeDiff = timeDiff;
                bestIndex = i;
            }
        }
    }

    return bestIndex;
}

bool AudioTimestampRingQueue::unregisterConsumer(int consumerId) {
    QMutexLocker locker(&mutex_);

    if (!consumerStatus_.contains(consumerId)) {
        return false; // 消费者不存在
    }
    consumerStatus_.remove(consumerId);
    return true;
}

