// ============================================================================
//  Canvas3DGl.hpp —— 3D 视口贴图层 GL 化（阶段 1；SceneCanvasWidget 专用）
//
//  【为什么存在】paintGL3D 原用 QPainter+QImage 贴片段面片，三个结构性
//  代价（2026-11-17 用户拍板 GL 化）：
//    * HAP 帧必须解压成 QImage 才能进 QPainter（QPainter 只认 CPU 像素，
//      无法消费 DXT 压缩纹理）——worker 每帧软解 1-3ms 白付；
//    * 变形/姿态四边形贴图只能两三角仿射分块——透视四边形数学上就是
//      错的（对角线缝 + 直线弯曲）；
//    * 姿态 rot_x/rot_y 透视画面无法正确呈现（正交预投影 + 仿射贴图
//      双重失真——体感"XY 旋转不是真 3D"）。
//  本类把【片段面片】搬进裸 GL：纹理常驻显存（缓存+重建）、DXT 压缩
//  直传（glCompressedTexImage2D，S3TC 硬件采样时解压）、单应映射经 w
//  分量送光栅插值（透视校正逐点精确，两三角仿射缺陷根治）。
//  网格/标注/边框/选中框仍归 QPainter（Qt 官方 beginNativePainting
//  混用模式，夹层由 SceneCanvasWidget 管理）。
//
//  【数学：预投影四边形的透视正确贴图】
//    片段矩形 (u,v)∈[0,1]² 经姿态透视 + 正交相机投影成屏幕四边形 q。
//    该映射整体是单应 H（仿射 ∘ 透视除法 仍是单应）。GPU 只对"顶点带
//    透视 w"的图元做透视校正插值——故不直接送屏幕坐标（那是把 H 拍扁
//    成仿射，错），而是每角算 H 的分子 (Nx,Ny) 与分母 W=Nz·u+…+1，
//    顶点送 (Nx, Ny, W)；着色器输出 clip=(Nx·kx−W, −Ny·ky+W, 0, W)，
//    光栅端 /W 插值逐点复现 H —— 与 display 端 pose3d 链同构精确。
//    平行四边形（无姿态）退化为 W≡1 仿射，同一条路径无分支。
//
//  【状态隔离（QPainter 共存契约，沿用 GlRenderer 两次 paint 黑屏教训）】
//    * attrib 状态存于 VAO 对象内部，QPainter 不持 VAO——它直接在
//      "当前绑定 VAO"上改自己的 attrib 且不恢复。本类每 draw 幂等重配
//      全部 attrib（布局常量，开销可忽略），draw 完 glBindVertexArray(0)
//      ——QPainter 落在缺省 VAO 上，动不到本类 VAO 的状态；
//    * glBufferData 孤儿式重分配（驱动端异步安全，GlRenderer 同款）；
//    * 混合 out = src.rgb·src.a·opa + dst·(1−src.a·opa)（alpha 通道
//      separate——与 GlRenderer/SoftRenderer 同公式）。
//
//  【纹理缓存】key = clip_id（工作副本内稳定）：首帧建存储、后续帧
//    全量重传内容（视频帧本就逐帧变；图片重传成本与 QPainter 路径的
//    逐帧 raster 上传同级——内容哈希脏检查留待后续，避免半吊子缓存）。
//    尺寸/格式变化 → 删对象重建。容量上限 64（超出逐出最久未用）。
//
//  【失败语义】loadGl/程序链接/S3TC 首传探测 任一失败 → failed_ 永久
//  置位（一次性日志），调用方回退 QPainter 原路径——贴图绝不黑屏。
//  MS_CANVAS_GL3D=0 环境变量 = 会话级禁用（回归对照入口，宿主侧判空）。
// ============================================================================

#pragma once

#include <cstdint>
#include <string>
#include <unordered_map>

class QPointF;

namespace ms {

class Canvas3DGl {
public:
    /// 纹理载荷（DeliverItem 三形态域之二；CUDA native 域编辑器 3D 不消费
    /// ——协商失败自动回退下载路径，交付端已保证此处只见 CPU 域）
    struct TexData {
        const std::uint8_t* rgba = nullptr;   ///< CPU 域：RGBA8888 内存序
        std::uint32_t       stride = 0;       ///< RGBA 行距（字节）
        const std::uint8_t* dxt = nullptr;    ///< DXT 域：裸压缩块流（非空=优先）
        std::uint32_t       dxt_format = 0;   ///< 0x83F0(DXT1)/0x83F3(DXT5)
        int                 width = 0, height = 0;
    };

