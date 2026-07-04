#pragma once

#include "ImageGpuPass.h"

#include <QRect>

namespace ROINode
{

inline NodeDataTypes::GpuTextureHandle cropTexture(const NodeDataTypes::GpuTextureHandle& src,
                                                   const QRect& roiPx)
{
    if (!src.valid() || roiPx.width() <= 0 || roiPx.height() <= 0) {
        return {};
    }

    const int fw = src.width;
    const int fh = src.height;
    if (fw <= 0 || fh <= 0) {
        return {};
    }

    const float u0 = static_cast<float>(roiPx.x()) / static_cast<float>(fw);
    const float v0 = static_cast<float>(roiPx.y()) / static_cast<float>(fh);
    const float u1 = static_cast<float>(roiPx.x() + roiPx.width()) / static_cast<float>(fw);
    const float v1 = static_cast<float>(roiPx.y() + roiPx.height()) / static_cast<float>(fh);

    return NodeDataTypes::ImageGpuPass::instance().resample(
        src, roiPx.width(), roiPx.height(), u0, v0, u1, v1);
}

} // namespace ROINode
