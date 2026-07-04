#pragma once

/**
 * @file ImageOperateCommon.hpp
 * @brief ImageOperates 插件内部公共辅助（取帧、GPU 输出、CPU 回退路径）
 *
 * ## 数据流
 * - 节点间通过 ImageData 共享句柄 + ImageTimestampRingQueue 交换 ImageFrame
 * - GPU 算子主路径只读写 GpuTextureHandle；CPU Mat 辅助函数供 legacy 或读回场景
 *
 * ## 与节点生命周期的关系（详见 Doc.md §3）
 * - resolveInputGpuFrame / resolveDualInputGpuFrames：tick 内取输入纹理（含静态源回退）
 * - pushGpuResult：tick 内写输出，时间戳用 currentSystemTimestamp()
 * - ensureSharedOutput：保证 m_outImageData / m_outBuffer 单例，避免每帧新建 ImageData
 *
 * 其他模块请使用 ImageData.h 公开 API，勿直接 include 本文件。
 */

// 仅供 ImageOperates 插件内部使用的公共辅助头文件，其他模块请使用 ImageData.h 中的 API。

#include "NodeDataList.hpp"
#include "ImageReadback.h"
#include "ImageGpuPass.h"
#include "TimestampGenerator/TimestampGenerator.hpp"

#include <opencv2/imgproc.hpp>

#include <algorithm>
#include <memory>

