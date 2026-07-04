#pragma once

#include "GpuTextureHandle.h"

#include <QImage>
#include <QString>

class QOpenGLFunctions;

namespace NodeDataTypes
{

/**
 * @file ImageReadback.h
 * @brief CPU ↔ GPU 图像转换及 Mat/QImage 工具
 *
 * 职责边界：
 *   - **不属于** ImageData / ring buffer 的职责范围
 *   - 提供 Mat 上传、纹理读回、文件解码、ImageFrame 统一取 Mat 等底层能力
 *   - 被 ImageGpuUpload（共享上下文路径）与各节点（预览 / legacy OpenCV 算子）调用
 *
 * 性能提示：
 *   - GPU 主路径应尽量避免 readback；仅在 GUI 预览或 OpenCV 算子必须时使用
 *   - uploadBgra8Mat 快路径要求 CV_8UC4 连续内存，VideoDecoder 等应优先使用
 *   - matFromFrame 优先返回 CPU 缓存，无缓存时才触发 GPU→CPU 读回
 */

/**
 * @brief CPU ↔ GPU 图像转换工具类（全部静态方法）
 */
class DATATYPES_EXPORT ImageReadback
{
public:
    /**
     * @brief 在指定 GL 上下文中将 cv::Mat 上传为纹理
     * @param f    当前已 makeCurrent 的 QOpenGLFunctions
     * @param mat  任意通道/深度，内部规范化到 BGRA8 后 glTexImage2D
     */
    static GpuTextureHandle uploadMat(QOpenGLFunctions* f, const cv::Mat& mat);

    /**
     * @brief 连续 CV_8UC4 BGRA Mat 直传，跳过 BGR→BGRA 转换
     * @note 非连续 Mat 会先 clone 再上传
     */
    static GpuTextureHandle uploadBgra8Mat(QOpenGLFunctions* f, const cv::Mat& mat);

    /**
     * @brief GPU 纹理读回为 BGR cv::Mat
     * @param flipY  默认 true：OpenGL 纹理 V=0 在底部，读回后 flip 使 row 0 对应图像顶部
     */
    static cv::Mat readTextureToBgrMat(QOpenGLFunctions* f,
                                       const GpuTextureHandle& handle,
                                       bool flipY = true);

    /** @brief 读回为 QImage（RGB888），内部经 BGR Mat 中转 */
    static QImage readTextureToQImage(QOpenGLFunctions* f,
                                      const GpuTextureHandle& handle,
                                      bool flipY = true);

    /** @brief 从文件解码图像（Windows 宽字符路径，支持中文路径） */
    static cv::Mat decodeImageFromFile(const QString& fileName);

    /** @brief cv::Mat → QImage（BGR/BGRA 自动转 RGB/RGBA，返回 deep copy） */
    static QImage matToQImage(const cv::Mat& mat);

    /**
     * @brief 从 ImageFrame 取 Mat（原样返回 CPU 缓存，不做格式转换）
     *
     * 优先级：
     *   1. frame.image 非空 → 直接返回（BGRA/BGR/灰度等与缓存时一致）
     *   2. frame.texture 有效 → GPU 读回为 BGR（readTextureToBgrMat）
     *
     * @param f  可选：已 makeCurrent 的 GL 函数表；为 nullptr 时委托 ImageGpuUpload 共享上下文读回
     *
     * OpenCV 算子请使用 resolveBgrMatFromFrame，会在读取侧按需 ensureBgr。
     */
    static cv::Mat matFromFrame(const ImageFrame& frame, QOpenGLFunctions* f = nullptr);
};

} // namespace NodeDataTypes
