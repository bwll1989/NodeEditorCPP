#pragma once

#include "DataTypesExport.h"

#include <memory>

#include <QtGlobal>

#include <opencv2/core/mat.hpp>

namespace NodeDataTypes
{

/**
 * @file GpuTextureHandle.h
 * @brief GPU 纹理句柄与 ImageFrame 定义
 *
 * 本模块是图像数据流中 GPU 侧的最小单元：
 *   - GpuTextureHandle：轻量、可拷贝的纹理引用（含 RAII 生命周期）
 *   - ImageFrame：单帧图像，GPU 纹理 + 可选 CPU 缓存 + 全局 tick 时间戳
 *
 * 设计原则：
 *   1. 纹理在进程内 OpenGL share group 中创建，可跨 QOpenGLContext bind
 *   2. 拷贝 GpuTextureHandle / ImageFrame 仅增加 shared_ptr 引用，不复制像素
 *   3. CPU 缓存（image）与 GPU 纹理（texture）可独立存在，按需 lazy 同步
 */

/**
 * @brief 共享 OpenGL 纹理句柄
 *
 * 不拥有 OpenGL 上下文，仅持有 textureId 与尺寸元数据。
 * lifetime 字段通过 shared_ptr 自定义 deleter 在最后一个引用释放时调用
 * ImageGpuUpload::destroyTexture，避免纹理泄漏。
 *
 * 典型流转：生产者 upload / GPU pass 输出 → push 到 ring buffer →
 * 消费者 bind 到 shader 或 readback，全程无需 glDeleteTextures 手动管理。
 */
struct DATATYPES_EXPORT GpuTextureHandle
{
    unsigned int textureId = 0;          ///< OpenGL 纹理对象 ID（0 表示无效）
    int width = 0;                       ///< 纹理宽度（像素）
    int height = 0;                      ///< 纹理高度（像素）
    unsigned int internalFormat = 0x8058; ///< 内部格式，默认 GL_RGBA8

    /** @brief 引用计数 deleter，最后一个 handle 销毁时释放 GL 纹理 */
    std::shared_ptr<void> lifetime;

    /** @brief textureId 非零且宽高为正时视为可用 */
    bool valid() const { return textureId != 0 && width > 0 && height > 0; }
};

/**
 * @brief 带全局帧计数时间戳的图像帧（GPU 纹理 + 可选 CPU 缓存）
 *
 * timestamp 语义：
 *   - 通常等于 TimestampGenerator::getCurrentFrameCount()
 *   - 用于 ring buffer 按 tick 对齐、去重 push、消费者精确/模糊查找
 *   - 值为 -1 表示无效帧（empty() 为 true）
 *
 * image 字段语义：
 *   - 从 cv::Mat 上传时**始终**保留 CPU 副本（格式与源 Mat 一致：BGRA/BGR/灰度等，不做转换）
 *   - 上传失败时作为 fallback，供 ensureGpuTexture() 重试
 *   - 纯纹理路径（fromTexture / GPU 算子输出）无 CPU 副本，按需 GPU 读回
 *   - OpenCV 算子需 BGR 时在读取侧（resolveBgrMatFromFrame）按需转换
 *
 * texture 字段语义：
 *   - 有效时可直接 bind 到 fragment shader，无需 CPU 往返
 *   - 与 image 可同时存在（Mat 上传路径）；也可单独存在（GPU 算子链）
 */
struct DATATYPES_EXPORT ImageFrame
{
    GpuTextureHandle texture;
    cv::Mat image;
    qint64 timestamp = -1;

    ImageFrame() = default;

    /**
     * @brief 从 cv::Mat 构造：上传 GPU 并保留 CPU 副本
     * @param mat 按值传入——右值 move 进帧，左值为浅拷贝后 move
     * @param timestamp 全局帧号，默认 -1（无效）
     */
    explicit ImageFrame(cv::Mat mat, qint64 timestamp = -1);

    /**
     * @brief 从已有纹理句柄构造（无 CPU 缓存，纯 GPU 路径）
     * @param handle 纹理所有权随 lifetime 转移
     */
    explicit ImageFrame(GpuTextureHandle handle, qint64 timestamp = -1);

    /** @brief 工厂：Mat → ImageFrame，语义同构造函数 */
    static ImageFrame fromMat(cv::Mat mat, qint64 timestamp = -1);

    /**
     * @brief 快路径：连续 CV_8UC4 BGRA Mat 直接上传（VideoDecoder 等）
     * 跳过 BGR→BGRA 色彩转换，同样保留 CPU 副本
     */
    static ImageFrame fromBgra8Mat(cv::Mat&& mat, qint64 timestamp = -1);

    /** @brief 工厂：纯 GPU 输出包装为 ImageFrame */
    static ImageFrame fromTexture(GpuTextureHandle handle, qint64 timestamp);

    /** @brief 纹理与 Mat 均无效 */
    bool empty() const { return !texture.valid() && image.empty(); }

    /** @brief 是否已有 CPU 缓存（无需 GPU 读回即可取到 Mat） */
    bool hasCpuCache() const { return !image.empty(); }

    /**
     * @brief 若仅有 CPU 图像，尝试 lazy 上传到共享 GL 上下文
     * @return 上传后 texture.valid() 为 true；无 CPU 数据或上传失败返回 false
     */
    bool ensureGpuTexture();
};

/** @brief 将任意格式 cv::Mat 规范化后上传到共享 GL 上下文 */
DATATYPES_EXPORT GpuTextureHandle gpuTextureFromMat(const cv::Mat& mat);

/** @brief 连续 BGRA8 Mat 直传，跳过色彩转换（性能敏感路径） */
DATATYPES_EXPORT GpuTextureHandle gpuTextureFromBgra8Mat(const cv::Mat& mat);

/**
 * @brief 为已存在的 textureId 绑定 RAII 生命周期
 *
 * 用于 ImageGpuPass / ImageReadback 等自行 glGenTextures 的场景，
 * 将裸 ID 包装为 GpuTextureHandle，最后一个引用释放时自动 destroyTexture。
 */
DATATYPES_EXPORT GpuTextureHandle makeTextureLifetime(unsigned int textureId);

} // namespace NodeDataTypes
