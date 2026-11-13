// ============================================================================
//  GlContextWGL.cpp —— Win32 窗口 + WGL 3.3 core 上下文实现
// ============================================================================

#include "GlContextWGL.hpp"

#include <cstdio>
#include <cstdlib>
#include <vector>

#include "GlLoader.hpp"

namespace ms {

// ---------------------------------------------------------------------------
// 内部辅助：窗口类注册（CS_OWNDC——像素格式绑定窗口，每窗口独立 DC）
// ---------------------------------------------------------------------------
namespace {

constexpr wchar_t kWindowClass[] = L"MediaServerGLWindow";

bool ensureWindowClass() {
    WNDCLASSEXW wc{};
    wc.cbSize = sizeof(wc);
    static bool registered = false;
    if (registered) return true;
    // MMC 同款样式：CS_HREDRAW|CS_VREDRAW（resize 全重绘）+ CS_OWNDC
    //（每窗口独立 DC——像素格式专属，DC 句柄可长期缓存）
    wc.style         = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wc.lpfnWndProc   = GlContextWGL::wndProc;    // 尺寸跟踪/关闭标记（见下）
    wc.hInstance     = GetModuleHandleW(nullptr);
    wc.hCursor       = LoadCursor(nullptr, IDC_ARROW); // TCHAR 宏（兼容 ANSI 构建域）
    // 黑色背景刷（MMC 同款）：窗口 resize/遮挡移开瞬间新暴露的客户区被
    // 系统刷黑，而不是未定义内容（DWM 对未定义区域常填白——"白边/白屏闪现"
    // 的根源）；常规帧由 GL 满视口覆盖，此刷只在 resize 瞬间生效。
    wc.hbrBackground = reinterpret_cast<HBRUSH>(GetStockObject(BLACK_BRUSH));
    wc.lpszClassName = kWindowClass;
    if (RegisterClassExW(&wc) == 0) {
        const DWORD err = GetLastError();
        if (err != ERROR_CLASS_ALREADY_EXISTS) {
            std::fprintf(stderr, "[gl] RegisterClass 失败: %lu\n",
                         static_cast<unsigned long>(err));
            return false;
        }
    }
    registered = true;
    return true;
}

/// 1.1 兼容/兜底像素格式（ChoosePixelFormat 路径；无 ARB 依赖）
/// 【MMC 对齐 2026-11-04】恒双缓冲（单缓冲兼容模式已随呈现冻结排查移除）
bool setLegacyPixelFormat(HDC hdc) {
    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize    = sizeof(pfd);
    pfd.nVersion = 1;
    pfd.dwFlags  = PFD_DRAW_TO_WINDOW | PFD_SUPPORT_OPENGL | PFD_DOUBLEBUFFER;
    pfd.iPixelType = PFD_TYPE_RGBA;
    pfd.cColorBits = 24;
    pfd.cDepthBits = 24;
    const int fmt = ChoosePixelFormat(hdc, &pfd);
    if (fmt == 0) return false;
    return SetPixelFormat(hdc, fmt, &pfd) != FALSE;
}

} // namespace

// ---------------------------------------------------------------------------
// 显示器枚举（与 PreviewWindowWin32 同语义：EnumDisplayMonitors 枚举序）
// ---------------------------------------------------------------------------
BOOL CALLBACK GlContextWGL::monitorEnumProc(HMONITOR, HDC, LPRECT rect,
                                            LPARAM lparam) {
    auto* monitors = reinterpret_cast<std::vector<RECT>*>(lparam);
    monitors->push_back(*rect);
    return TRUE; // 继续枚举
}

// ---------------------------------------------------------------------------
// 正式窗口过程：尺寸跟踪 + 关闭标记（GL 渲染线程的消息泵入口）
// ---------------------------------------------------------------------------
LRESULT CALLBACK GlContextWGL::wndProc(HWND hwnd, UINT msg, WPARAM wp,
                                       LPARAM lp) {
    auto* self = reinterpret_cast<GlContextWGL*>(
        GetWindowLongPtrW(hwnd, GWLP_USERDATA));
    switch (msg) {
    case WM_SIZE:
        // 最小化（0×0）不更新——恢复时 WM_SIZE 会再来
        if (self != nullptr && wp != SIZE_MINIMIZED) {
            self->client_w_ = LOWORD(lp);
            self->client_h_ = HIWORD(lp);
        }
        return 0;
    case WM_CLOSE:
        if (self != nullptr) self->alive_ = false;
        // 【关窗即退出】用户关闭输出窗 → 置进程退出旗标（display 主循环
        // 轮询 → 优雅退出链）。见 userClosedFlag() 注释。
        userClosedFlag().store(true);
        DestroyWindow(hwnd); // 关窗即销毁（合成端 alive_ 感知后跳过绘制）
        return 0;
    case WM_DESTROY:
        if (self != nullptr) self->alive_ = false;
        userClosedFlag().store(true); // 销毁路径同置位（系统注销等）
        return 0;
    default:
        return DefWindowProcW(hwnd, msg, wp, lp);
    }
}

// ---------------------------------------------------------------------------
// create —— 两步法总编排
// ---------------------------------------------------------------------------
bool GlContextWGL::create(const WindowGeom& geom) {
    if (valid()) return true; // 幂等防御
    geom_ = geom;
    alive_ = true;

    if (!ensureWindowClass()) return false;

    // ---- 第①步：临时窗口 + 1.1 上下文（取 WGL 扩展函数）----------------
    if (!createTempWindow()) {
        destroy();
        return false;
    }
    if (wglMakeCurrent(temp_hdc_, temp_hglrc_) == FALSE) {
        std::fprintf(stderr, "[gl] 临时上下文 makeCurrent 失败\n");
        destroy();
        return false;
    }
    // 扩展函数指针填充（阶段① loadWgl：仅 WGL 扩展——shader 等 GL 1.2+
    // 函数在 1.1 兼容上下文下取不到，留到阶段②正式上下文再加载）
    const bool wgl_ext_ok = gl::loadWgl() &&
                            gl::wglCreateContextAttribsARB != nullptr;
    if (!wgl_ext_ok) {
        std::fprintf(stderr, "[gl] WGL 扩展不可用（驱动过旧？）回退软渲染\n");
        wglMakeCurrent(nullptr, nullptr);
        destroy();
        return false;
    }

    // ---- 第②步：正式窗口 + ARB 像素格式 + 3.3 core 上下文 --------------
    if (!createRenderWindow()) {
        wglMakeCurrent(nullptr, nullptr);
        destroy();
        return false;
    }
    if (wglMakeCurrent(hdc_, hglrc_) == FALSE) {
        std::fprintf(stderr, "[gl] 正式上下文 makeCurrent 失败\n");
        destroy();
        return false;
    }

    // ---- GL 函数表填充（阶段② loadGl：3.3 上下文 current 后才有效）------
    if (!gl::loadGl()) {
        std::fprintf(stderr, "[gl] GL 3.3 函数加载失败（驱动过旧？）回退软渲染\n");
        destroy();
        return false;
    }
    const char* ver = reinterpret_cast<const char*>(
        gl::glGetString(gl::GL_VERSION));
    const char* ren = reinterpret_cast<const char*>(
        gl::glGetString(gl::GL_RENDERER));
    std::printf("[gl] 上下文就绪: %s | %s（vsync 开，MMC 呈现模型）\n",
                ver ? ver : "?", ren ? ren : "?");

    // ---- vsync 开启（MMC 同款呈现模型 2026-11-04）--------------------------
    // SwapBuffers 阻塞节流渲染循环；帧产出节奏仍由 FramePacer 控制，两者
    // 并存（Pacer 决定"何时画"，vsync 决定"何时上屏"——至多叠加一个刷新
    // 周期延迟）。曾用 vsync(0) 追求栅格相位对齐，但呈现冻结排查实证
    // 单/双缓冲在 vsync 关闭路径下均不上屏，弃用。
    // 【诊断开关 2026-11-05】MS_GL_SWAP_INTERVAL=0|1 覆盖该默认值：
    // 现场排查"SwapBuffers 单帧阻塞上百 ms（渲染循环被呈现层节流）"时，
    // 用它一次对照即可区分"呈现层节流"与"合成/解码慢"。
    int swap_interval = 1;
    if (const char* v = std::getenv("MS_GL_SWAP_INTERVAL"))
        swap_interval = std::atoi(v);
    setSwapInterval(swap_interval);
    if (swap_interval != 1)
        std::printf("[gl] vsync 间隔被 MS_GL_SWAP_INTERVAL 覆盖为 %d\n",
                    swap_interval);

    return true;
}

// ---------------------------------------------------------------------------
// 运行期 vsync 切换（2026-11-05）
//   背景：vsync(1) 的语义是"SwapBuffers 等到下一次垂直同步"。正常情况下
//   单帧 ≤16.7ms；但远程桌面 / 虚拟显示器 / Optimus 跨 GPU 呈现链路下，
//   合成器可能迟迟不消费帧，SwapBuffers 实测阻塞 250ms 级 → 渲染循环被
//   呈现层反压成 ~5fps（editor 改了、display 半天不动）。
//   本函数供 GlRenderer 在测到该病态后降级为 0（节拍交还 FramePacer）。
// ---------------------------------------------------------------------------
bool GlContextWGL::setSwapInterval(int interval) {
    if (!gl::wglSwapIntervalEXT) return false;
    if (interval == swap_interval_) return true;
    gl::wglSwapIntervalEXT(interval);
    swap_interval_ = interval;
    return true;
}

// ---------------------------------------------------------------------------
// 临时窗口（两步法第①步）：最小开销，只为 wglGetProcAddress 提供上下文
// ---------------------------------------------------------------------------
bool GlContextWGL::createTempWindow() {
    // 临时窗口用独立类名（WS_OVERLAPPED 不显示——不闪黑框）
    WNDCLASSEXW wc{};
    wc.cbSize         = sizeof(wc);
    wc.style          = CS_OWNDC;
    wc.lpfnWndProc    = DefWindowProcW;
    wc.hInstance      = GetModuleHandleW(nullptr);
    wc.lpszClassName  = L"MediaServerGLTemp";
    RegisterClassExW(&wc); // 重复注册失败可忽略（进程内已注册）

    temp_hwnd_ = CreateWindowExW(0, L"MediaServerGLTemp", L"",
                                 WS_OVERLAPPED, 0, 0, 8, 8,
                                 nullptr, nullptr,
                                 GetModuleHandleW(nullptr), nullptr);
    if (temp_hwnd_ == nullptr) return false;
    temp_hdc_ = GetDC(temp_hwnd_);
    if (temp_hdc_ == nullptr) return false;
    if (!setLegacyPixelFormat(temp_hdc_)) return false;
    temp_hglrc_ = wglCreateContext(temp_hdc_);
    return temp_hglrc_ != nullptr;
}

// ---------------------------------------------------------------------------
// 正式窗口（两步法第②步）：几何定位 + ARB 像素格式 + 3.3 core 上下文
// ---------------------------------------------------------------------------
bool GlContextWGL::createRenderWindow() {
    // ---- 窗口几何（语义复刻 PreviewWindowWin32：fullscreen 铺满目标屏；
    //      window 模式 screen 基准屏内偏移/居中，默认 960x540）------------
    int win_x = 0, win_y = 0, win_w = 960, win_h = 540;
    if (geom_.w > 0) win_w = geom_.w;
    if (geom_.h > 0) win_h = geom_.h;

    std::vector<RECT> monitors;
    EnumDisplayMonitors(nullptr, nullptr, monitorEnumProc,
                        reinterpret_cast<LPARAM>(&monitors));
    RECT monitor_rect{};
    bool have_monitor = false;
    if (geom_.screen >= 0) {
        if (geom_.screen < static_cast<int>(monitors.size())) {
            monitor_rect = monitors[static_cast<std::size_t>(geom_.screen)];
            have_monitor = true;
        } else {
            std::fprintf(stderr,
                         "[gl] screen=%d 越界（本机 %d 块显示器），回退默认屏\n",
                         geom_.screen, static_cast<int>(monitors.size()));
        }
    } else if (geom_.fullscreen) {
        // fullscreen 未指定屏 → 主屏
        MONITORINFO mi{sizeof(mi)};
        if (GetMonitorInfoW(MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY),
                            &mi)) {
            monitor_rect = mi.rcMonitor;
            have_monitor = true;
        }
    }
    if (have_monitor && geom_.fullscreen) {
        // 无边框铺满：客户区 = 屏矩形（WS_POPUP 无边框无标题栏）
        win_x = monitor_rect.left;
        win_y = monitor_rect.top;
        win_w = monitor_rect.right - monitor_rect.left;
        win_h = monitor_rect.bottom - monitor_rect.top;
    } else if (have_monitor && (geom_.x >= 0 || geom_.y >= 0)) {
        // 屏内偏移（x/y 任一显式给出即按偏移语义；未给的轴居中）
        const int mon_w = monitor_rect.right - monitor_rect.left;
        const int mon_h = monitor_rect.bottom - monitor_rect.top;
        win_x = geom_.x >= 0 ? monitor_rect.left + geom_.x
                             : monitor_rect.left + (mon_w - win_w) / 2;
        win_y = geom_.y >= 0 ? monitor_rect.top + geom_.y
                             : monitor_rect.top + (mon_h - win_h) / 2;
    } else if (have_monitor) {
        // 指定屏未给位置 → 屏内居中
        const int mon_w = monitor_rect.right - monitor_rect.left;
        const int mon_h = monitor_rect.bottom - monitor_rect.top;
        win_x = monitor_rect.left + (mon_w - win_w) / 2;
        win_y = monitor_rect.top + (mon_h - win_h) / 2;
    } else {
        win_x = CW_USEDEFAULT; // 系统默认
        win_y = 0;
    }

