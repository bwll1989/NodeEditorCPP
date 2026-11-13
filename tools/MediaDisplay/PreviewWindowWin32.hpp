// ============================================================================
//  PreviewWindowWin32 —— 显示端输出预览窗口（Win32 GDI；media_display 专用）
//
//  需求背景（用户反馈"显示端打开后只有终端窗口，没有输出画面"）：
//    显示端 media_display.exe 是无 Qt 的命令行程序，渲染结果原本只有
//    诊断 BMP 导出（--render-out）可查。本类用纯 Win32 API 画一个独立
//    预览窗口，把渲染线程合成的每一帧拉伸显示——零新依赖（GDI 系统库），
//    不影响"显示端无 Qt"的部署约定。
//
//  线程模型：
//    * 消息泵线程（threadMain）：注册窗口类 → 建窗 → 33ms 定时器 →
//      GetMessage 循环；WM_TIMER 主动 InvalidateRect/UpdateWindow 重绘；
//    * present()（渲染线程上下文调用）：互斥锁内把 RGBA 帧拷进后备缓冲
//      ——锁内只做内存拷贝，无格式转换/系统调用（满足 RenderThread
//      frame_sink"不得长阻塞"契约，拖累 present 相位）；
//    * WM_PAINT（窗口线程）：锁内快照 → RGBA→BGRX 换序 → StretchDIBits
//      拉伸到客户区（窗口尺寸独立于帧尺寸，整幅可见）。
//
//  降级约定：
//    * start() 失败（无桌面会话/服务模式/类注册失败）→ 返回 false，
//      调用方保持 headless 继续跑（显示端核心功能不受影响）；
//    * 用户点窗口 X → 仅关闭预览窗口（DestroyWindow→PostQuitMessage），
//      显示端进程与同步输出继续；alive() 随之变 false。
// ============================================================================

#pragma once

#include <cstdint>

#if defined(_WIN32) // 非 Windows 平台无本类（media_display 保持 headless）

namespace ms {

class PreviewWindowWin32 {
public:
    /// 创建预览窗口并启动消息泵线程（幂等：已在跑直接返回 true）。
    /// \param title        窗口标题（内部拷贝，调用方无需保证存活期）
    /// \param screen_index 目标显示器索引（EnumDisplayMonitors 枚举序，
    ///                     0 = 第一块；负值 = 系统默认定位。多实例部署
    ///                     时各实例指定不同屏，预览不叠在一起）
    /// \param fullscreen   true = 无边框铺满目标屏（配合 screen_index；
    ///                     未指定屏时铺主屏。铺满时 win_x/y/w/h 无效）
    /// \param win_x/win_y  窗口位置（fullscreen=false 时生效）：有屏基准
    ///                     （screen_index 有效）时为屏内偏移，否则为虚拟
    ///                     屏全局坐标；某轴 <0 = 该轴居中/默认
    /// \param win_w/win_h  窗口客户区尺寸（fullscreen=false 时生效；
    ///                     <=0 = 默认 960×540）
    /// \return false = 窗口创建失败（调用方降级 headless，不影响渲染）
    static bool start(const wchar_t* title = L"media_display 输出预览",
                      int screen_index = -1, bool fullscreen = false,
                      int win_x = -1, int win_y = -1,
                      int win_w = 0, int win_h = 0);

    /// 提交一帧（渲染线程上下文调用）：互斥拷贝到后备缓冲，供 WM_PAINT
    /// 重绘取用。RGBA 行主序 top-down；\p stride 为源行字节数（= w×4）。
    /// 窗口未启动/已关闭时零开销直返（帧数据不进本类）。
    static void present(int w, int h, const std::uint8_t* rgba,
                        std::uint32_t stride);

    /// 关闭窗口并 join 消息泵线程（幂等；进程退出前调用）
    static void stop();

    /// 窗口线程是否存活（用户点 X 后为 false；核心输出不受影响）
    static bool alive();

    /// 【关窗即退出（2026-11-04 用户需求）】用户已关闭预览窗（点 X/
    /// Alt+F4）。display 主循环轮询此旗标，置位即走优雅退出链——旧行为
    /// "关预览窗后显示端继续运行"退役：关窗即认为用户要退出 display
    /// （与 GL 输出窗同语义，见 GlContextWGL::userClosedFlag）。
    static bool userClosed();

    /// 【指派 id 2026-11-06】运行期更新窗口标题（主循环上下文调用）：
    /// display 采用 Editor 指派 id 后标题跟随（现场核验"这块屏是谁"
    /// 的直接证据——标题显示的即实际渲染的屏 id）。窗口未开零开销。
    static void updateTitle(const wchar_t* title);

    // -------------------------------------------------------------------
    // 素材同步进度条（媒体文件同步可视化；纯 GDI 自绘——零新依赖）
    // -------------------------------------------------------------------
    /// 更新素材同步进度状态（拉取线程上下文调用；内部互斥快照 +
    /// 异步请求重绘，不阻塞调用方）。
    /// \param active true = 显示进度条（底部叠加）；false = 收起
    /// \param done   已完成素材数 / \p total 总数（total<=0 时不显示计数）
    /// \param path   当前素材相对路径（UTF-8；可空 = 只显示计数）
    /// \param frac   当前文件进度 [0,1]；负值 = 不定进度（未知总大小）
    static void setAssetProgress(bool active, int done, int total,
                                 const char* path, double frac);
};

} // namespace ms

#endif // _WIN32
