#pragma once

#include "ImageGpuPass.h"

#include <QColor>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>

namespace Nodes
{
namespace ImageConstGpu
{
/** 纯色填充 — 单 pass fragment shader，适合 Image Constant 节点 */
inline NodeDataTypes::GpuTextureHandle solidColor(int width, int height, const QColor& color)
{
    if (width <= 0 || height <= 0) {
        return {};
    }

    static const char kFragSolidColor[] = R"(
uniform vec4 uColor;
varying vec2 vTexCoord;
void main() {
    gl_FragColor = uColor;
}
)";

    return NodeDataTypes::ImageGpuPass::instance().runFragmentPass(
        width,
        height,
        kFragSolidColor,
        [&](QOpenGLShaderProgram& program) {
            program.setUniformValue(
                "uColor", color.redF(), color.greenF(), color.blueF(), color.alphaF());
        },
        [](QOpenGLFunctions*) {});
}
} // namespace ImageConstGpu
} // namespace Nodes
