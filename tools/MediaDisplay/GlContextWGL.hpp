// ============================================================================
//  GlContextWGL.hpp —— Win32 窗口 + WGL 3.3 core 上下文（GL 后端的宿主）
//
//  设计依据：《hw-decode-implementation-plan.md》②-2
//    * 职责单一：为 GlRenderer 提供一块"可渲染的 GL 表面"——窗口创建/
//      定位（与 PreviewWindowWin32 同套 screen/fullscreen 几何语义）、
//      ARB 像素格式、3.3 core 上下文、vsync 关闭、消息泵；不掺任何
//      合成逻辑（纹理/着色器/顶点都在 GlRenderer）。
//    * 两步创建法（WGL 标准姿势）：wglGetProcAddress 依赖 current
//      context，而创建 3.3 上下文的 wglCreateContextAttribsARB 本身就是
//      扩展函数——鸡生蛋问题用临时窗口+1.1 上下文解：
//        ① 临时隐藏窗口 + 1.1 兼容上下文 → makeCurrent → 取 WGL 扩展；
//        ② 正式窗口 + ARB 像素格式 + 3.3 core 上下文 → makeCurrent →
//           gl::loadAll() 填充函数表 → 销毁临时资源。
//    * 线程契约：create/destroy/pumpMessages/所有 GL 调用必须在同一线程
//      （渲染线程）——GL 上下文线程亲和，HGLRC 只能被拥有线程使用。
//    * 生命周期：窗口销毁（用户关闭/系统注销）→ alive_=false，此后
//      pumpMessages 维持空转、GlRenderer 的 draw 全部跳过——同步输出
//      不受影响（与 PreviewWindowWin32"关窗不影响同步"语义一致）。
//    * vsync：wglSwapIntervalEXT(1) 开启（2026-11-04 对齐 MPV_Media_Console
//      PlayerWindow 的呈现模型）——SwapBuffers 阻塞节流渲染循环，与
//      FramePacer 相位控制并存（帧产出节奏仍由 Pacer 决定，swap 至多多等
//      一个刷新周期）。曾用 vsync(0)+单缓冲 glFlush 规避 flip 不上屏，
//      实测单/双缓冲均呈现冻结（DWM 表面不更新），弃用该路线。
// ============================================================================

#pragma once

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX // 防 min/max 宏污染 C++ 头
#endif

#include <windows.h>

#include <atomic>
#include <string>

namespace ms {

class GlContextWGL {
public:
    /// 【关窗即退出（2026-11-04 用户需求）】用户关闭输出窗（Alt+F4/任务栏
    /// 右键 → WM_CLOSE/WM_DESTROY）时置位——display 主循环轮询此旗标，
    /// 置位即走优雅退出链（PreviewWindowWin32::stop → player.stop → 全部
    /// 线程 join → 进程退出）。旧行为"关窗后进程继续空转（同步输出不受
    /// 影响）"曾是有意设计，但无边框输出窗没有标题栏按钮，用户关窗即
    /// 认为"display 该退了"——残留进程占着监听端口，重启必撞端口占用。
    /// 静态而非实例成员：wndProc 静态回调可能拿不到 self（防御路径），
    /// 且主循环与渲染线程无共享实例可查——进程级旗标最直接。
    static std::atomic<bool>& userClosedFlag() noexcept {
        static std::atomic<bool> flag{false};
        return flag;
    }
    /// 便捷读（主循环用）
    static bool userClosed() noexcept { return userClosedFlag().load(); }
public:
    /// 窗口几何（语义与 PreviewWindowWin32::start 参数一一对应）
    struct WindowGeom {
        bool        fullscreen = false; ///< 无边框铺满目标屏
        int         screen     = -1;    ///< 显示器索引（-1=系统默认主屏）
        int         x          = -1;    ///< 窗口位置 X（<0=屏内居中；fullscreen 无效）
        int         y          = -1;    ///< 窗口位置 Y（同上）
        int         w          = 0;     ///< 客户区宽（<=0=默认 960）
        int         h          = 0;     ///< 客户区高（<=0=默认 540）
        bool        hidden     = false; ///< 离屏测试：窗口不显示（对拍渲染）
        std::wstring title;             ///< 窗口标题
        /// 【置于末尾】本结构多处按位置聚合初始化（测试/装配点），新增
        /// 字段一律尾部追加，勿插中间破坏既有位置语义。
        /// 【已删 single_buffer 2026-11-04】单缓冲 glFlush 直绘路线在呈现
        /// 冻结排查中实证无效（清屏色循环实验：单/双缓冲窗口均恒首帧色），
        /// 已按 MMC（MPV_Media_Console）呈现模型移除——恒双缓冲 +
        /// SwapBuffers + vsync(1)。
        /// 无边框窗口（2026-11-04 用户决策）：WS_POPUP 无系统装饰——媒体
        /// 服务器输出窗纯净呈现（与 fullscreen 同款样式但按 window 几何
        /// 定位，不强制铺满屏）。关闭靠 Alt+F4 / 任务栏右键（WM_CLOSE）。
        /// 【结构默认 true】与"display 端输出窗无边框"产品语义一致。
        bool        borderless    = true;
    };