    // hidden（离屏对拍测试）：WS_POPUP 无边框——客户区 == 窗口尺寸（
    // beginFrame 按画布尺寸 SetWindowPos 时客户区精确对齐，对拍逐像素
    // 才能成立）；窗口不显示，渲染照常（GL 不依赖可见性）
    // 【无边框 2026-11-04 用户决策】borderless=true 的 window 模式与
    // fullscreen 同用 WS_POPUP（无系统装饰），区别仅在几何：
    // fullscreen 铺满目标屏；borderless 按 window_* 配置定位（缺省 960x540
    // 屏内居中）。关闭靠 Alt+F4 / 任务栏右键（WM_CLOSE → alive_ 感知）。
    const DWORD style = (geom_.hidden || geom_.fullscreen || geom_.borderless)
                            ? WS_POPUP
                            : (WS_OVERLAPPEDWINDOW | WS_CAPTION);
    // hidden（离屏对拍测试）：不显示窗口，渲染照常（GL 不依赖可见性）
    const DWORD ex_style = 0;
    hwnd_ = CreateWindowExW(ex_style, kWindowClass,
                            geom_.title.c_str(), style,
                            win_x, win_y, win_w, win_h,
                            nullptr, nullptr,
                            GetModuleHandleW(nullptr), nullptr);
    if (hwnd_ == nullptr) {
        std::fprintf(stderr, "[gl] CreateWindow 失败: %lu\n",
                     static_cast<unsigned long>(GetLastError()));
        return false;
    }
    // GWLP_USERDATA 绑定 this（wndProc 取用；须在首个消息前完成）
    SetWindowLongPtrW(hwnd_, GWLP_USERDATA,
                      reinterpret_cast<LONG_PTR>(this));

