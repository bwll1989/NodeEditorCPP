// ============================================================================
//  GlRenderer.cpp —— GL 合成后端实现（着色器 + 顶点构造 + 纹理上传 + 回读）
// ============================================================================

#include "GlRenderer.hpp"

#include <chrono>
#include <cstdio>
#include <cstring>
#include <string>
#include <vector>

#include "GlLoader.hpp"

extern "C" {
#include <libavutil/frame.h> // AVFrame（GpuCudaNv12 域的 native 句柄实体）
}

namespace ms {

// ---------------------------------------------------------------------------
// 着色器源码（GLSL 330 core；坐标/采样语义见头文件"坐标系与防上下反"节）
// ---------------------------------------------------------------------------
namespace {

/// 顶点着色器：像素坐标 → NDC（等价 ortho(0,W,0,H) 投影）+ uv 直传。
/// a_pos 为 GL 窗口坐标域（y 向上，顶点构造时已完成 H−y 翻转）。
/// a_alpha 逐顶点融合系数（M4 MPCDI alphaMap 顶点采样；quad 直通路径
/// 用常量 1.0——drawQuad 每次 disable attrib2 并置常量，防 VAO 状态互串）。
constexpr char kVertexShader[] = R"(
#version 330 core
layout(location = 0) in vec2 a_pos;   // GL 窗口像素坐标（y 向上）
layout(location = 1) in vec2 a_uv;    // 帧纹理坐标（归一化）
layout(location = 2) in float a_alpha; // 逐顶点融合（quad 路径常量 1）
uniform vec2 u_outSize;               // 画布尺寸（像素）——像素→NDC 基准
out vec2 v_uv;
out float v_alpha;
void main() {
    vec2 ndc = a_pos / u_outSize * 2.0 - 1.0;
    gl_Position = vec4(ndc, 0.0, 1.0);
    v_uv = a_uv;
    v_alpha = a_alpha;
}
)";

/// 片元着色器：出界 discard（旋转角外露底色）+ 纹理采样 + 不透明度。
/// 容差域 [-0.5, frameSize+0.5]（帧像素、角落原点）与 SoftRenderer
/// 仿射路径的 fx/fy 判定逐点一致；CLAMP_TO_EDGE 等价帧内 clamp。
/// v_alpha 乘 rgb（呈现关混合、输出窗 alpha 无意义）：MPCDI 无效映射
/// 顶点 alpha=0 → 黑；羽化融合按顶点插值平滑过渡。
constexpr char kFragmentShader[] = R"(
#version 330 core
in vec2 v_uv;
in float v_alpha;
uniform sampler2D u_tex;
uniform vec2 u_frameSize;
uniform float u_opacity;
out vec4 fragColor;
void main() {
    vec2 px = v_uv * u_frameSize;
    if (px.x < -0.5 || px.y < -0.5 ||
        px.x > u_frameSize.x + 0.5 || px.y > u_frameSize.y + 0.5)
        discard;                       // 旋转角区域：露画布底色
    vec4 c = texture(u_tex, v_uv);
    fragColor = vec4(c.rgb * v_alpha, c.a * u_opacity);
}
)";

/// NV12 片元着色器（③阶段 0 拷贝路径）：双平面采样 + BT.601 limited
/// YUV→RGB 转换。与 FFmpeg swscale 的 yuv420p（limited，bt601）语义一致
/// ——硬解帧不下载、软解路径经 swscale 的色彩都对得上：
///   Y ∈ [16,235] → Y' = 1.164×(Y−16/255)；UV ∈ [16,240] 偏置 128。
///   R = 1.164(Y−16/255) + 1.596(V−128/255)
///   G = 1.164(Y−16/255) − 0.391(U−128/255) − 0.813(V−128/255)
///   B = 1.164(Y−16/255) + 2.018(U−128/255)
/// 亮度 R8 纹理采样 .r；色度 RG8 纹理采样 .rg（U 在 R 分量、V 在 G 分量
/// ——D2D 拷贝按 UV 内存序直传，FFmpeg NV12 内存序即 U 前V 后）。
/// alpha 恒 1（NV12 无 alpha 通道），不透明度乘 u_opacity。
constexpr char kFragmentShaderNv12[] = R"(
#version 330 core
in vec2 v_uv;
in float v_alpha;
uniform sampler2D u_tex_y;   // 亮度平面（R8，w×h）
uniform sampler2D u_tex_uv;  // 色度平面（RG8，w/2×h/2，UV 交错）
uniform vec2 u_frameSize;
uniform float u_opacity;
out vec4 fragColor;
void main() {
    vec2 px = v_uv * u_frameSize;
    if (px.x < -0.5 || px.y < -0.5 ||
        px.x > u_frameSize.x + 0.5 || px.y > u_frameSize.y + 0.5)
        discard;                       // 旋转角区域：露画布底色
    float y = texture(u_tex_y, v_uv).r;
    vec2 uv = texture(u_tex_uv, v_uv).rg;
    float yy = y - (16.0 / 255.0);
    float uu = uv.x - 0.5;
    float vv = uv.y - 0.5;
    float r = 1.164 * yy + 1.596 * vv;
    float g = 1.164 * yy - 0.391 * uu - 0.813 * vv;
    float b = 1.164 * yy + 2.018 * uu;
    // v_alpha 恒 1（NV12 走 quad 直通路径，attrib2 常量——见 drawQuad）
    fragColor = vec4(clamp(r, 0.0, 1.0) * v_alpha,
                     clamp(g, 0.0, 1.0) * v_alpha,
                     clamp(b, 0.0, 1.0) * v_alpha, u_opacity);
}
)";

/// 着色器编译辅助（失败打 infoLog）
bool compileShader(unsigned int type, const char* src, unsigned int& out) {
    out = gl::glCreateShader(type);
    gl::glShaderSource(out, 1, &src, nullptr);
    gl::glCompileShader(out);
    int ok = 0;
    gl::glGetShaderiv(out, gl::GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024] = "";
        gl::glGetShaderInfoLog(out, sizeof(log), nullptr, log);
        std::fprintf(stderr, "[gl] 着色器编译失败: %s\n", log);
        return false;
    }
    return true;
}

} // namespace

// ---------------------------------------------------------------------------
// 析构：GL 资源释放（渲染线程调用——loop 退出时 backend_.reset() 在
// 同线程执行，满足 GL 对象的线程亲和契约）
// ---------------------------------------------------------------------------
GlRenderer::~GlRenderer() {
    if (!inited_) return; // 未初始化：无 GL 资源（ctx_ 析构自清理）
#ifndef MS_GL_NO_CUDA
    // CUDA interop 注销必须在纹理销毁【前】（注册句柄引用 GL 纹理对象；
    // 顺带释放 CUDA 侧资源——渲染线程上 CUDA 上下文仍 current）
    interop_.unregisterTextures();
    interop_.shutdown();
    for (int g = 0; g < 2; ++g) { // 双缓冲两组纹理逐一销毁
        if (tex_uv_[g]) gl::glDeleteTextures(1, &tex_uv_[g]);
        if (tex_y_[g])  gl::glDeleteTextures(1, &tex_y_[g]);
    }
    if (program_nv12_)  gl::glDeleteProgram(program_nv12_);
#endif
    if (tex_)     gl::glDeleteTextures(1, &tex_);
    if (vbo_)     gl::glDeleteBuffers(1, &vbo_);
    if (vao_)     gl::glDeleteVertexArrays(1, &vao_);
    if (program_) gl::glDeleteProgram(program_);
    // 变形资源（离屏画布；纹理先删再删 FBO——附着引用顺序惯例）
    if (fbo_tex_) gl::glDeleteTextures(1, &fbo_tex_);
    if (fbo_)     gl::glDeleteFramebuffers(1, &fbo_);
    // ctx_（GlContextWGL）成员析构自动 destroy（makeCurrent(NULL)+删上下文）
}

