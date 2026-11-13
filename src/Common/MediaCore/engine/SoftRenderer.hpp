// ============================================================================
//  SoftRenderer.hpp —— 软件合成渲染后端（RGBA 内存画布，参考实现 + 测试基准）
//
//  设计依据：《media-server-sync-design.md》§5.6 Renderer
//    * 合成职责：按 ScreenMapper 的 DrawItem（本机像素矩形 + z 序）把
//      各媒体帧合成到帧缓冲——本类是【平台无关参考后端】：直接操作内存
//      RGBA 画布，可在无 GPU/窗口环境逐像素验证合成正确性（z 遮挡、
//      位置映射、透明度混合、scissor 裁剪），同时充当 GL 后端的对拍基准；
//    * 生产后端（OpenGL ES 3.0，§5.6 统一后端）在 platform/ 阶段实现，
//      消费同一 DrawItem/FrameView 数据结构——后端可替换，解算层不变。
//
//  合成语义（与 GL 后端约定一致）：
//    * 绘制顺序 = 调用顺序 = DrawItem 的 z 升序（后画覆盖先画）；
//    * 目标矩形超出画布的部分裁掉（等价 viewport/scissor，跨屏素材
//      自然连续拼接的关键）；矩形与画布不相交 → 不做任何写入；
//    * 像素采样：最近邻（缩放比大时够用；双线性留给 GL 后端）；
//    * 透明度：out = src*alpha + dst*(1-alpha)，alpha = src_a/255 × opacity；
//    * 旋转：affine 路径逐像素逆仿射采样（ScreenMapper 提供 output→canvas
//      逆仿射 + 画布域 clip 矩形，本类按帧尺寸合成帧采样仿射）——clip 旋转
//      与 screen 旋转（取景窗）统一支持；轴对齐走原快速路径逐像素一致。
//      最近邻采样的旋转锯齿由 GL 后端的双线性/硬件采样消除。
//
//  线程契约：渲染线程单线程调用（与 MediaScheduler 同一线程）。
// ============================================================================

#pragma once

#include <cstdint>
#include <vector>

#include "engine/MediaFactory.hpp"   // FrameView
#include "engine/ScreenMapper.hpp"   // DrawItem

namespace ms {

class SoftRenderer {
public:
    /// 帧开始：绑定输出尺寸并按清屏色填充画布（复用缓冲，零逐帧分配）
    /// \param w/h 画布尺寸（本机输出分辨率）
    void beginFrame(int w, int h) {
        if (w <= 0 || h <= 0) { w = h = 0; }
        width_  = w;
        height_ = h;
        // 【单遍清屏】resize 保持 capacity（跨帧零分配，与原 assign 等价的
        // 复用语义），四通道清屏色在同一遍写入——原实现 assign(0) 全幅写
        // 零 + 循环再写清屏色 = 两遍全幅写（4K 画布每帧多耗 ~33MB 内存
        // 带宽）；合并后一遍完成（固定模式循环，编译器可自动向量化）。
        canvas_.resize(static_cast<std::size_t>(w) * 4 * h);
        // 清屏色四通道展开（默认不透明黑）
        const std::uint8_t r = static_cast<std::uint8_t>(clear_color_ >> 24 & 0xFF);
        const std::uint8_t g = static_cast<std::uint8_t>(clear_color_ >> 16 & 0xFF);
        const std::uint8_t b = static_cast<std::uint8_t>(clear_color_ >> 8  & 0xFF);
        const std::uint8_t a = static_cast<std::uint8_t>(clear_color_       & 0xFF);
        for (std::size_t i = 0; i + 3 < canvas_.size(); i += 4) {
            canvas_[i + 0] = r;
            canvas_[i + 1] = g;
            canvas_[i + 2] = b;
            canvas_[i + 3] = a;
        }
    }

    /// 设置清屏色（0xRRGGBBAA，默认不透明黑）
    void setClearColor(std::uint32_t rgba) noexcept { clear_color_ = rgba; }

    /// 合成一个绘制项：把 frame 按 DrawItem 矩形采样写入画布
    /// \return false = 矩形与画布不相交或帧非法（调用方计为跳过）
    bool draw(const DrawItem& di, const FrameView& frame);

    // ---- 合成结果访问（测试断言 / 平台层上屏）----

    int width()  const noexcept { return width_; }
    int height() const noexcept { return height_; }

    /// RGBA 像素缓冲（行主序，stride = width*4）；beginFrame 前为空
    const std::vector<std::uint8_t>& pixels() const noexcept { return canvas_; }

    /// 读单像素（越界返回 0）；便于测试断言
    std::uint32_t pixel(int x, int y) const noexcept {
        if (x < 0 || y < 0 || x >= width_ || y >= height_) return 0;
        const std::size_t o = (static_cast<std::size_t>(y) * width_ + x) * 4;
        return static_cast<std::uint32_t>(canvas_[o]) << 24 |
               static_cast<std::uint32_t>(canvas_[o + 1]) << 16 |
               static_cast<std::uint32_t>(canvas_[o + 2]) << 8 |
               static_cast<std::uint32_t>(canvas_[o + 3]);
    }

private:
    int width_ = 0;                 ///< 画布宽（像素）
    int height_ = 0;                ///< 画布高（像素）
    std::uint32_t clear_color_ = 0x000000FFu; ///< 清屏色 0xRRGGBBAA（默认不透明黑）
    std::vector<std::uint8_t> canvas_;        ///< RGBA 行主序帧缓冲

    /// 单像素 alpha 混合写入（2026-11-09 从轴对齐/仿射/四边形三路径的
    /// 重复混合逻辑抽取单源）：dst_row+px 处 ← s 帧像素 × clip_a。
    /// 语义：a=0 保持目标 / a≥255 直接覆盖 / 其余 out = src*a + dst*(1-a)
    ///（四通道 +127 四舍五入；255*255>>8=254 精度坑见 .cpp 注释）。
    void blendPixel(std::uint8_t* dst_row, int px,
                    const std::uint8_t* s, std::uint32_t clip_a) noexcept;

    /// 四边形两三角形仿射采样（corner pin 变形路径内核，见 .cpp 注释）——
    /// 三角分解与 GL drawQuad 条带同构，对拍逐像素可比
    void drawQuadSampling(const DrawItem& di, const FrameView& frame,
                          int x0, int y0, int x1, int y1,
                          std::uint32_t clip_a);
};

} // namespace ms
