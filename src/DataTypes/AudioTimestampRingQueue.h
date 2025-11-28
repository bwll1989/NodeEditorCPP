//
// Created by WuBin on 2025/8/29.
//
#pragma once
#include "QtNodes/NodeData"
#include <QMetaType>
#include <QQueue>
#include <QMutex>
#include <QAtomicInt>
#include <memory>
#include <QDateTime>
#include <QMap>
#include <QObject>  // 添加QObject头文件
#include "TimestampGenerator/TimestampGenerator.hpp"
#include <QHash>
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
    explicit AudioTimestampRingQueue(int maxSize = 16,QObject* parent = nullptr);
    ~AudioTimestampRingQueue() ;

    bool pushFrame(const AudioFrame& frame);

    // 使用当前有效帧数估算使用率（0.0-1.0）
    double getUsedRatio() const ;

    // 无消费者版本：先匹配 target，再匹配 target+1
    bool getFrameByTimestamp(qint64 targetFrameCount, AudioFrame& frame) ;

    void clear() ;

    void setActive(bool active) ;
    bool isActive() const ;

signals:
    void frameWritten(int WriteIndex);
    void newFrameWritten(AudioFrame);

private:
    QVector<AudioFrame> frames_;                  // 固定大小的环形缓冲区
    int maxSize_;                                 // 队列最大长度
    bool isActive_;                               // 队列激活状态
    int writeIndex_;                              // 写索引（0到maxSize-1循环）
    mutable QMutex mutex_;                        // 线程安全互斥锁
    QHash<qint64, int> timestampToIndex_;         // 时间戳到索引的快速映射
    int validFrames_ = 0;                         // 有效帧数量（timestamp>0）

    // 读取侧小缓存：加速顺序访问，减少哈希查找次数
    qint64 lastHitTimestamp_ = 0;
    int lastHitIndex_ = -1;
};