// ---------------------------------------------------------------------------
// beginFrame —— 懒初始化 + 清屏 + 帧状态绑定
// ---------------------------------------------------------------------------
void GlRenderer::beginFrame(int w, int h) {
    if (failed_) return;
    if (!ensureInit()) { failed_ = true; return; }
    // 用户已关窗（WM_CLOSE → alive_=false）：跳过全部 GL 调用（DC 已随
    // 窗口销毁失效）——消息泵由 pumpOsMessages 继续兜底，同步输出不受影响。
    // ExternalCtx 模式无自有窗口（宿主=Qt 管理的 FBO），跳过 alive 检查。
    if (!external_ && !ctx_.alive()) return;

    // 离屏模式（hidden）：窗口客户区必须与画布同尺寸——readPixels 对拍
    // 才能逐像素对齐。WS_POPUP 无边框（见 GlContextWGL）→ SetWindowPos
    // 尺寸即客户区尺寸（测试路径画布固定，仅首帧触发）。
    if (geom_.hidden && inited_ &&
        (ctx_.clientW() != w || ctx_.clientH() != h) && w > 0 && h > 0) {
        SetWindowPos(ctx_.hwnd(), nullptr, 0, 0, w, h,
                     SWP_NOMOVE | SWP_NOZORDER | SWP_NOACTIVATE);
        RECT rc{};
        if (GetClientRect(ctx_.hwnd(), &rc)) {
            ctx_.notifyClientRect(rc.right - rc.left, rc.bottom - rc.top);
        }
    }

    canvas_w_ = w;
    canvas_h_ = h;

    // ---- 输出变形（两段式）：合成改画进 FBO（呈现时按 mesh 贴回输出）----
    // ExternalCtx 不参与（预览画布恒 1:1）；FBO 失败已由 ensureFbo 置
    // warp_failed_ 永久降级直出——此处不再分支，后续调用 warpActive 为假。
    warped_this_frame_ = false;
    if (warpActive()) {
        if (warp_dirty_) {
            std::fprintf(stderr,
                         "[gl] 输出变形已启用: mode=%d 网格 %dx%d\n",
                         warp_.mode, warp_.cols, warp_.rows);
            std::fflush(stderr);
            warp_dirty_ = false;
        }
        if (ensureFbo(w, h))
            gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, fbo_);
    }

    // 视口：绘制目标决定基准——
    //   * ExternalCtx 模式 = 宿主指定完整矩形 (vp_w_,vp_h_)，未设置（0）时
    //     回退 (w,h)=beginFrame 尺寸（【修复 2026-11-05】旧版恒用 (w,h)=comp
    //     域尺寸——宿主内容域映射矩形 ≠ comp 尺寸时（dom 外扩/dpr 缩放）
    //     内容被错误压缩，联测实证 818/929=0.88 缩放 + 视口原点错位的
    //     "内容偏移"伪影）
    //   * 变形激活（FBO 已绑定）= (w,h)=FBO 纹理尺寸——合成必须 1:1 写满
    //     FBO 画布。【错配 bug 2026-11-09】旧版此处沿用窗口客户区
    //     (clientW×clientH)：客户区 < screen 尺寸时整幅画面被压缩进 FBO
    //     左下角客户区大小的一角（GL 帧缓冲 y 向上），warpBlit 再把 FBO
    //     全图贴回窗口 → 素材缩在输出窗左下角（实测 960×501 客户区 +
    //     1920×1080 screen：素材 480×232 恰为 (client/screen)² 比例）。
    //     hidden 离屏对拍路径客户区恒被同步为 (w,h)，错配不显现——
    //     真实窗口专属 bug。
    //   * 其余（直出）= 窗口客户区，配合投影 uniform=(w,h) 构成拉伸铺满
    if (external_) {
        gl::glViewport(vp_x_, vp_y_,
                       vp_w_ > 0 ? vp_w_ : w,
                       vp_h_ > 0 ? vp_h_ : h);
    } else if (warpActive()) {
        // 走到这里 = ensureFbo 成功且 FBO 已绑定（失败路径 warp_failed_
        // 已置位 → warpActive 为假 → 落入下方直出分支，自洽）
        gl::glViewport(0, 0, w, h);
    } else {
        gl::glViewport(0, 0, ctx_.clientW(), ctx_.clientH());
    }
    // 【MMC_GL_AN=1 呈现诊断 2026-11-04】环境钩子：清屏色按帧号循环
    // （深蓝→暗红→暗绿，60fps 下每 0.5s 换色）。display 单独运行（无
    // Editor/无场景）时窗口应持续变色 = 呈现链每帧上屏；恒首帧色 =
    // 呈现冻结（进程内二分，替代已无法逼近的 python 复刻）。
    {
        static const bool s_anim = [] {
            const bool on = std::getenv("MMC_GL_ANIM") != nullptr;
            if (on) std::fprintf(stderr,
                                 "[gl] MMC_GL_ANIM=1：清屏色循环诊断开启\n");
            return on;
        }();
        if (s_anim) {
            static std::uint64_t anim_fn = 0;
            const int ph = static_cast<int>((anim_fn++ / 30) % 3);
            static const float kCols[3][4] = {
                {0.0f, 0.0f, 0.35f, 1.0f}, // 深蓝
                {0.55f, 0.0f, 0.0f, 1.0f}, // 暗红
                {0.0f, 0.45f, 0.0f, 1.0f}, // 暗绿
            };
            gl::glClearColor(kCols[ph][0], kCols[ph][1], kCols[ph][2], 1.0f);
        } else {
            gl::glClearColor(0.0f, 0.0f, 0.0f, 1.0f); // 画布底色黑（兜底）
        }
    }
    // 【上下文状态诊断 MS_CANVAS_DBG=1】QPainter 介入前后逐帧 dump 关键
    // 绑定状态——frame#1（画得出）与 frame#2+（画不出）的差异即污染源。
    {
        static const bool s_dbg = std::getenv("MS_CANVAS_DBG") != nullptr;
        if (s_dbg && external_) {
            static int s_cnt = 0;
            ++s_cnt;
            if (s_cnt <= 12) {
                int fbo = -1, rfbo = -1, vao = -1, vbo = -1, atex = -1,
                    prog = -1;
                int vp4[4] = {};
                gl::glGetIntegerv(gl::GL_DRAW_FRAMEBUFFER_BINDING, &fbo);
                gl::glGetIntegerv(static_cast<unsigned int>(0x8CA7),
                                  &rfbo); // GL_READ_FRAMEBUFFER_BINDING
                gl::glGetIntegerv(gl::GL_VERTEX_ARRAY_BINDING, &vao);
                gl::glGetIntegerv(gl::GL_ARRAY_BUFFER_BINDING, &vbo);
                gl::glGetIntegerv(gl::GL_ACTIVE_TEXTURE, &atex);
                gl::glGetIntegerv(gl::GL_CURRENT_PROGRAM, &prog);
                gl::glGetIntegerv(static_cast<unsigned int>(0x0BA2),
                                  vp4); // GL_VIEWPORT（数值字面量防宏污染）
                std::fprintf(stderr,
                             "[gl-dbg] begin#%d draw-fbo=%d read-fbo=%d "
                             "vao=%d vbo=%d atex=0x%X prog=%u "
                             "vp=(%d,%d %dx%d) err=0x%X\n",
                             s_cnt, fbo, rfbo, vao, vbo, atex, prog,
                             vp4[0], vp4[1], vp4[2], vp4[3],
                             gl::glGetError());
                std::fflush(stderr);
            }
        }
    }
    gl::glClear(gl::GL_COLOR_BUFFER_BIT);
    // 【诊断】clear 后探针（时点①）——若此处已非黑则 clear 前 FBO 有旧内容
    {
        static const bool s_dbg = std::getenv("MS_CANVAS_DBG") != nullptr;
        if (s_dbg && external_) {
            static int s_cnt = 0;
            if (++s_cnt <= 12) {
                const int px = vp_x_ + canvas_w_ * 7 / 10;
                const int py = vp_y_ + canvas_h_ * 4 / 10;
                unsigned char buf[4] = {};
                gl::glReadPixels(px, py, 1, 1, gl::GL_RGBA,
                                 gl::GL_UNSIGNED_BYTE, buf);
                std::fprintf(stderr,
                             "[gl-dbg] after-clear#%d probe=(%u,%u,%u)\n",
                             s_cnt, buf[0], buf[1], buf[2]);
                std::fflush(stderr);
            }
        }
    }

    // 帧级状态：程序/混合/投影（draw 之间不变的部分集中在此）
    gl::glUseProgram(program_);
    // 【共享上下文防御 2026-11-04】ExternalCtx 模式下同一上下文还跑
    // QPainter（QOpenGLWidget 标注层）——Qt 的 GL 绘制引擎会遗留全局
    // 光栅状态（深度测试/面剔除/剪裁/颜色掩码），下一帧裸 GL 合成若不
    // 复位即"静默画不出"（GL 无错误、项都在、画面黑——编辑器联测实
    // 证）。合成器每帧重设全部依赖的状态，双模式一致（display 端这些
    // 恒为默认关闭，禁用是幂等空操作，行为零变化）。
    gl::glDisable(gl::GL_DEPTH_TEST);
    gl::glDisable(gl::GL_CULL_FACE);
    gl::glDisable(gl::GL_SCISSOR_TEST);
    gl::glDisable(gl::GL_STENCIL_TEST);
    gl::glColorMask(1, 1, 1, 1);
    gl::glEnable(gl::GL_BLEND);
    // out.rgb = src.rgb*src.a + dst.rgb*(1-src.a)；alpha 同构（与
    // SoftRenderer 的 d[3] = a + d3*(1-a) 公式一致）
    gl::glBlendFuncSeparate(gl::GL_SRC_ALPHA, gl::GL_ONE_MINUS_SRC_ALPHA,
                            gl::GL_ONE, gl::GL_ONE_MINUS_SRC_ALPHA);
    gl::glUniform2f(loc_out_size_,
                    static_cast<float>(w), static_cast<float>(h));
    gl::glUniform1i(loc_tex_, 0);

    // NV12 程序的投影 uniform 同步（uniform 是 per-program 状态——GPU 帧
    // 绘制切用 program_nv12_，漏设会导致 ndc = pos/0 → NaN 全帧裁剪黑屏）
#ifndef MS_GL_NO_CUDA
    if (program_nv12_ != 0) {
        gl::glUseProgram(program_nv12_);
        gl::glUniform2f(loc_nv_out_size_,
                        static_cast<float>(w), static_cast<float>(h));
        gl::glUseProgram(program_); // 恢复 RGBA 程序为当前（beginFrame 契约）
    }
#endif
}

// ---------------------------------------------------------------------------
// ensureReady —— 启动即建窗（IComposeRenderer；RenderThread loop 开头调用）
// 与 beginFrame 的懒初始化共用 ensureInit（幂等）：display 一启动渲染线程
// 就创建输出窗口 + 3.3 core 上下文，无 Editor/无场景时也有黑屏待播窗口
// （对齐旧 soft 后端"GDI 预览窗无条件启动"的部署核对体验）。
// ---------------------------------------------------------------------------
bool GlRenderer::ensureReady() {
    if (failed_) return false;
    // ExternalCtx 模式：无自有窗口可预建（首帧 beginFrame 懒建程序——
    // 须 Qt 上下文 current 的 paintGL 内才安全），恒成功。
    if (external_) return true;
    if (!ensureInit()) return false;
    // 【黑屏待播兑现 2026-11-04 白屏修复】窗口类 hbrBackground=nullptr
    // （GL 自绘，免 GDI 擦除闪烁）→ 建窗后、首帧前的客户区是**未定义
    // 内容**（实测白色）——用户误读为"白屏故障"。此处建窗成功立即按
    // 正常帧路径清屏黑 + 上屏一次：此后窗口内容由 GL 后备缓冲保持，
    // 要么真帧覆盖、要么恒黑待播。
    // 走 beginFrame/endFrame 而非裸 GL：viewport/blend 等帧级状态与正常
    // 帧路径完全一致；hidden 离屏模式 endFrame 内不 swap，无副作用。
    if (ctx_.alive() && ctx_.clientW() > 0 && ctx_.clientH() > 0) {
        beginFrame(ctx_.clientW(), ctx_.clientH());
        endFrame();
    }
    return true;
}

// ---------------------------------------------------------------------------
// pumpOsMessages —— 消息泵转发（渲染线程每帧必调）。
// 有帧路径 endFrame 内已泵；本方法覆盖【无场景早退路径】（启动后场景到达
// 前的空窗期）与【窗口已被用户关闭后的收尾期】——窗口消息不被泵则 Windows
// 判定"未响应"（用户实测：启动即建窗后窗口标题变白/拖动关闭无响应）。
// ---------------------------------------------------------------------------
void GlRenderer::pumpOsMessages() {
    if (external_) return; // 窗口归 Qt 管理：无需消息泵
    if (!inited_ || failed_) return;
    ctx_.pumpMessages();
}