    Canvas3DGl() = default;
    ~Canvas3DGl() = default; // GL 资源释放在 shutdown()（owner 于上下文
                             ///  current 时显式调用；本类 dtor 不碰 GL——
                             ///  上下文可能已不 current，删除调用会静默无效）

    bool failed() const noexcept { return failed_; }

    /// 帧参数（paintGL3D 开头每帧调用；纯记录零 GL 调用）：
    /// 逻辑尺寸驱动 NDC 换算（顶点用逻辑坐标，与 pr.quad 同域），dpr 只
    /// 决定 glViewport 设备像素——分辨率无关映射由 NDC 完成
    void frame(int logical_w, int logical_h, float dpr) {
        lw_ = logical_w;
        lh_ = logical_h;
        dpr_ = dpr;
        ++frame_no_; // 帧计数推进（纹理逐出的时间基准）
    }

    /// 惰性初始化（幂等；须 GL 上下文 current——paintGL 内成立）：
    /// 加载函数表 + 编译程序 + VAO/VBO。失败置 failed_（会话内永久）。
    bool ensure();

    /// 绘制一张透视校正贴图四边形（缓存上传 + 单应解算 + 提交一次完成）。
    /// \param key     纹理缓存键（clip_id）
    /// \param sx/sy   屏幕逻辑坐标四角（序 TL TR BR BL，与 Prim.quad 同序）
    /// \param opacity 片段透明度（tween 求值后；[0,1] 外钳制）
    /// \param u0/v0/u1/v1 裁切采样窗（归一化 [0,1]；0/1 全窗 = 未裁切逐位
    ///        历史路径）。顶点【位置】仍由单位方解单应，仅【采样 uv】映进
    ///        窗——与 display ScreenMapper crop_u/v 同式（2026-11-17 预览
    ///        裁切对齐：几何按裁后可见矩形解算 + 内容不缩放）
    /// \param rgb_gain 片段级 RGB 线性增益（2026-11-20 预览调色；nullptr =
    ///        默认 1,1,1 逐位历史路径）；gamma 伽马指数（1.0 = 不校正）。
    ///        shader 段与 display GlRenderer 同式：先乘增益、γ≠1 再
    ///        pow(clamp(c,0,1), 1/γ) —— 预览与输出像素级同源
    /// \return false = GL 未能绘制（退化四边形/上传失败）——调用方走
    ///         QPainter 原路径兜底（占位/棋盘语义自动接住）
    bool drawClipFace(const std::string& key, const TexData& d,
                      const double sx[4], const double sy[4], float opacity,
                      float u0 = 0.0f, float v0 = 0.0f,
                      float u1 = 1.0f, float v1 = 1.0f,
                      const double rgb_gain[3] = nullptr,
                      double gamma = 1.0);

    // ---- 逐屏离屏 FBO（阶段2 面片精确化；配 GlRenderer ExternalCtx）------
    // 【语义】绑定屏的 display 输出预览不再裁全局总画面（线性 UV 窗在
    // 透视合成下非线性失真），改为把 display 端同一 buildDrawList 的
    // 屏本地位绘制项经 GlRenderer(ExternalCtx) 画进本屏专属 FBO——
    // display 怎么画，预览面片就是那张画（同 buildVerts/着色器，像素级
    // 同源）。会话三步：
    //   beginScreenFace(key,w,h)   建/复用 FBO 并绑定（保存宿主绑定+视口）
    //   GlRenderer::beginFrame/draw/endFrame
    //       逐屏绘制项画进 FBO（宿主先 setExternalViewport(0,0,w,h)）
    //   endScreenFace()            恢复宿主绑定与视口
    //   drawScreenFaceTex(key,…)   FBO 纹理单应贴到投影四边形
    // 【v 惯例】FBO 渲染产物 GL 底向上（GlRenderer 顶点构造 y 翻转）——
    // 内容顶在 v=1，与 drawClipFace 的视频帧惯例（row0=顶=v=0）相反；
    // drawScreenFaceTex 内部已反接，调用方恒传屏幕四角即可。
    /// \return false = FBO 创建/完整性校验失败（调用方回退全局帧路径；
    ///         本会话该 key 不再重试——避免日志刷屏）
    bool beginScreenFace(const std::string& key, int w, int h);
    /// 恢复宿主 draw framebuffer 与视口（beginScreenFace 配对收尾；
    /// 无会话时调用无操作）
    void endScreenFace();
    /// 把 beginScreenFace 会话产出的 FBO 纹理贴到投影四边形（全窗采样）
    /// \return false = 会话/FBO 不存在（调用方回退全局帧路径）
    bool drawScreenFaceTex(const std::string& key,
                           const double sx[4], const double sy[4],
                           float opacity);

