#pragma once

#include "ImageData.h"
#include "ImageReadback.h"
#include "TimestampGenerator/TimestampGenerator.hpp"

#include <QSize>

namespace ROINode
{

inline int frameWidth(const NodeDataTypes::ImageFrame& frame)
{
    if (frame.texture.valid()) {
        return frame.texture.width;
    }
    return frame.image.cols;
}

inline int frameHeight(const NodeDataTypes::ImageFrame& frame)
{
    if (frame.texture.valid()) {
        return frame.texture.height;
    }
    return frame.image.rows;
}

inline bool resolveInputFrame(const std::shared_ptr<NodeDataTypes::ImageData>& input,
                              qint64 timestamp,
                              NodeDataTypes::ImageFrame& frame)
{
    if (!input || !input->isConnectedToSharedBuffer()) {
        return false;
    }

    auto buffer = input->getSharedImageBuffer();
    if (!buffer) {
        return false;
    }

    const qint64 normalizedTimestamp = timestamp >= 0
        ? timestamp
        : TimestampGenerator::getInstance()->getCurrentFrameCount();

    for (qint64 offset = 0; offset <= 64; ++offset) {
        if (buffer->getFrameByTimestamp(normalizedTimestamp - offset, frame) && !frame.empty()) {
            return true;
        }
    }

    return buffer->getLatestFrame(frame) && !frame.empty();
}

inline void pushTextureToOutput(const std::shared_ptr<NodeDataTypes::ImageTimestampRingQueue>& buffer,
                                  qint64& lastPushedTimestamp,
                                  NodeDataTypes::GpuTextureHandle texture,
                                  qint64 timestamp)
{
    if (!buffer || !texture.valid()) {
        return;
    }

    NodeDataTypes::ImageFrame frame = NodeDataTypes::ImageFrame::fromTexture(std::move(texture), timestamp);
    NodeDataTypes::pushFrameToImageBufferDedup(buffer, std::move(frame), lastPushedTimestamp);
}

} // namespace ROINode