    // Win11 DWM 默认给 borderless 窗口裁圆角（全屏四角露出下层桌面，浅色
    // 桌面上呈"白角"）。DONOTROUND 关闭；dwmapi 按需动态加载，旧系统无此
    // 属性时静默跳过（MMC 同款处理）。
    if (HMODULE dwm = LoadLibraryW(L"dwmapi.dll")) {
        using DwmSetAttr_t = HRESULT (WINAPI*)(HWND, DWORD, LPCVOID, DWORD);
        auto set_attr = reinterpret_cast<DwmSetAttr_t>(
            reinterpret_cast<void*>(GetProcAddress(dwm, "DwmSetWindowAttribute")));
        if (set_attr) {
            constexpr int kDwmwaWindowCornerPreference = 33; // DWMWA_WINDOW_CORNER_PREFERENCE
            constexpr int kDwmcpDoNotRound = 1;              // DWMWCP_DONOTROUND
            const int pref = kDwmcpDoNotRound;
            set_attr(hwnd_, kDwmwaWindowCornerPreference, &pref, sizeof(pref));
        }
        FreeLibrary(dwm);
    }

    hdc_ = GetDC(hwnd_);
    if (hdc_ == nullptr) return false;

    // ---- ARB 像素格式（MMC 同款：RGBA + 硬件加速 + 恒双缓冲）------------
    // 【2026-11-04 对齐 MPV_Media_Console PlayerWindow】属性清单与 MMC 完全
    // 一致：DRAW_TO_WINDOW/SUPPORT_OPENGL/DOUBLE_BUFFER=1/RGBA/硬件加速/
    // COLOR_BITS=24/ALPHA=8/DEPTH=24。此前附加的 WGL_SWAP_METHOD=SWAP_COPY
    // 请求与单缓冲模式均已移除——呈现冻结排查实证：NVIDIA 572.61 忽略
    // SWAP_COPY 仍给 EXCHANGE，python 复刻矩阵在 EXCHANGE 格式上全部正常
    // 持续上屏；单缓冲 glFlush 路线同样冻结。swap method 交给驱动默认
    // （MMC 即如此，AMD/NVIDIA 双机长期验证）。
    const int pf_attribs[] = {
        gl::WGL_DRAW_TO_WINDOW_ARB,  1,
        gl::WGL_SUPPORT_OPENGL_ARB,  1,
        gl::WGL_DOUBLE_BUFFER_ARB,   1,
        gl::WGL_PIXEL_TYPE_ARB,      gl::WGL_TYPE_RGBA_ARB,
        gl::WGL_ACCELERATION_ARB,    gl::WGL_FULL_ACCELERATION_ARB,
        gl::WGL_COLOR_BITS_ARB,      24,
        gl::WGL_ALPHA_BITS_ARB,      8,
        gl::WGL_DEPTH_BITS_ARB,      24,
        0, 0,
    };
    int pix_fmt = 0;
    unsigned int num_fmts = 0;
    if (gl::wglChoosePixelFormatARB(hdc_, pf_attribs, nullptr, 1,
                                    &pix_fmt,
                                    &num_fmts) == FALSE || num_fmts == 0) {
        // ARB 请求失败（个别驱动不支持该组合）→ legacy 像素格式兜底
        std::fprintf(stderr, "[gl] wglChoosePixelFormatARB 失败/0 匹配——"
                             "回退 legacy 像素格式\n");
        if (!setLegacyPixelFormat(hdc_)) return false;
        pix_fmt = GetPixelFormat(hdc_);
        if (pix_fmt == 0) return false;
        num_fmts = 1;
    }
    PIXELFORMATDESCRIPTOR pfd{};
    pfd.nSize = sizeof(pfd);
    if (DescribePixelFormat(hdc_, pix_fmt, sizeof(pfd), &pfd) == 0) return false;
    if (SetPixelFormat(hdc_, pix_fmt, &pfd) == FALSE) {
        std::fprintf(stderr, "[gl] SetPixelFormat 失败: %lu\n",
                     static_cast<unsigned long>(GetLastError()));
        return false;
    }
    // 呈现方式取证：驱动实际给到的 swap method（EXCHANGE=flip；COPY=blit；
    // MMC 同款双缓冲模型在 AMD/NVIDIA 双机均正常呈现，此日志仅留痕）。
    if (pfd.dwFlags & PFD_SWAP_EXCHANGE) {
        std::fprintf(stderr, "[gl] 像素格式 %d: swap=EXCHANGE（flip 呈现）\n",
                     pix_fmt);
    } else if (pfd.dwFlags & PFD_SWAP_COPY) {
        std::fprintf(stderr, "[gl] 像素格式 %d: swap=COPY（blit 呈现）\n",
                     pix_fmt);
    }

