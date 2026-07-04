#include "ImageGpuPass.h"

#include "ImageGpuUpload.h"

#include <QHash>
#include <QOpenGLFunctions>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>

namespace NodeDataTypes
{

namespace
{

/**
 * @brief 内置 vertex shader：全屏四边形（TRIANGLE_STRIP）
 *
 * 顶点布局（interleaved float[4]）：position.xy + texCoord.xy
 * NDC 四顶点覆盖 [-1,1]²，texCoord 默认 (0,0) 左下 → (1,1) 右上
 */
const char kVertShader[] = R"(
attribute vec2 vertex;
attribute vec2 texCoord;
varying vec2 vTexCoord;
void main() {
    gl_Position = vec4(vertex, 0.0, 1.0);
    vTexCoord = texCoord;
}
)";

/**
 * @brief 内置 resample fragment shader
 *
 * uCropUV = (minU, minV, maxU, maxV)：将 vTexCoord [0,1]² 映射到源纹理子区域
 * uFlipH/V：在 UV 映射前对 vTexCoord 做镜像（用于修正坐标系差异）
 */
const char kFragResample[] = R"(
uniform sampler2D uTexture;
uniform vec4 uCropUV;
uniform bool uFlipH;
uniform bool uFlipV;
varying vec2 vTexCoord;
void main() {
    vec2 uv = vTexCoord;
    if (uFlipH) uv.x = 1.0 - uv.x;
    if (uFlipV) uv.y = 1.0 - uv.y;
    uv = mix(uCropUV.xy, uCropUV.zw, uv);
    gl_FragColor = texture2D(uTexture, uv);
}
)";

/** @brief 按 fragment 源码字符串缓存已链接的 QOpenGLShaderProgram，避免重复编译 */
QOpenGLShaderProgram& cachedProgram(const char* fragmentSource)
{
    static QHash<QString, QOpenGLShaderProgram*> cache;
    const QString key = QString::fromUtf8(fragmentSource);
    if (!cache.contains(key)) {
        auto* program = new QOpenGLShaderProgram();
        program->addShaderFromSourceCode(QOpenGLShader::Vertex, kVertShader);
        program->addShaderFromSourceCode(QOpenGLShader::Fragment, fragmentSource);
        program->link();
        cache.insert(key, program);
    }
    return *cache[key];
}

/** @brief 分配空 RGBA8 2D 纹理（未填充像素，由后续 draw 写入） */
GpuTextureHandle allocateRgbaTexture(QOpenGLFunctions* f, int width, int height)
{
    GpuTextureHandle handle;
    if (!f || width <= 0 || height <= 0) {
        return handle;
    }

    unsigned int texId = 0;
    f->glGenTextures(1, &texId);
    f->glBindTexture(GL_TEXTURE_2D, texId);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    // GL_BGRA 外部格式匹配 Windows/Qt 常用字节序
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
    f->glBindTexture(GL_TEXTURE_2D, 0);

    handle.textureId = texId;
    handle.width = width;
    handle.height = height;
    handle.lifetime = makeTextureLifetime(texId).lifetime;
    return handle;
}

/**
 * @brief 将全屏四边形绘制到 target 纹理（通过临时 FBO 附着）
 *
 * @return draw 成功返回 true；FBO / program 异常时返回 false，调用方应丢弃输出纹理
 */
