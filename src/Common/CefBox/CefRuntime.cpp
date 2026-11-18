#include "CefRuntime.h"

#include "include/cef_app.h"
#include "include/cef_browser.h"
#include "include/wrapper/cef_helpers.h"

#include <windows.h>

#include <filesystem>

namespace cefbox {

namespace {

constexpr UINT_PTR kScheduleTimerId = 1;   // OnScheduleMessagePumpWork 驱动（低延迟路径）
constexpr UINT_PTR kHeartbeatTimerId = 2;  // 16ms 兜底泵（动画/合成持续推进）
constexpr UINT kHeartbeatMs = 16;
const wchar_t kPumpWndClassName[] = L"FlowCefPumpWnd";

std::wstring utf8ToWide(const std::string& utf8) {
    if (utf8.empty())
        return std::wstring();
    int n = MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), nullptr, 0);
    std::wstring out((size_t)n, L'\0');
    if (n > 0)
        MultiByteToWideChar(CP_UTF8, 0, utf8.data(), (int)utf8.size(), out.data(), n);
    return out;
}

// ---- OSR 媒体播放关键命令行加固（后台/遮挡节流会杀死无窗口渲染）----
void tuneCommandLine(CefRefPtr<CefCommandLine> cmd) {
    // Win 窗口遮挡度计算：OSR 无可见窗口，遮挡判定触发节流 → OnPaint 停帧
    const char* kOcclusionFeature = "CalculateNativeWinOcclusion";
    std::string features = cmd->GetSwitchValue("disable-features").ToString();
    if (features.empty()) {
        cmd->AppendSwitchWithValue("disable-features", kOcclusionFeature);
    } else if (features.find(kOcclusionFeature) == std::string::npos) {
        cmd->AppendSwitchWithValue("disable-features", features + "," + kOcclusionFeature);
    }
    cmd->AppendSwitch("disable-backgrounding-occluded-windows");
    cmd->AppendSwitch("disable-renderer-backgrounding");
    cmd->AppendSwitch("disable-background-timer-throttling");
    cmd->AppendSwitchWithValue("autoplay-policy", "no-user-gesture-required");
}

// ---- 泵窗口过程：全部计时器逻辑收敛于此，类内只保留 pumpOnce/scheduleWork ----
LRESULT CALLBACK pumpWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (msg == WM_TIMER) {
        if (wp == kScheduleTimerId) {
            KillTimer(hwnd, wp);
            CefRuntime::instance().pumpOnce();
            return 0;
        }
        if (wp == kHeartbeatTimerId) {
            CefRuntime::instance().pumpOnce();
            return 0;
        }
    }
    return DefWindowProcW(hwnd, msg, wp, lp);
}

}  // namespace

// =============================================================================
// CefBoxApp — 浏览器主进程 App；转发泵调度
// =============================================================================
class CefBoxApp : public CefApp, public CefBrowserProcessHandler {
public:
    CefBoxApp() = default;

    CefRefPtr<CefBrowserProcessHandler> GetBrowserProcessHandler() override { return this; }

    void OnBeforeCommandLineProcessing(const CefString& process_type,
                                       CefRefPtr<CefCommandLine> command_line) override {
        // 仅浏览器主进程（process_type 为空）加固命令行
        if (process_type.empty())
            tuneCommandLine(command_line);
    }

    // external_message_pump=true：CEF 要求宿主在此时安排一次 CefDoMessageLoopWork
    void OnScheduleMessagePumpWork(int64_t delay_ms) override {
        CefRuntime::instance().scheduleWork(delay_ms);
    }

private:
    IMPLEMENT_REFCOUNTING(CefBoxApp);
    DISALLOW_COPY_AND_ASSIGN(CefBoxApp);
};

// =============================================================================
// CefRuntime
// =============================================================================
CefRuntime& CefRuntime::instance() {
    static CefRuntime s;
    return s;
}

CefRuntime::~CefRuntime() {
    // 全局单例析构期不做 CefShutdown（静态析构顺序不可靠）；退出必须显式 shutdown()
}

bool CefRuntime::initialize(const Settings& settings) {
    if (m_initialized)
        return true;
    if (m_shutdown)
        return false;  // init-once 策略：shutdown 后禁止复活

    createPumpWindow();
    if (!m_pumpWnd)
        return false;

    CefMainArgs mainArgs(GetModuleHandleW(nullptr));

    CefSettings cefSettings;
    cefSettings.no_sandbox = true;                    // 不用 sandbox
    cefSettings.windowless_rendering_enabled = true;  // OSR 总开关
    cefSettings.external_message_pump = true;         // D1：回调落主线程
    cefSettings.multi_threaded_message_loop = false;
    if (settings.backgroundColor != 0)
        cefSettings.background_color = settings.backgroundColor;
    if (!settings.locale.empty())
        CefString(&cefSettings.locale).FromString(settings.locale);

    // 子进程与资源定位：全部锚定本进程 exe 所在目录（宽字符，无 ACP 风险）
    wchar_t exePathW[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, exePathW, MAX_PATH);
    std::filesystem::path exeDir = std::filesystem::path(exePathW).parent_path();

    CefString(&cefSettings.browser_subprocess_path)
        .FromWString((exeDir / L"FlowCefHelper.exe").wstring());
    CefString(&cefSettings.resources_dir_path).FromWString(exeDir.wstring());
    CefString(&cefSettings.locales_dir_path).FromWString((exeDir / L"locales").wstring());
    if (!settings.cacheDir.empty())
        CefString(&cefSettings.cache_path).FromWString(utf8ToWide(settings.cacheDir));

    CefRefPtr<CefBoxApp> app = new CefBoxApp();
    if (!CefInitialize(mainArgs, cefSettings, app, nullptr)) {
        DestroyWindow((HWND)m_pumpWnd);
        m_pumpWnd = nullptr;
        return false;
    }

    m_initialized = true;
    SetTimer((HWND)m_pumpWnd, kHeartbeatTimerId, kHeartbeatMs, nullptr);
    return true;
}

void CefRuntime::shutdown() {
    if (m_shutdown)
        return;
    m_shutdown = true;
    if (!m_initialized)
        return;

    // 先停泵再关 CEF：shutdown 期间不允许任何泵入
    HWND wnd = (HWND)m_pumpWnd;
    KillTimer(wnd, kHeartbeatTimerId);
    KillTimer(wnd, kScheduleTimerId);

    CefShutdown();

    DestroyWindow(wnd);
    m_pumpWnd = nullptr;
    m_initialized = false;
}

void CefRuntime::pumpOnce() {
    if (m_initialized)
        CefDoMessageLoopWork();
}

void CefRuntime::createPumpWindow() {
    WNDCLASSEXW wc = {};
    wc.cbSize = sizeof(wc);
    wc.lpfnWndProc = pumpWndProc;
    wc.hInstance = GetModuleHandleW(nullptr);
    wc.lpszClassName = kPumpWndClassName;
    RegisterClassExW(&wc);

    // message-only 窗口：不可见、不进任务栏、只收消息
    m_pumpWnd = (void*)CreateWindowExW(0, kPumpWndClassName, L"", 0, 0, 0, 0, 0, HWND_MESSAGE,
                                       nullptr, wc.hInstance, nullptr);
}

void CefRuntime::scheduleWork(int64_t delayMs) {
    if (!m_pumpWnd)
        return;
    UINT ms = delayMs <= 0 ? 1 : (delayMs > 1000 ? 1000 : (UINT)delayMs);
    SetTimer((HWND)m_pumpWnd, kScheduleTimerId, ms, nullptr);
}

}  // namespace cefbox
