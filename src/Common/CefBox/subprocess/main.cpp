// =============================================================================
// FlowCefHelper.exe — CEF 子进程宿主（browser/renderer/gpu 等进程统一入口）
//
// 为什么不让 Flow.exe 充当子进程：
// - Flow.exe 有单实例互斥、重启参数解析、CrashDump 安装，被 CEF 以特殊参数
//   二次拉起会走错分支；
// - 子进程树统一挂到 JobGuard（KILL_ON_JOB_CLOSE），退出时整体收割，杜绝
//   幽灵进程。
//
// 本 exe 是 WIN32 子系统、无窗口、无 Qt；除 CefExecuteProcess 外不做任何事。
// =============================================================================
#include "include/cef_app.h"

#include <windows.h>

namespace {
// 子进程无需任何 handler（V8 扩展后续按需加）；仅补齐引用计数使 CefApp 可实例化
class CefHelperApp : public CefApp {
public:
    CefHelperApp() = default;

private:
    IMPLEMENT_REFCOUNTING(CefHelperApp);
    DISALLOW_COPY_AND_ASSIGN(CefHelperApp);
};
}  // namespace

int APIENTRY wWinMain(HINSTANCE hInstance, HINSTANCE, LPWSTR, int) {
    CefMainArgs mainArgs(hInstance);
    CefRefPtr<CefApp> app = new CefHelperApp();
    return CefExecuteProcess(mainArgs, app, nullptr);
}
