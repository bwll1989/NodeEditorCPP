//
// Created by WuBin on 24-10-29.
//
#ifndef AUDIODATA_H
#define AUDIODATA_H
#include "QtNodes/NodeData"
#include <QMetaType>
#include <QQueue>
#include <QMutex>
#include <QAtomicInt>
#include <memory>
#include <QDateTime>
#include <QMap>

using QtNodes::NodeData;
using QtNodes::NodeDataType;

// 定义音频帧数据结构
struct AudioFrame {
    QByteArray data;        // 音频数据
    int sampleRate;         // 采样率
    int channels;           // 通道数
    int bitsPerSample;     // 每个采样的位数
    qint64 timestamp;       // 时间戳

    AudioFrame() : sampleRate(0), channels(0), bitsPerSample(0), timestamp(0) {}
};


/**
 * @brief 基于时间戳的有限音频队列
 * AudioDecoder始终写入队首，AudioDeviceOut根据时间戳提取最接近的帧
 * 根据队列长度动态调整播放速度，避免缓存溢出
 */
class AudioTimestampQueue {
public:
    /**
     * @brief 构造函数
     * @param maxSize 队列最大长度（帧数）
     */
    explicit AudioTimestampQueue(int maxSize = 8)
        : maxSize_(maxSize), isActive_(true) {
    }
    
    /**
     * @brief 写入音频帧到队首（AudioDecoder调用）
     * @param frame 音频帧数据
     * @return 写入成功返回true，队列满时自动移除最旧帧
     */
    bool pushFrame(const AudioFrame& frame) {
        QMutexLocker locker(&mutex_);
        
        if (!isActive_) return false;
        
        // 如果队列满了，移除最旧的帧（队尾）
        if (frames_.size() >= maxSize_) {
            frames_.removeLast();
        }
        
        // 插入到队首，保持时间戳有序
        frames_.prepend(frame);
        
        return true;
    }
    
    /**
     * @brief 根据目标时间戳查找最接近的音频帧（AudioDeviceOut调用）
     * @param consumerId 消费者ID
     * @param targetTimestamp 目标时间戳（毫秒）
     * @param tolerance 时间容差（毫秒），默认50ms
     * @param frame 输出的音频帧
     * @return 找到合适帧返回true，否则返回false
     */
    bool getFrameByTimestamp(int consumerId, qint64 targetTimestamp, qint64 tolerance, AudioFrame& frame) {
        QMutexLocker locker(&mutex_);
        
        if (frames_.isEmpty()) return false;
        
        // 检查消费者是否已注册
        if (!consumerReadIndices_.contains(consumerId)) {
            return false;
        }

        int bestIndex = -1;
        qint64 minTimeDiff = LLONG_MAX;
        
        for (int i = 0; i < frames_.size(); ++i) {
            qint64 timeDiff = std::abs(frames_[i].timestamp - targetTimestamp);
            if (timeDiff <= tolerance && timeDiff < minTimeDiff) {
                minTimeDiff = timeDiff;
                bestIndex = i;
            }
        }

        if (bestIndex >= 0) {
            frame = frames_[bestIndex];
            // 更新该消费者的读取索引到下一个位置
            consumerReadIndices_[consumerId] = bestIndex;
            return true;
        }
        
        return false;
    }
    
    /**
     * @brief 获取当前队列长度
     * @return 队列中的帧数
     */
    int getCurrentSize() const {
        QMutexLocker locker(&mutex_);
        return frames_.size();
    }
    
    /**
     * @brief 获取队列使用率（0.0-1.0）
     * @return 使用率，基于消费者读取索引的最大值与队列最大容量的比值
     */
    double getUsedRatio() const {
        QMutexLocker locker(&mutex_);
        
        if (consumerReadIndices_.isEmpty()) {
            return -1; // 没有消费者时使用率为0
        }
        
        // 找到所有消费者中的最大读取索引
        int maxReadIndex = 0;
        for (auto it = consumerReadIndices_.begin(); it != consumerReadIndices_.end(); ++it) {
            maxReadIndex = std::max(maxReadIndex, it.value());
        }
        
        // 计算使用率：最大读取索引与队列最大容量的比值
        return static_cast<double>(maxReadIndex) / maxSize_;
    }
    
    /**
     * @brief 清空队列
     */
    void clear() {
        QMutexLocker locker(&mutex_);
        frames_.clear();
    }
    