    // ---- 3.3 core 上下文 -------------------------------------------------
    const int ctx_attribs[] = {
        gl::WGL_CONTEXT_MAJOR_VERSION_ARB, 3,
        gl::WGL_CONTEXT_MINOR_VERSION_ARB, 3,
        gl::WGL_CONTEXT_PROFILE_MASK_ARB,  gl::WGL_CONTEXT_CORE_PROFILE_BIT_ARB,
        0, 0,
    };
    hglrc_ = gl::wglCreateContextAttribsARB(hdc_, nullptr, ctx_attribs);
    if (hglrc_ == nullptr) {
        std::fprintf(stderr, "[gl] 3.3 core 上下文创建失败（错误 %lu）\n",
                     static_cast<unsigned long>(GetLastError()));
        return false;
    }

    // ---- 显示 + 客户区尺寸 ------------------------------------------------
    if (!geom_.hidden) ShowWindow(hwnd_, SW_SHOWNOACTIVATE);
    RECT rc{};
    if (GetClientRect(hwnd_, &rc)) {
        client_w_ = rc.right - rc.left;
        client_h_ = rc.bottom - rc.top;
    }

    // ---- 两步法中间资源清理（正式上下文已建立，临时资源使命完成）--------
    if (temp_hglrc_) { wglDeleteContext(temp_hglrc_); temp_hglrc_ = nullptr; }
    if (temp_hdc_)   { ReleaseDC(temp_hwnd_, temp_hdc_); temp_hdc_ = nullptr; }
    if (temp_hwnd_)  { DestroyWindow(temp_hwnd_); temp_hwnd_ = nullptr; }

