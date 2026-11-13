// ============================================================================
//  PreviewWindowWin32 实现 —— 纯 Win32 GDI 预览窗口（接口说明见 .hpp）
//
//  绘制链路（每帧）：
//    渲染线程 present() ──互斥──► 后备缓冲(RGBA) ──WM_PAINT 锁内快照──►
//    RGBA→BGRX 换序（GDI BI_RGB 32bpp 内存布局）──► StretchDIBits 拉伸
//
//  字节序说明：SoftRenderer 画布是 RGBA 行主序 top-down；GDI 的 32bpp
//  BI_RGB 是 BGRX 内存布局，逐像素 R/B 互换、alpha 位补 0xFF。biHeight
//  传负值 = top-down（与画布同向，免行序翻转）。
// ============================================================================

#include "PreviewWindowWin32.hpp"

#if defined(_WIN32)

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX // 防止 windows.h 的 min/max 宏污染 std::min/max
#include <windows.h>

#include <chrono>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cwchar>
#include <atomic>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

namespace ms {
namespace {

// ---------------------------------------------------------------------------
// 模块级状态（静态接口 → 文件内单例；生命周期 = 进程）
// ---------------------------------------------------------------------------
std::mutex                g_mu;     ///< 后备帧交接锁（present ↔ WM_PAINT）
std::vector<std::uint8_t> g_back;   ///< 后备帧快照（RGBA 原样，行主序）
int                       g_w = 0;  ///< 快照宽（>0 = 已收到过帧）
int                       g_h = 0;  ///< 快照高
std::thread               g_thread; ///< 消息泵线程
HWND                      g_hwnd = nullptr; ///< 窗口句柄（窗口线程内创建）
std::wstring              g_title = L"media_display 输出预览"; // 内部拷贝
int                       g_screen_index = -1; ///< 目标显示器（-1=系统默认）
bool                      g_fullscreen   = false; ///< 无边框铺满目标屏
int                       g_win_x = -1, g_win_y = -1; ///< 窗口位置（<0=居中/默认）
int                       g_win_w = 0,  g_win_h = 0;  ///< 客户区尺寸（<=0=默认）
/// 【关窗即退出（2026-11-04）】用户点 X/Alt+F4 → WM_CLOSE 置位——display
/// 主循环轮询此旗标走优雅退出链。旧语义"关预览窗后核心继续运行"废除：
/// 显示端唯一窗口就是输出/预览窗，用户关窗即预期进程退出（残留进程占
/// 监听端口，重启必撞端口占用）。
std::atomic<bool>         g_user_closed{false};

const wchar_t kClassName[] = L"MediaDisplayPreviewWnd"; // 窗口类名（全局唯一）

// ---------------------------------------------------------------------------
// 素材同步进度状态（setAssetProgress[拉取线程] ↔ WM_PAINT[窗口线程]）
// 【纯 GDI 自绘】不引 msctls_progress32 公共控件——保持"零新依赖"部署
// 约定（显示端安装包只有 exe + FFmpeg/core 运行时，无任何 UI 库）。
// ---------------------------------------------------------------------------
std::mutex      g_prog_mu;
bool            g_prog_active = false;  ///< true = 底部叠加进度条
int             g_prog_done   = 0;      ///< 已完成素材数
int             g_prog_total  = 0;      ///< 素材总数
double          g_prog_frac   = -1.0;   ///< 当前文件进度 [0,1]；负 = 不定
std::wstring    g_prog_text;            ///< 当前素材路径（UTF-16 预转换）

// ---------------------------------------------------------------------------
// WM_PAINT 上下文：锁内快照 + RGBA→BGRX + StretchDIBits 拉伸到客户区
// ---------------------------------------------------------------------------
void blitFrame(HDC dc, int dst_w, int dst_h) {
    // ---- 快照（锁内只拷贝；换序在拷贝同时做——960×540 约 2MB，亚毫秒级，
    //      省去"先 memcpy 再换序"的第二次全量遍历）----
    std::vector<std::uint8_t> bgrx;
    int w = 0, h = 0;
    {
        std::lock_guard<std::mutex> lk(g_mu);
        if (g_w <= 0 || g_h <= 0 || g_back.empty()) return; // 尚无帧
        w = g_w;
        h = g_h;
        bgrx.resize(static_cast<std::size_t>(w) * h * 4);
        for (int y = 0; y < h; ++y) {
            const std::uint8_t* src =
                &g_back[static_cast<std::size_t>(y) * w * 4];
            std::uint8_t* dst = &bgrx[static_cast<std::size_t>(y) * w * 4];
            for (int x = 0; x < w; ++x) {
                dst[x * 4 + 0] = src[x * 4 + 2]; // B ← R（字节交换）
                dst[x * 4 + 1] = src[x * 4 + 1]; // G 原位
                dst[x * 4 + 2] = src[x * 4 + 0]; // R ← B
                dst[x * 4 + 3] = 0xFF;           // X：不透明填充
            }
        }
    }

    // ---- 拉伸上屏（负 biHeight = top-down，与画布同向）----
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize        = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth       = w;
    bmi.bmiHeader.biHeight      = -h;
    bmi.bmiHeader.biPlanes      = 1;
    bmi.bmiHeader.biBitCount    = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    StretchDIBits(dc,
                  0, 0, dst_w, dst_h,   // 目标：客户区全幅（拉伸适配窗口）
                  0, 0, w, h,           // 源：整帧
                  bgrx.data(), &bmi, DIB_RGB_COLORS, SRCCOPY);
}

// ---------------------------------------------------------------------------
// 素材同步进度条（WM_PAINT 尾部叠加）：底部黑条 + 状态文本 + 进度填充
// 布局（相对客户区底边）：高 44px；条内文本行 20px + 进度槽 6px 居中。
// ---------------------------------------------------------------------------
void drawAssetProgress(HDC dc, const RECT& client) {
    bool   active;
    int    done, total;
    double frac;
    std::wstring text;
    {
        std::lock_guard<std::mutex> lk(g_prog_mu);
        active = g_prog_active;
        if (!active) return; // 未在同步：窗口线程零开销直返
        done = g_prog_done;
        total = g_prog_total;
        frac = g_prog_frac;
        text = g_prog_text;
    }

    const int W   = client.right - client.left;
    const int H   = client.bottom - client.top;
    const int bar_h = 44;
    RECT bar{client.left, client.bottom - bar_h, client.right, client.bottom};

    // ---- 1. 半透效果（GDI AlphaBlend 需内存 DC + 全条位图，重载不划算
    //      → 纯深灰实心底，与画面边界分明；每帧叠加在画面之上）----
    HBRUSH bg = CreateSolidBrush(RGB(0x1C, 0x1C, 0x1C));
    FillRect(dc, &bar, bg);
    DeleteObject(bg);

    // ---- 2. 状态文本：同步媒体 2/5 · media/demo.mp4（45.2%）----
    wchar_t head[64];
    if (total > 0)
        std::swprintf(head, 64, L"同步媒体 %d/%d", done, total);
    else
        std::swprintf(head, 64, L"同步媒体…");
    std::wstring line = head;
    if (!text.empty()) {
        line += L" · " + text;
        if (frac >= 0.0)
            line += L"  " + std::to_wstring(int(frac * 100.0 + 0.5)) + L"%";
    }
    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(0xE8, 0xE8, 0xE8));
    RECT text_rc{bar.left + 12, bar.top + 4, bar.right - 12, bar.top + 26};
    DrawTextW(dc, line.c_str(), -1, &text_rc,
              DT_LEFT | DT_VCENTER | DT_SINGLELINE | DT_END_ELLIPSIS |
                  DT_NOPREFIX);

