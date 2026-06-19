//
// Created by WuBin on 2025/8/29.
//
#pragma once
#include "TimestampRingQueueLockFree.hpp"
#include "QtNodes/NodeData"
#include <QMetaType>
#include <QQueue>
#include <atomic>
#include <memory>
#include <vector>
#include <QDateTime>
#include <QMap>
#include <QObject>
#include "TimestampGenerator/TimestampGenerator.hpp"
#include "DataTypesExport.h"
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
 * @brief 基于时间戳的环形音频队列
 * AudioDecoder始终写入队首，AudioDeviceOut根据时间戳提取帧
 */
class DATATYPES_EXPORT AudioTimestampRingQueue : public QObject {
    Q_OBJECT
public:
    explicit AudioTimestampRingQueue(int maxSize = 64,QObject* parent = nullptr);
    ~AudioTimestampRingQueue() ;
    /**
     * @brief 推入帧
     * @param frame 帧
     * @return 是否成功
     */
    bool pushFrame(const AudioFrame& frame);

    /**
     * @brief 使用当前有效帧数估算使用率（0.0-1.0）
     * @return 使用率
     */
    double getUsedRatio() const ;

    /**
     * @brief 无消费者版本：先匹配 target，再匹配 target+1
     * @param targetFrameCount 目标帧数
     * @param frame 帧
     * @return 是否成功
     */
    bool getFrameByTimestamp(qint64 targetFrameCount, AudioFrame& frame) ;

    /**
     * @brief 清空队列
     */
    void clear() ;

    /**
     * @brief 设置队列激活状态
     * @param active 激活状态
     */
    void setActive(bool active) ;
    /**
     * @brief 获取队列激活状态
     * @return 激活状态
     */
    bool isActive() const ;

signals:
    /**
     * @brief 帧写入信号
     * @param WriteIndex 写入索引
     */
    void frameWritten(int WriteIndex);
    /**
     * @brief 新帧写入信号
     * @param frame 帧
     */
    void newFrameWritten(AudioFrame);

private:
    using Slot = TimestampRingQueueDetail::LockFreeSlot<AudioFrame>;

    std::vector<Slot> slots_;                     // 固定大小的环形缓冲区
    int maxSize_;                                 // 队列最大长度
    std::atomic<bool> isActive_{true};            // 队列激活状态
    std::atomic<int> writeIndex_{0};              // 写索引（0到maxSize-1循环）
    std::atomic<int> validFrames_{0};             // 有效帧数量（timestamp>0）

    // 读取侧小缓存：加速顺序访问，减少哈希查找次数
    qint64 lastHitTimestamp_ = 0;
    int lastHitIndex_ = -1;
    std::atomic<qint64> latestTimestamp_{0};
    std::atomic<int> latestIndex_{-1};
};