// ---------------------------------------------------------------------------
// ensureInit —— 窗口/上下文/程序/VAO/VBO/纹理一次性创建
// ---------------------------------------------------------------------------
bool GlRenderer::ensureInit() {
    if (inited_) return true;

    if (external_) {
        // ---- ExternalCtx：宿主（QOpenGLWidget）已提供 3.3+ 上下文并
        // makeCurrent——只需填充 GL 函数表（进程级只读，一次即可）。
        // 不走 GlContextWGL 两步法（不建窗口、不建上下文、不碰像素格式）。
        if (!gl_loaded_) {
            if (!gl::loadGl()) {
                std::fprintf(stderr, "[gl] ExternalCtx 函数表加载失败"
                                     "（上下文过旧/驱动缺失）——回退软路径\n");
                return false;
            }
            gl_loaded_ = true;
        }
    } else if (!ctx_.create(geom_)) {
        // 窗口 + 3.3 core 上下文（渲染线程；失败原因已由 GlContextWGL 打日志）
        std::fprintf(stderr, "[gl] GL 后端初始化失败（回退可用 "
                             "--render-backend=soft 显式指定）\n");
        return false;
    }

    if (!buildProgram()) return false;

    // VAO + 动态 VBO（4 顶点 × 4 float：pos(2) + uv(2)，条带序）
    gl::glGenVertexArrays(1, &vao_);
    gl::glBindVertexArray(vao_);
    gl::glGenBuffers(1, &vbo_);
    gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vbo_);
    gl::glBufferData(gl::GL_ARRAY_BUFFER, 4 * 4 * sizeof(float), nullptr,
                     gl::GL_DYNAMIC_DRAW);
    // location 0: a_pos（2 float，偏移 0）
    gl::glEnableVertexAttribArray(0);
    gl::glVertexAttribPointer(0, 2, gl::GL_FLOAT, gl::GL_FALSE,
                              4 * sizeof(float),
                              reinterpret_cast<const void*>(0));
    // location 1: a_uv（2 float，偏移 8 字节）
    gl::glEnableVertexAttribArray(1);
    gl::glVertexAttribPointer(1, 2, gl::GL_FLOAT, gl::GL_FALSE,
                              4 * sizeof(float),
                              reinterpret_cast<const void*>(8));
    // location 2（a_alpha）：初始基线 disable + 常量 1（quad 布局）；
    // 每帧幂等重配见 drawQuad / warpBlit（5-float mesh 布局互斥切换）
    gl::glDisableVertexAttribArray(2);
    gl::glVertexAttrib1f(2, 1.0f);

    // 帧纹理单槽（首次 draw 时按帧尺寸分配）
    gl::glGenTextures(1, &tex_);

    // 【core profile 纪律】此处不解绑 VAO（glBindVertexArray(0) 在 3.3
    // core 非法——会静默报 INVALID_OPERATION 且绑定保持）。VAO 绑定留给
    // 首个 drawQuad 显式重设；ARRAY_BUFFER=vbo_ 遗留同样无害（drawQuad
    // 每帧显式重绑）。共享上下文下的 attrib 防御见 drawQuad。
    inited_ = true;
    return true;}

// ---------------------------------------------------------------------------
// buildProgram —— 合成着色器程序
// ---------------------------------------------------------------------------
bool GlRenderer::buildProgram() {
    unsigned int vs = 0, fs = 0;
    if (!compileShader(gl::GL_VERTEX_SHADER, kVertexShader, vs)) return false;
    if (!compileShader(gl::GL_FRAGMENT_SHADER, kFragmentShader, fs)) {
        gl::glDeleteShader(vs);
        return false;
    }
    program_ = gl::glCreateProgram();
    gl::glAttachShader(program_, vs);
    gl::glAttachShader(program_, fs);
    gl::glLinkProgram(program_);
    gl::glDeleteShader(vs); // 链接后着色器对象可删
    gl::glDeleteShader(fs);
    int ok = 0;
    gl::glGetProgramiv(program_, gl::GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024] = "";
        gl::glGetProgramInfoLog(program_, sizeof(log), nullptr, log);
        std::fprintf(stderr, "[gl] 程序链接失败: %s\n", log);
        return false;
    }
    // uniform location 一次性缓存（链接后 location 固定，热路径零查找）
    loc_out_size_   = gl::glGetUniformLocation(program_, "u_outSize");
    loc_tex_        = gl::glGetUniformLocation(program_, "u_tex");
    loc_frame_size_ = gl::glGetUniformLocation(program_, "u_frameSize");
    loc_opacity_    = gl::glGetUniformLocation(program_, "u_opacity");
    return true;
}

// ---------------------------------------------------------------------------
// ensureNv12Program —— NV12→RGB 着色器程序（惰性构建；GPU 帧首次绘制时）
// ---------------------------------------------------------------------------
#ifndef MS_GL_NO_CUDA
bool GlRenderer::ensureNv12Program() {
    if (program_nv12_ != 0) return true; // 已构建

    unsigned int vs = 0, fs = 0;
    if (!compileShader(gl::GL_VERTEX_SHADER, kVertexShader, vs)) return false;
    if (!compileShader(gl::GL_FRAGMENT_SHADER, kFragmentShaderNv12, fs)) {
        gl::glDeleteShader(vs);
        return false;
    }
    program_nv12_ = gl::glCreateProgram();
    gl::glAttachShader(program_nv12_, vs);
    gl::glAttachShader(program_nv12_, fs);
    gl::glLinkProgram(program_nv12_);
    gl::glDeleteShader(vs);
    gl::glDeleteShader(fs);
    int ok = 0;
    gl::glGetProgramiv(program_nv12_, gl::GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024] = "";
        gl::glGetProgramInfoLog(program_nv12_, sizeof(log), nullptr, log);
        std::fprintf(stderr, "[gl] NV12 程序链接失败: %s\n", log);
        return false;
    }

    // 采样器绑定（一次性：Y=纹理单元0、UV=单元1——NV12 绘制路径固定绑定）
    gl::glUseProgram(program_nv12_);
    gl::glUniform1i(gl::glGetUniformLocation(program_nv12_, "u_tex_y"), 0);
    gl::glUniform1i(gl::glGetUniformLocation(program_nv12_, "u_tex_uv"), 1);
    // uniform location 一次性缓存（同 buildProgram；NV12 热路径零查找）
    loc_nv_out_size_   = gl::glGetUniformLocation(program_nv12_, "u_outSize");
    loc_nv_frame_size_ = gl::glGetUniformLocation(program_nv12_, "u_frameSize");
    loc_nv_opacity_    = gl::glGetUniformLocation(program_nv12_, "u_opacity");
    gl::glUseProgram(0);
    return true;
}
#endif // !MS_GL_NO_CUDA

// ---------------------------------------------------------------------------
// ensureNv12Textures —— NV12 纹理对创建/重建 + CUDA interop 注册
//   * 首帧：创建 R8（Y，w×h）+ RG8（UV，w/2×h/2）并注册进 CUDA；
//   * 帧尺寸变化：重建存储 + 重注册（CUDA 注册绑定纹理对象，glTexImage2D
//     重建存储后句柄仍有效，但保险起见显式重注册——驱动实现差异防御）；
//   * 采样参数与 RGBA 路径一致（双线性 + CLAMP_TO_EDGE）。
// ---------------------------------------------------------------------------
#ifndef MS_GL_NO_CUDA
bool GlRenderer::ensureNv12Textures(int w, int h) {
    if (w <= 0 || h <= 0) return false;
    if (w == tex_y_w_ && h == tex_y_h_) return true; // 已就绪（同尺寸复用）

    // ---- 存储（重）分配：immutable（glTexStorage2D）-----------------------
    // 【immutable 必要性】CUDA interop 注册对 mutable RG8 纹理返回
    // invalid value（实测驱动行为）——glTexStorage2D 分配的不可变存储
    // 是注册的稳妥口径。immutable 不可原地 resize：尺寸变化时删旧纹理
    // 对象重建（注册句柄一并重挂），glDeleteTextures 对 0 号名无害。
    // 【双缓冲】每组各建一对（共 2 组 4 张）；采样参数两组一致。
    interop_.unregisterTextures(); // 先注销旧注册（纹理对象即将销毁）
    for (int g = 0; g < 2; ++g) {
        if (tex_y_[g])  gl::glDeleteTextures(1, &tex_y_[g]);
        if (tex_uv_[g]) gl::glDeleteTextures(1, &tex_uv_[g]);
        gl::glGenTextures(1, &tex_y_[g]);
        gl::glGenTextures(1, &tex_uv_[g]);

        // Y 平面：R8，w×h（单 mipmap 层；视频帧无 mip 链）
        gl::glBindTexture(gl::GL_TEXTURE_2D, tex_y_[g]);
        gl::glTexStorage2D(gl::GL_TEXTURE_2D, 1, gl::GL_R8, w, h);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MIN_FILTER,
                            gl::GL_LINEAR);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MAG_FILTER,
                            gl::GL_LINEAR);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_S,
                            gl::GL_CLAMP_TO_EDGE);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_T,
                            gl::GL_CLAMP_TO_EDGE);

        // UV 平面：RG8，w/2×h/2（NV12 色度半分辨率 UV 交错）
        gl::glBindTexture(gl::GL_TEXTURE_2D, tex_uv_[g]);
        gl::glTexStorage2D(gl::GL_TEXTURE_2D, 1, gl::GL_RG8, w / 2, h / 2);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MIN_FILTER,
                            gl::GL_LINEAR);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MAG_FILTER,
                            gl::GL_LINEAR);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_S,
                            gl::GL_CLAMP_TO_EDGE);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_T,
                            gl::GL_CLAMP_TO_EDGE);
    }

    // 分配校验（GL 错误粘滞——此处失败注册必然失败，显式暴露原因）
    // 【状态原子化】尺寸成员 tex_y_w_/tex_y_h_ 只在全部步骤成功后提交：
    // 若在错误检查前提前更新，失败路径下下次调用会因"尺寸相同"误判
    // 已就绪而复用坏纹理（存储分配失败 = incomplete texture，采样黑屏）
    const int gl_err = static_cast<int>(gl::glGetError());
    if (gl_err != 0) {
        std::fprintf(stderr, "[gl] NV12 纹理存储分配失败（GL 错误 0x%X）\n",
                     gl_err);
        return false; // tex_y_w_ 未更新：下次调用走完整重建
    }

    // ---- CUDA interop 注册（GL 上下文 current 的渲染线程）----------------
    // 【存储物化前置】注册前先让 GL 提交纹理存储（1 像素写入 + glFinish）
    // ——immutable 纹理的 backing storage 由驱动延迟提交，若注册时存储
    // 尚未实体化，CUDA 句柄捕获的 cudaArray 与稍后 GL 采样的存储脱钩
    // （实测：首帧 D2D 数据在 CUDA 侧回读正确、GL 采样却为黑，第二帧起
    // 自愈；先触碰再注册可彻底避免）。双组 4 张全部触碰。
    {
        const std::uint8_t zero[2] = {0, 0};
        gl::glPixelStorei(gl::GL_UNPACK_ALIGNMENT, 1);
        for (int g = 0; g < 2; ++g) {
            gl::glBindTexture(gl::GL_TEXTURE_2D, tex_y_[g]);
            gl::glTexSubImage2D(gl::GL_TEXTURE_2D, 0, 0, 0, 1, 1, gl::GL_RED,
                                gl::GL_UNSIGNED_BYTE, zero);
            gl::glBindTexture(gl::GL_TEXTURE_2D, tex_uv_[g]);
            gl::glTexSubImage2D(gl::GL_TEXTURE_2D, 0, 0, 0, 1, 1, gl::GL_RG,
                                gl::GL_UNSIGNED_BYTE, zero);
        }
        gl::glPixelStorei(gl::GL_UNPACK_ALIGNMENT, 4);
        gl::glFinish(); // 强制提交，注册捕获已实体化的存储
    }
    if (interop_.ready() &&
        !interop_.registerTextures(tex_y_[0], tex_uv_[0],
                                   tex_y_[1], tex_uv_[1])) {
        std::fprintf(stderr, "[gl] CUDA 纹理注册失败（%dx%d）\n", w, h);
        return false; // 同上：尺寸成员未提交，重试走完整重建
    }
    // 全部成功：一次性提交"已就绪"状态（失败路径绝不残留半新状态）
    tex_y_w_ = w;
    tex_y_h_ = h;
    return true;
}
#endif // !MS_GL_NO_CUDA