    // ---- 3. 进度槽 + 填充（绿色 = 传输进行中；不定进度 = 全宽暗绿）----
    const int margin = 12;
    RECT track{bar.left + margin, bar.top + 28,
               bar.right - margin, bar.top + 34};
    HBRUSH track_br = CreateSolidBrush(RGB(0x3A, 0x3A, 0x3A));
    FillRect(dc, &track, track_br);
    DeleteObject(track_br);
    const double slot_w = static_cast<double>(track.right - track.left);
    double fill_w = 0.0;
    if (frac >= 0.0)
        fill_w = slot_w * (frac < 0.0 ? 0.0 : (frac > 1.0 ? 1.0 : frac));
    else
        fill_w = slot_w; // 不定进度：全宽暗绿（未知总大小，只有"在动"语义）
    if (fill_w > 0.5) {
        const COLORREF fill_c = (frac >= 0.0) ? RGB(0x3D, 0xB3, 0x5C)
                                              : RGB(0x2A, 0x6B, 0x38);
        RECT fill{track.left, track.top,
                  track.left + static_cast<LONG>(fill_w), track.bottom};
        HBRUSH fill_br = CreateSolidBrush(fill_c);
        FillRect(dc, &fill, fill_br);
        DeleteObject(fill_br);
    }
    (void)W; (void)H; // 预留（自适应布局扩展点）
}

// ---------------------------------------------------------------------------
// 窗口过程：重绘节拍 / 关闭语义 / 免闪
// ---------------------------------------------------------------------------
LRESULT CALLBACK previewWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    switch (msg) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        RECT rc;
        GetClientRect(hwnd, &rc);
        if (!g_w || !g_h) {
            // 尚无帧：黑底 + 居中等待提示（用户反馈"预览窗口完全没动静"——
            // 纯黑窗口无法区分"等待"与"故障"，明确写出激活条件）。
            // DrawTextW DT_WORDBREAK 按客户区宽度自动换行居中，一行搞定。
            FillRect(dc, &rc,
                     static_cast<HBRUSH>(GetStockObject(BLACK_BRUSH)));
            SetBkMode(dc, TRANSPARENT);
            SetTextColor(dc, RGB(0x90, 0x90, 0x90));
            const wchar_t* tip =
                L"等待输出画面……\n"
                L"在编辑器中「添加显示端」并下发场景，点击播放后\n"
                L"此处实时显示该显示端的输出画面";
            RECT tip_rc = rc;
            DrawTextW(dc, tip, -1, &tip_rc,
                      DT_CENTER | DT_VCENTER | DT_WORDBREAK |
                          DT_NOPREFIX | DT_END_ELLIPSIS);
            // 尚无画面也可在同步素材——进度条照常叠加（底部）
            drawAssetProgress(dc, rc);
            EndPaint(hwnd, &ps);
            return 0;
        }
        blitFrame(dc, rc.right - rc.left, rc.bottom - rc.top);
        drawAssetProgress(dc, rc); // 底部叠加素材同步进度（非同步时零开销）
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_TIMER:
        // ~30fps 主动重绘（不依赖系统脏区事件，节奏确定；帧更新经
        // present() 已进后备缓冲，这里只负责取最新快照上屏）
        InvalidateRect(hwnd, nullptr, FALSE);
        UpdateWindow(hwnd);
        return 0;
    case WM_ERASEBKGND:
        return 1; // 全幅 StretchDIBits 覆盖背景，擦除只会造成闪烁
    case WM_CLOSE:
        // 【关窗即退出】用户点 X：置退出旗标（display 主循环轮询 → 优雅
        // 退出链：PreviewWindowWin32::stop → player.stop → 全线程 join）
        g_user_closed.store(true);
        DestroyWindow(hwnd);
        return 0;
    case WM_DESTROY:
        KillTimer(hwnd, 1);
        PostQuitMessage(0); // 消息循环退出 → 线程结束 → stop() 可 join
        return 0;
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

// ---------------------------------------------------------------------------
// 消息泵线程主体：注册类 → 建窗 → 定时器 → GetMessage 循环
// ---------------------------------------------------------------------------
void previewThreadMain() {
    WNDCLASSEXW wc  = {};
    wc.cbSize       = sizeof(wc);
    wc.lpfnWndProc  = previewWndProc;
    wc.hInstance    = GetModuleHandleW(nullptr);
    wc.lpszClassName= kClassName;
    wc.hCursor      = LoadCursorW(nullptr,
                                  reinterpret_cast<LPCWSTR>(IDC_ARROW));
    wc.hbrBackground= nullptr; // WM_ERASEBKGND 自管，免默认擦除闪烁
    if (!RegisterClassExW(&wc)) return; // 失败 → 线程退出 → start() 超时降级

    // ---- 窗口几何决策（优先级从高到低）--------------------------------
    //   1. fullscreen=true → 无边框铺满目标屏（win_x/y/w/h 无效——铺满即
    //      几何全部确定）；未指定屏时铺主屏（原点所在显示器）
    //   2. window_x/y/w/h（fullscreen=false 时生效）：
    //      * 位置：有屏基准（screen 指定）时为屏内偏移，否则为虚拟屏
    //        全局坐标；某轴 <0 = 该轴屏内居中（无屏基准时系统默认）
    //      * 尺寸：客户区 >0 生效，否则默认 960×540
    //   3. screen 指定、未给 x/y → 普通窗口居中于目标屏
    //   4. 什么都没配 → CW_USEDEFAULT 系统默认（行为同旧版）
    // screen 越界（拔屏/配置残留）回退系统默认并打日志——预览是辅助 UI，
    // 不因定位失败而降级 headless。
    int win_x = CW_USEDEFAULT, win_y = CW_USEDEFAULT;
    int win_w = 0, win_h = 0;              // 0 = 按客户区默认反推外框
    DWORD style = WS_OVERLAPPEDWINDOW;
    {
        // ---- 枚举全部显示器矩形（虚拟屏坐标；EnumDisplayMonitors 枚举序）--
        std::vector<RECT> monitors;
        EnumDisplayMonitors(nullptr, nullptr,
            [](HMONITOR, HDC, LPRECT rc, LPARAM lp) -> BOOL {
                // 无捕获 lambda → 可转函数指针；逐屏收集矩形
                reinterpret_cast<std::vector<RECT>*>(lp)->push_back(*rc);
                return TRUE; // 继续枚举
            },
            reinterpret_cast<LPARAM>(&monitors));

        RECT monitor_rect{};
        bool have_monitor = false;
        if (g_screen_index >= 0) {
            if (g_screen_index < static_cast<int>(monitors.size())) {
                monitor_rect = monitors[static_cast<std::size_t>(g_screen_index)];
                have_monitor = true;
            } else {
                std::fprintf(stderr,
                             "[display] --screen=%d 越界（本机 %d 块显示器），"
                             "预览窗口回退默认位置\n",
                             g_screen_index, static_cast<int>(monitors.size()));
            }
        } else if (g_fullscreen) {
            // 未指定屏的全屏：铺主屏（虚拟屏原点 (0,0) 所在显示器）
            const HMONITOR primary =
                MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY);
            MONITORINFO mi{sizeof(mi)};
            if (GetMonitorInfoW(primary, &mi)) {
                monitor_rect = mi.rcMonitor;
                have_monitor = true;
            }
        }

        if (have_monitor && g_fullscreen) {
            // 分支 1：无边框铺满整屏（多实例各占一屏 ≈ 实际投屏效果）
            style = WS_POPUP;
            win_x = monitor_rect.left;
            win_y = monitor_rect.top;
            win_w = monitor_rect.right - monitor_rect.left;
            win_h = monitor_rect.bottom - monitor_rect.top;
        } else {
            // 分支 2-4：普通窗口（WS_OVERLAPPEDWINDOW，画面区可调）
            // 客户区尺寸：手动指定 > 默认 960×540；反推含标题栏/边框的外框
            const int cw = g_win_w > 0 ? g_win_w : 960;
            const int ch = g_win_h > 0 ? g_win_h : 540;
            RECT frame{0, 0, cw, ch};
            AdjustWindowRect(&frame, style, FALSE);
            win_w = frame.right - frame.left;
            win_h = frame.bottom - frame.top;

            const int mon_w = have_monitor
                                  ? monitor_rect.right - monitor_rect.left
                                  : 0;
            const int mon_h = have_monitor
                                  ? monitor_rect.bottom - monitor_rect.top
                                  : 0;
            // 位置基准：目标屏左上角；无屏基准时全局坐标原点（主屏左上角）
            const int base_x = have_monitor ? monitor_rect.left : 0;
            const int base_y = have_monitor ? monitor_rect.top : 0;
            // 某轴手动指定（>=0）→ 屏内偏移；否则该轴屏内居中（无屏基准
            // 时保持 CW_USEDEFAULT）
            if (g_win_x >= 0 || g_win_y >= 0 || have_monitor) {
                win_x = g_win_x >= 0
                            ? base_x + g_win_x
                            : (have_monitor ? base_x + (mon_w - win_w) / 2
                                            : CW_USEDEFAULT);
                win_y = g_win_y >= 0
                            ? base_y + g_win_y
                            : (have_monitor ? base_y + (mon_h - win_h) / 2
                                            : CW_USEDEFAULT);
            }
        }
    }
    g_hwnd = CreateWindowExW(
        0, kClassName, g_title.c_str(), style,
        win_x, win_y, win_w, win_h,
        nullptr, nullptr, wc.hInstance, nullptr);
    if (!g_hwnd) return; // 建窗失败（无桌面会话等）→ 同上降级

