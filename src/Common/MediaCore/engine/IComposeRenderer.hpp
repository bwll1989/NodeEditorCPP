// ============================================================================
//  IComposeRenderer.hpp —— 合成后端统一接口（第②阶段 GL 渲染管线的解耦点）
//
//  设计依据：《hw-decode-implementation-plan.md》②阶段 + SoftRenderer 头
//  注释早已预留的"后端可替换"位：
//    * SoftRenderer（参考实现/测试基准）与 GlRenderer（display 端 GL 后端）
//      消费同一 DrawItem/FrameView 数据结构，实现同一接口——解算层
//      （ScreenMapper/RenderLoop）零改动切换后端；
//    * 接口方法与 SoftRenderer 既有签名同构（beginFrame/draw），
//      新增 endFrame（GL 的 SwapBuffers 时机）/readPixels（诊断导帧/
//      测试对拍的像素回读）两个后端能力。
//
//  帧域约定（③阶段 0 拷贝）：
//    * FrameView.domain = CpuRgba → 常规上传路径（软解/回退档）；
//    * domain = GpuCudaNv12 → CUDA-GL interop 直通（仅 GL 后端支持，
//      Soft 后端遇到 GPU 帧返回 false 由上层按跳过处理——装配层保证
//      "GPU 交付只在 GL 后端开启"，此分支为防御性兜底）。
//
//  线程契约：全部方法在渲染线程调用（RenderThread 自持线程——GL 上下文
//  与 CUDA context 的 makeCurrent/setCurrent 均由后端 init/draw 管理）。
// ============================================================================

#pragma once

#include <cstdint>
#include <vector>

#include "engine/MediaFactory.hpp"   // FrameView
#include "engine/ScreenMapper.hpp"   // DrawItem
#include "engine/SoftRenderer.hpp"   // SoftRendererBackend 包装对象（完整类型）

namespace ms {

// ---------------------------------------------------------------------------
// WarpSpec —— 输出变形网格（渲染端消费形态；2026-11-09 手动网格变形）
//
// 语义：合成管线先把本机画面 1:1 画进离屏画布（GL = FBO），帧结束呈现
// 前按本网格把画布内容贴到输出——节点给出输出窗上的目标位置，内容 UV
// 恒为规则网格。坐标 [0,1] 归一化、y 向下、行主序（与 SceneModel::
// ScreenWarp 同构；渲染端用 float——顶点管线的天然精度）。
//
// mode=2（MPCDI 顶点网格，M4 收编自 MMC media_core）：与 mode=1 同一
// "输出位置 → 内容采样"模型，但位置与内容 UV 均为任意值（PFM warp 场
// 降采样结果，规则拓扑上承载非规则映射）：
//   * uvs 非空（= cols*rows*2）——逐节点内容 UV（画布归一化域，NaN/无效
//     映射约定 (-1,-1)：渲染端 uv 越界 discard → 输出黑）；空 = 规则
//     网格（向后兼容 mode=1 语义）。
//   * alphas 非空（= cols*rows）——逐节点 alpha（alphaMap 按网格顶点
//     采样值；羽化边缘融合）。空 = 全 1（无融合）。
//   * uv 的 y 翻转（buffer 视觉顶起 → FBO 纹理 v 轴）由渲染端统一处理
//     （与 mode=1 内容顶=v1 同源），注入方按"视觉 y 向下"语义给值。
// ---------------------------------------------------------------------------
struct WarpSpec {
    int mode = 0;               ///< 0=无 1=手动网格 2=MPCDI 顶点网格
    int cols = 0;               ///< 网格列数（节点数；mode=1/2 时 ≥2）
    int rows = 0;               ///< 网格行数
    std::vector<float> pts;     ///< 节点位置 x,y 交错，size = cols*rows*2
    std::vector<float> uvs;     ///< mode=2：内容 uv x,y 交错（cols*rows*2；空=规则网格）
    std::vector<float> alphas;  ///< mode=2：逐节点 alpha（cols*rows；空=全 1）

    bool active() const {
        if (mode == 0 || cols < 2 || rows < 2 ||
            pts.size() < static_cast<std::size_t>(cols) * rows * 2)
            return false;
        // mode=2 附加场：给了就必须给够（尺寸错 = 上游装配 bug，拒绝激活
        // 走直出降级——绝不拿半截数据渲染）
        if (mode == 2) {
            if (!uvs.empty() &&
                uvs.size() < static_cast<std::size_t>(cols) * rows * 2)
                return false;
            if (!alphas.empty() &&
                alphas.size() < static_cast<std::size_t>(cols) * rows)
                return false;
        }
        return true;
    }
};

class IComposeRenderer {
public:
    virtual ~IComposeRenderer() = default;

    /// 设置输出变形网格（每帧可调；实现内部自行比对去重）。
    /// 默认空实现：不支持变形的后端（Soft/预览）静默忽略——变形是
    /// display 输出端能力，编辑器预览画布恒 1:1 直显。
    virtual void setWarp(const WarpSpec&) {}