    /// 显式释放全部 GL 资源（owner 析构中 makeCurrent 后调用；幂等）
    void shutdown();

private:
    /// 4 点单应解算（单位方 (0,0)(1,0)(1,1)(0,1) → dst 八元组）：
    /// 8×8 线性系统 + 部分主元消元；dst 退化（共线/重复点）返回 false
    static bool solveHomography(const double dst[8], double h[8]);

    /// 单应贴图提交（drawClipFace/drawScreenFaceTex 共用尾段）：
    /// 解算单位方→四角单应、构造透视 w 顶点、幂等重配状态并绘制。
    /// 本函数自行把 tex 绑到 GL_TEXTURE0（幂等——调用方无需预绑定）。
    /// rgb_gain/gamma 仅 drawClipFace 链消费（屏幕 FBO 画面已在
    /// GlRenderer 阶段调色，二次应用会错）；nullptr = 默认直通。
    bool submitFace(unsigned int tex, const double sx[4], const double sy[4],
                    float opacity, float u0, float v0, float u1, float v1,
                    const double rgb_gain[3] = nullptr, double gamma = 1.0);

    /// 逐屏离屏会话资源（beginScreenFace 建；key = "__screen_fbo_"+屏id）。
    /// FBO 渲染坐标 GL 底向上——GlRenderer 顶点构造已做 H−y 翻转，
    /// 采样时内容顶 = v=1（drawScreenFaceTex 内部反接）。
    struct ScreenFbo {
        unsigned int  fbo = 0;        ///< 帧缓冲对象（0 = 未建）
        unsigned int  tex = 0;        ///< 颜色纹理（RGBA；合成画布 1:1）
        int           w = 0, h = 0;   ///< 分配尺寸（变化 → 重建）
        std::uint64_t last_used = 0;  ///< 逐出依据（frame 计数；屏删除后
                                      ///  残留会话的 GPU 泄漏防御）
    };

    struct TexSlot {
        unsigned int  tex = 0;        ///< GL 纹理名（0 = 未建）
        int           w = 0, h = 0;   ///< 分配尺寸（变化 → 重建存储）
        std::uint32_t fmt = 0;        ///< 0 = RGBA 可变存储；否则 DXT 内部格式
        std::uint64_t last_used = 0;  ///< 容量逐出依据（frame 计数）
    };

    std::unordered_map<std::string, TexSlot> cache_;
    std::unordered_map<std::string, ScreenFbo> screen_fbos_;
    int saved_fbo_ = 0;                    ///< beginScreenFace 保存的宿主
                                           ///<  draw framebuffer 绑定
    int saved_vp_[4] = {0, 0, 0, 0};       ///< 同上（宿主视口矩形）
    bool fbo_session_ = false;             ///< begin/endScreenFace 会话标记
    unsigned int program_ = 0;  ///< 贴图着色器程序（透视 w 顶点 + 采样片元）
    unsigned int vao_ = 0;      ///< 顶点数组（6 顶点 × {pos3,uv2}）
    unsigned int vbo_ = 0;      ///< 顶点缓冲（每 draw 孤儿式重分配 120B）
    int loc_scale_ = -1;        ///< u_scale（逻辑→NDC 系数；链接后缓存）
    int loc_tex_ = -1;          ///< u_tex 采样器
    int loc_opacity_ = -1;      ///< u_opacity
    int loc_gain_ = -1;         ///< u_rgb_gain（片段级调色；链接后缓存）
    int loc_gamma_ = -1;        ///< u_gamma（片段级调色；链接后缓存）
    int lw_ = 0, lh_ = 0;       ///< 本帧逻辑尺寸（frame 注入）
    float dpr_ = 1.0f;
    bool  inited_ = false;      ///< ensure 完成（程序/VAO/VBO 就绪）
    bool  failed_ = false;      ///< 初始化失败（此后全部拒绝，会话内永久）
    bool  dxt_probed_ = false;  ///< S3TC 支持已探测（首个 DXT 上传后查错）
    bool  dxt_dead_ = false;    ///< S3TC 探测失败（永久跳过，一次性日志）
    std::uint64_t frame_no_ = 0; ///< 帧计数（逐出依据；frame() 递增）
};

} // namespace ms