// ---------------------------------------------------------------------------
// disableGpuDelivery —— 翻转 0 拷贝协商旗标（解码端自动回退下载路径）
// ---------------------------------------------------------------------------
#ifndef MS_GL_NO_CUDA
void GlRenderer::disableGpuDelivery(const char* why) {
    if (!gpu_dead_) {
        std::fprintf(stderr,
                     "[gl] CUDA 0 拷贝不可用（%s）——已翻转协商旗标，"
                     "解码端回退 av_hwframe_transfer_data 下载路径\n", why);
        gpu_dead_ = true; // 本地快速失败：后续 GPU 帧不再重试（<1s 自愈）
    }
    if (gpu_delivery_ok_) gpu_delivery_ok_->store(false);
}
#endif // !MS_GL_NO_CUDA

// ---------------------------------------------------------------------------
// buildVerts —— 顶点构造（RGBA/NV12 双域共用；轴对齐/仿射双路径）
//   参数化帧尺寸（fw/fh），坐标与采样数学对帧域无感知——GPU NV12 帧
//   与 CPU RGBA 帧走同一套顶点（防上下反推导与对拍结论在此统一收口）。
// ---------------------------------------------------------------------------
bool GlRenderer::buildVerts(const DrawItem& di, int frame_w, int frame_h,
                            float verts[4][4]) {
    (void)frame_w; // uv 为归一化裁切窗（v3）后帧尺寸不参与顶点构造；
    (void)frame_h; // 参数保留：签名稳定 + 轴对齐/仿射共用调用点不动
    if (!di.affine) {
        // ================================================================
        // 轴对齐路径：目标矩形 top-down → gl_y = H − y 翻转；
        // uv：顶边（画面顶）v=crop_v0、底边 v=crop_v1，左边 u=crop_u0、
        // 右边 u=crop_u1（v3 裁切采样窗；未裁切窗=0/1 → 历史直通
        // 逐位一致，与 SoftRenderer 的 (px+0.5−x)×su 采样逐点对齐）
        // ================================================================
        const float x0 = static_cast<float>(di.x);
        const float y0 = static_cast<float>(di.y);
        const float x1 = static_cast<float>(di.x + di.w);
        const float y1 = static_cast<float>(di.y + di.h);
        const float gtop = static_cast<float>(canvas_h_) - y0; // 画面顶
        const float gbot = static_cast<float>(canvas_h_) - y1; // 画面底
        verts[0][0] = x0;  verts[0][1] = gtop;
        verts[0][2] = static_cast<float>(di.crop_u0);
        verts[0][3] = static_cast<float>(di.crop_v0);
        verts[1][0] = x0;  verts[1][1] = gbot;
        verts[1][2] = static_cast<float>(di.crop_u0);
        verts[1][3] = static_cast<float>(di.crop_v1);
        verts[2][0] = x1;  verts[2][1] = gtop;
        verts[2][2] = static_cast<float>(di.crop_u1);
        verts[2][3] = static_cast<float>(di.crop_v0);
        verts[3][0] = x1;  verts[3][1] = gbot;
        verts[3][2] = static_cast<float>(di.crop_u1);
        verts[3][3] = static_cast<float>(di.crop_v1);
        return true;
    }

    // ====================================================================
    // 仿射路径：顶点 = 内容四边形四角（clip 旋转四角映射到 comp 域），
    // uv = (0,0)(1,0)(0,1)(1,1) 直接对应。quad 与内容四边形精确重合。
    //   【2026-11-05 弃用旧 AABB 方案】旧实现 quad=AABB 四角 + uv=M(角)：
    //   AABB 内插值=M(p) 数学等价、内容位置正确，但 AABB 的四角区域
    //   （内容四边形之外）uv 越界 → CLAMP_TO_EDGE 拉伸纹理边缘像素，
    //   视觉上"内容溢出标注框"（联测定位：GL quad 与标注框外接矩形
    //   ±0.5px 逐位重合，偏移是伪影非错位）。改为内容四边形后越界区
    //   域不再存在，与 SoftRenderer"clip 矩形外不写"语义对齐。
    //   内容角换算与 ScreenMapper 四角公式同式：局部 (±hw,±hh) →
    //   R(θ)（y 向下顺时针）→ canvas → st 正向（inv_map 逆）→ comp。
    //   uv 对应已验证：局部 (−hw,−hh) 角 → M → frame(0,0)=uv(0,0)。
    //   顶点仍 gl_y = H − y 翻转。
    // ====================================================================
    // 防御（同 SoftRenderer：数据损坏宁缺勿错）
    for (int i = 0; i < 6; ++i)
        if (!std::isfinite(di.inv_map[i])) return false;
    if (!std::isfinite(di.canvas_x) || !std::isfinite(di.canvas_y) ||
        !std::isfinite(di.canvas_w) || !std::isfinite(di.canvas_h) ||
        !std::isfinite(di.rotation) ||
        di.canvas_w <= 0.0 || di.canvas_h <= 0.0)
        return false;

    // inv_map 正向（canvas→output）= 仿射逆：x_c = a·x_o + b·y_o + c，
    // y_c = d·x_o + e·y_o + f → [x_o; y_o] = [[e,−b],[−d,a]]/det · [x_c−c; y_c−f]
    const double ia = di.inv_map[0], ib = di.inv_map[1],
                 ic = di.inv_map[2], id = di.inv_map[3],
                 ie = di.inv_map[4], iff = di.inv_map[5];
    const double det = ia * ie - ib * id;
    if (!std::isfinite(det) || std::fabs(det) < 1e-12) return false;

    constexpr double kPi = 3.14159265358979323846;
    const double rad = di.rotation * kPi / 180.0;
    const double ct  = std::cos(rad), stn = std::sin(rad);
    // 旋转枢轴 = 锚点（2026-11-09；默认 0=中心与历史"绕中心"逐位一致——
    // ScreenMapper/SoftRenderer/画布同口径）
    double au = 0.5, av = 0.5;
    Transform::anchorUV(di.anchor, au, av);
    const double ccx = di.canvas_x + au * di.canvas_w;
    const double ccy = di.canvas_y + av * di.canvas_h;

    // 四角循环：i 的 bit0 选 u、bit1 选 v（uv 表与局部角一一对应）→
    // 顶点序 [LT, RT, LB, RB]。Transform::corner 序 = CornerIdx
    // [LT, RT, RB, LB] → 顶点 i 取位移经 kVtxToCorner 映射（LB/RB 互换）。
    // 旋转：未旋转角 → 相对【锚点】偏移 → R(θ) → + 锚点（p' = R·(p−A)+A，
    // 锚点在旋转中不动；anchor=中心时与历史"绕中心"逐位一致）+ 四角点
    // 位移（屏幕空间拖角语义——与 ScreenMapper buildDrawItemsForScreen
    // 同式，两端同源）
    static constexpr int kVtxToCorner[4] = {0, 1, 3, 2}; // 顶点 i → CornerIdx
    for (int i = 0; i < 4; ++i) {
        double wx, wy; // canvas 域角
        if (di.pose3d) {
            // 俯仰/偏航（v3 透视）：四角已由 cg::quadCorners3D 预投影
            // （ScreenMapper 填 di.quad，画布域 CornerIdx 序——滚转/
            // 锚点/角位移/透视缩放已烘焙），此处仅顶点序重排 + comp 映射。
            wx = di.quad[kVtxToCorner[i]][0];
            wy = di.quad[kVtxToCorner[i]][1];
        } else {
            // 2D 路径（原实现）：未旋转角 → 相对【锚点】偏移 → R(θ) →
            // + 锚点（p' = R·(p−A)+A，anchor=中心时与历史"绕中心"逐位
            // 一致）+ 四角点位移（屏幕空间拖角语义——与 ScreenMapper
            // buildDrawItemsForScreen 同式，两端同源）。
            const double vx = di.canvas_x + ((i & 1) ? di.canvas_w : 0.0);
            const double vy = di.canvas_y + ((i & 2) ? di.canvas_h : 0.0);
            const double rx = vx - ccx, ry = vy - ccy; // 相对锚点偏移
            wx = ccx + ct * rx - stn * ry
                    + di.corner[kVtxToCorner[i] * 2 + 0];
            wy = ccy + stn * rx + ct * ry
                    + di.corner[kVtxToCorner[i] * 2 + 1];
        }
        const double dx = wx - ic, dy = wy - iff;
        const double ox = ( ie * dx - ib * dy) / det;  // comp 域角
        const double oy = (-id * dx + ia * dy) / det;
        verts[i][0] = static_cast<float>(ox);
        verts[i][1] = static_cast<float>(canvas_h_) -
                      static_cast<float>(oy);          // y 翻转
        // uv 采样窗（v3 裁切）：bit0 选窗右、bit1 选窗下；未裁切窗=
        // 全帧 → 历史 0/1 直通逐位一致
        verts[i][2] = static_cast<float>((i & 1) ? di.crop_u1 : di.crop_u0);
        verts[i][3] = static_cast<float>((i & 2) ? di.crop_v1 : di.crop_v0);
    }
    return true;
}