    /// 后端就绪（输出窗口/上下文/着色器等一次性资源创建）。
    /// 【启动即建窗（2026-11-04 回归修复）】RenderThread 在 loop 开头、
    /// 收到场景之前调用——保证 display 一启动就有输出窗口（黑屏待播），
    /// 而非等首帧合成才建窗（旧惰性行为：无 Editor 接入 → 无场景 →
    /// 渲染循环早退 → 窗口永不出现，用户只见终端，误读为故障）。
    ///   * GL 后端：创建窗口 + 3.3 core 上下文 + 程序/VAO/VBO（幂等，
    ///     首帧 beginFrame 内的 ensureInit 再次调用时直接短路）；
    ///   * Soft 后端：CPU 画布无窗口概念，恒返回 true。
    /// 线程契约：与 draw 系方法同线程（渲染线程）。
    /// \return false = 初始化失败（GL 驱动过旧等；后续 beginFrame 走失败降级）
    virtual bool ensureReady() = 0;

    /// 输出窗口消息泵（渲染线程每帧调用；非阻塞 PeekMessage 取空即返）。
    /// 【2026-11-04"未响应"修复】Win32 窗口消息必须由创建线程处理，否则
    /// Windows 判定窗口"未响应"（标题变白/拖动关闭全无响应）。有帧路径
    /// 在 endFrame 内已泵；【无场景早退路径】（启动即建窗后、场景到达前的
    /// 空窗期，以及窗口被用户关闭后）靠本方法兜底——每帧必调，语义即
    /// "渲染线程同时兼任输出窗口的消息泵"。
    /// 默认空实现：无窗口后端（Soft CPU 画布）无需处理。
    virtual void pumpOsMessages() {}

    /// 帧开始：绑定输出尺寸并清屏（GL = glViewport + glClear）
    virtual void beginFrame(int w, int h) = 0;

    /// 合成一个绘制项（z 序由调用方保证；返回 false = 跳过该项）
    virtual bool draw(const DrawItem& di, const FrameView& frame) = 0;

    /// 帧结束：提交呈现（GL = SwapBuffers；Soft = 空操作）。
    /// 调用时机在诊断导帧【之后】——保证导帧读到的是完整帧内容。
    virtual void endFrame() = 0;

    /// 实际输出表面尺寸（像素；0 = 后端无实际表面/尚不可知——上层走兜底）。
    /// 【显示分辨率模型 2026-11-10】渲染光栅恒 = 输出设备实际尺寸（GL 窗口
    /// = 客户区，WM_SIZE 实时维护；全屏铺满 = 设备物理分辨率）——display
    /// 输出恒点对点 1:1，取景矩形（Screen.w/h）只决定画布内容映射进该
    /// 光栅（比例失配被吸收进取景映射段，输出段无失配可言）。窗口模式
    /// 每帧查询 = resize 自动跟随，无需跨线程推送。默认 0：无窗口后端
    /// （Soft CPU 画布——画布尺寸恰由上层 beginFrame 决定，反向依赖）。
    virtual int outputWidth()  const { return 0; }
    virtual int outputHeight() const { return 0; }

    /// 像素回读（诊断 BMP 导出/测试对拍）：返回 RGBA top-down 像素
    /// \param out_w/out_h 实际读出尺寸（GL 后端 = 窗口客户区，可能与画布
    ///        尺寸不同——调用方按此尺寸写 BMP，勿用画布尺寸假设）
    /// \return false = 后端不支持回读（如窗口已销毁/初始化失败）
    virtual bool readPixels(std::vector<std::uint8_t>& out_rgba,
                            int& out_w, int& out_h,
                            std::uint32_t& out_stride) = 0;
};

// ---------------------------------------------------------------------------
// SoftRendererBackend —— SoftRenderer 的接口适配（零行为变化）
//
// RenderThread 的活动后端指向本适配器（soft 模式）：包装既有 SoftRenderer
// 实例（同一对象，测试的 renderer() 访问器与逐像素断言全部保持）。
// readPixels 直接拷画布（诊断导帧 1s/次，拷贝开销可忽略）。
// ---------------------------------------------------------------------------
class SoftRendererBackend final : public IComposeRenderer {
public:
    explicit SoftRendererBackend(SoftRenderer& soft) : soft_(soft) {}

    bool ensureReady() override { return true; } // CPU 画布：无窗口资源可预建

    void beginFrame(int w, int h) override { soft_.beginFrame(w, h); }
    bool draw(const DrawItem& di, const FrameView& frame) override {
        return soft_.draw(di, frame);
    }
    void endFrame() override {} // 软渲染无呈现概念（frame_sink 负责上屏）

    bool readPixels(std::vector<std::uint8_t>& out_rgba,
                    int& out_w, int& out_h,
                    std::uint32_t& out_stride) override {
        if (soft_.width() <= 0 || soft_.height() <= 0) return false;
        out_w     = soft_.width();
        out_h     = soft_.height();
        out_stride = static_cast<std::uint32_t>(out_w) * 4;
        out_rgba.assign(soft_.pixels().begin(), soft_.pixels().end());
        return true;
    }

    SoftRenderer& impl() noexcept { return soft_; } ///< 底层画布访问

private:
    SoftRenderer& soft_; ///< 画布实体（RenderThread 持有，生命周期覆盖）
};

} // namespace ms
