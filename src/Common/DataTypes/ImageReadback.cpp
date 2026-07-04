#include "ImageReadback.h"

#include "ImageGpuUpload.h"

#include <QOpenGLFramebufferObject>
#include <QOpenGLFunctions>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>

#include <cstdio>
#include <io.h>
#include <vector>

#include <opencv2/imgcodecs.hpp>
#include <opencv2/imgproc.hpp>

namespace NodeDataTypes
{

namespace
{

/**
 * @brief 将任意通道/深度的 Mat 统一为连续 BGRA8，供 glTexImage2D(GL_BGRA) 使用
 *
 * 支持 1/3/4 通道；非常规通道数返回空 Mat。
 * 非连续或 stride 不等于 cols*4 时会 clone，保证 glTexImage2D 可安全读取。
 */
cv::Mat toBgra8(const cv::Mat& mat)
{
    if (mat.empty()) {
        return {};
    }

    cv::Mat normalized = mat;
    if (mat.depth() != CV_8U) {
        mat.convertTo(normalized, CV_8U);
    }

    cv::Mat bgra;
    switch (normalized.channels()) {
    case 1:
        cv::cvtColor(normalized, bgra, cv::COLOR_GRAY2BGRA);
        break;
    case 3:
        cv::cvtColor(normalized, bgra, cv::COLOR_BGR2BGRA);
        break;
    case 4:
        bgra = normalized;
        break;
    default:
        return {};
    }

    if (!bgra.isContinuous() || bgra.step != static_cast<size_t>(bgra.cols * 4)) {
        bgra = bgra.clone();
    }
    return bgra;
}

/**
 * @brief 用全屏四边形 shader 将源纹理 blit 到 FBO
 *
 * 读回路径无法直接 glReadPixels 绑定纹理，需先 blit 到 FBO 再 toImage()/readPixels。
 * 静态 program 只链接一次，后续复用。
 */
bool blitTextureToFbo(QOpenGLFunctions* f, const GpuTextureHandle& handle, QOpenGLFramebufferObject& targetFbo)
{
    static QOpenGLShaderProgram program;
    if (!program.isLinked()) {
        program.addShaderFromSourceCode(QOpenGLShader::Vertex, R"(
            attribute vec2 vertex;
            attribute vec2 texCoord;
            varying vec2 vTexCoord;
            void main() {
                gl_Position = vec4(vertex, 0.0, 1.0);
                vTexCoord = texCoord;
            }
        )");
        program.addShaderFromSourceCode(QOpenGLShader::Fragment, R"(
            uniform sampler2D uTexture;
            varying vec2 vTexCoord;
            void main() {
                gl_FragColor = texture2D(uTexture, vTexCoord);
            }
        )");
        if (!program.link()) {
            return false;
        }
    }

    targetFbo.bind();
    f->glViewport(0, 0, handle.width, handle.height);
    f->glDisable(GL_DEPTH_TEST);
    f->glDisable(GL_BLEND);
    f->glClearColor(0.f, 0.f, 0.f, 1.f);
    f->glClear(GL_COLOR_BUFFER_BIT);

    static const float vertices[] = {
        -1.f, -1.f, 0.f, 0.f,
         1.f, -1.f, 1.f, 0.f,
        -1.f,  1.f, 0.f, 1.f,
         1.f,  1.f, 1.f, 1.f,
    };

    f->glActiveTexture(GL_TEXTURE0);
    f->glBindTexture(GL_TEXTURE_2D, handle.textureId);

    program.bind();
    program.setUniformValue("uTexture", 0);
    program.enableAttributeArray("vertex");
    program.enableAttributeArray("texCoord");
    program.setAttributeArray("vertex", GL_FLOAT, vertices, 2, sizeof(float) * 4);
    program.setAttributeArray("texCoord", GL_FLOAT, vertices + 2, 2, sizeof(float) * 4);
    f->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    program.disableAttributeArray("vertex");
    program.disableAttributeArray("texCoord");
    program.release();
    f->glBindTexture(GL_TEXTURE_2D, 0);
    targetFbo.release();
    return true;
}

/** @brief 底层上传：假定 data 已是 BGRA8 连续 buffer */
GpuTextureHandle uploadBgraBuffer(QOpenGLFunctions* f, int cols, int rows, const void* data)
{
    GpuTextureHandle handle;
    if (!f || cols <= 0 || rows <= 0 || !data) {
        return handle;
    }

    unsigned int textureId = 0;
    f->glGenTextures(1, &textureId);
    f->glBindTexture(GL_TEXTURE_2D, textureId);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    f->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    f->glTexImage2D(GL_TEXTURE_2D,
                    0,
                    GL_RGBA,
                    cols,
                    rows,
                    0,
                    GL_BGRA,
                    GL_UNSIGNED_BYTE,
                    data);
    f->glBindTexture(GL_TEXTURE_2D, 0);

    handle.textureId = textureId;
    handle.width = cols;
    handle.height = rows;
    handle.internalFormat = GL_RGBA8;
    handle.lifetime = makeTextureLifetime(textureId).lifetime;
    return handle;
}

} // namespace

GpuTextureHandle ImageReadback::uploadMat(QOpenGLFunctions* f, const cv::Mat& mat)
{
    if (!f || mat.empty()) {
        return {};
    }

    const cv::Mat bgra = toBgra8(mat);
    if (bgra.empty()) {
        return {};
    }

    return uploadBgraBuffer(f, bgra.cols, bgra.rows, bgra.data);
}

GpuTextureHandle ImageReadback::uploadBgra8Mat(QOpenGLFunctions* f, const cv::Mat& mat)
{
    if (!f || mat.empty() || mat.type() != CV_8UC4) {
        return {};
    }

    if (mat.isContinuous() && mat.step == static_cast<size_t>(mat.cols * 4)) {
        return uploadBgraBuffer(f, mat.cols, mat.rows, mat.data);
    }

    const cv::Mat packed = mat.clone();
    return uploadBgraBuffer(f, packed.cols, packed.rows, packed.data);
}

cv::Mat ImageReadback::readTextureToBgrMat(QOpenGLFunctions* f,
                                           const GpuTextureHandle& handle,
                                           bool flipY)
{
    if (!f || !handle.valid()) {
        return {};
    }

    // FBO 尺寸与源纹理一致，blit 后通过 QOpenGLFramebufferObject::toImage 读回 CPU
    QOpenGLFramebufferObject fbo(QSize(handle.width, handle.height));
    if (!fbo.isValid() || !blitTextureToFbo(f, handle, fbo)) {
        return {};
    }

    QImage rgba = fbo.toImage().convertToFormat(QImage::Format_ARGB32);
    cv::Mat bgra(rgba.height(), rgba.width(), CV_8UC4, const_cast<uchar*>(rgba.bits()), rgba.bytesPerLine());
    cv::Mat copy = bgra.clone();

    // OpenGL 纹理原点在左下；flipY 使 Mat row 0 对应视觉顶部（与 OpenCV 一致）
    if (flipY) {
        cv::flip(copy, copy, 0);
    }

    cv::Mat bgr;
    cv::cvtColor(copy, bgr, cv::COLOR_BGRA2BGR);
    return bgr;
}

QImage ImageReadback::readTextureToQImage(QOpenGLFunctions* f,
                                          const GpuTextureHandle& handle,
                                          bool flipY)
{
    const cv::Mat bgr = readTextureToBgrMat(f, handle, flipY);
    if (bgr.empty()) {
        return {};
    }

    cv::Mat rgb;
    cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
    return QImage(rgb.data, rgb.cols, rgb.rows, static_cast<int>(rgb.step), QImage::Format_RGB888).copy();
}

cv::Mat ImageReadback::decodeImageFromFile(const QString& fileName)
{
    // Windows 中文路径：OpenCV imread 对 wchar 支持不稳定，改用手动读文件 + imdecode
    const std::wstring wpath = fileName.toStdWString();
    FILE* fp = nullptr;
    const errno_t err = _wfopen_s(&fp, wpath.c_str(), L"rb");
    if (err != 0 || !fp) {
        qWarning() << "无法打开文件:" << fileName;
        return {};
    }

    fseek(fp, 0, SEEK_END);
    const long size = ftell(fp);
    rewind(fp);
    std::vector<uchar> buffer(static_cast<size_t>(size));
    fread(buffer.data(), 1, static_cast<size_t>(size), fp);
    fclose(fp);
    return cv::imdecode(buffer, cv::IMREAD_UNCHANGED);
}

QImage ImageReadback::matToQImage(const cv::Mat& mat)
{
    if (mat.empty()) {
        return {};
    }

    cv::Mat outputMat;
    QImage::Format targetFormat = QImage::Format_RGB888;

    if (mat.channels() == 4) {
        cv::cvtColor(mat, outputMat, cv::COLOR_BGRA2RGBA);
        targetFormat = QImage::Format_RGBA8888;
    } else {
        cv::cvtColor(mat, outputMat, cv::COLOR_BGR2RGB);
    }

    // .copy() 确保 QImage 拥有独立 buffer，不依赖 Mat 生命周期
    return QImage(outputMat.data,
                  outputMat.cols,
                  outputMat.rows,
                  static_cast<int>(outputMat.step),
                  targetFormat)
        .copy();
}

cv::Mat ImageReadback::matFromFrame(const ImageFrame& frame, QOpenGLFunctions* f)
{
    if (!frame.image.empty()) {
        return frame.image;
    }
    if (!frame.texture.valid()) {
        return {};
    }
    if (f) {
        return readTextureToBgrMat(f, frame.texture);
    }
    // 无显式 GL 上下文时，委托 ImageGpuUpload 在共享上下文中读回
    return ImageGpuUpload::instance().readbackToBgrMat(frame.texture);
}

} // namespace NodeDataTypes