// ---------------------------------------------------------------------------
// draw —— 单个绘制项（帧域分发：CpuRgba 常规路径 / GpuCudaNv12 0 拷贝路径）
// ---------------------------------------------------------------------------
bool GlRenderer::draw(const DrawItem& di, const FrameView& frame) {
    if (!inited_ || failed_) return false;
    if (!external_ && !ctx_.alive()) return false; // 窗口已关：静默跳过

    if (!frame.valid() || canvas_w_ <= 0 || canvas_h_ <= 0) return false;
    // NaN/Inf 防御（与 SoftRenderer 入口同构；两域共用）
    if (!std::isfinite(di.x) || !std::isfinite(di.y) ||
        !std::isfinite(di.w) || !std::isfinite(di.h))
        return false;
    if (di.w <= 0 || di.h <= 0) return false;

    // ---- 顶点构造（域无关；含仿射防御）-----------------------------------
    float verts[4][4];
    if (!buildVerts(di, frame.width, frame.height, verts)) return false;

    // 【诊断 MS_CANVAS_DBG=1】仿射 UV 追踪（内容偏移定位）：顶点=合成域
    // AABB 角（与标注框同源必然重合），内容偏移只能在 UV=M(角点)——此处
    // 打印 di 全参数 + M 复合中间量 + 4 顶点全值，与手算逐位比对。
    {
        static const bool s_dbg = std::getenv("MS_CANVAS_DBG") != nullptr;
        if (s_dbg && external_) {
            static int s_cnt = 0;
            if (++s_cnt <= 24) {
                std::fprintf(stderr,
                             "[gl-dbg] draw#%d aabb=(%.1f,%.1f %.1fx%.1f) "
                             "aff=%d rot=%.2f op=%.2f cv=(%.1f,%.1f %.1fx%.1f) "
                             "inv=[%.4f %.4f %.4f %.4f %.4f %.4f] "
                             "frame=%dx%d dom=%d canvas=%dx%d\n",
                             s_cnt, di.x, di.y, di.w, di.h, (int)di.affine,
                             di.rotation, di.opacity,
                             di.canvas_x, di.canvas_y, di.canvas_w, di.canvas_h,
                             di.inv_map[0], di.inv_map[1], di.inv_map[2],
                             di.inv_map[3], di.inv_map[4], di.inv_map[5],
                             frame.width, frame.height, (int)frame.domain,
                             canvas_w_, canvas_h_);
                for (int i = 0; i < 4; ++i)
                    std::fprintf(stderr,
                                 "  v%d=(%.2f,%.2f uv %.4f,%.4f)\n",
                                 i, verts[i][0], verts[i][1],
                                 verts[i][2], verts[i][3]);
                std::fflush(stderr);
            }
        }
    }

    // ---- 不透明度钳制（两域共用；全透明 = 合法矩形但无写入）--------------
    const double op = di.opacity < 0.0 ? 0.0 : (di.opacity > 1.0 ? 1.0 : di.opacity);
    if (op <= 0.0) return true; // 全透明：无写入但矩形合法（同 SoftRenderer）

    // ========================================================================
    // ③阶段：GpuCudaNv12 域 —— CUDA-GL interop 0 拷贝路径
    //   流程：interop 惰性探测（失败→翻旗标回退）→ 纹理对 ensure/注册 →
    //   D2D 拷贝（显存直进纹理）→ NV12 着色器绘制。任何一步失败都翻
    //   协商旗标：解码端下一帧起自动回退下载路径（无黑屏自愈）。
    //   （MS_GL_NO_CUDA：editor 目标整段编译掉——GPU 帧按未知域拒绝）
    // ========================================================================
#ifndef MS_GL_NO_CUDA
    if (frame.domain == FrameDomain::GpuCudaNv12) {
        // 【协商前置】无协商旗标 = 装配层未开启 GPU 交付（解码端不会发布
        // GPU 帧）——消费端对称拒绝，保持 FrameView 域纪律（②阶段防御
        // 契约的延续；发布端/消费端必须同为"已协商"状态）
        if (!gpu_delivery_ok_) return false;
        if (gpu_dead_) return false; // 已降级：快速失败（旗标已翻，勿重试）
        if (!ensureNv12Program()) {
            disableGpuDelivery("NV12 着色器构建失败");
            return false;
        }
        // interop 惰性初始化（GL 上下文 current 的渲染线程）：探测服务
        // 当前 GL 上下文的 CUDA 设备——GL 落在核显（双显卡常态）即失败
        if (!interop_.ready() && !interop_.init()) {
            disableGpuDelivery("GL 上下文不在 NVIDIA GPU（interop 探测 0 台）");
            return false;
        }
        // 纹理对 + CUDA 注册（首帧/尺寸变化触发；失败同样降级）
        if (!ensureNv12Textures(frame.width, frame.height)) {
            disableGpuDelivery("NV12 纹理创建/注册失败");
            return false;
        }
        // D2D 拷贝：AVFrame 显存指针 → 已注册纹理（map→memcpy→unmap）
        // 【双缓冲】按帧奇偶取当前组：map 本帧组时上一帧 draw 采样的是
        // 另一组（早已画完），隐式同步链断裂——map 从 ≈16ms 回亚毫秒。
        // 【诊断 MS_CANVAS_DBG=1】draw 慢细分第三层：uploadNv12 整体耗时
        // vs 着色器段耗时——interop 内部 map/copy/unmap 各段已证实 <8ms，
        // 若 upload 整体仍慢则为段间同步/驱动队列开销；若 upload 快而
        // gl 慢则为 NV12 着色器/纹理绑定/驱动提交问题。
        const int g = nv_parity_;
        const auto* avf = static_cast<const AVFrame*>(frame.native.get());
        const auto t_up0 = std::chrono::steady_clock::now();
        if (avf == nullptr || !interop_.uploadNv12(avf, g)) {
            disableGpuDelivery("NV12 D2D 拷贝失败");
            return false;
        }
        const auto t_up1 = std::chrono::steady_clock::now();
        const auto up_us = std::chrono::duration_cast<std::chrono::microseconds>(
                               t_up1 - t_up0).count();
        // NV12 程序绘制（采样器绑定惰性期已固定：Y=unit0、UV=unit1）
        gl::glUseProgram(program_nv12_);
        // u_outSize 必须在此设置：NV12 程序可能在本帧才首次构建
        // （ensureNv12Program），beginFrame 末尾的同步赶不上首帧——漏设
        // 则 ndc = pos/0 = NaN，全帧被裁剪（症状即"首帧黑、次帧起正常"）
        gl::glUniform2f(loc_nv_out_size_,
                        static_cast<float>(canvas_w_),
                        static_cast<float>(canvas_h_));
        gl::glActiveTexture(gl::GL_TEXTURE0);
        gl::glBindTexture(gl::GL_TEXTURE_2D, tex_y_[g]);
        gl::glActiveTexture(gl::GL_TEXTURE1);
        gl::glBindTexture(gl::GL_TEXTURE_2D, tex_uv_[g]);
        gl::glUniform2f(loc_nv_frame_size_,
                        static_cast<float>(frame.width),
                        static_cast<float>(frame.height));
        gl::glUniform1f(loc_nv_opacity_, static_cast<float>(op));
        drawQuad(verts);
        gl::glActiveTexture(gl::GL_TEXTURE0); // 活动单元复位（RGBA 路径约定 unit0）
        nv_parity_ ^= 1; // 组翻转：下一帧用另一组（双缓冲交替核心）
        // 【诊断】着色器段收口计时：upload 与 gl 两段同打，便于对照归因
        {
            static const bool s_probe = std::getenv("MS_CANVAS_DBG") != nullptr;
            if (s_probe) {
                const auto t_gl1 = std::chrono::steady_clock::now();
                const auto gl_us = std::chrono::duration_cast<std::chrono::microseconds>(
                                       t_gl1 - t_up1).count();
                if (up_us > 8000 || gl_us > 8000) {
                    std::fprintf(stderr,
                                 "[draw-probe] nv12 upload=%lldus gl=%lldus\n",
                                 static_cast<long long>(up_us),
                                 static_cast<long long>(gl_us));
                    std::fflush(stderr);
                }
            }
        }
        return true;
    }
#endif // !MS_GL_NO_CUDA
    if (frame.domain != FrameDomain::CpuRgba) return false; // 未知域防御

    // ========================================================================
    // CPU RGBA 域（②阶段既有语义；软解/硬解下载回退统一入口）
    // ========================================================================

    // ---- 纹理上传（单槽复用；尺寸变化时重建存储）------------------------
    gl::glBindVertexArray(vao_);
    gl::glActiveTexture(gl::GL_TEXTURE0);
    gl::glBindTexture(gl::GL_TEXTURE_2D, tex_);
    if (frame.width != tex_w_ || frame.height != tex_h_) {
        // 存储重建（stride 对齐按 RGBA 紧排语义由 ROW_LENGTH 处理）
        gl::glTexImage2D(gl::GL_TEXTURE_2D, 0, gl::GL_RGBA,
                         frame.width, frame.height, 0,
                         gl::GL_RGBA, gl::GL_UNSIGNED_BYTE, nullptr);
        // 滤波/包裹（双线性放大 + 边缘钳制；语义见头文件）
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MIN_FILTER,
                            gl::GL_LINEAR);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MAG_FILTER,
                            gl::GL_LINEAR);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_S,
                            gl::GL_CLAMP_TO_EDGE);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_T,
                            gl::GL_CLAMP_TO_EDGE);
        tex_w_ = frame.width;
        tex_h_ = frame.height;
    }
    // 行距解耦：FrameView.stride 允许 ≠ width*4（防御性设置；当前恒等）
    gl::glPixelStorei(gl::GL_UNPACK_ROW_LENGTH,
                      static_cast<int>(frame.stride / 4));
    gl::glTexSubImage2D(gl::GL_TEXTURE_2D, 0, 0, 0,
                        frame.width, frame.height,
                        gl::GL_RGBA, gl::GL_UNSIGNED_BYTE, frame.rgba);
    gl::glPixelStorei(gl::GL_UNPACK_ROW_LENGTH, 0);

    // ---- uniform：帧尺寸 + 不透明度（RGBA 程序）---------------------------
    // 【程序显式绑定 2026-11-05 图片"画出视频内容"根因修复】此前依赖
    // "beginFrame 已绑定 RGBA 程序"的帧级假设——但 NV12 项（cuda 0 拷贝
    // 视频）draw 时 glUseProgram(program_nv12_) 后从不切回：同帧内 NV12
    // 项之后绘制 CpuRgba 项（z 序视频 z=0 → 图片 z=10）时活动程序仍是
    // NV12——unit0 采到本 tex_ 的 R 通道当 Y、unit1 还挂着视频 UV 平面
    // （NV12 采样器绑定惰性期已固定），图片矩形画出"视频色度×图片亮度"
    // 混合体（E2E 四象限 logo 实证：位置精确吻合、内容=视频测试图）。
    // 该 bug 此前被"图片永不发布帧"（单图 seek 根因）掩盖——图片修好后
    // 即显形，也是用户"图像素材颜色错乱"的真正根因。修复：RGBA 路径
    // 显式 glUseProgram(program_)，两分支各自自足，不依赖绘制顺序。
    gl::glUseProgram(program_);
    gl::glUniform2f(loc_frame_size_,
                    static_cast<float>(frame.width),
                    static_cast<float>(frame.height));
    gl::glUniform1f(loc_opacity_, static_cast<float>(op));

    drawQuad(verts);
    return true;
}

