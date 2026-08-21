#pragma once

#include "NodeDataList.hpp"
#include "GpuTextureHandle.h"
#include "ImageGpuUpload.h"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include "Elements/ColorEditorWidget/ColorEditorWidget.hpp"

#include <memory>
#include <QJsonArray>
#include <QJsonObject>
#include <QSignalBlocker>

namespace Nodes
{
namespace ImageConstHelpers
{
inline void applyRgbaToEditor(ColorEditorWidget *editor, const QVector<float> &rgba)
{
    if (!editor) {
        return;
    }
    const float r = rgba.size() > 0 ? rgba[0] : 0.0f;
    const float g = rgba.size() > 1 ? rgba[1] : 0.0f;
    const float b = rgba.size() > 2 ? rgba[2] : 0.0f;
    const float a = rgba.size() > 3 ? rgba[3] : 1.0f;
    const QVector<float> hsv = NodeDataTypes::hsvFromRgb(r, g, b);
    const QSignalBlocker blocker(editor);
    editor->setHsvF(hsv[0], hsv[1], hsv[2], a);
}

inline QVector<float> loadRgbaValue(const QJsonObject &values,
                                    const QString &arrayKey,
                                    const QString &hexKey,
                                    const QVector<float> &fallback)
{
    const QJsonValue arr = values.value(arrayKey);
    if (arr.isArray()) {
        return NodeDataTypes::rgbaVectorFromVariant(arr.toArray().toVariantList());
    }
    const QJsonValue hex = values.value(hexKey);
    if (hex.isString()) {
        const QColor c(hex.toString());
        if (c.isValid()) {
            return NodeDataTypes::rgbaFromQColor(c);
        }
    }
    return fallback;
}

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