    /**
     * @brief 设置队列激活状态
     */
    void setActive(bool active) {
        QMutexLocker locker(&mutex_);
        isActive_ = active;
    }
    
    /**
     * @brief 获取队列激活状态
     */
    bool isActive() const {
        QMutexLocker locker(&mutex_);
        return isActive_;
    }
    /**
       * @brief 注册消费者并分配读取索引（自动分配ID）
       * @return 分配的消费者ID，-1表示失败
       */
    int registerNewConsumer() {
        static QAtomicInt nextId(1);
        int consumerId = nextId.fetchAndAddAcquire(1);

        if (registerConsumer(consumerId)) {
            return consumerId;
        }
        return -1;
    }

    /**
     * @brief 注册指定ID的消费者
     * @param consumerId 消费者ID
     * @return 是否注册成功
     */
    bool registerConsumer(int consumerId) {
        if (consumerReadIndices_.contains(consumerId)) {
            return false; // 消费者已存在
        }
        consumerReadIndices_[consumerId] = 0;

        return true;
    }

    /**
     * @brief 注销消费者
     * @param consumerId 消费者ID
     * @return 是否注销成功
     */
    bool unregisterConsumer(int consumerId) {

        if (!consumerReadIndices_.contains(consumerId)) {
            return false; // 消费者不存在
        }
        consumerReadIndices_.remove(consumerId);
        return true;
    }
private:
    QList<AudioFrame> frames_;          // 音频帧队列（按时间戳排序）
    int maxSize_;                       // 队列最大长度
    bool isActive_;                     // 队列激活状态
    mutable QMutex mutex_;              // 线程安全互斥锁
    QMap<int, int> consumerReadIndices_;      // 消费者读取索引表 <消费者ID, 读取索引>
};
/**
 * @brief 基于时间戳的环形音频队列
 * AudioDecoder始终写入队首，AudioDeviceOut根据时间戳提取最接近的帧
 * 根据队列长度动态调整播放速度，避免缓存溢出
 */


// 消费者信息结构体
struct ConsumerInfo {
    int consumerId;                 // 消费者ID
    int readIndex;                  // 当前读取位置

    qint64 readTimestamp;      // 最后读取时间戳

    ConsumerInfo() 
        : consumerId(-1)
        , readIndex(0)

        , readTimestamp(0) 
    {}
};

class AudioTimestampRingQueue {
public:
    /**
     * @brief 构造函数
     * @param maxSize 队列最大长度（帧数）
     */
    explicit AudioTimestampRingQueue(int maxSize = 8)
        : maxSize_(maxSize), isActive_(true), writeIndex_(0) {
        frames_.resize(maxSize_);  // 预分配固定大小的缓冲区
    }
    
    /**
     * @brief 析构函数
     */
    ~AudioTimestampRingQueue() {
        clear();
    }
    
    /**
     * @brief 写入音频帧到环形队列（AudioDecoder调用）
     * @param frame 音频帧数据
     * @return 写入成功返回true
     */
    bool pushFrame(const AudioFrame& frame) {
        QMutexLocker locker(&mutex_);
        if (!isActive_) {
            return false;
        }
        
        // 将帧写入当前写索引位置
        frames_[writeIndex_] = frame;
        
        // 更新写索引（环形递增）
        writeIndex_ = (writeIndex_ + 1) % maxSize_;
        // qDebug() << "push frame " << writeIndex_;
        return true;
    }
    
    /**
     * @brief 计算消费者可读取的帧数
     * @param consumerReadIndex 消费者的读索引
     * @return 可读取的帧数
     */
    int getAvailableFrameCount(int consumerReadIndex) const {
        if (consumerReadIndex == writeIndex_) {
            return 0; // 没有新数据
        }
        
        if (writeIndex_ > consumerReadIndex) {
            return writeIndex_ - consumerReadIndex;
        } else {
            return maxSize_ - consumerReadIndex + writeIndex_;
        }
    }
    