bool drawToTarget(QOpenGLFunctions* f,
                  const GpuTextureHandle& target,
                  QOpenGLShaderProgram& program,
                  const ImageGpuPass::UniformBinder& bindUniforms,
                  const ImageGpuPass::TextureBinder& bindTextures)
{
    if (!f || !target.valid() || !program.isLinked()) {
        return false;
    }

    unsigned int fbo = 0;
    f->glGenFramebuffers(1, &fbo);
    f->glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    f->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, target.textureId, 0);

    f->glViewport(0, 0, target.width, target.height);
    f->glDisable(GL_DEPTH_TEST);
    f->glDisable(GL_BLEND);
    f->glClearColor(0.f, 0.f, 0.f, 0.f);
    f->glClear(GL_COLOR_BUFFER_BIT);

    bindTextures(f);
    if (!program.bind()) {
        f->glBindFramebuffer(GL_FRAMEBUFFER, 0);
        f->glDeleteFramebuffers(1, &fbo);
        return false;
    }

    bindUniforms(program);

    // interleaved: xy position + uv
    static const float vertices[] = {
        -1.f, -1.f, 0.f, 0.f,
         1.f, -1.f, 1.f, 0.f,
        -1.f,  1.f, 0.f, 1.f,
         1.f,  1.f, 1.f, 1.f,
    };
    program.enableAttributeArray("vertex");
    program.enableAttributeArray("texCoord");
    program.setAttributeArray("vertex", GL_FLOAT, vertices, 2, sizeof(float) * 4);
    program.setAttributeArray("texCoord", GL_FLOAT, vertices + 2, 2, sizeof(float) * 4);
    f->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    program.disableAttributeArray("vertex");
    program.disableAttributeArray("texCoord");
    program.release();

    f->glBindTexture(GL_TEXTURE_2D, 0);
    f->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    f->glDeleteFramebuffers(1, &fbo);
    return true;
}

/** @brief runFragmentPass 的 GL 线程内部实现：分配输出纹理 + draw */
GpuTextureHandle runPassInternal(QOpenGLFunctions* f,
                                 int outW,
                                 int outH,
                                 const char* fragmentSource,
                                 const ImageGpuPass::UniformBinder& bindUniforms,
                                 const ImageGpuPass::TextureBinder& bindTextures)
{
    GpuTextureHandle out = allocateRgbaTexture(f, outW, outH);
    if (!out.valid()) {
        return {};
    }
    auto& program = cachedProgram(fragmentSource);
    if (!drawToTarget(f, out, program, bindUniforms, bindTextures)) {
        return {};
    }
    return out;
}

} // namespace

ImageGpuPass& ImageGpuPass::instance()
{
    static ImageGpuPass service;
    return service;
}

void ImageGpuPass::bindTexture(QOpenGLFunctions* f, int unit, unsigned int textureId)
{
    if (!f) {
        return;
    }
    f->glActiveTexture(GL_TEXTURE0 + unit);
    f->glBindTexture(GL_TEXTURE_2D, textureId);
}

GpuTextureHandle ImageGpuPass::runFragmentPass(int outWidth,
                                              int outHeight,
                                              const char* fragmentShaderSource,
                                              const UniformBinder& bindUniforms,
                                              const TextureBinder& bindTextures)
{
    if (outWidth <= 0 || outHeight <= 0 || !fragmentShaderSource) {
        return {};
    }
    // 所有 GL 操作必须在 ImageGpuUpload 管理的共享上下文中执行
    return ImageGpuUpload::instance().runGl([&](QOpenGLFunctions* f) {
        return runPassInternal(f, outWidth, outHeight, fragmentShaderSource, bindUniforms, bindTextures);
    });
}

GpuTextureHandle ImageGpuPass::resample(const GpuTextureHandle& src,
                                        int outWidth,
                                        int outHeight,
                                        float uvMinU,
                                        float uvMinV,
                                        float uvMaxU,
                                        float uvMaxV,
                                        bool flipHorizontal,
                                        bool flipVertical)
{
    if (!src.valid() || outWidth <= 0 || outHeight <= 0) {
        return {};
    }
    return runFragmentPass(
        outWidth,
        outHeight,
        kFragResample,
        [=](QOpenGLShaderProgram& program) {
            program.setUniformValue("uTexture", 0);
            program.setUniformValue("uCropUV", uvMinU, uvMinV, uvMaxU, uvMaxV);
            program.setUniformValue("uFlipH", flipHorizontal);
            program.setUniformValue("uFlipV", flipVertical);
        },
        [&](QOpenGLFunctions* f) { bindTexture(f, 0, src.textureId); });
}

GpuTextureHandle ImageGpuPass::resize(const GpuTextureHandle& src, int outWidth, int outHeight)
{
    return resample(src, outWidth, outHeight);
}

} // namespace NodeDataTypes
