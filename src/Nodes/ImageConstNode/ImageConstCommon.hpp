#pragma once

#include "NodeDataList.hpp"
#include "GpuTextureHandle.h"
#include "ImageGpuUpload.h"
#include "TimestampGenerator/TimestampGenerator.hpp"

#include <memory>

namespace Nodes
{
namespace ImageConstHelpers
{
inline qint64 currentTimestamp()
{
    return TimestampGenerator::getInstance()->getCurrentFrameCount();
}

inline void ensureOutputBuffer(std::shared_ptr<NodeDataTypes::ImageData>& outData,
                               std::shared_ptr<NodeDataTypes::ImageTimestampRingQueue>& buffer,
                               int maxSize = 8)
{
    NodeDataTypes::ensureImageDataBuffer(outData, buffer, maxSize);
}

inline void pushMatFrame(const std::shared_ptr<NodeDataTypes::ImageTimestampRingQueue>& buffer,
                         cv::Mat&& mat,
                         qint64& lastPushedTimestamp)
{
    if (!buffer || mat.empty()) {
        return;
    }
    const qint64 ts = currentTimestamp();
    NodeDataTypes::pushFrameToImageBufferDedup(
        buffer, NodeDataTypes::ImageFrame::fromMat(std::move(mat), ts), lastPushedTimestamp);
}

inline void pushBgraFrame(const std::shared_ptr<NodeDataTypes::ImageTimestampRingQueue>& buffer,
                          cv::Mat&& mat,
                          qint64& lastPushedTimestamp)
{
    if (!buffer || mat.empty()) {
        return;
    }
    const qint64 ts = currentTimestamp();
    NodeDataTypes::pushFrameToImageBufferDedup(
        buffer, NodeDataTypes::ImageFrame::fromBgra8Mat(std::move(mat), ts), lastPushedTimestamp);
}

inline void pushTextureFrame(const std::shared_ptr<NodeDataTypes::ImageTimestampRingQueue>& buffer,
                             NodeDataTypes::GpuTextureHandle&& texture,
                             qint64& lastPushedTimestamp)
{
    if (!buffer || !texture.valid()) {
        return;
    }
    const qint64 ts = currentTimestamp();
    NodeDataTypes::pushFrameToImageBufferDedup(
        buffer, NodeDataTypes::ImageFrame::fromTexture(std::move(texture), ts), lastPushedTimestamp);
}

inline bool resolveLatestGpuFrame(const std::shared_ptr<NodeDataTypes::ImageData>& imageData,
                                  NodeDataTypes::ImageFrame& frame)
{
    if (!imageData || !NodeDataTypes::getLatestImageFrame(imageData, frame) || frame.empty()) {
        return false;
    }
    return frame.texture.valid() || frame.ensureGpuTexture();
}
} // namespace ImageConstHelpers
} // namespace Nodes