// ---------------------------------------------------------------------------
// drawQuad —— 顶点上传 + 条带绘制（4 顶点 = 2 三角形）
//   VAO 绑定收口在此（RGBA/NV12 双路径共用；幂等——core profile 下
//   VAO=0 非法，不能依赖"别处已绑"的隐式状态）
// ---------------------------------------------------------------------------
void GlRenderer::drawQuad(const float verts[4][4]) {
    gl::glBindVertexArray(vao_);
    gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vbo_);
    // 【共享上下文防御 2026-11-05 —— 二次 paint 黑屏根因】ExternalCtx 下
    // 同一上下文还跑 QPainter（标注层）。Qt 的 GL 绘制引擎在 QOpenGLWidget
    // 上不持有自己的 VAO——它直接在"当前绑定 VAO"（恰为我们上一帧遗留的
    // vao_）上 enable/disable 自己的 attrib、绑自己的 buffer，end 后 attrib
    // 0/1 = disabled。而 attrib 状态存于 VAO 对象内部，重新 bind 不会恢复
    // ——下一帧 DrawArrays 时顶点数据全禁 → shader 收恒定内置值 → 4 顶点
    // 重合一点 → 零像素：无 GL 错误、glValidateProgram 通过、画面黑（本
    // 会话编辑器联测实证）。故 attrib 配置从 ensureInit 一次性改为每帧
    // 幂等重配（顶点布局常量，开销可忽略）。
    gl::glEnableVertexAttribArray(0); // a_pos（2 float，偏移 0）
    gl::glVertexAttribPointer(0, 2, gl::GL_FLOAT, gl::GL_FALSE,
                              4 * sizeof(float),
                              reinterpret_cast<const void*>(0));
    gl::glEnableVertexAttribArray(1); // a_uv（2 float，偏移 8 字节）
    gl::glVertexAttribPointer(1, 2, gl::GL_FLOAT, gl::GL_FALSE,
                              4 * sizeof(float),
                              reinterpret_cast<const void*>(8));
    // attrib2（a_alpha）：quad 直通无逐顶点 alpha——disable + 常量 1.0。
    // 必须每帧幂等执行：warpBlit（同 VAO）会 enable attrib2 指向 5-float
    // mesh 布局，状态残留在 VAO 内（同 QPainter 泄漏教训——attrib 状态存
    // 于 VAO 对象，重绑不恢复），缺省值 0 会把 quad 内容乘成黑。
    gl::glDisableVertexAttribArray(2);
    gl::glVertexAttrib1f(2, 1.0f);
    gl::glBufferData(gl::GL_ARRAY_BUFFER, 4 * 4 * sizeof(float), verts,
                     gl::GL_DYNAMIC_DRAW); // 孤儿式重分配（驱动端异步安全）
    // （2026-11-06 去冗余：此处原连续两次相同 glBufferData——第二次把刚
    // 孤儿化的存储再次孤儿化，纯复制粘贴残留；每帧热路径省一次上传。）
    // 【诊断 MS_CANVAS_DBG=1】绑定生效性确认（bind 调用本身可能无效——
    // 对象已删/上下文异常时静默失败）
    {
        static const bool s_dbg = std::getenv("MS_CANVAS_DBG") != nullptr;
        if (s_dbg && external_) {
            static int s_cnt = 0;
            ++s_cnt;
            if (s_cnt <= 12) {
                int bvao = -1, bvbo = -1, e0 = -1, bb0 = -1, e1 = -1,
                    bb1 = -1;
                gl::glGetIntegerv(gl::GL_VERTEX_ARRAY_BINDING, &bvao);
                gl::glGetIntegerv(gl::GL_ARRAY_BUFFER_BINDING, &bvbo);
                gl::glGetVertexAttribiv(0, gl::GL_VERTEX_ATTRIB_ARRAY_ENABLED,
                                        &e0);
                gl::glGetVertexAttribiv(
                    0, gl::GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &bb0);
                gl::glGetVertexAttribiv(1, gl::GL_VERTEX_ATTRIB_ARRAY_ENABLED,
                                        &e1);
                gl::glGetVertexAttribiv(
                    1, gl::GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING, &bb1);
                float uos[2] = {-1, -1}, uop = -1, ufs[2] = {-1, -1};
                gl::glGetUniformfv(program_, loc_out_size_, uos);
                gl::glGetUniformfv(program_, loc_opacity_, &uop);
                gl::glGetUniformfv(program_, loc_frame_size_, ufs);
                std::fprintf(stderr,
                             "[gl-dbg] drawq#%d vao_=%u alive=%u vbo_=%u "
                             "bound=(vao=%d vbo=%d) attr0=(en%d,bb%d) "
                             "attr1=(en%d,bb%d) u_out=(%.0f,%.0f) op=%.2f "
                             "u_frame=(%.0f,%.0f) verts0=(%.1f,%.1f) "
                             "err=0x%X\n",
                             s_cnt, vao_, gl::glIsVertexArray(vao_), vbo_,
                             bvao, bvbo, e0, bb0, e1, bb1, uos[0], uos[1],
                             uop, ufs[0], ufs[1], verts[0][0], verts[0][1],
                             gl::glGetError());
                std::fflush(stderr);
            }
        }
    }
    gl::glDrawArrays(gl::GL_TRIANGLE_STRIP, 0, 4);
    // 【诊断 MS_CANVAS_DBG=1】draw 后探针（时点②）——区分"draw 本身没
    // 画出"（此处黑）与"draw 画了但 endFrame 前被改"（此处有色）。
    // 逐项调用（多 item 多行）；探针点 = 内容域 (70%,40%)（clipB 图案区）。
    {
        static const bool s_dbg = std::getenv("MS_CANVAS_DBG") != nullptr;
        if (s_dbg && external_) {
            static int s_cnt = 0;
            if (++s_cnt <= 12) {
                const int px = vp_x_ + canvas_w_ * 7 / 10;
                const int py = vp_y_ + canvas_h_ * 4 / 10;
                unsigned char buf[4] = {};
                int rdisc = -1, bl = -1;
                gl::glGetIntegerv(static_cast<unsigned int>(0x8C89),
                                  &rdisc); // GL_RASTERIZER_DISCARD
                gl::glGetIntegerv(gl::GL_BLEND, &bl);
                gl::glReadPixels(px, py, 1, 1, gl::GL_RGBA,
                                 gl::GL_UNSIGNED_BYTE, buf);
                std::fprintf(stderr,
                             "[gl-dbg] post-draw#%d probe=(%u,%u,%u) "
                             "rdisc=%d blend=%d err=0x%X\n",
                             s_cnt, buf[0], buf[1], buf[2], rdisc, bl,
                             gl::glGetError());
                std::fflush(stderr);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// setWarp —— 输出变形网格注入（渲染线程每帧可调；内部比对去重）
// ---------------------------------------------------------------------------
void GlRenderer::setWarp(const WarpSpec& warp) {
    if (external_) return; // 预览画布恒 1:1 直显：ExternalCtx 模式静默忽略
    if (warp.mode == warp_.mode && warp.cols == warp_.cols &&
        warp.rows == warp_.rows && warp.pts == warp_.pts &&
        warp.uvs == warp_.uvs && warp.alphas == warp_.alphas)
        return;            // 每帧注入的成本收口：未变化零副作用
    warp_ = warp;
    warp_dirty_ = true;    // 变更标记（beginFrame 打一次性启用日志用）
}

// ---------------------------------------------------------------------------
// 实际输出表面尺寸（显示分辨率模型 2026-11-10）
// ---------------------------------------------------------------------------
int GlRenderer::outputWidth() const {
    // ExternalCtx：宿主自管视口，上层按 cfg 显式尺寸合成。
    // hidden 离屏对拍：语义是"窗口客户区跟随画布"（beginFrame 内
    // SetWindowPos 同步——readPixels 对拍 1:1 前提）；若此处返回客户区
    // 则与"画布跟随客户区"互为反向依赖死锁（初始默认客户区 960×540
    // 永不再被纠正到 screen 尺寸，对拍 BMP 尺寸断言即崩）——离屏对拍
    // 不参与显示分辨率模型，恒 0 走上层兜底（cfg 显式 > screen）。
    if (external_ || geom_.hidden) return 0;
    return ctx_.valid() ? ctx_.clientW() : 0; // 未建窗/已销毁 → 0 走兜底
}

int GlRenderer::outputHeight() const {
    if (external_ || geom_.hidden) return 0;
    return ctx_.valid() ? ctx_.clientH() : 0;
}

// ---------------------------------------------------------------------------
// warpActive —— 变形管线激活判定（任一条件不满足即直出降级）
// ---------------------------------------------------------------------------
bool GlRenderer::warpActive() const noexcept {
    // mode=2 的附加场尺寸校验已收口在 WarpSpec::active（给了就必须给够）
    return !external_ && !warp_failed_ && warp_.active();
}

// ---------------------------------------------------------------------------
// ensureFbo —— 离屏合成画布（RGBA 纹理 + 帧缓冲对象；尺寸变化重建）
// ---------------------------------------------------------------------------
bool GlRenderer::ensureFbo(int w, int h) {
    if (w <= 0 || h <= 0) return false;
    if (fbo_ != 0 && fbo_w_ == w && fbo_h_ == h) return true; // 已就绪

    if (fbo_tex_ == 0) gl::glGenTextures(1, &fbo_tex_);
    if (fbo_ == 0)     gl::glGenFramebuffers(1, &fbo_);

    // 颜色纹理：RGBA8（合成画布同构；双线性 + CLAMP——mesh 边缘采样平滑）
    gl::glBindTexture(gl::GL_TEXTURE_2D, fbo_tex_);
    gl::glTexImage2D(gl::GL_TEXTURE_2D, 0, gl::GL_RGBA, w, h, 0,
                     gl::GL_RGBA, gl::GL_UNSIGNED_BYTE, nullptr);
    gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MIN_FILTER,
                        gl::GL_LINEAR);
    gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MAG_FILTER,
                        gl::GL_LINEAR);
    gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_S,
                        gl::GL_CLAMP_TO_EDGE);
    gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_T,
                        gl::GL_CLAMP_TO_EDGE);

    gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, fbo_);
    gl::glFramebufferTexture2D(gl::GL_FRAMEBUFFER, gl::GL_COLOR_ATTACHMENT0,
                               gl::GL_TEXTURE_2D, fbo_tex_, 0);
    const unsigned int st = gl::glCheckFramebufferStatus(gl::GL_FRAMEBUFFER);
    if (st != gl::GL_FRAMEBUFFER_COMPLETE) {
        std::fprintf(stderr,
                     "[gl] 变形 FBO 创建失败（status=0x%X）——本会话降级"
                     "直出（无变形）\n", st);
        gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, 0);
        fbo_w_ = fbo_h_ = 0;
        warp_failed_ = true; // 永久降级：变形失败绝不黑屏
        return false;
    }
    fbo_w_ = w;
    fbo_h_ = h;
    return true;
}

