// ============================================================================
//  GlLoader.hpp —— 手写 WGL/GL 函数加载器（零第三方依赖的 GL 3.3 core）
//
//  设计依据：《hw-decode-implementation-plan.md》②-2（GL 上下文）
//    * 为什么手写而不用 GLAD/GLEW：显示端 exe 的交付原则是"零 UI/图形
//      库依赖"（安装包只有 exe + FFmpeg/core 运行时，预览窗连 Qt 都不用）
//      ——本工程只消费一个固定 GL 子集（3.3 core： textured-quad 合成），
//      X 宏表一次性声明全部所需函数指针，wglGetProcAddress 逐个填充，
//      代码量百行级、无任何外部构建依赖；
//    * 声明语法注记：函数指针声明必须把名字放进声明符中间
//      （ret (WINAPI* name)(params)）——故 X 宏为三段式 (名字, 返回类型,
//      参数表)，声明/定义/加载三处用同一张表展开（单一事实源）；
//    * 分层：GL 1.1 经典函数（glViewport/glClear/glTexImage2D 等）由
//      opengl32.lib 直接链接（wglGetProcAddress 对它们返回 NULL）；本表
//      只列 1.2+ 的函数（shader/program/VAO/VBO/blend-separate/uniform）；
//    * WGL 扩展（像素格式 ARB / 3.3 上下文 ARB / vsync EXT）同样经
//      wglGetProcAddress 获取——它们要求"已有当前上下文"才能取到，
//      所以加载时序为：临时窗口+1.1 上下文（取 WGL 扩展函数）→ 建
//      3.3 上下文 → makeCurrent → 加载 GL 函数表（见 GlContextWGL）。
//
//  线程契约：loadAll 在持有当前 GL 上下文的线程调用（渲染线程）；函数
//  指针为进程级只读（初始化后不变），跨线程只读访问安全。
// ============================================================================

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX // 防 min/max 宏污染 C++ 头
#endif

#include <windows.h>

#include <cstddef> // ptrdiff_t（glBufferData 签名）

