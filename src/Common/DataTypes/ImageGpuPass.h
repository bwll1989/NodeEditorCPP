#pragma once

#include "GpuTextureHandle.h"

#include <functional>

class QOpenGLFunctions;
class QOpenGLShaderProgram;

namespace NodeDataTypes
{

/**
 * @file ImageGpuPass.h
 * @brief GPU 全屏四边形渲染基础设施（FBO + fragment shader）
 *
 * 提供节点间复用的「渲染到纹理」能力，不包含任何业务算法。
 * 各节点的具体 shader（调色、合成、检测预处理等）应定义在节点自身代码中，
 * 通过 runFragmentPass() 注入。
 *
 * 线程约束：所有 GL 调用均通过 ImageGpuUpload::runGl() 序列化到共享上下文线程执行，
 * 调用方可在 GUI 线程或 worker 线程发起，但不得直接调用 OpenGL API。
 *
 * 坐标系说明：
 *   - 全屏四边形使用 NDC（-1..1），V 轴向上为正
 *   - 默认 texCoord 为 (0,0) 左下 → (1,1) 右上
 *   - OpenGL 纹理原点在左下；从 cv::Mat（行 0 在顶部）上传的纹理，
 *     若未翻转 V，显示时可能上下颠倒——需在 resample/blit 时显式 flipVertical
 */

/**
 * @brief GPU 图像 pass 单例服务
 */
class DATATYPES_EXPORT ImageGpuPass
{
public:
    /** @brief 绑定 uniform 变量的回调（在 program.bind() 之后、draw 之前调用） */
    using UniformBinder = std::function<void(QOpenGLShaderProgram&)>;

    /** @brief 绑定输入纹理的回调（在 program.bind() 之前调用） */
    using TextureBinder = std::function<void(QOpenGLFunctions*)>;

    static ImageGpuPass& instance();

    /**
     * @brief 执行自定义 fragment shader，渲染到新建 RGBA8 纹理
     *
     * 流程：allocateRgbaTexture → 创建临时 FBO → 全屏 draw → 返回 GpuTextureHandle
     *
     * @param outWidth/outHeight  输出纹理尺寸
     * @param fragmentShaderSource  GLSL 1.x fragment 源码
     *        必须声明 varying vec2 vTexCoord（与内置 vertex shader 对接）
     * @param bindUniforms  设置 uniform（如 uTexture 单元号、参数等）
     * @param bindTextures  绑定 sampler 输入（通常 glActiveTexture + glBindTexture）
     */
    GpuTextureHandle runFragmentPass(int outWidth,
                                     int outHeight,
                                     const char* fragmentShaderSource,
                                     const UniformBinder& bindUniforms,
                                     const TextureBinder& bindTextures);

    /**
     * @brief UV 重映射：缩放 / 裁剪 / 翻转（内置通用采样 shader）
     *
     * 将源纹理的 [uvMinU, uvMinV]–[uvMaxU, uvMaxV] 矩形区域
     * 线性采样到 outWidth × outHeight 的输出纹理。
     *
     * 典型用途：
     *   - resize()：全图 [0,1]² → 目标尺寸
     *   - ROI / Crop：归一化子矩形裁剪
     *   - flipVertical：修正 Mat 上传导致的上下颠倒
     *
     * @param uvMinU/v  源纹理 UV 左下角（默认 0,0）
     * @param uvMaxU/v  源纹理 UV 右上角（默认 1,1）
     */
    GpuTextureHandle resample(const GpuTextureHandle& src,
                              int outWidth,
                              int outHeight,
                              float uvMinU = 0.f,
                              float uvMinV = 0.f,
                              float uvMaxU = 1.f,
                              float uvMaxV = 1.f,
                              bool flipHorizontal = false,
                              bool flipVertical = false);

    /** @brief 线性缩放到目标尺寸（等价于 resample 全图 [0,1]²） */
    GpuTextureHandle resize(const GpuTextureHandle& src, int outWidth, int outHeight);

    /** @brief 辅助：绑定 2D 纹理到指定纹理单元 */
    static void bindTexture(QOpenGLFunctions* f, int unit, unsigned int textureId);

private:
    ImageGpuPass() = default;
};

} // namespace NodeDataTypes