// ---------------------------------------------------------------------------
// buildWarpMesh —— 变形网格顶点（非索引三角形：每 quad 6 顶点；5 float/顶点
//   = pos2 + uv2 + alpha1——attrib2 逐顶点融合，quad 直通路径用常量 1）
//   位置：节点归一化 [0,1]（y 向下）× 后备缓冲尺寸 → gl_y 翻转；
//   uv：mode=1 内容恒规则网格（行/列比例）——只移动节点位置、不重采样
//   内容；mode=2 查 WarpSpec.uvs——MPCDI 规范 warp uv 本就是 GL 纹理约定
//   （v=0=buffer 底），与场景 FBO 同域 → 直采不翻转（2026-11-11 实证：SC20
//   直采后内容正向；此前多加 1-v 翻转致内容垂直镜像）。无效映射 (-1,-1)：
//   u=-1 < -0.5 触发 discard → 露黑底，对齐 MPCDI "NaN 渲染黑"语义。
//   alpha：mode=2 查 WarpSpec.alphas（alphaMap 顶点采样值）；其余恒 1。
//   FBO 纹理的 GL 惯例 v=0 在底 → 内容顶（r=0）采 v=1。
// ---------------------------------------------------------------------------
void GlRenderer::buildWarpMesh(std::vector<float>& out) {
    out.clear();
    const int bw = ctx_.clientW();
    const int bh = ctx_.clientH();
    const int cols = warp_.cols;
    const int rows = warp_.rows;
    const float fw = static_cast<float>(bw);
    const float fh = static_cast<float>(bh);
    // mode=2 附加场（空 = 兼容 mode=1 语义：规则网格 / 全 1）
    const bool has_uv = warp_.mode == 2 &&
                        warp_.uvs.size() >= static_cast<std::size_t>(cols) *
                                                rows * 2;
    const bool has_alpha =
        warp_.mode == 2 &&
        warp_.alphas.size() >= static_cast<std::size_t>(cols) * rows;
    // 节点 (c,r) → {glx, gly, u, v, a}
    const auto node = [&](int c, int r, float v5[5]) {
        const std::size_t i =
            (static_cast<std::size_t>(r) * cols + c) * 2;
        const float px = static_cast<float>(warp_.pts[i]);     // [0,1]
        const float py = static_cast<float>(warp_.pts[i + 1]); // [0,1] y 向下
        v5[0] = px * fw;
        v5[1] = fh - py * fh;                                  // y 翻转
        if (has_uv) {
            // MPCDI warp uv 与场景 FBO 同为 GL 约定（v=0=底）→ 直采；
            // 无效 (-1,-1)：u=-1 < -0.5 触发 discard（黑底）
            v5[2] = static_cast<float>(warp_.uvs[i]);
            v5[3] = static_cast<float>(warp_.uvs[i + 1]);
        } else {
            v5[2] = cols > 1 ? static_cast<float>(c) / (cols - 1) : 0.0f;
            v5[3] = rows > 1
                        ? 1.0f - static_cast<float>(r) / (rows - 1)
                        : 1.0f;                            // 内容顶 = v 1
        }
        v5[4] = has_alpha
                    ? static_cast<float>(warp_.alphas[static_cast<
                          std::size_t>(r) * cols + c])
                    : 1.0f;
    };
    out.reserve(static_cast<std::size_t>(cols - 1) * (rows - 1) * 6 * 5);
    for (int r = 0; r + 1 < rows; ++r) {
        for (int c = 0; c + 1 < cols; ++c) {
            float tl[5], bl[5], br[5], tr[5];
            node(c, r, tl);
            node(c, r + 1, bl);
            node(c + 1, r + 1, br);
            node(c + 1, r, tr);
            // 两三角形（CULL 已禁用，绕序不敏感）
            out.insert(out.end(),
                       {tl[0], tl[1], tl[2], tl[3], tl[4],
                        bl[0], bl[1], bl[2], bl[3], bl[4],
                        br[0], br[1], br[2], br[3], br[4]});
            out.insert(out.end(),
                       {tl[0], tl[1], tl[2], tl[3], tl[4],
                        br[0], br[1], br[2], br[3], br[4],
                        tr[0], tr[1], tr[2], tr[3], tr[4]});
        }
    }
}

// ---------------------------------------------------------------------------
// warpBlit —— 变形呈现（解绑 FBO → mesh 贴 FBO 纹理到后备缓冲）
//   调用点：endFrame（SwapBuffers 前——正常呈现路径）与 readPixels
//   （诊断导帧在 endFrame 之前到达——RenderThread::maybeCapture 时序，
//   须先贴图才能读到变形后内容）。幂等：warped_this_frame_ 守卫。
// ---------------------------------------------------------------------------
void GlRenderer::warpBlit() {
    if (!warpActive() || warped_this_frame_) return;
    if (ctx_.clientW() <= 0 || ctx_.clientH() <= 0) return;

    // ---- 解绑 FBO → 后备缓冲，视口切到物理输出域 -------------------------
    gl::glBindFramebuffer(gl::GL_FRAMEBUFFER, 0);
    gl::glViewport(0, 0, ctx_.clientW(), ctx_.clientH());
    // 【残留 bug 2026-11-10】先清后备缓冲再贴 mesh——beginFrame 的
    // glClear 作用在 FBO（当时绑定态），BACK 从未被清：变形网格不满铺
    // （节点内收/异形）时 mesh 未覆盖区域保留历史帧像素 = 时间码重影、
    // 旧网格边缘逐帧堆积成"多层残影"（恒等满铺时全覆盖重写不显现，
    // 故此前未暴露）。输出底色恒黑（动画诊断色只进 FBO 清屏）。
    gl::glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    gl::glClear(gl::GL_COLOR_BUFFER_BIT);
    // 全覆盖不透明贴图：关闭混合（FBO 内容已按 alpha 合成完毕，此处是
    // 纯几何重映射；FBO 清屏底 alpha=1 → 输出 alpha 恒 1）
    gl::glDisable(gl::GL_BLEND);
    gl::glUseProgram(program_);
    // 投影域 = 后备缓冲（mesh 顶点坐标系）；discard 域 = FBO 尺寸（uv∈[0,1]
    // 恒在容差域内，discard 永不触发——uniform 仅保持语义自洽）
    gl::glUniform2f(loc_out_size_,
                    static_cast<float>(ctx_.clientW()),
                    static_cast<float>(ctx_.clientH()));
    gl::glUniform2f(loc_frame_size_,
                    static_cast<float>(fbo_w_), static_cast<float>(fbo_h_));
    gl::glUniform1f(loc_opacity_, 1.0f);
    gl::glActiveTexture(gl::GL_TEXTURE0);
    gl::glBindTexture(gl::GL_TEXTURE_2D, fbo_tex_);

    // ---- mesh 上传 + 绘制（attrib 幂等重配——同 drawQuad 防御纪律）------
    // 5 float/顶点 = pos2 + uv2 + alpha1（M4 MPCDI：attrib2 逐顶点融合）
    buildWarpMesh(warp_mesh_);
    gl::glBindVertexArray(vao_);
    gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vbo_);
    gl::glEnableVertexAttribArray(0); // a_pos（2 float，偏移 0）
    gl::glVertexAttribPointer(0, 2, gl::GL_FLOAT, gl::GL_FALSE,
                              5 * sizeof(float),
                              reinterpret_cast<const void*>(0));
    gl::glEnableVertexAttribArray(1); // a_uv（2 float，偏移 8 字节）
    gl::glVertexAttribPointer(1, 2, gl::GL_FLOAT, gl::GL_FALSE,
                              5 * sizeof(float),
                              reinterpret_cast<const void*>(8));
    gl::glEnableVertexAttribArray(2); // a_alpha（1 float，偏移 16 字节）
    gl::glVertexAttribPointer(2, 1, gl::GL_FLOAT, gl::GL_FALSE,
                              5 * sizeof(float),
                              reinterpret_cast<const void*>(16));
    gl::glBufferData(gl::GL_ARRAY_BUFFER,
                     static_cast<ptrdiff_t>(warp_mesh_.size() * sizeof(float)),
                     warp_mesh_.data(), gl::GL_DYNAMIC_DRAW);
    gl::glDrawArrays(gl::GL_TRIANGLES, 0,
                     static_cast<int>(warp_mesh_.size() / 5));

    // ---- 状态恢复（混合回到帧级契约；viewport 由下一帧 beginFrame 重设）--
    gl::glEnable(gl::GL_BLEND);
    warped_this_frame_ = true;
}

