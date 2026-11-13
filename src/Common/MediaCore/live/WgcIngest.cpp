// ============================================================================
//  WgcIngest 实现 —— WGC free-threaded 帧池 + 轮询拉帧（技术路线见头注释）
// ============================================================================

#include "live/WgcIngest.hpp"

#include <cstdio>
#include <cstring>

#include <d3d11.h>
#include <dwmapi.h>
#include <windows.h>

#include <wrl/client.h>

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.h> // SizeInt32（item.Size()/帧池创建参数）
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h> // DirectXPixelFormat（顶层命名空间）
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>

#include <windows.graphics.capture.interop.h> // IGraphicsCaptureItemInterop
#include <windows.graphics.directx.direct3d11.interop.h> // CreateDirect3D11DeviceFromDXGIDevice
                                                        // + IDirect3DDxgiInterfaceAccess

#include "live/DxReadback.hpp" // 纹理→CPU 读回（与 SpoutIngest 共享）

namespace ms {

namespace wgc = winrt::Windows::Graphics::Capture;
namespace wgx = winrt::Windows::Graphics::DirectX; // 像素格式枚举
namespace wgd = winrt::Windows::Graphics::DirectX::Direct3D11;

namespace {

using Microsoft::WRL::ComPtr;

/// 轮询节流目标帧间隔（60fps；WGC 帧池有新帧时 TryGetNextFrame 即取）
constexpr DWORD kPollIntervalMs = 16;

// ---------------------------------------------------------------------------
// ref 定位后缀解析："标题 [hwnd:000A1234]" → 0xA1234
// ---------------------------------------------------------------------------
bool parseRefHandle(const std::string& ref, const char* tag, std::uintptr_t& out) {
    const std::size_t lb = ref.rfind('[');
    if (lb == std::string::npos) return false;
    std::string body = ref.substr(lb + 1);
    if (!body.empty() && body.back() == ']') body.pop_back();
    const std::string prefix = std::string(tag) + ":";
    if (body.rfind(prefix, 0) != 0) return false; // 前缀不符
    const std::string hex = body.substr(prefix.size());
    if (hex.empty()) return false;
    out = std::strtoull(hex.c_str(), nullptr, 16);
    return out != 0;
}

// ---------------------------------------------------------------------------
// 枚举辅助：宽字符标题 → UTF-8（utf8Of 共享实现见 IngestBase）
// ---------------------------------------------------------------------------
struct WindowEnumCtx {
    std::vector<IngestSource>* out = nullptr;
};

BOOL CALLBACK windowEnumProc(HWND hwnd, LPARAM lparam) {
    auto* ctx = reinterpret_cast<WindowEnumCtx*>(lparam);
    if (!IsWindowVisible(hwnd)) return TRUE;
    if (GetWindowTextLengthW(hwnd) == 0) return TRUE; // 无标题（内部窗口）
    const LONG_PTR ex = GetWindowLongPtrW(hwnd, GWL_EXSTYLE);
    if (ex & WS_EX_TOOLWINDOW) return TRUE; // 工具窗/提示窗
    BOOL cloaked = FALSE;
    if (FAILED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked,
                                     sizeof(cloaked))) || cloaked)
        return TRUE; // UWP 挂起/虚拟桌面隐藏窗口
    wchar_t wtitle[256]{};
    GetWindowTextW(hwnd, wtitle, 256);
    const std::string label = utf8Of(wtitle);
    if (label.empty()) return TRUE;
    char ref[512]{};
    std::snprintf(ref, sizeof(ref), "%s [hwnd:%llX]", label.c_str(),
                  static_cast<unsigned long long>(
                      reinterpret_cast<std::uintptr_t>(hwnd)));
    ctx->out->push_back({label, ref});
    return TRUE;
}

struct MonitorEnumCtx {
    std::vector<IngestSource>* out = nullptr;
    int next = 1; // 显示器序号（1 起）
};

BOOL CALLBACK monitorEnumProc(HMONITOR hmon, HDC, LPRECT, LPARAM lparam) {
    auto* ctx = reinterpret_cast<MonitorEnumCtx*>(lparam);
    MONITORINFOEXW mi{};
    mi.cbSize = sizeof(mi);
    if (!GetMonitorInfoW(hmon, &mi)) return TRUE;
    const int w = mi.rcMonitor.right - mi.rcMonitor.left;
    const int h = mi.rcMonitor.bottom - mi.rcMonitor.top;
    char label[96]{};
    std::snprintf(label, sizeof(label), "显示器 %d（%dx%d%s）", ctx->next, w, h,
                  (mi.dwFlags & MONITORINFOF_PRIMARY) != 0 ? "，主显" : "");
    char ref[128]{};
    std::snprintf(ref, sizeof(ref), "显示器 %d [hmon:%llX]", ctx->next,
                  static_cast<unsigned long long>(
                      reinterpret_cast<std::uintptr_t>(hmon)));
    ctx->out->push_back({label, ref});
    ++ctx->next;
    return TRUE;
}

