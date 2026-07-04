#include "GpuTextureHandle.h"

#include "ImageGpuUpload.h"

#include <utility>

namespace NodeDataTypes
{

namespace
{
/** @brief 将源 Mat move 进 ImageFrame 的 CPU 缓存槽，空 Mat 不覆盖已有缓存 */
void storeCpuCache(cv::Mat& dst, cv::Mat&& src)
{
    if (!src.empty()) {
        dst = std::move(src);
    }
}
} // namespace

GpuTextureHandle makeTextureLifetime(unsigned int textureId)
{
    GpuTextureHandle handle;
    handle.textureId = textureId;
    if (textureId == 0) {
        return handle;
    }

    // 用 shared_ptr<void> + 自定义 deleter 实现纹理 RAII：
    // 指针值仅存储 textureId（非真实堆地址），deleter 中还原 ID 并委托 ImageGpuUpload 销毁。
    // 这样 GpuTextureHandle 可安全拷贝/移动，最后一个引用消失时才 glDeleteTextures。
    handle.lifetime = std::shared_ptr<void>(
        reinterpret_cast<void*>(static_cast<intptr_t>(textureId)),
        [](void* ptr) {
            const unsigned int id = static_cast<unsigned int>(reinterpret_cast<intptr_t>(ptr));
            if (id == 0) {
                return;
            }
            ImageGpuUpload::instance().destroyTexture(id);
        });
    return handle;
}

GpuTextureHandle gpuTextureFromMat(const cv::Mat& mat)
{
    // 委托 ImageGpuUpload 在共享 GL 上下文中执行格式规范化 + glTexImage2D
    return ImageGpuUpload::instance().upload(mat);
}

GpuTextureHandle gpuTextureFromBgra8Mat(const cv::Mat& mat)
{
    return ImageGpuUpload::instance().uploadBgra8(mat);
}

ImageFrame::ImageFrame(cv::Mat mat, qint64 timestamp)
    : timestamp(timestamp)
{
    if (mat.empty()) {
        return;
    }

    // 先上传 GPU，再 move Mat 到 image 缓存（上传需要 Mat 数据，不能先 move）
    texture = gpuTextureFromMat(mat);
    storeCpuCache(image, std::move(mat));
}

ImageFrame::ImageFrame(GpuTextureHandle handle, qint64 timestamp)
    : texture(std::move(handle))
    , timestamp(timestamp)
{
}

ImageFrame ImageFrame::fromMat(cv::Mat mat, qint64 timestamp)
{
    return ImageFrame(std::move(mat), timestamp);
}

ImageFrame ImageFrame::fromBgra8Mat(cv::Mat&& mat, qint64 timestamp)
{
    ImageFrame frame;
    frame.timestamp = timestamp;
    if (mat.empty()) {
        return frame;
    }

    frame.texture = gpuTextureFromBgra8Mat(mat);
    storeCpuCache(frame.image, std::move(mat));
    return frame;
}

ImageFrame ImageFrame::fromTexture(GpuTextureHandle handle, qint64 timestamp)
{
    return ImageFrame(std::move(handle), timestamp);
}

bool ImageFrame::ensureGpuTexture()
{
    if (texture.valid()) {
        return true;
    }
    if (image.empty()) {
        return false;
    }

    // lazy 上传：legacy 节点只写 Mat 时，GPU 消费者调用此方法补全纹理
    texture = gpuTextureFromMat(image);
    return texture.valid();
}

} // namespace NodeDataTypes
