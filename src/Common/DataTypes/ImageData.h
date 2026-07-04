//
// Created by pablo on 2/28/24.
//

#pragma once

#include <QtNodes/NodeData>

#include "ImageTimestampRingQueue.h"

#include <memory>

#include "DataTypesExport.h"

namespace NodeDataTypes
{

/**
 * @file ImageData.h
 * @brief 节点编辑器中 IMAGE 端口的共享数据类型
 *
 * ImageData 本身**不存储像素**，仅持有 ImageTimestampRingQueue 的 shared_ptr。
 * 节点间连线时，上游 outData() 与下游 setInData() 传递的是同一个 buffer 句柄，
 * 实现零拷贝帧共享。
 *
 * 数据流约定：
 * ┌─────────────┐  pushFrame(ImageFrame)   ┌──────────────────────┐
 * │  生产者节点  │ ──────────────────────► │ ImageTimestampRingQueue │
 * └─────────────┘                         └──────────┬───────────┘
 *                                                    │ shared_ptr
 * ┌─────────────┐  getLatestFrame / byTimestamp    │
 * │  消费者节点  │ ◄─────────────────────────────────┘
 * └─────────────┘         ImageData 句柄
 *
 * 调度：消费者通常在 TimestampGenerator::frameCountUpdated（QueuedConnection）
 * 回调中按 tick 拉帧，而非在 setInData 中同步处理像素。
 */

/**
 * @brief 图像节点数据 — ImageTimestampRingQueue 的共享句柄
 *
 * 生产者：构建 ImageFrame{texture 或 image, timestamp} 并 push 到 buffer。
 * 消费者：通过 getSharedImageBuffer() 按 tick 取帧；**勿**在 ImageData 成员上缓存像素。
 */
class DATATYPES_EXPORT ImageData final : public QtNodes::NodeData
{
public:
    ImageData();

    QtNodes::NodeDataType type() const override;

    /** @brief 绑定（或替换）共享 ring buffer；连线时由图模型注入同一实例 */
    void setSharedImageBuffer(std::shared_ptr<ImageTimestampRingQueue> buffer);

    /** @brief 获取共享 buffer；未连接时返回 nullptr */
    std::shared_ptr<ImageTimestampRingQueue> getSharedImageBuffer() const;

    /** @brief 是否已绑定 ring buffer（连线状态，与 buffer 是否为空帧无关） */
    bool isConnectedToSharedBuffer() const;

    /** @brief 断开 buffer 引用（析构 / 断线清理） */
    void disconnect();

    /** @brief buffer 中是否存在至少一帧有效数据 */
    bool isEmpty() const;

private:
    std::shared_ptr<ImageTimestampRingQueue> sharedImageBuffer_;
};

/**
 * @brief 确保输出侧 ImageData + ring buffer 已创建并完成绑定
 *
 * 节点构造时调用一次即可。maxSize 默认 8，与系统 tick 历史深度对齐，
 * 允许消费者在精确 timestamp 未命中时向前回溯若干帧。
 *
 * @param outImageData  输出 ImageData 智能指针（可为空，函数内 lazy 创建）
 * @param outBuffer     ring buffer 智能指针（可为空，函数内 lazy 创建）
 * @param maxSize       环形队列容量
 */
DATATYPES_EXPORT void ensureImageDataBuffer(std::shared_ptr<ImageData>& outImageData,
                                            std::shared_ptr<ImageTimestampRingQueue>& outBuffer,
                                            int maxSize = 8);

/** @brief 便捷判断：ImageData 非空且已绑定 buffer */
DATATYPES_EXPORT bool usesSharedImageBuffer(const std::shared_ptr<ImageData>& imageData);

/** @brief 便捷判断：ImageData 为空或未绑定 buffer 或 buffer 内无有效帧 */
DATATYPES_EXPORT bool imageDataIsEmpty(const std::shared_ptr<ImageData>& imageData);

/** @brief 从 buffer 取最新一帧（显示节点 / fallback 主路径） */
DATATYPES_EXPORT bool getLatestImageFrame(const std::shared_ptr<ImageData>& imageData, ImageFrame& frame);

/**
 * @brief 按目标帧号精确查找（tick 对齐路径）
 *
 * 仅当 buffer 中存在 timestamp == targetTimestamp 的帧时返回 true。
 * 未命中时不做 fallback；需要模糊匹配时由节点侧自行 offset 回溯或调用 getLatestImageFrame。
 */
DATATYPES_EXPORT bool resolveImageFrameAtTimestamp(const std::shared_ptr<ImageData>& imageData,
                                                   qint64 targetTimestamp,
                                                   ImageFrame& frame);

/**
 * @brief 去重 push：同一 timestamp 不重复写入
 *
 * lastPushedTimestamp 由节点持有，用于防止同一 tick 内多次 push 相同帧号
 * （例如参数未变时的重复 processImage 调用）。
 *
 * @param lastPushedTimestamp  上次成功 push 的 timestamp，成功 push 后由本函数更新
 * @return push 成功或已 dedup 跳过返回 true；帧无效返回 false
 */
DATATYPES_EXPORT bool pushFrameToImageBufferDedup(const std::shared_ptr<ImageTimestampRingQueue>& buffer,
                                                  ImageFrame&& frame,
                                                  qint64& lastPushedTimestamp);

/** @brief Mat 便捷重载：内部包装为 ImageFrame::fromMat 后 dedup push */
DATATYPES_EXPORT bool pushFrameToImageBufferDedup(const std::shared_ptr<ImageTimestampRingQueue>& buffer,
                                                  cv::Mat&& image,
                                                  qint64 timestamp,
                                                  qint64& lastPushedTimestamp);

} // namespace NodeDataTypes