namespace ms {
namespace gl {

// ---------------------------------------------------------------------------
// 函数指针表（X 宏三段式：X(名字, 返回类型, 参数表)——单一事实源，
// 声明/定义/加载三处共用；新增函数只改这里）
// ---------------------------------------------------------------------------

/// GL 1.1 经典函数（opengl32.dll 导出；【不能】走 wglGetProcAddress——
/// 硬件 ICD 驱动对 1.1 函数返回 NULL，必须经模块本体 GetProcAddress）
#define MS_GL11_FUNCS(X)                                                       \
    X(glViewport,      void,          (int, int, int, int))                    \
    X(glClearColor,    void,          (float, float, float, float))            \
    X(glClear,         void,          (unsigned int))                          \
    X(glEnable,        void,          (unsigned int))                          \
    X(glDisable,       void,          (unsigned int))                          \
    X(glColorMask,     void,          (unsigned char, unsigned char,          \
                                      unsigned char, unsigned char))          \
    X(glDepthMask,     void,          (unsigned char))                         \
    X(glReadPixels,    void,          (int, int, int, int, unsigned int,      \
                                      unsigned int, void*))                    \
    X(glPixelStorei,   void,          (unsigned int, int))                     \
    X(glTexParameteri, void,          (unsigned int, unsigned int, int))       \
    X(glTexImage2D,    void,          (unsigned int, int, int, int, int,      \
                                      int, unsigned int, unsigned int,        \
                                      const void*))                            \
    X(glTexSubImage2D, void,          (unsigned int, int, int, int, int,      \
                                      int, unsigned int, unsigned int,        \
                                      const void*))                            \
    X(glBindTexture,   void,          (unsigned int, unsigned int))            \
    X(glGenTextures,   void,          (int, unsigned int*))                    \
    X(glDeleteTextures, void,         (int, const unsigned int*))              \
    X(glDrawArrays,    void,          (unsigned int, int, int))                \
    X(glFinish,        void,          (void))                                  \
    X(glFlush,         void,          (void))                                  \
    X(glGetError,      unsigned int,  (void))                                  \
    X(glGetIntegerv,   void,          (unsigned int, int*))                    \
    X(glIsTexture,     unsigned char, (unsigned int))                          \
    X(glGetString,     const unsigned char*, (unsigned int))

/// GL 3.3 core 所需的 1.2+ 函数（经 wglGetProcAddress；依赖 current context）
#define MS_GL_EXT_FUNCS(X)                                                     \
    /* 2.0 着色器/程序 */                                                       \
    X(glCreateShader,          unsigned int, (unsigned int))                    \
    X(glShaderSource,          void, (unsigned int, int,                       \
                                      const char* const*, const int*))         \
    X(glCompileShader,         void, (unsigned int))                            \
    X(glGetShaderiv,           void, (unsigned int, unsigned int, int*))        \
    X(glGetShaderInfoLog,      void, (unsigned int, int, int*, char*))          \
    X(glDeleteShader,          void, (unsigned int))                            \
    X(glCreateProgram,         unsigned int, (void))                            \
    X(glAttachShader,          void, (unsigned int, unsigned int))              \
    X(glLinkProgram,           void, (unsigned int))                            \
    X(glGetProgramiv,          void, (unsigned int, unsigned int, int*))        \
    X(glGetProgramInfoLog,     void, (unsigned int, int, int*, char*))          \
    X(glValidateProgram,       void, (unsigned int))                            \
    X(glDeleteProgram,         void, (unsigned int))                            \
    X(glUseProgram,            void, (unsigned int))                            \
    X(glGetUniformLocation,    int,  (unsigned int, const char*))               \
    X(glUniform1f,             void, (int, float))                              \
    X(glUniform2f,             void, (int, float, float))                       \
    X(glUniform1i,             void, (int, int))                                \
    /* 3.0 VAO / 1.5 VBO */                                                    \
    X(glGenVertexArrays,       void, (int, unsigned int*))                      \
    X(glBindVertexArray,       void, (unsigned int))                            \
    X(glDeleteVertexArrays,    void, (int, const unsigned int*))                \
    X(glIsVertexArray,         unsigned char, (unsigned int))                   \
    X(glGetVertexAttribiv,     void, (unsigned int, unsigned int, int*))        \
    X(glGetUniformfv,          void, (unsigned int, int, float*))               \
    X(glGenBuffers,            void, (int, unsigned int*))                      \
    X(glBindBuffer,            void, (unsigned int, unsigned int))              \
    X(glBufferData,            void, (unsigned int, ptrdiff_t, const void*,    \
                                      unsigned int))                           \
    X(glDeleteBuffers,         void, (int, const unsigned int*))                \
    X(glVertexAttribPointer,   void, (unsigned int, int, unsigned int,         \
                                      unsigned char, int, const void*))        \
    X(glEnableVertexAttribArray, void, (unsigned int))                          \
    X(glDisableVertexAttribArray, void, (unsigned int))                         \
    X(glVertexAttrib1f,        void, (unsigned int, float))                     \
    /* 1.3/1.4 采样与混合（合成 alpha 语义的关键函数）*/                        \
    X(glActiveTexture,         void, (unsigned int))                            \
    X(glBlendFuncSeparate,     void, (unsigned int, unsigned int,              \
                                      unsigned int, unsigned int))             \
    /* 对象存活检查（诊断：QPainter glyph cache 生命周期误删实证）*/            \
    X(glIsBuffer,              unsigned char, (unsigned int))                   \
    X(glIsProgram,             unsigned char, (unsigned int))                   \
    /* 4.2/ARB_texture_storage（immutable 存储——CUDA interop 注册对 RG8 */     \
    /*  可变存储纹理返回 invalid value，immutable 是注册的稳妥口径）*/          \
    X(glTexStorage2D,          void, (unsigned int, int, unsigned int,         \
                                      int, int))                               \
    /* 3.0 帧缓冲对象（离屏合成——输出变形的两段式渲染：先 1:1 画进 */           \
    /*  FBO，呈现前按变形网格贴到输出；wglGetProcAddress 取核心名）*/            \
    X(glGenFramebuffers,       void, (int, unsigned int*))                     \
    X(glBindFramebuffer,       void, (unsigned int, unsigned int))             \
    X(glFramebufferTexture2D,  void, (unsigned int, unsigned int,              \
                                      unsigned int, unsigned int, int))        \
    X(glDeleteFramebuffers,    void, (int, const unsigned int*))               \
    X(glCheckFramebufferStatus, unsigned int, (unsigned int))

/// WGL 扩展函数（取用前必须已有当前上下文）
#define MS_WGL_FUNCS(X)                                                        \
    X(wglChoosePixelFormatARB,    int,   (HDC, const int*, const FLOAT*,       \
                                          unsigned int, int*, unsigned int*))  \
    X(wglCreateContextAttribsARB, HGLRC, (HDC, HGLRC, const int*))             \
    X(wglSwapIntervalEXT,         int,   (int))

// ---- 声明（extern ret (WINAPI* name) params;）------------------------------
#define MS_GL_DECLARE(name, ret, params) \
    extern ret (WINAPI* name) params;
MS_GL11_FUNCS(MS_GL_DECLARE)
MS_GL_EXT_FUNCS(MS_GL_DECLARE)
MS_WGL_FUNCS(MS_GL_DECLARE)
#undef MS_GL_DECLARE

// ---------------------------------------------------------------------------
// 加载（两阶段，配合 GlContextWGL 两步创建法）
// ---------------------------------------------------------------------------

/// 阶段①：仅加载 WGL 扩展函数（临时窗口 + 1.1 兼容上下文 current 时调用
/// ——wglChoosePixelFormatARB 等在任意上下文下均可获取）
/// \return false = WGL 扩展不可用（驱动过旧）——无法创建 3.3 上下文
bool loadWgl();

/// 阶段②：加载全部 GL 函数（1.1 模块导出 + 1.2+ 扩展；3.3 core 上下文
/// current 后调用——shader/VAO 等函数在 1.1 上下文下取不到）
/// \return false = 有关键函数取不到（驱动过旧/上下文非 3.3+）——调用方
///         应放弃 GL 路径回退软渲染（运行行为永远有兜底）
bool loadGl();

/// 常量（免 include 全套 GL 头；仅本工程消费的枚举值）
enum : unsigned int {
    GL_COMPILE_STATUS          = 0x8B81,
    GL_LINK_STATUS             = 0x8B82,
    GL_VALIDATE_STATUS         = 0x8B83,
    // attrib 状态查询（QPainter 交叉污染 attrib 定位用）
    GL_VERTEX_ATTRIB_ARRAY_ENABLED        = 0x8622,
    GL_VERTEX_ATTRIB_ARRAY_BUFFER_BINDING = 0x88F4,
    GL_VERTEX_SHADER           = 0x8B31,
    GL_FRAGMENT_SHADER         = 0x8B30,
    GL_ARRAY_BUFFER            = 0x8892,
    GL_STATIC_DRAW             = 0x88E4,
    GL_DYNAMIC_DRAW            = 0x88E8,
    GL_TEXTURE0                = 0x84C0,
    GL_TEXTURE_2D              = 0x0DE1,
    GL_RGBA                    = 0x1908,
    GL_UNSIGNED_BYTE           = 0x1401,
    GL_LINEAR                  = 0x2601,
    GL_NEAREST                 = 0x2600,
    GL_CLAMP_TO_EDGE           = 0x812F,
    GL_TEXTURE_WRAP_S          = 0x2802,
    GL_TEXTURE_WRAP_T          = 0x2803,
    GL_TEXTURE_MIN_FILTER      = 0x2801,
    GL_TEXTURE_MAG_FILTER      = 0x2800,
    GL_UNPACK_ROW_LENGTH       = 0x0CF2,
    GL_UNPACK_ALIGNMENT        = 0x0CF5,
    GL_SRC_ALPHA               = 0x0302,
    GL_ONE_MINUS_SRC_ALPHA     = 0x0303,
    GL_ONE                     = 1,
    GL_ZERO                    = 0,
    GL_BLEND                   = 0x0BE2,
    GL_COLOR_BUFFER_BIT        = 0x4000,
    // ---- 共享上下文防御（Qt QPainter 交叉污染重置用；beginFrame 每帧复位）--
    GL_DEPTH_TEST              = 0x0B71,
    GL_CULL_FACE               = 0x0B44,
    GL_SCISSOR_TEST            = 0x0C11,
    GL_STENCIL_TEST            = 0x0B90,
    // ---- 上下文状态诊断（QPainter 交叉污染对比；beginFrame dump 用）-------
    GL_DRAW_FRAMEBUFFER_BINDING = 0x8CA6, // 当前绘制 FBO（Qt widget FBO 非 0）
    GL_VERTEX_ARRAY_BINDING     = 0x85B5, // 绑定的 VAO（QPainter 遗留嫌疑）
    GL_ARRAY_BUFFER_BINDING     = 0x8894, // 绑定的 VBO
    GL_ACTIVE_TEXTURE           = 0x84E0, // 活动纹理单元（QPainter 可能停非 0）
    GL_CURRENT_PROGRAM          = 0x8B8D, // 当前 program
    GL_TRIANGLES               = 0x0004,
    GL_TRIANGLE_STRIP          = 0x0005,
    // ---- 离屏合成（FBO；输出变形两段式渲染 2026-11-09）---------------------
    GL_FRAMEBUFFER             = 0x8D40,             // 帧缓冲目标（绘制+读取）
    GL_COLOR_ATTACHMENT0       = 0x8CE0,             // 颜色附着点
    GL_FRAMEBUFFER_COMPLETE    = 0x8CD5,             // 完整性检查通过
    GL_FLOAT                   = 0x1406,
    GL_FALSE                   = 0,
    GL_VENDOR                  = 0x1F00,
    GL_RENDERER                = 0x1F01,
    GL_VERSION                 = 0x1F02,
    // ---- ③阶段 NV12 双平面纹理（单/双通道内部格式；GL 3.0+ core）------
    GL_R8                      = 0x8229, // 亮度纹理内部格式（单通道 8bit）
    GL_RED                     = 0x1903, // 亮度纹理上传格式
    GL_RG8                     = 0x822B, // 色度纹理内部格式（双通道 8bit）
    GL_RG                      = 0x8227, // 色度纹理上传格式（UV 交错）
    GL_TEXTURE1                = 0x84C1, // 色度采样器绑定单元
};

enum : int {
    // WGL_ARB_pixel_format 属性
    WGL_DRAW_TO_WINDOW_ARB     = 0x2001,
    WGL_SUPPORT_OPENGL_ARB     = 0x2010,
    WGL_DOUBLE_BUFFER_ARB      = 0x2011,
    // 硬件加速请求（0x2003 属性 + 0x2027 取值）——对齐 MPV_Media_Console
    // PlayerWindow 的 ARB 请求清单（强制硬件加速格式，拒 MCD/软渲染格式）
    WGL_ACCELERATION_ARB       = 0x2003,
    WGL_PIXEL_TYPE_ARB         = 0x2013,
    WGL_COLOR_BITS_ARB         = 0x2014,
    WGL_ALPHA_BITS_ARB         = 0x201B,
    WGL_DEPTH_BITS_ARB         = 0x2022,
    WGL_NO_ACCELERATION_ARB    = 0x2025,
    WGL_GENERIC_ACCELERATION_ARB = 0x2026,
    WGL_FULL_ACCELERATION_ARB  = 0x2027,
    WGL_TYPE_RGBA_ARB          = 0x202B,
    // WGL_ARB_pixel_format 呈现方式（swap method）——仅用于 SetPixelFormat
    // 后 DescribePixelFormat 取证打印（驱动实际给到 COPY/EXCHANGE 与否）
    WGL_SWAP_METHOD_ARB        = 0x2007,
    WGL_SWAP_UNDEFINED_ARB     = 0x202A,
    WGL_SWAP_COPY_ARB          = 0x2028,
    WGL_SWAP_EXCHANGE_ARB      = 0x2029,
    // WGL_ARB_create_context 属性
    WGL_CONTEXT_MAJOR_VERSION_ARB    = 0x2091,
    WGL_CONTEXT_MINOR_VERSION_ARB    = 0x2092,
    WGL_CONTEXT_FLAGS_ARB            = 0x2094,
    WGL_CONTEXT_PROFILE_MASK_ARB     = 0x9126,
    WGL_CONTEXT_CORE_PROFILE_BIT_ARB = 0x00000001,
};

} // namespace gl
} // namespace ms