    return true;
}

// ---------------------------------------------------------------------------
// pumpMessages —— 每帧非阻塞消息泵（关窗/尺寸变化响应）
// 【MMC 对齐 2026-11-04】PeekMessageW 过滤 hwnd 改为 nullptr 全量泵：
//   ① 过滤式泵永远收不到 WM_QUIT（线程消息不关联窗口）——线程退出信号
//      会永远滞留队列；
//   ② display 渲染线程只拥有本窗口，全量泵无副作用；MMC 的
//      run_until_shutdown 同款写法（主线程全量泵）。
// ---------------------------------------------------------------------------
void GlContextWGL::pumpMessages() {
    if (hwnd_ == nullptr || !alive_) return;
    MSG msg;
    while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
        if (msg.message == WM_QUIT) { alive_ = false; return; }
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
        if (msg.message == WM_CLOSE) { alive_ = false; }
    }
}

// ---------------------------------------------------------------------------
// destroy —— 全资源释放（幂等；与 create 同线程调用）
// ---------------------------------------------------------------------------
void GlContextWGL::destroy() {
    // 当前上下文若指向本对象资源 → 先解除（删上下文前必须）
    if (hglrc_ != nullptr && wglGetCurrentContext() == hglrc_)
        wglMakeCurrent(nullptr, nullptr);
    if (hglrc_)       { wglDeleteContext(hglrc_); hglrc_ = nullptr; }
    if (hdc_ && hwnd_) { ReleaseDC(hwnd_, hdc_); hdc_ = nullptr; }
    if (hwnd_)        { DestroyWindow(hwnd_); hwnd_ = nullptr; }
    // 两步法中间资源（createRenderWindow 中途失败时的兜底清理）
    if (temp_hglrc_)  { wglDeleteContext(temp_hglrc_); temp_hglrc_ = nullptr; }
    if (temp_hdc_)    { ReleaseDC(temp_hwnd_, temp_hdc_); temp_hdc_ = nullptr; }
    if (temp_hwnd_)   { DestroyWindow(temp_hwnd_); temp_hwnd_ = nullptr; }
    alive_ = false;
    client_w_ = client_h_ = 0;
}

} // namespace ms