    ShowWindow(g_hwnd, SW_SHOW);
    UpdateWindow(g_hwnd);
    SetTimer(g_hwnd, 1, 33, nullptr); // ~30fps 重绘节拍

    MSG msg;
    while (GetMessageW(&msg, nullptr, 0, 0) > 0) {
        TranslateMessage(&msg);
        DispatchMessageW(&msg);
    }
    g_hwnd = nullptr; // 退出前清句柄（present()/alive() 据此零开销直返）
}

} // namespace

// ---------------------------------------------------------------------------
// 生命周期
// ---------------------------------------------------------------------------
bool PreviewWindowWin32::start(const wchar_t* title, int screen_index,
                               bool fullscreen, int win_x, int win_y,
                               int win_w, int win_h) {
    if (g_thread.joinable()) return true; // 幂等：消息泵已在跑
    if (title) g_title = title;           // 内部拷贝（wstring，无生命周期负担）
    g_screen_index = screen_index;        // start 先于线程创建：简单赋值
    g_fullscreen   = fullscreen;          // 即建立 happens-before，无需原子
    g_win_x = win_x;
    g_win_y = win_y;
    g_win_w = win_w;
    g_win_h = win_h;

    g_thread = std::thread(&previewThreadMain);

    // 等窗口创建完成（上限 1s）：创建成功 → g_hwnd 非空；
    // 创建失败 → 线程自行退出 → join 返回 → 返回 false（降级 headless）
    for (int i = 0; i < 100 && !g_hwnd; ++i)
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    if (!g_hwnd) {
        stop();
        return false;
    }
    return true;
}

