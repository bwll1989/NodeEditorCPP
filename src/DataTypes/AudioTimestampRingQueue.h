//
// Created by WuBin on 2025/8/29.
//

#ifndef AUDIOTIMESTAMPRINGQUEUE_H
#define AUDIOTIMESTAMPRINGQUEUE_H

#include "QtNodes/NodeData"
#include <QMetaType>
#include <QQueue>
#include <QMutex>
#include <QAtomicInt>
#include <memory>
#include <QDateTime>
#include <QMap>
#include <QObject>  // 添加QObject头文件

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

/**
 * @brief 基于时间戳的环形音频队列
 * AudioDecoder始终写入队首，AudioDeviceOut根据时间戳提取最接近的帧
 * 根据队列长度动态调整播放速度，避免缓存溢出
 */

class AudioTimestampRingQueue : public QObject {
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param maxSize 队列最大长度（帧数）
     * @param parent 父对象指针
     */
    explicit AudioTimestampRingQueue(int maxSize = 8,QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~AudioTimestampRingQueue() ;

    /**
     * @brief 写入音频帧到环形队列（AudioDecoder调用）
     * @param frame 音频帧数据
     * @return 写入成功返回true
     */
    bool pushFrame(const AudioFrame& frame);

    /**
     * @brief 计算消费者可读取的帧数
     * @param consumerReadIndex 消费者的读索引
     * @return 可读取的帧数
     */
    int getAvailableFrameCount(int consumerReadIndex) const ;

    /**
     * @brief 获取队列使用率（0.0-1.0）
     * @return 使用率，基于最慢消费者的进度
     */
    double getUsedRatio() const ;

    /**
     * @brief 根据目标时间戳查找最接近的音频帧（AudioDeviceOut调用）
     * @param consumerId 消费者ID
     * @param targetTimestamp 目标时间戳（毫秒）
     * @param tolerance 时间容差（毫秒），默认50ms
     * @param frame 输出的音频帧
     * @return 找到合适帧返回true，否则返回false
     */
    bool getFrameByTimestamp(int consumerId, qint64 targetTimestamp, qint64 tolerance, AudioFrame& frame) ;

    /**
     * @brief 清空队列
     */
    void clear() ;

    /**
     * @brief 设置队列激活状态
     */
    void setActive(bool active) ;
    /**
     * @brief 获取队列激活状态
     */
    bool isActive() const ;

    /**
     * @brief 注册消费者并分配读取索引（自动分配ID）
     * @param syncToWriteIndex 是否同步到最近的写索引
     * @return 分配的消费者ID，-1表示失败
     */
    int registerNewConsumer(bool syncToWriteIndex=false) ;

    /**
    * @brief 注册指定ID的消费者
    * @param consumerId 消费者ID
    * * @param syncToWriteIndex 是否同步到最近的写索引
    * @return 是否注册成功
    */
    bool registerConsumer(int consumerId,bool syncToWriteIndex) ;


    /**
    * @brief 根据时间戳查找最佳索引位置
    * @param targetTimestamp 目标时间戳
    * @return 最佳索引，-1表示未找到
    */
    int findBestIndexByTimestamp(qint64 targetTimestamp) const ;
    /**
     * @brief 注销消费者
     * @param consumerId 消费者ID
     * @return 是否注销成功
     */
    bool unregisterConsumer(int consumerId) ;

signals:

    void frameWritten(int WriteIndex);

    void newFrameWritten(AudioFrame);

private:
    QVector<AudioFrame> frames_;                  // 固定大小的环形缓冲区
    int maxSize_;                                 // 队列最大长度
    bool isActive_;                               // 队列激活状态
    int writeIndex_;                              // 写索引（0到maxSize-1循环）
    mutable QMutex mutex_;                        // 线程安全互斥锁
    QMap<int, ConsumerInfo> consumerStatus_;      // 消费者状态表 <消费者ID, 消费者信息>
};



#endif //AUDIOTIMESTAMPRINGQUEUE_H
