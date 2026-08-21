#include "ImageGpuUpload.h"

#include "ImageReadback.h"

#include <QOffscreenSurface>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QSurfaceFormat>

namespace NodeDataTypes
{
ImageGpuUpload& ImageGpuUpload::instance()
{
    // qDebug() << "ImageGpuUpload::instance()";
    static ImageGpuUpload uploadService;
    return uploadService;
}

ImageGpuUpload::ImageGpuUpload() = default;

ImageGpuUpload::~ImageGpuUpload()
{
    releaseContext();
}

bool ImageGpuUpload::isReady() const
{
    QMutexLocker locker(&m_mutex);
    return m_ready;
}

bool ImageGpuUpload::warmup()
{
    QMutexLocker locker(&m_mutex);
    return ensureContext();
}

bool ImageGpuUpload::ensureContext()
{
    QOpenGLContext* shareContext = QOpenGLContext::globalShareContext();

    // 已就绪且 share 关系未变则复用
    if (m_ready && m_context) {
        if (shareContext && m_context->shareContext() != shareContext) {
            releaseContext();
        } else {
            return true;
        }
    }

    m_context = new QOpenGLContext();
    m_context->setFormat(QSurfaceFormat::defaultFormat());
    if (shareContext) {
        m_context->setShareContext(shareContext);
    }
    if (!m_context->create()) {
        releaseContext();
        return false;
    }

    m_surface = new QOffscreenSurface();
    m_surface->setFormat(m_context->format());
    m_surface->create();
    if (!m_context->makeCurrent(m_surface)) {
        releaseContext();
        return false;
    }

    m_context->doneCurrent();
    m_ready = true;
    return true;
}

void ImageGpuUpload::releaseContext()
{
    if (m_context && m_surface && m_context->makeCurrent(m_surface)) {
        m_context->doneCurrent();
    }

    delete m_surface;
    m_surface = nullptr;

    delete m_context;
    m_context = nullptr;
    m_ready = false;
}

bool ImageGpuUpload::runGlImpl(const std::function<void(QOpenGLFunctions*)>& task)
{
    QMutexLocker locker(&m_mutex);
    if (!ensureContext() || !m_context->makeCurrent(m_surface)) {
        return false;
    }
    task(m_context->functions());
    m_context->doneCurrent();
    return true;
}

GpuTextureHandle ImageGpuUpload::upload(const cv::Mat& mat)
{
    QMutexLocker locker(&m_mutex);

    if (mat.empty() || !ensureContext() || !m_context->makeCurrent(m_surface)) {
        return {};
    }

    // 实际上传逻辑委托 ImageReadback::uploadMat
    const GpuTextureHandle handle = ImageReadback::uploadMat(m_context->functions(), mat);
    m_context->doneCurrent();
    return handle;
}

GpuTextureHandle ImageGpuUpload::uploadBgra8(const cv::Mat& mat)
{
    QMutexLocker locker(&m_mutex);

    if (mat.empty() || !ensureContext() || !m_context->makeCurrent(m_surface)) {
        return {};
    }

    const GpuTextureHandle handle = ImageReadback::uploadBgra8Mat(m_context->functions(), mat);
    m_context->doneCurrent();
    return handle;
}

void ImageGpuUpload::destroyTexture(unsigned int textureId)
{
    if (textureId == 0) {
        return;
    }

    QMutexLocker locker(&m_mutex);
    if (!m_ready || !m_context || !m_surface) {
        return;
    }
    if (!m_context->makeCurrent(m_surface)) {
        return;
    }

    unsigned int texId = textureId;
    m_context->functions()->glDeleteTextures(1, &texId);
    m_context->doneCurrent();
}

cv::Mat ImageGpuUpload::readbackToBgrMat(const GpuTextureHandle& handle, bool flipY)
{
    QMutexLocker locker(&m_mutex);

    if (!handle.valid() || !ensureContext() || !m_context->makeCurrent(m_surface)) {
        return {};
    }

    const cv::Mat result = ImageReadback::readTextureToBgrMat(m_context->functions(), handle, flipY);
    m_context->doneCurrent();
    return result;
}
} // namespace NodeDataTypes