void PreviewWindowWin32::present(int w, int h, const std::uint8_t* rgba,
                                 std::uint32_t stride) {
    if (!g_hwnd || !rgba || w <= 0 || h <= 0) return; // 窗口关闭：零开销
    std::lock_guard<std::mutex> lk(g_mu);
    const std::size_t need = static_cast<std::size_t>(w) * h * 4;
    if (g_back.size() != need) g_back.assign(need, 0); // 尺寸变化自适应重分配
    // 逐行拷贝（stride 按行处理——当前渲染器恒为 w*4，此处保持接口通用）
    for (int y = 0; y < h; ++y)
        std::memcpy(&g_back[static_cast<std::size_t>(y) * w * 4],
                    rgba + static_cast<std::size_t>(y) * stride,
                    static_cast<std::size_t>(w) * 4);
    g_w = w;
    g_h = h;
}

void PreviewWindowWin32::stop() {
    // 唤醒消息循环退出（WM_CLOSE→DestroyWindow→WM_DESTROY→PostQuitMessage）
    const HWND hwnd = g_hwnd;
    if (hwnd) PostMessageW(hwnd, WM_CLOSE, 0, 0);
    if (g_thread.joinable()) g_thread.join();
}

bool PreviewWindowWin32::alive() {
    return g_thread.joinable() && g_hwnd != nullptr;
}