    /**
     * @brief 获取队列使用率（0.0-1.0）
     * @return 使用率，基于最慢消费者的进度
     */
    double getUsedRatio() const {
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
    
    /**
     * @brief 根据目标时间戳查找最接近的音频帧（AudioDeviceOut调用）
     * @param consumerId 消费者ID
     * @param targetTimestamp 目标时间戳（毫秒）
     * @param tolerance 时间容差（毫秒），默认50ms
     * @param frame 输出的音频帧
     * @return 找到合适帧返回true，否则返回false
     */
    bool getFrameByTimestamp(int consumerId, qint64 targetTimestamp, qint64 tolerance, AudioFrame& frame) {
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

    /**
     * @brief 清空队列
     */
    void clear() {
        QMutexLocker locker(&mutex_);
        writeIndex_ = 0;
        // 重置所有消费者的读索引
        for (auto it = consumerStatus_.begin(); it != consumerStatus_.end(); ++it) {
            it.value().readIndex = 0;
        }
    }
    
    /**
     * @brief 设置队列激活状态
     */
    void setActive(bool active) {
        QMutexLocker locker(&mutex_);
        isActive_ = active;
    }
    
    /**
     * @brief 获取队列激活状态
     */
    bool isActive() const {
        QMutexLocker locker(&mutex_);
        return isActive_;
    }
    
    /**
     * @brief 注册消费者并分配读取索引（自动分配ID）
     * @return 分配的消费者ID，-1表示失败
     */
    int registerNewConsumer() {
        static QAtomicInt nextId(1);
        int consumerId = nextId.fetchAndAddAcquire(1);
        
        if (registerConsumer(consumerId)) {
            return consumerId;
        }
        return -1;
    }
    
    /**
 * @brief 注册指定ID的消费者
 * @param consumerId 消费者ID
 * @return 是否注册成功
 */
bool registerConsumer(int consumerId) {
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
    } else {
        // 如果找不到合适的帧，使用当前写索引
        info.readIndex = writeIndex_;
        qDebug() << "Consumer" << consumerId << "registered with writeIndex" << writeIndex_ 
                 << "(no suitable frame found)";
    }
    
    consumerStatus_[consumerId] = info;
    return true;
}


/**
 * @brief 根据时间戳查找最佳索引位置
 * @param targetTimestamp 目标时间戳
 * @return 最佳索引，-1表示未找到
 */
int findBestIndexByTimestamp(qint64 targetTimestamp) const {
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
    /**
     * @brief 注销消费者
     * @param consumerId 消费者ID
     * @return 是否注销成功
     */
    bool unregisterConsumer(int consumerId) {
        QMutexLocker locker(&mutex_);
        
        if (!consumerStatus_.contains(consumerId)) {
            return false; // 消费者不存在
        }
        consumerStatus_.remove(consumerId);
        return true;
    }

private:
    QVector<AudioFrame> frames_;                  // 固定大小的环形缓冲区
    int maxSize_;                                 // 队列最大长度
    bool isActive_;                               // 队列激活状态
    int writeIndex_;                              // 写索引（0到maxSize-1循环）
    mutable QMutex mutex_;                        // 线程安全互斥锁
    QMap<int, ConsumerInfo> consumerStatus_;      // 消费者状态表 <消费者ID, 消费者信息>
};



namespace NodeDataTypes
{
    /**
     * @brief 音频数据类，直接持有共享环形缓冲区指针
     */
    class AudioData : public NodeData
    {
    public:
        AudioData() {
            qRegisterMetaType<AudioData>("AudioData");
        }

        NodeDataType type() const override {
            return NodeDataType{"Audio", "audio"};
        }

        /**
         * @brief 设置共享环形缓冲区（用于连接建立时）
         * @param buffer 共享的环形缓冲区指针
         */
        void setSharedAudioBuffer(std::shared_ptr<AudioTimestampRingQueue> buffer) {
            sharedAudioBuffer_ = buffer;
        }

        /**
         * @brief 获取共享环形缓冲区指针
         * @return 环形缓冲区指针，nullptr表示未连接
         */
        std::shared_ptr<AudioTimestampRingQueue> getSharedAudioBuffer() const {
            return sharedAudioBuffer_;
        }

        /**
         * @brief 检查是否已连接到共享缓冲区
         * @return true表示已连接，false表示未连接
         */
        bool isConnectedToSharedBuffer() const {
            return sharedAudioBuffer_ != nullptr;
        }

        /**
         * @brief 断开连接
         */
        void disconnect() {
            sharedAudioBuffer_.reset();
        }

        
    private:
        // std::shared_ptr<AudioTimestampQueue> sharedAudioBuffer_; // 共享环形缓冲区指针
        std::shared_ptr<AudioTimestampRingQueue> sharedAudioBuffer_; // 共享环形缓冲区指针
    };
}
#endif //AUDIODATA_H

    