    GlContextWGL() = default;
    ~GlContextWGL() { destroy(); }
    GlContextWGL(const GlContextWGL&) = delete;
    GlContextWGL& operator=(const GlContextWGL&) = delete;

    /// 创建窗口 + 3.3 core 上下文并 makeCurrent（渲染线程调用；幂等防御）
    /// \return false = 任一步失败（已清理全部中间资源；原因打日志）
    bool create(const WindowGeom& geom);

    /// 销毁（同线程；幂等）——makeCurrent(NULL) → 删上下文 → 删窗口/类
    void destroy();

    bool valid() const noexcept { return hwnd_ != nullptr && hglrc_ != nullptr; }
    HWND hwnd() const noexcept { return hwnd_; }
    HDC  dc() const noexcept { return hdc_; } ///< 窗口 DC（SwapBuffers 用）
    bool alive() const noexcept { return alive_; } ///< 窗口未被用户关闭

    /// 客户区尺寸（像素；GL viewport/输出画布即此值）
    int clientW() const noexcept { return client_w_; }
    int clientH() const noexcept { return client_h_; }

    /// 客户区尺寸外部更新（GlRenderer 离屏 resize 后同步——与 wndProc
    /// WM_SIZE 同一写入点语义）
    void notifyClientRect(int w, int h) noexcept {
        client_w_ = w;
        client_h_ = h;
    }

    /// 【运行期 vsync 切换，2026-11-05】设置 wglSwapIntervalEXT（0=不等
    /// 垂直同步，1=每帧等一次）。用途：GlRenderer 监测到 SwapBuffers 被
    /// 呈现层长阻塞（远程桌面/虚拟显示器/Optimus 跨 GPU 呈现等场景实测
    /// 单帧 250ms → 整机 5fps）时，自适应降为 0，改由 FramePacer 独占
    /// 节拍；无 wglSwapIntervalEXT 扩展时空操作（返回 false）。
    bool setSwapInterval(int interval);

    /// 当前生效的 vsync 间隔（-1 = 扩展缺失，未设置）
    int swapInterval() const noexcept { return swap_interval_; }

    /// 像素格式恒双缓冲（2026-11-04 起——单缓冲兼容模式已移除，
    /// endFrame 恒走 SwapBuffers；字段删除后此访问器一并撤销）

    /// 正式窗口过程：WM_SIZE 跟踪客户区尺寸（GL viewport 数据源）、
    /// WM_CLOSE 标记关闭并销毁窗口（alive_=false → 合成项跳过）。
    /// 公开仅为窗口类注册（匿名空间辅助函数）可访问，勿直接调用。
    static LRESULT CALLBACK wndProc(HWND, UINT, WPARAM, LPARAM);

    /// 每帧消息泵（渲染线程调用）：处理 WM_CLOSE/WINDOWS 尺寸变化等——
    /// 非阻塞（PeekMessage 取空即返回，不等待）
    void pumpMessages();

private:
    /// 枚举显示器矩形（EnumDisplayMonitors 回调的静态蹦床）
    static BOOL CALLBACK monitorEnumProc(HMONITOR, HDC, LPRECT, LPARAM);

    /// 临时窗口创建（两步法第①步：只为取得 WGL 扩展函数指针）
    bool createTempWindow();

    /// 正式窗口创建 + 像素格式 + 上下文（两步法第②步）
    bool createRenderWindow();

    // ---- 资源句柄（渲染线程独占）----
    HWND  hwnd_ = nullptr;      ///< 正式窗口
    HDC   hdc_ = nullptr;       ///< 窗口 DC（CS_OWNDC：每窗口独立像素格式）
    HGLRC hglrc_ = nullptr;     ///< 3.3 core 上下文
    // 两步法中间资源（createRenderWindow 成功后销毁）
    HWND  temp_hwnd_ = nullptr;
    HDC   temp_hdc_ = nullptr;
    HGLRC temp_hglrc_ = nullptr;

    WindowGeom geom_{};         ///< 创建参数（记录/复用）
    int  client_w_ = 0;         ///< 客户区宽（WM_SIZE 维护）
    int  client_h_ = 0;
    bool alive_ = false;        ///< 窗口存活（用户关闭后 false）
    bool class_registered_ = false; ///< 窗口类已注册（正式/临时共用一类）
    /// vsync 间隔（-1 = 扩展缺失/未设置；1 = 建上下文时的默认）
    int  swap_interval_ = -1;
};

} // namespace ms
