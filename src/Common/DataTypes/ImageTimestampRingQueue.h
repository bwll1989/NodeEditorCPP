#pragma once

#include "GpuTextureHandle.h"
#include "TimestampRingQueueLockFree.hpp"

#include <QObject>
#include <atomic>
#include <memory>
#include <vector>

namespace NodeDataTypes
{

/**
 * @file ImageTimestampRingQueue.h
 * @brief 基于全局 tick 帧号的图像环形缓冲队列
 *
 * 语义对齐 AudioTimestampRingQueue，供 ImageData 共享句柄引用。
 *
 * 生产者 / 消费者约定：
 *   - 生产者 pushFrame(ImageFrame)，timestamp 通常为 TimestampGenerator::getCurrentFrameCount()
 *   - 消费者 getLatestFrame() 取最新有效帧（显示节点主路径，O(1)）
 *   - 消费者 getFrameByTimestamp(n) 按帧号精确查找（tick 对齐；带 lastHit 局部性缓存）
 *
 * 槽位存储 ImageFrame { GpuTextureHandle texture, cv::Mat image, qint64 timestamp }：
 *   - GPU 主路径通常仅填 texture（零 CPU 拷贝）
 *   - image 供 legacy OpenCV 节点或 ensureGpuTexture fallback
 *
 * 线程安全：
 *   - 基于 TimestampRingQueueDetail 的无锁槽位读写（seqlock 语义）
 *   - 单生产者 + 多消费者场景下安全；多生产者需外部串行化 push
 *
 * 默认容量 8 帧（与 ensureImageDataBuffer 一致），覆盖约 8 个系统 tick 的历史，
 * 允许消费者在精确 timestamp 未命中时向前回溯若干帧。
 */

/**
 * @brief 图像帧环形队列（QObject，可 emit 新帧信号供 UI 订阅）
 */
class DATATYPES_EXPORT ImageTimestampRingQueue final : public QObject
{
    Q_OBJECT

public:
    explicit ImageTimestampRingQueue(int maxSize = 8, QObject* parent = nullptr);
    ~ImageTimestampRingQueue() override;

    /**
     * @brief 写入下一槽位（覆盖最旧帧）
     * @return 队列 active 且帧 timestamp >= 0 时成功
     * @note 写入后 emit frameWritten(writeIndex) 与 newFrameWritten(frame)
     */
    bool pushFrame(const ImageFrame& frame);
    bool pushFrame(ImageFrame&& frame);

    /** @brief 有效帧数 / 容量，用于监控缓冲占用（0.0 ~ 1.0） */
    double getUsedRatio() const;

    /**
     * @brief 按目标帧号查找
     *
     * 内部维护 lastHitTimestamp_/lastHitIndex_ 缓存，
     * 连续 tick 查询时避免全表扫描（详见 TimestampRingQueueDetail::lookupFrameByTimestamp）。
     */
    bool getFrameByTimestamp(qint64 targetFrameCount, ImageFrame& frame);

    /** @brief 取最新一帧（latestIndex_ / latestTimestamp_ 快速路径） */
    bool getLatestFrame(ImageFrame& frame) const;

    /** @brief 清空所有槽位并重置索引/计数器 */
    void clear();

    /**
     * @brief 启用/禁用队列
     * @note inactive 时 push 失败、get 返回 false；节点析构时可 setActive(false) 阻止后续写入
     */
    void setActive(bool active);
    bool isActive() const;

signals:
    /** @brief 写入完成，参数为本次写入的槽索引 */
    void frameWritten(int writeIndex);

    /** @brief 新帧可用（拷贝/移动语义取决于 Qt 信号槽连接方式，订阅方宜尽快处理） */
    void newFrameWritten(ImageFrame frame);

private:
    using Slot = TimestampRingQueueDetail::LockFreeSlot<ImageFrame>;

    std::vector<Slot> slots_;
    int maxSize_ = 0;
    std::atomic<bool> isActive_{true};

    std::atomic<int> writeIndex_{0};   ///< 下一写入槽索引（环形递增 % maxSize_）
    std::atomic<int> validFrames_{0};  ///< 当前有效帧计数（timestamp >= 0 的槽位数）

    /** @brief getFrameByTimestamp 局部性缓存（mutable：const get 也可更新） */
    mutable qint64 lastHitTimestamp_ = 0;
    mutable int lastHitIndex_ = -1;

    std::atomic<qint64> latestTimestamp_{0}; ///< 最近一次 push 的有效 timestamp
    std::atomic<int> latestIndex_{-1};       ///< 最近一次 push 的槽索引
};

} // namespace NodeDataTypes