namespace Nodes
{
namespace ImageOperateHelpers
{
/**
 * @brief 将未指定的目标时间戳规整为当前全局时间戳
 * @param targetTimestamp 目标时间戳，传负数时自动使用当前全局时间戳
 * @return 可用于环形缓存检索的时间戳
 */
inline qint64 normalizeTargetTimestamp(qint64 targetTimestamp)
{
    return targetTimestamp >= 0 ? targetTimestamp : TimestampGenerator::getInstance()->getCurrentFrameCount();
}

/** @brief 当前系统帧号，用于算子输出帧打戳（与输入帧时间戳解耦） */
inline qint64 currentSystemTimestamp()
{
    return TimestampGenerator::getInstance()->getCurrentFrameCount();
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
                               int maxSize = 8)
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
    // 上传 GPU 纹理并保留 BGR CPU 副本，供下游 OpenCV 节点避免重复读回
    NodeDataTypes::ImageFrame frame = NodeDataTypes::ImageFrame::fromMat(image, timestamp);
    const bool pushed = outBuffer->pushFrame(std::move(frame));
    if (pushed) {
        lastPushedTimestamp = timestamp;
    }
    return pushed;
}

/**
 * @brief 按目标时间戳解析输入帧（先 normalize，再委托 ImageData API）
 */
inline bool resolveImageFrameAtTargetTimestamp(const std::shared_ptr<NodeDataTypes::ImageData>& imageData,
                                               qint64 targetTimestamp,
                                               NodeDataTypes::ImageFrame& frame)
{
    const qint64 normalizedTimestamp = normalizeTargetTimestamp(targetTimestamp);
    return NodeDataTypes::resolveImageFrameAtTimestamp(imageData, normalizedTimestamp, frame);
}

/**
 * @brief 从 ImageFrame 解析 BGR Mat，供 OpenCV 算子使用
 *
 * 1. matFromFrame 取 CPU 缓存或 GPU 读回
 * 2. ensureBgr 按需做 BGRA/灰度 → BGR（缓存阶段不再提前转换）
 *
 * OpenCV 算子节点应使用本函数，而非直接读 frame.image。
 */
inline cv::Mat resolveBgrMatFromFrame(const NodeDataTypes::ImageFrame& frame,
                                      QOpenGLFunctions* f = nullptr)
{
    return ensureBgr(NodeDataTypes::ImageReadback::matFromFrame(frame, f));
}

/**
 * @brief 解析输入帧并取 BGR Mat（优先 CPU 缓存，否则经 ImageGpuUpload 读回纹理）
 */
inline bool resolveInputBgrMat(const std::shared_ptr<NodeDataTypes::ImageData>& imageData,
                               qint64 targetTimestamp,
                               NodeDataTypes::ImageFrame& frame,
                               cv::Mat& outMat)
{
    if (!resolveImageFrameAtTargetTimestamp(imageData, targetTimestamp, frame) || frame.empty()) {
        return false;
    }
    outMat = resolveBgrMatFromFrame(frame);
    return !outMat.empty();
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
    return buffer->getLatestFrame(frame) && !frame.empty();
}

/**
 * @brief 为合成/双输入节点解析帧：先按时间戳检索，失败则回退到最新帧
 *
 * 静态背景或更新较慢的输入往往只有旧时间戳；ImageData::resolveImageFrameAtTimestamp
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
        if (buffer->getFrameByTimestamp(candidateTimestamp, frame) && !frame.empty()) {
            return true;
        }
    }

    NodeDataTypes::ImageFrame latestFrame;
    if (buffer->getLatestFrame(latestFrame) && !latestFrame.empty()) {
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
    return okA && okB && !frameA.empty() && !frameB.empty();
}

/**
 * @brief 双路输入：解析帧并取 BGR Mat
 */
inline bool resolveDualInputBgrMatsForOperate(const std::shared_ptr<NodeDataTypes::ImageData>& inputA,
                                              const std::shared_ptr<NodeDataTypes::ImageData>& inputB,
                                              qint64 targetTimestamp,
                                              NodeDataTypes::ImageFrame& frameA,
                                              NodeDataTypes::ImageFrame& frameB,
                                              cv::Mat& matA,
                                              cv::Mat& matB,
                                              qint64 searchBackRange = 64)
{
    if (!resolveDualInputFramesForOperate(inputA, inputB, targetTimestamp, frameA, frameB, searchBackRange)) {
        return false;
    }
    matA = resolveBgrMatFromFrame(frameA);
    matB = resolveBgrMatFromFrame(frameB);
    return !matA.empty() && !matB.empty();
}

// ---------------------------------------------------------------------------
// GPU pass 辅助（具体 fragment shader 定义在各节点的 {Name}Gpu 命名空间内）
// ---------------------------------------------------------------------------

/** @brief 将纹理缩放到目标尺寸（尺寸已匹配则零拷贝返回原句柄） */
inline NodeDataTypes::GpuTextureHandle matchTextureSize(const NodeDataTypes::GpuTextureHandle& src,
                                                        int outWidth,
                                                        int outHeight)
{
    if (!src.valid()) {
        return {};
    }
    if (src.width == outWidth && src.height == outHeight) {
        return src;
    }
    return NodeDataTypes::ImageGpuPass::instance().resize(src, outWidth, outHeight);
}

/** @brief 确保 ImageFrame 含有效 GPU 纹理（必要时从 CPU 缓存上传） */
inline bool ensureFrameGpuTexture(NodeDataTypes::ImageFrame& frame)
{
    if (frame.texture.valid()) {
        return true;
    }
    return frame.ensureGpuTexture();
}

/** @brief 解析输入帧并保证 texture 可用（支持静态源：精确帧号未命中时回退最新帧） */
inline bool resolveInputGpuFrame(const std::shared_ptr<NodeDataTypes::ImageData>& imageData,
                                 qint64 targetTimestamp,
                                 NodeDataTypes::ImageFrame& frame,
                                 qint64 searchBackRange = 64)
{
    if (!resolveInputFrameForOperate(imageData, targetTimestamp, frame, searchBackRange) || frame.empty()) {
        return false;
    }
    return ensureFrameGpuTexture(frame);
}

/** @brief 双路输入解析并保证 texture 可用 */
inline bool resolveDualInputGpuFrames(const std::shared_ptr<NodeDataTypes::ImageData>& inputA,
                                      const std::shared_ptr<NodeDataTypes::ImageData>& inputB,
                                      qint64 targetTimestamp,
                                      NodeDataTypes::ImageFrame& frameA,
                                      NodeDataTypes::ImageFrame& frameB,
                                      qint64 searchBackRange = 64)
{
    if (!resolveDualInputFramesForOperate(inputA, inputB, targetTimestamp, frameA, frameB, searchBackRange)) {
        return false;
    }
    return ensureFrameGpuTexture(frameA) && ensureFrameGpuTexture(frameB);
}

/** @brief 将 GPU 纹理写入输出 ring buffer；时间戳为当前系统帧号，不写 CPU 副本 */
inline void pushGpuResult(const std::shared_ptr<NodeDataTypes::ImageTimestampRingQueue>& outBuffer,
                          NodeDataTypes::GpuTextureHandle&& texture,
                          qint64& lastPushedTimestamp)
{
    if (!outBuffer || !texture.valid()) {
        return;
    }
    const qint64 outputTimestamp = currentSystemTimestamp();
    NodeDataTypes::ImageFrame frame = NodeDataTypes::ImageFrame::fromTexture(std::move(texture), outputTimestamp);
    if (outBuffer->pushFrame(std::move(frame))) {
        lastPushedTimestamp = outputTimestamp;
    }
}

inline void pushGpuResultDual(const std::shared_ptr<NodeDataTypes::ImageTimestampRingQueue>& outBuffer,
                              NodeDataTypes::GpuTextureHandle&& texture,
                              qint64& lastPushedTimestamp)
{
    pushGpuResult(outBuffer, std::move(texture), lastPushedTimestamp);
}
} // namespace ImageOperateHelpers
} // namespace Nodes