// ---------------------------------------------------------------------------
// endFrame —— 呈现 + 消息泵
// ---------------------------------------------------------------------------
void GlRenderer::endFrame() {
    if (!inited_ || failed_) return;
    if (external_) {
        // ExternalCtx 模式：呈现由 Qt（QOpenGLWidget 合成器）负责——
        // 只 flush 本帧 GL 命令（同一上下文内后续 QPainter 绘制天然有序，
        // flush 保底跨帧完整性），不 SwapBuffers、不泵消息。
        // 【诊断 MS_CANVAS_DBG=1】检查本帧 GL 错误位（Qt 上下文交叉污染
        // 探测——QPainter 与裸 GL 混用的状态泄漏会在此现形）
        {
            static const bool s_dbg = std::getenv("MS_CANVAS_DBG") != nullptr;
            if (s_dbg) {
                static int s_cnt = 0;
                const unsigned int e = gl::glGetError();
                // 回读探针：内容域中心 4x4 均值——区分"GL 合成没画出"
                // （此处黑）与"画出了但被后续 QPainter 抹掉"（此处有色）
                float r = 0, g = 0, b2 = 0;
                {
                    // 探针点 = 内容域 (70%, 40%)——测试场景下落在 clipB
                    // 图案区内（亮色；中心点可能撞 clipA 首帧黑区）
                    const int px = vp_x_ + canvas_w_ * 7 / 10;
                    const int py = vp_y_ + canvas_h_ * 4 / 10;
                    unsigned char buf[4 * 16] = {};
                    gl::glReadPixels(px - 2, py - 2, 4, 4, gl::GL_RGBA,
                                     gl::GL_UNSIGNED_BYTE, buf);
                    for (int i = 0; i < 16; ++i) {
                        r += buf[i * 4];
                        g += buf[i * 4 + 1];
                        b2 += buf[i * 4 + 2];
                    }
                    r /= 16.f; g /= 16.f; b2 /= 16.f;
                }
                ++s_cnt;
                if (e != 0 || s_cnt <= 30)  // 前 30 帧逐帧打印（点击触发的少数 repaint 不被限频吞掉）
                    std::fprintf(stderr,
                                 "[gl-dbg] frame#%d glErr=0x%X "
                                 "probe=(%.0f,%.0f,%.0f) vp=(%d,%d) "
                                 "canvas=%dx%d\n",
                                 s_cnt, e, r, g, b2, vp_x_, vp_y_,
                                 canvas_w_, canvas_h_);
                std::fflush(stderr);
            }
        }
        // 【诊断 MS_CANVAS_DBG=1】内容边界扫描（前 3 帧）：程序化定位
        // quad 实际渲染边界 vs 顶点数学预测（comp TL(514.6,25.2) → FBO
        // (602.4,28.6)；视口 (18,0,929,460)，fbo 1075x460，dpr=1）。
        // 预测：top-down y=60 行内容区 x∈[588,670]；x=620 列 y∈[37,270]。
        {
            static const bool s_dbg = std::getenv("MS_CANVAS_DBG") != nullptr;
            static int s_scan = 0;
            if (s_dbg && ++s_scan <= 2) {
                // 5 行全宽扫描：top-down y=80/140/200/260/320（gl_y=379..139）
                for (int row = 0; row < 5; ++row) {
                    const int ytop = 80 + row * 60;
                    const int gly = 460 - ytop - 1;
                    unsigned char buf[4 * 260] = {};
                    gl::glReadPixels(380, gly, 260, 1, gl::GL_RGBA,
                                     gl::GL_UNSIGNED_BYTE, buf);
                    std::fprintf(stderr, "[gl-dbg] scan y=%d:", ytop);
                    int run_start = -1, prev = -100;
                    for (int i = 0; i < 260; i += 2) {
                        const bool lit = buf[i * 4] + buf[i * 4 + 1] +
                                         buf[i * 4 + 2] > 45;
                        if (lit && run_start < 0) run_start = 380 + i;
                        if ((!lit || i == 256) && run_start >= 0) {
                            std::fprintf(stderr, " [%d..%d]", run_start,
                                         lit ? 380 + i : prev);
                            run_start = -1;
                        }
                        if (lit) prev = 380 + i;
                    }
                    std::fprintf(stderr, "\n");
                }
                std::fflush(stderr);
            }
        }
        gl::glFlush();
        return;
    }
    // ---- 输出变形呈现：FBO 内容按 mesh 贴到后备缓冲（SwapBuffers 前）----
    // warp 未激活/已贴图（readPixels 先到）时内部幂等跳过。
    warpBlit();
    if (!geom_.hidden && ctx_.alive()) {
        // 可见模式：呈现（DC 为 create 时持有的 CS_OWNDC，恒定句柄无泄漏）。
        // alive 检查：用户关窗后窗口/DC 已销毁，SwapBuffers 无谓失败。
        // 【时序说明 2026-11-04】诊断导帧（readPixels）现位于 endFrame
        // 之后（RenderThread::maybeCapture）——读到的是交换后的后备缓冲
        // （= 上一帧内容，EXCHANGE 语义）——诊断导帧 1s 节流，延迟一帧
        // 无碍；旧契约"导帧在 endFrame 之前"随呈现调用收口到本函数而
        // 作废（当时 endFrame 在 renderOnce 中根本未被调用——黑屏根因）。
        // alive 检查：用户关窗后窗口/DC 已销毁，SwapBuffers 无谓失败。
        // 【MMC 呈现模型 2026-11-04】恒双缓冲 + SwapBuffers + vsync(1)：
        // 呈现冻结排查实证单缓冲 glFlush 与三连招（glFinish+InvalidateRect+
        // DwmFlush）均无效，窗口恒首帧色；python 复刻矩阵（同格式 EXCHANGE
        // 双缓冲）全部正常——按 MMC PlayerWindow 模型统一 SwapBuffers 路径。
        // 【失败诊断 2026-11-04】SwapBuffers 返回值检查保留——后备缓冲有
        // 画面（readPixels 导帧实证）而屏幕恒黑时区分"交换链故障"与
        // "DWM 不提交"。返回 FALSE 打一次性醒目日志。
        const auto sw_t0 = std::chrono::steady_clock::now(); // 呈现耗时计时起点
        const BOOL sw_ok = SwapBuffers(ctx_.dc());
        // ---- 【呈现层病态自适应 2026-11-05】swap 耗时 EMA → 是否降级 ----
        // 判决指标用 EMA 而非"连续 N 帧超限"：病态场景（远程桌面/虚拟屏）
        // 往往是"十几帧正常 + 偶尔 250ms"的间歇卡顿——连续计数会被正常帧
        // 清零永远不触发，EMA 则稳步抬升到几十毫秒级，一次即判出。
        const std::int64_t sw_us = std::chrono::duration_cast<
            std::chrono::microseconds>(
            std::chrono::steady_clock::now() - sw_t0).count();
        ++swap_samples_;
        swap_ema_us_ = swap_ema_us_ == 0
                           ? sw_us
                           : swap_ema_us_ - swap_ema_us_ / 16 + sw_us / 16;
        if (!vsync_downgraded_ && swap_samples_ >= 30 &&
            swap_ema_us_ > 60'000 && ctx_.swapInterval() > 0) {
            // 环境显式指定过间隔 → 尊重用户，不自作主张
            if (std::getenv("MS_GL_SWAP_INTERVAL") == nullptr) {
                vsync_downgraded_ = ctx_.setSwapInterval(0);
                if (vsync_downgraded_) {
                    std::fprintf(
                        stderr,
                        "\n[gl] W-呈现层被 vsync 反压（swap 平均 %.1fms，"
                        "约合 %.0f fps）→ 已自动关闭 vsync，改由 FramePacer "
                        "控节拍\n"
                        "  常见成因：远程桌面 / 虚拟显示器 / Optimus 跨 GPU "
                        "呈现 / 驱动合成异常。\n"
                        "  若要强制保留 vsync：设 MS_GL_SWAP_INTERVAL=1 "
                        "重启（画面可能仍卡顿）。\n\n",
                        static_cast<double>(swap_ema_us_) / 1000.0,
                        swap_ema_us_ > 0 ? 1e6 / static_cast<double>(swap_ema_us_)
                                         : 0.0);
                    std::fflush(stderr);
                }
            }
        }
        if (sw_ok == FALSE && !swap_fail_logged_) {
            swap_fail_logged_ = true;
            std::fprintf(stderr,
                         "\n[gl] W-SwapBuffers 失败（错误 %lu）——后备缓冲"
                         "有画面但无法提交屏幕！\n"
                         "  可能原因：像素格式/DC 失效/驱动交换链异常；"
                         "可回退 --render-backend=soft\n\n",
                         static_cast<unsigned long>(GetLastError()));
            std::fflush(stderr);
        }
        // ---- 实际呈现帧率统计（MS_CANVAS_DBG=1 门控；5s 窗口节流）----
        // 【2026-11-05 新增】player 诊断心跳的 frame 是播放头换算值
        // （t/16.6ms），不能反映实际渲染——量化"卡顿"必须数 SwapBuffers。
        // 卡顿表现 = 本行 fps 显著低于素材/画布帧率（如 20-40）。
        {
            static const bool s_fps_dbg =
                std::getenv("MS_CANVAS_DBG") != nullptr;
            ++present_count_;
            ++present_total_;
            if (s_fps_dbg) {
                const auto now_ms = static_cast<std::int64_t>(
                    GetTickCount64());
                if (present_win_t0_ == 0) present_win_t0_ = now_ms;
                const auto elapsed = now_ms - present_win_t0_;
                if (elapsed >= 5000) {
                    std::fprintf(stderr,
                                 "[gl] 呈现帧率: %lld fps（5s 内 swap=%lld"
                                 "，累计 %lld）\n",
                                 present_count_ * 1000 / elapsed,
                                 present_count_, present_total_);
                    std::fflush(stderr);
                    present_count_ = 0;
                    present_win_t0_ = now_ms;
                }
            }
        }
    }
    ctx_.pumpMessages(); // 非阻塞：关窗/尺寸变化响应
}

// ---------------------------------------------------------------------------
// readPixels —— backbuffer 回读（bottom-up → top-down 翻转）
// ---------------------------------------------------------------------------
bool GlRenderer::readPixels(std::vector<std::uint8_t>& out_rgba,
                            int& out_w, int& out_h,
                            std::uint32_t& out_stride) {
    if (!inited_ || failed_) return false;
    // 【变形守卫】导帧时序在 endFrame 之前（RenderThread::maybeCapture）——
    // 变形激活时合成内容还在 FBO 里，先完成 mesh 贴图再读 BACK，诊断
    // 导帧才能拿到变形后的最终输出画面（幂等：endFrame 侧不再重复）。
    warpBlit();
    // ExternalCtx：读当前帧逻辑尺寸（canvas_w_/h_ = beginFrame 参数；
    // 宿主 FBO 尺寸归 Qt 管，导帧场景仅 editor 诊断用）
    const int w = external_ ? canvas_w_ : ctx_.clientW();
    const int h = external_ ? canvas_h_ : ctx_.clientH();
    if (w <= 0 || h <= 0) return false;
    if (!external_ && !ctx_.alive()) return false;

    std::vector<std::uint8_t> buf(static_cast<std::size_t>(w) * 4 * h);
    gl::glReadPixels(0, 0, w, h, gl::GL_RGBA, gl::GL_UNSIGNED_BYTE,
                     buf.data()); // GL 底行优先 → 需翻转为 top-down

    out_rgba.assign(buf.size(), 0);
    const std::size_t row_bytes = static_cast<std::size_t>(w) * 4;
    for (int y = 0; y < h; ++y) {
        std::memcpy(out_rgba.data() + static_cast<std::size_t>(y) * row_bytes,
                    buf.data() + static_cast<std::size_t>(h - 1 - y) * row_bytes,
                    row_bytes);
    }
    out_w     = w;
    out_h     = h;
    out_stride = static_cast<std::uint32_t>(row_bytes);
    return true;
}

} // namespace ms
