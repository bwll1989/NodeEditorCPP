//
// Created by pablo on 2/28/24.
//

#include "ImageData.h"

using namespace NodeDataTypes;

ImageData::ImageData()
{
    qRegisterMetaType<ImageData>("ImageData");
}

QtNodes::NodeDataType ImageData::type() const
{
    return QtNodes::NodeDataType{"image", "image"};
}

void ImageData::setSharedImageBuffer(std::shared_ptr<ImageTimestampRingQueue> buffer)
{
    sharedImageBuffer_ = std::move(buffer);
}

std::shared_ptr<ImageTimestampRingQueue> ImageData::getSharedImageBuffer() const
{
    return sharedImageBuffer_;
}

bool ImageData::isConnectedToSharedBuffer() const
{
    return sharedImageBuffer_ != nullptr;
}

void ImageData::disconnect()
{
    sharedImageBuffer_.reset();
}

bool ImageData::isEmpty() const
{
    if (!isConnectedToSharedBuffer()) {
        return true;
    }
    ImageFrame frame;
    return !sharedImageBuffer_->getLatestFrame(frame) || frame.empty();
}

void NodeDataTypes::ensureImageDataBuffer(std::shared_ptr<ImageData>& outImageData,
                                          std::shared_ptr<ImageTimestampRingQueue>& outBuffer,
                                          int maxSize)
{
    // buffer 与 ImageData 分开持有：节点可同时访问 buffer（push）与 ImageData（outData 返回）
    if (!outBuffer) {
        outBuffer = std::make_shared<ImageTimestampRingQueue>(maxSize);
    }
    if (!outImageData) {
        outImageData = std::make_shared<ImageData>();
    }
    outImageData->setSharedImageBuffer(outBuffer);
}

bool NodeDataTypes::usesSharedImageBuffer(const std::shared_ptr<ImageData>& imageData)
{
    return imageData && imageData->isConnectedToSharedBuffer();
}

bool NodeDataTypes::imageDataIsEmpty(const std::shared_ptr<ImageData>& imageData)
{
    if (!imageData) {
        return true;
    }
    return imageData->isEmpty();
}

bool NodeDataTypes::getLatestImageFrame(const std::shared_ptr<ImageData>& imageData, ImageFrame& frame)
{
    frame = ImageFrame();
    if (!imageData || !imageData->isConnectedToSharedBuffer()) {
        return false;
    }
    const auto buffer = imageData->getSharedImageBuffer();
    if (!buffer) {
        return false;
    }
    return buffer->getLatestFrame(frame) && !frame.empty();
}

bool NodeDataTypes::resolveImageFrameAtTimestamp(const std::shared_ptr<ImageData>& imageData,
                                                 qint64 targetTimestamp,
                                                 ImageFrame& frame)
{
    frame = ImageFrame();
    if (!imageData || !imageData->isConnectedToSharedBuffer()) {
        return false;
    }

    const auto buffer = imageData->getSharedImageBuffer();
    if (!buffer) {
        return false;
    }

    return buffer->getFrameByTimestamp(targetTimestamp, frame) && !frame.empty();
}

bool NodeDataTypes::pushFrameToImageBufferDedup(const std::shared_ptr<ImageTimestampRingQueue>& buffer,
                                                ImageFrame&& frame,
                                                qint64& lastPushedTimestamp)
{
    if (!buffer || frame.empty() || frame.timestamp < 0) {
        return false;
    }
    // 同一 tick 已 push 过则跳过（返回 true 表示"无需再处理"）
    if (lastPushedTimestamp == frame.timestamp) {
        return true;
    }
    const bool pushed = buffer->pushFrame(frame);
    if (pushed) {
        lastPushedTimestamp = frame.timestamp;
    }
    return pushed;
}

bool NodeDataTypes::pushFrameToImageBufferDedup(const std::shared_ptr<ImageTimestampRingQueue>& buffer,
                                                cv::Mat&& image,
                                                qint64 timestamp,
                                                qint64& lastPushedTimestamp)
{
    return pushFrameToImageBufferDedup(
        buffer, ImageFrame::fromMat(std::move(image), timestamp), lastPushedTimestamp);
}