bool PreviewWindowWin32::userClosed() {
    return g_user_closed.load();
}

// ---------------------------------------------------------------------------
// 标题更新（指派 id 采用后调用；主循环上下文）
// 同进程跨线程 SetWindowTextW 安全：内部向窗口线程投递 WM_SETTEXT，由
// 消息泵（threadMain 的 GetMessage 循环）处理；窗口未开/已关零开销直返。
// ---------------------------------------------------------------------------
void PreviewWindowWin32::updateTitle(const wchar_t* title) {
    const HWND hwnd = g_hwnd;
    if (hwnd && title) SetWindowTextW(hwnd, title);
}

// ---------------------------------------------------------------------------
// 素材同步进度状态更新（拉取线程上下文）
// 互斥快照写 + 异步请求重绘（InvalidateRect 只投递脏区，不阻塞调用方；
// 窗口线程 33ms 定时器本来就在重绘，此处保证"完成瞬间"也立即上屏）。
// ---------------------------------------------------------------------------
void PreviewWindowWin32::setAssetProgress(bool active, int done, int total,
                                          const char* path, double frac) {
    {
        std::lock_guard<std::mutex> lk(g_prog_mu);
        g_prog_active = active;
        g_prog_done   = done;
        g_prog_total  = total;
        g_prog_frac   = frac;
        // UTF-8 → UTF-16（Asset.path 约定 UTF-8；GDI DrawTextW 需 UTF-16）
        g_prog_text.clear();
        if (path && *path) {
            const int need = MultiByteToWideChar(
                CP_UTF8, 0, path, -1, nullptr, 0);
            if (need > 1) {
                g_prog_text.resize(static_cast<std::size_t>(need - 1));
                MultiByteToWideChar(CP_UTF8, 0, path, -1, &g_prog_text[0],
                                    need);
            }
        }
    }
    // 窗口已关（用户点 X）：零开销直返（状态照写，重绘免了）
    const HWND hwnd = g_hwnd;
    if (hwnd) InvalidateRect(hwnd, nullptr, FALSE);
}

} // namespace ms

#else // 非 Windows：空翻译单元（media_display 不含预览逻辑，保持 headless）
typedef int ms_preview_window_win32_unused_t;
#endif // _WIN32
