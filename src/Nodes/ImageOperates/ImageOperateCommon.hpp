#pragma once

// 仅供 ImageOperates 插件内部使用的公共辅助头文件，其他模块请使用 ImageData.h 中的 API。

#include "NodeDataList.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include "Common/NodeWorker/NodeProcessCommon.hpp"
#include "Common/NodeWorker/NodeWorkerQueue.hpp"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <memory>

namespace Nodes
{
namespace ImageOperateHelpers
{
using ImageOperateTickState = NodeWorker::NodeTickState;
using ImageOperateDualTickState = NodeWorker::NodeDualTickState;
using ImageOperateWorkerQueue = NodeWorker::NodeWorkerQueue<cv::Mat>;

/**
 * @brief 将未指定的目标时间戳规整为当前全局时间戳
 * @param targetTimestamp 目标时间戳，传负数时自动使用当前全局时间戳
 * @return 可用于环形缓存检索的时间戳
 */
inline qint64 normalizeTargetTimestamp(qint64 targetTimestamp)
{
    return targetTimestamp >= 0 ? targetTimestamp : TimestampGenerator::getInstance()->getCurrentFrameCount();
}

/**
 * @brief 将图像转换为 8 位格式，便于后续统一处理
 * @param image 输入图像
 * @return 8 位图像
 */
inline cv::Mat normalizeTo8Bit(const cv::Mat& image)
{
    if (image.empty() || image.depth() == CV_8U) {
        return image;
    }

    cv::Mat converted;
    image.convertTo(converted, CV_MAKETYPE(CV_8U, image.channels()));
    return converted;
}

/**
 * @brief 将图像统一转换为 3 通道 BGR 图像
 * @param image 输入图像
 * @return 3 通道 BGR 图像
 */
inline cv::Mat ensureBgr(const cv::Mat& image)
{
    if (image.empty()) {
        return image;
    }

    if (image.channels() == 3) {
        return image;
    }

    cv::Mat converted;
    switch (image.channels()) {
    case 1:
        cv::cvtColor(image, converted, cv::COLOR_GRAY2BGR);
        break;
    case 4:
        cv::cvtColor(image, converted, cv::COLOR_BGRA2BGR);
        break;
    default:
        converted = image.clone();
        break;
    }
    return converted;
}

/**
 * @brief 将数值约束到非负范围
 * @param value 输入值
 * @return 非负整数
 */
inline int clampNonNegative(int value)
{
    return std::max(0, value);
}

/**
 * @brief 判断输入图像是否连接到共享图像环形缓存
 * @param imageData 输入图像数据
 * @return 连接共享缓存返回 true
 */
inline bool usesSharedImageBuffer(const std::shared_ptr<NodeDataTypes::ImageData>& imageData)
{
    return NodeDataTypes::usesSharedImageBuffer(imageData);
}

/**
 * @brief 判断多路输入中是否至少存在一路共享图像环形缓存
 * @param imageData 输入图像数据列表
 * @return 任一路连接共享缓存则返回 true
 */
template <typename... ImageArgs>
inline bool hasSharedImageBufferInput(const ImageArgs&... imageData)
{
    return (... || ::Nodes::ImageOperateHelpers::usesSharedImageBuffer(imageData));
}

/**
 * @brief 确保输出图像使用共享环形缓存，避免每次输出都创建新的 ImageData
 * @param outImageData 输出图像数据句柄（建议保持为稳定指针）
 * @param outBuffer 输出环形缓存句柄
 * @param maxSize 环形缓存容量
 */
inline void ensureSharedOutput(std::shared_ptr<NodeDataTypes::ImageData>& outImageData,
                               std::shared_ptr<NodeDataTypes::ImageTimestampRingQueue>& outBuffer,
                               int maxSize = 64)
{
    NodeDataTypes::ensureImageDataBuffer(outImageData, outBuffer, maxSize);
}

/**
 * @brief 将结果帧写入输出环形缓存，并基于时间戳做简单去重
 * @param outBuffer 输出环形缓存
 * @param image 输出图像
 * @param timestamp 输出时间戳
 * @param lastPushedTimestamp 上次写入的时间戳（用于去重）
 * @return 成功写入返回 true
 */
inline bool pushOutputFrame(const std::shared_ptr<NodeDataTypes::ImageTimestampRingQueue>& outBuffer,
                            cv::Mat&& image,
                            qint64 timestamp,
                            qint64& lastPushedTimestamp)
{
    if (!outBuffer || image.empty()) {
        return false;
    }
    if (lastPushedTimestamp == timestamp) {
        return true;
    }
    NodeDataTypes::ImageFrame frame;
    frame.image = std::move(image);
    frame.timestamp = timestamp;
    const bool pushed = outBuffer->pushFrame(frame);
    if (pushed) {
        lastPushedTimestamp = timestamp;
    }
    return pushed;
}

/**
 * @brief 按目标时间戳解析输入图像对应的当前帧，优先从共享环形缓存读取
 * @param imageData 输入图像数据
 * @param targetTimestamp 目标时间戳
 * @param frame 输出帧
 * @return 命中可用图像返回 true
 */
inline bool resolveImageFrameAtTimestamp(const std::shared_ptr<NodeDataTypes::ImageData>& imageData,
                                         qint64 targetTimestamp,
                                         NodeDataTypes::ImageFrame& frame)
{
    const qint64 normalizedTimestamp = normalizeTargetTimestamp(targetTimestamp);
    return NodeDataTypes::resolveImageFrameAtTimestamp(imageData, normalizedTimestamp, frame);
}

/**
 * @brief 判断输入是否已有可用图像（不依赖精确时间戳匹配）
 */
inline bool hasInputImage(const std::shared_ptr<NodeDataTypes::ImageData>& imageData)
{
    if (!imageData || !imageData->isConnectedToSharedBuffer()) {
        return false;
    }
    const auto buffer = imageData->getSharedImageBuffer();
    if (!buffer) {
        return false;
    }
    NodeDataTypes::ImageFrame frame;
    return buffer->getLatestFrame(frame) && !frame.image.empty();
}

/**
 * @brief 为合成/双输入节点解析帧：先按时间戳检索，失败则回退到最新帧
 *
 * 静态背景或更新较慢的输入往往只有旧时间戳；全局 resolveImageFrameAtTimestamp
 * 在 latest.timestamp > target 时会直接失败，导致整帧合成被清空。
 */
inline bool resolveInputFrameForOperate(const std::shared_ptr<NodeDataTypes::ImageData>& imageData,
                                        qint64 targetTimestamp,
                                        NodeDataTypes::ImageFrame& frame,
                                        qint64 searchBackRange = 64)
{
    frame = NodeDataTypes::ImageFrame();
    if (!imageData || !imageData->isConnectedToSharedBuffer()) {
        return false;
    }
    const auto buffer = imageData->getSharedImageBuffer();
    if (!buffer) {
        return false;
    }

    const qint64 normalizedTimestamp = normalizeTargetTimestamp(targetTimestamp);
    for (qint64 offset = 0; offset <= searchBackRange; ++offset) {
        const qint64 candidateTimestamp = normalizedTimestamp - offset;
        if (candidateTimestamp < 0) {
            break;
        }
        if (buffer->getFrameByTimestamp(candidateTimestamp, frame) && !frame.image.empty()) {
            return true;
        }
    }

    NodeDataTypes::ImageFrame latestFrame;
    if (buffer->getLatestFrame(latestFrame) && !latestFrame.image.empty()) {
        frame = latestFrame;
        return true;
    }
    return false;
}

/**
 * @brief 在同一目标时间戳下同步解析双路输入；两路均命中才返回 true
 */
inline bool resolveDualInputFramesForOperate(const std::shared_ptr<NodeDataTypes::ImageData>& inputA,
                                             const std::shared_ptr<NodeDataTypes::ImageData>& inputB,
                                             qint64 targetTimestamp,
                                             NodeDataTypes::ImageFrame& frameA,
                                             NodeDataTypes::ImageFrame& frameB,
                                             qint64 searchBackRange = 64)
{
    frameA = NodeDataTypes::ImageFrame();
    frameB = NodeDataTypes::ImageFrame();
    if (!hasInputImage(inputA) || !hasInputImage(inputB)) {
        return false;
    }
    const bool okA = resolveInputFrameForOperate(inputA, targetTimestamp, frameA, searchBackRange);
    const bool okB = resolveInputFrameForOperate(inputB, targetTimestamp, frameB, searchBackRange);
    return okA && okB && !frameA.image.empty() && !frameB.image.empty();
}

inline void clearDualInputOutput(ImageOperateWorkerQueue& worker,
                                 const std::shared_ptr<NodeDataTypes::ImageTimestampRingQueue>& outBuffer,
                                 qint64& lastPushedTimestamp,
                                 ImageOperateDualTickState& tickState)
{
    worker.cancelPending();
    if (outBuffer) {
        outBuffer->clear();
    }
    lastPushedTimestamp = -1;
    tickState.resetOutputState();
}

inline void pushWorkerResult(const std::shared_ptr<NodeDataTypes::ImageTimestampRingQueue>& outBuffer,
                             cv::Mat&& image,
                             qint64 outputTimestamp,
                             qint64& lastPushedTimestamp,
                             ImageOperateTickState& tickState,
                             qint64 inputTimestamp)
{
    if (!image.empty()) {
        pushOutputFrame(outBuffer, std::move(image), outputTimestamp, lastPushedTimestamp);
    }
    tickState.markProcessed(inputTimestamp);
}

inline void pushWorkerResultDual(const std::shared_ptr<NodeDataTypes::ImageTimestampRingQueue>& outBuffer,
                                 cv::Mat&& image,
                                 qint64 outputTimestamp,
                                 qint64& lastPushedTimestamp,
                                 ImageOperateDualTickState& tickState,
                                 qint64 inputTimestampA,
                                 qint64 inputTimestampB)
{
    if (!image.empty()) {
        pushOutputFrame(outBuffer, std::move(image), outputTimestamp, lastPushedTimestamp);
    }
    tickState.markProcessed(inputTimestampA, inputTimestampB);
}
} // namespace ImageOperateHelpers
} // namespace Nodes
