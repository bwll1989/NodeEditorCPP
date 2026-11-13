// ============================================================================
//  GlLoader.cpp —— GL/WGL 函数指针填充实现（X 宏表逐个 wglGetProcAddress）
// ============================================================================

#include "GlLoader.hpp"

#include <cstdio>

namespace ms {
namespace gl {

// ---- 函数指针定义（初始为空；loadAll 填充。定义语法同声明：名字在
//      声明符中间——ret (WINAPI* name) params）-----------------------------
#define MS_GL_DEFINE(name, ret, params) \
    ret (WINAPI* name) params = nullptr;
MS_GL11_FUNCS(MS_GL_DEFINE)
MS_GL_EXT_FUNCS(MS_GL_DEFINE)
MS_WGL_FUNCS(MS_GL_DEFINE)
#undef MS_GL_DEFINE

bool loadWgl() {
    // ---- WGL 扩展：wglGetProcAddress（任意上下文 current 即可取）---------
    bool ok = true;
#define MS_WGL_LOAD(name, ret, params)                                         \
    name = reinterpret_cast<ret (WINAPI*) params>(wglGetProcAddress(#name));   \
    if (name == nullptr) {                                                     \
        std::fprintf(stderr, "[gl] 缺少 WGL 扩展函数: %s\n", #name);            \
        ok = false;                                                            \
    }
    MS_WGL_FUNCS(MS_WGL_LOAD)
#undef MS_WGL_LOAD
    return ok;
}

bool loadGl() {
    bool ok = true;

    // ---- GL 1.1 经典函数：opengl32.dll 模块本体 ---------------------------
    // 【关键】硬件 ICD 驱动下 wglGetProcAddress 对 1.1 函数返回 NULL——
    // 1.1 导出在 opengl32.dll 本体（进程已链接 → GetModuleHandle 必得）
    const HMODULE ogl32 = GetModuleHandleW(L"opengl32.dll");
    if (ogl32 == nullptr) {
        std::fprintf(stderr, "[gl] opengl32.dll 模块句柄获取失败\n");
        return false;
    }
#define MS_GL11_LOAD(name, ret, params)                                        \
    name = reinterpret_cast<ret (WINAPI*) params>(                             \
        GetProcAddress(ogl32, #name));                                         \
    if (name == nullptr) {                                                     \
        std::fprintf(stderr, "[gl] 缺少 GL 1.1 函数: %s\n", #name);             \
        ok = false;                                                            \
    }
    MS_GL11_FUNCS(MS_GL11_LOAD)
#undef MS_GL11_LOAD

    // ---- GL 1.2+ 扩展函数：wglGetProcAddress（需 3.3 上下文 current）------
#define MS_GL_EXT_LOAD(name, ret, params)                                      \
    name = reinterpret_cast<ret (WINAPI*) params>(wglGetProcAddress(#name));   \
    if (name == nullptr) {                                                     \
        std::fprintf(stderr, "[gl] 缺少 GL 函数: %s（驱动/上下文过旧？）\n",    \
                     #name);                                                   \
        ok = false;                                                            \
    }
    MS_GL_EXT_FUNCS(MS_GL_EXT_LOAD)
#undef MS_GL_EXT_LOAD

    return ok;
}

} // namespace gl
} // namespace ms