/// D3D11 设备创建（BGRA 支持——WGC 帧格式固定 B8G8R8A8）
ComPtr<ID3D11Device> createD3DDevice() {
    ComPtr<ID3D11Device> device;
    UINT flags = D3D11_CREATE_DEVICE_BGRA_SUPPORT;
    D3D_FEATURE_LEVEL fl{};
    D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, flags,
                      nullptr, 0, D3D11_SDK_VERSION, device.GetAddressOf(),
                      &fl, nullptr);
    return device;
}

} // namespace

// ---------------------------------------------------------------------------
// 生命周期（线程/诊断/兜底壳继承 IngestBase；构造只落配置）
// ---------------------------------------------------------------------------
WgcIngest::WgcIngest(Kind kind, std::string source_ref, int latency_mode,
                     FrameSink sink)
    : IngestBase(std::move(source_ref), latency_mode, std::move(sink)),
      kind_(kind) {}

WgcIngest::~WgcIngest() = default;

bool WgcIngest::onStart() {
    if (!wgc::GraphicsCaptureSession::IsSupported()) {
        std::fprintf(stderr, "[wgc] 本系统不支持 Windows.Graphics.Capture\n");
        std::fflush(stderr);
        return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// 接收主体：定位目标 → 会话建立 → 拉帧循环（目标失效自愈重连）。
//（IngestBase 兜底壳内执行——漏接异常不击穿线程）
// ---------------------------------------------------------------------------
void WgcIngest::recvLoop() {
    winrt::init_apartment(winrt::apartment_type::multi_threaded); // 本线程 MTA

    // —— DX11 设备（线程私有）→ WinRT IDirect3DDevice ——
    ComPtr<ID3D11Device> device = createD3DDevice();
    if (device == nullptr) {
        std::fprintf(stderr, "[wgc] DX11 设备创建失败（Ingest 退出）\n");
        std::fflush(stderr);
        return;
    }
    ComPtr<IDXGIDevice> dxgi;
    device->QueryInterface(dxgi.GetAddressOf());
    winrt::com_ptr<::IInspectable> inspectable;
    if (FAILED(CreateDirect3D11DeviceFromDXGIDevice(dxgi.Get(),
                                                    inspectable.put()))) {
        std::fprintf(stderr, "[wgc] WinRT 设备包装失败（Ingest 退出）\n");
        std::fflush(stderr);
        return;
    }
    const auto rt_device = inspectable.as<wgd::IDirect3DDevice>();
    ComPtr<ID3D11DeviceContext> context;
    device->GetImmediateContext(context.GetAddressOf());

    DxTextureReadback readback;
    const auto interop = winrt::get_activation_factory<
        wgc::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
    const bool is_window = kind_ == Kind::Window;
    std::int64_t last_find_log = -1;

    while (running_.load(std::memory_order_acquire)) {
        // ---- 定位捕获目标 ----
        std::uintptr_t handle = 0;
        if (!parseRefHandle(source_ref_, is_window ? "hwnd" : "hmon",
                            handle)) {
            logThrottled("源标识缺少定位后缀（[hwnd:…] / [hmon:…]）",
                         last_find_log, 5000);
            sleepInterruptible(500);
            continue;
        }

        // ---- 会话建立（整段包裹：目标失效/受保护窗口在任何一步抛
        //      hresult_error → 节流日志 + 500ms 重试，异常不再击穿线程）----
        wgc::GraphicsCaptureItem item{nullptr};
        wgc::Direct3D11CaptureFramePool pool{nullptr};
        wgc::GraphicsCaptureSession session{nullptr};
        winrt::Windows::Graphics::SizeInt32 size{};
        auto closed_flag = std::make_shared<std::atomic<bool>>(false);
        bool started = false;
        try {
            if (is_window)
                interop->CreateForWindow(reinterpret_cast<HWND>(handle),
                                         winrt::guid_of<wgc::GraphicsCaptureItem>(),
                                         winrt::put_abi(item));
            else
                interop->CreateForMonitor(reinterpret_cast<HMONITOR>(handle),
                                          winrt::guid_of<wgc::GraphicsCaptureItem>(),
                                          winrt::put_abi(item));
            if (item != nullptr) {
                size = item.Size();
                pool = wgc::Direct3D11CaptureFramePool::CreateFreeThreaded(
                    rt_device, wgx::DirectXPixelFormat::B8G8R8A8UIntNormalized,
                    /*buffers=*/2, size);
                session = pool.CreateCaptureSession(item);
                try { session.IsCursorCaptureEnabled(false); } catch (...) {} // 18362+
                try { session.IsBorderRequired(false); } catch (...) {}       // 20348+
                // 目标关闭自愈（窗口关/显示器断开 → 事件置位 → 重连循环；
                // 先注册事件再 StartCapture，避免关闭事件落在注册之前）
                item.Closed([closed_flag](auto&&, auto&&) {
                    closed_flag->store(true, std::memory_order_release);
                });
                session.StartCapture();
                started = true;
            }
        } catch (...) {
            item = nullptr; pool = nullptr; session = nullptr;
        }
        if (!started) {
            logThrottled(is_window ? "窗口不存在或被捕获拒绝"
                                   : "显示器不可用",
                         last_find_log, 5000);
            sleepInterruptible(500);
            continue;
        }

        connected_.store(true, std::memory_order_release);
        std::fprintf(stderr, "[wgc] 开始捕获 %s (%dx%d)\n",
                     source_ref_.c_str(), size.Width, size.Height);
        std::fflush(stderr);

        // ---- 拉帧循环（TryGetNextFrame 轮询；帧池容量 2 保新帧；
        //      单帧异常不终结会话——继续轮询）----
        while (running_.load(std::memory_order_acquire) &&
               !closed_flag->load(std::memory_order_acquire)) {
            LiveFramePtr f;
            try {
                auto frame = pool.TryGetNextFrame();
                if (!frame) { // 无新帧（winrt 投影引用类型判空）
                    Sleep(kPollIntervalMs);
                    continue;
                }
                // 内容尺寸变化 → 帧池重建（源改分辨率/窗口 resize）
                const auto cs = frame.ContentSize();
                if (cs.Width != size.Width || cs.Height != size.Height) {
                    size = cs;
                    pool.Recreate(rt_device,
                                  wgx::DirectXPixelFormat::B8G8R8A8UIntNormalized,
                                  /*buffers=*/2, size);
                    continue;
                }
                // surface → ID3D11Texture2D → 统一读回
                ID3D11Texture2D* tex = nullptr;
                {
                    const auto access = frame.Surface().as<
                        ::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess>();
                    if (FAILED(access->GetInterface(
                            winrt::guid_of<ID3D11Texture2D>(),
                            reinterpret_cast<void**>(&tex)))) {
                        continue;
                    }
                }
                f = readback.read(context.Get(), tex);
                tex->Release(); // surface 引用独立——立即释放本指针
            } catch (...) {
                Sleep(kPollIntervalMs);
                continue;
            }
            if (f == nullptr) continue;
            emitFrame(std::move(f));
        }

        // ---- 本轮会话清理（目标关闭/停止——重连走外层循环）----
        try { session.Close(); } catch (...) {}
        try { pool.Close(); }   catch (...) {}
        item = nullptr; pool = nullptr; session = nullptr; // 释放本轮引用
        connected_.store(false, std::memory_order_release);
        if (running_.load(std::memory_order_acquire) &&
            !closed_flag->load(std::memory_order_acquire))
            continue; // 用户 stop 直接退出循环
        logThrottled("捕获会话结束，重连", last_find_log, 5000);
    }

    winrt::uninit_apartment();
}

// ---------------------------------------------------------------------------
// WGC 系统支持探测（单测门控入口；头文件不泄漏 winrt）
// ---------------------------------------------------------------------------
bool wgcProbeSupported() {
    try {
        return wgc::GraphicsCaptureSession::IsSupported();
    } catch (...) {
        return false; // 老系统无此 API
    }
}

// ---------------------------------------------------------------------------
// 注册表包装：枚举 + 工厂
// ---------------------------------------------------------------------------
std::vector<IngestSource> listWgcWindowSources(std::uint32_t wait_ms) {
    (void)wait_ms; // EnumWindows 即时返回
    std::vector<IngestSource> out;
    WindowEnumCtx ctx{&out};
    EnumWindows(windowEnumProc, reinterpret_cast<LPARAM>(&ctx));
    return out;
}

std::vector<IngestSource> listWgcMonitorSources(std::uint32_t wait_ms) {
    (void)wait_ms; // EnumDisplayMonitors 即时返回
    std::vector<IngestSource> out;
    MonitorEnumCtx ctx{&out, 1};
    EnumDisplayMonitors(nullptr, nullptr, monitorEnumProc,
                        reinterpret_cast<LPARAM>(&ctx));
    return out;
}

std::unique_ptr<ILiveIngest> makeWgcWindow(std::string source_name,
                                           int        latency_mode,
                                           ILiveIngest::FrameSink sink) {
    return std::make_unique<WgcIngest>(WgcIngest::Kind::Window,
                                       std::move(source_name), latency_mode,
                                       std::move(sink));
}

std::unique_ptr<ILiveIngest> makeWgcMonitor(std::string source_name,
                                            int        latency_mode,
                                            ILiveIngest::FrameSink sink) {
    return std::make_unique<WgcIngest>(WgcIngest::Kind::Monitor,
                                       std::move(source_name), latency_mode,
                                       std::move(sink));
}

} // namespace ms
