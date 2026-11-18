#pragma once
// =============================================================================
// CefRuntime — CEF 运行时底座（Qt-free）
//
// 设计要点（见 src/Nodes/WebRenderNode/CefRefactorPlan.md）：
// - init-once 策略：CefInitialize 全进程至多一次；CefShutdown 仅在进程退出时
//   调用一次，之后本进程不得再 initialize（无 init/shutdown 循环）。
// - external_message_pump=true（D1 决策）：全部 CEF 回调（含 OnPaint）落在
//   initialize() 调用线程 = GUI 主线程，与 ImageGpuUpload 的 GUI 线程契约一致。
//   泵实现 = message-only 窗口 + SetTimer → CefDoMessageLoopWork（细节在 .cpp）。
// - 子进程 = FlowCefHelper.exe（与本进程 exe 同目录），Flow.exe 绝不充当 CEF
//   子进程（单实例互斥/重启参数/CrashDump 均会受干扰）。
// - CEF 运行时文件（libcef.dll/*.pak/locales 等）平铺在 Flow.exe 旁。
//
// 线程约定：所有公开方法必须在 initialize() 的调用线程（CEF 主线程）使用。
// =============================================================================
#include "CefBoxExport.h"

#include <cstdint>
#include <string>

namespace cefbox {

class CefBoxApp;  // .cpp 内实现，OnScheduleMessagePumpWork 经 friend 调 scheduleWork

class CEFBOX_EXPORT CefRuntime {
public:
    struct Settings {
        std::string cacheDir;          ///< 浏览器缓存/用户数据目录（UTF-8；空 = 匿名会话不落盘）
        std::string locale;            ///< 语言（空 = 系统默认）
        uint32_t backgroundColor = 0;  ///< ARGB 背景；0 = 不设置（默认白底）
    };

    static CefRuntime& instance();

    /// 幂等。首次调用执行 CefInitialize，此后直接返回 true。
    /// 必须在 GUI 主线程调用；该线程从此成为 CEF 主线程（泵所在线程）。
    bool initialize(const Settings& settings);

    bool isInitialized() const { return m_initialized; }

    /// 进程退出路径调用（GUI 主线程）。幂等。
    /// 注意：调用前必须已关闭全部浏览器并等待 OnBeforeClose，否则 CefShutdown 挂起。
    void shutdown();

    /// 手动泵一次（诊断用；正常由 .cpp 内 SetTimer 自动驱动）
    void pumpOnce();

private:
    CefRuntime() = default;
    ~CefRuntime();  // 全局单例：析构不做 CefShutdown（退出路径必须显式 shutdown()）

    friend class CefBoxApp;

    void createPumpWindow();
    void scheduleWork(int64_t delayMs);  // CefBoxApp::OnScheduleMessagePumpWork 转发点

    bool m_initialized = false;
    bool m_shutdown = false;
    void* m_pumpWnd = nullptr;  ///< message-only 泵窗口 HWND（类型隔离在 .cpp）
};

}  // namespace cefbox
