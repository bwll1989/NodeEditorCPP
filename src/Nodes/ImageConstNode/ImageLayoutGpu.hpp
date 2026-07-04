#pragma once

#include "ImageConstGpu.hpp"
#include "ImageGpuPass.h"
#include "ImageGpuUpload.h"

#include <QColor>
#include <QMatrix4x4>
#include <QOpenGLFunctions>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>

#include <vector>

namespace Nodes
{
namespace ImageLayoutGpu
{
struct LayoutCell
{
    NodeDataTypes::GpuTextureHandle texture;
    int x = 0;
    int y = 0;
    int w = 0;
    int h = 0;
};

/** 将多路纹理按网格位置合成到画布（背景 shader + resize + GPU blit，避免嵌套 runGl） */
inline NodeDataTypes::GpuTextureHandle compose(int canvasW,
                                               int canvasH,
                                               const QColor& background,
                                               const std::vector<LayoutCell>& cells)
{
    if (canvasW <= 0 || canvasH <= 0) {
        return {};
    }

    NodeDataTypes::GpuTextureHandle canvas = ImageConstGpu::solidColor(canvasW, canvasH, background);
    if (!canvas.valid()) {
        return {};
    }
    if (cells.empty()) {
        return canvas;
    }

    std::vector<LayoutCell> prepared;
    prepared.reserve(cells.size());
    for (const LayoutCell& cell : cells) {
        if (!cell.texture.valid() || cell.w <= 0 || cell.h <= 0) {
            continue;
        }
        LayoutCell item = cell;
        if (cell.texture.width != cell.w || cell.texture.height != cell.h) {
            item.texture = NodeDataTypes::ImageGpuPass::instance().resize(cell.texture, cell.w, cell.h);
        }
        if (item.texture.valid()) {
            prepared.push_back(item);
        }
    }
    if (prepared.empty()) {
        return canvas;
    }

    NodeDataTypes::GpuTextureHandle result;
    NodeDataTypes::ImageGpuUpload::instance().runGl([&](QOpenGLFunctions* f) {
        if (!f) {
            return;
        }

        unsigned int fbo = 0;
        f->glGenFramebuffers(1, &fbo);
        f->glBindFramebuffer(GL_FRAMEBUFFER, fbo);
        f->glFramebufferTexture2D(
            GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, canvas.textureId, 0);

        f->glViewport(0, 0, canvasW, canvasH);
        f->glEnable(GL_BLEND);
        f->glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        static QOpenGLShaderProgram* program = nullptr;
        if (!program) {
            program = new QOpenGLShaderProgram();
            program->addShaderFromSourceCode(QOpenGLShader::Vertex, R"(
                attribute vec2 vertex;
                attribute vec2 texCoord;
                uniform mat4 projection;
                uniform mat4 modelView;
                varying vec2 vTexCoord;
                void main() {
                    gl_Position = projection * modelView * vec4(vertex, 0.0, 1.0);
                    vTexCoord = texCoord;
                }
            )");
            program->addShaderFromSourceCode(QOpenGLShader::Fragment, R"(
                uniform sampler2D uTexture;
                varying vec2 vTexCoord;
                void main() {
                    gl_FragColor = texture2D(uTexture, vTexCoord);
                }
            )");
            program->link();
        }

        QMatrix4x4 projection;
        projection.ortho(0, canvasW, canvasH, 0, -1, 1);

        static const GLfloat vertices[] = {
            0.f, 0.f, 0.f, 1.f,
            1.f, 0.f, 1.f, 1.f,
            0.f, 1.f, 0.f, 0.f,
            1.f, 1.f, 1.f, 0.f,
        };

        for (const LayoutCell& cell : prepared) {
            if (!program->bind()) {
                continue;
            }

            program->setUniformValue("projection", projection);
            QMatrix4x4 modelView;
            modelView.translate(static_cast<float>(cell.x), static_cast<float>(cell.y));
            modelView.scale(static_cast<float>(cell.w), static_cast<float>(cell.h));
            program->setUniformValue("modelView", modelView);
            NodeDataTypes::ImageGpuPass::bindTexture(f, 0, cell.texture.textureId);
            program->setUniformValue("uTexture", 0);

            program->enableAttributeArray("vertex");
            program->enableAttributeArray("texCoord");
            program->setAttributeArray("vertex", GL_FLOAT, vertices, 2, sizeof(GLfloat) * 4);
            program->setAttributeArray("texCoord", GL_FLOAT, vertices + 2, 2, sizeof(GLfloat) * 4);
            f->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
            program->disableAttributeArray("vertex");
            program->disableAttributeArray("texCoord");
            program->release();
            f->glBindTexture(GL_TEXTURE_2D, 0);
        }

        f->glBindFramebuffer(GL_FRAMEBUFFER, 0);
        f->glDeleteFramebuffers(1, &fbo);
        result = std::move(canvas);
    });

    return result.valid() ? result : NodeDataTypes::GpuTextureHandle{};
}
} // namespace ImageLayoutGpu
} // namespace Nodes
