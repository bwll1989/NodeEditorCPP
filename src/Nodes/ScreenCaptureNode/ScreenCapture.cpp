#include "ScreenCapture.h"

#ifndef NOMINMAX
#define NOMINMAX
#endif
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

#include <windows.h>
#include <dwmapi.h>
#include <d3d11_4.h>
#include <dxgi1_2.h>

#include <winrt/base.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Graphics.h>
#include <winrt/Windows.Graphics.Capture.h>
#include <winrt/Windows.Graphics.DirectX.h>
#include <winrt/Windows.Graphics.DirectX.Direct3D11.h>
#include <windows.graphics.directx.direct3d11.interop.h>

#include <QDebug>
#include <QFileInfo>
#include <QMutexLocker>

#include <algorithm>
#include <chrono>
#include <cstring>
#include <mutex>
#include <utility>

namespace {

constexpr int kMaxCaptureEdge = 16384;
constexpr QChar kIdSep(0x1f);
#ifndef PW_RENDERFULLCONTENT
#define PW_RENDERFULLCONTENT 0x00000002
#endif

struct __declspec(uuid("3628E81B-3CAC-4C60-B7F4-23CE0E0C3356")) IGraphicsCaptureItemInterop : IUnknown {
    virtual HRESULT STDMETHODCALLTYPE CreateForWindow(HWND window, REFIID riid, void** result) = 0;
    virtual HRESULT STDMETHODCALLTYPE CreateForMonitor(HMONITOR monitor, REFIID riid, void** result) = 0;
};

QString hresultMessage(const winrt::hresult_error& error)
{
    return QStringLiteral("%1 (0x%2)")
        .arg(QString::fromWCharArray(error.message().c_str()))
        .arg(static_cast<uint32_t>(error.code()), 8, 16, QChar('0'));
}

QString windowId(const QString& exe, const QString& className, const QString& title)
{
    return QStringLiteral("win") + kIdSep + exe + kIdSep + className + kIdSep + title;
}

QString windowPrefix(const QString& id)
{
    const QStringList parts = id.split(kIdSep);
    if (parts.size() != 4) {
        return {};
    }
    return parts.at(0) + kIdSep + parts.at(1) + kIdSep + parts.at(2) + kIdSep;
}

bool isCapturableWindow(HWND hwnd)
{
    if (!IsWindow(hwnd) || !IsWindowVisible(hwnd)) {
        return false;
    }
    const LONG_PTR style = GetWindowLongPtr(hwnd, GWL_EXSTYLE);
    if (style & WS_EX_TOOLWINDOW) {
        return false;
    }
    DWORD cloaked = 0;
    if (SUCCEEDED(DwmGetWindowAttribute(hwnd, DWMWA_CLOAKED, &cloaked, sizeof(cloaked))) && cloaked != 0) {
        return false;
    }
    wchar_t title[512] = {};
    return GetWindowTextW(hwnd, title, 512) > 0;
}

QString processBaseName(HWND hwnd)
{
    DWORD pid = 0;
    GetWindowThreadProcessId(hwnd, &pid);
    if (pid == 0) {
        return {};
    }
    HANDLE process = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, pid);
    if (!process) {
        return {};
    }
    wchar_t path[MAX_PATH] = {};
    DWORD size = MAX_PATH;
    const BOOL ok = QueryFullProcessImageNameW(process, 0, path, &size);
    CloseHandle(process);
    if (!ok) {
        return {};
    }
    return QFileInfo(QString::fromWCharArray(path)).fileName();
}

BOOL CALLBACK enumWindowProc(HWND hwnd, LPARAM param)
{
    if (!isCapturableWindow(hwnd)) {
        return TRUE;
    }
    auto* list = reinterpret_cast<QList<ScreenSource>*>(param);
    wchar_t title[512] = {};
    wchar_t className[256] = {};
    GetWindowTextW(hwnd, title, 512);
    GetClassNameW(hwnd, className, 256);

    const QString titleText = QString::fromWCharArray(title).trimmed();
    const QString classText = QString::fromWCharArray(className);
    const QString exe = processBaseName(hwnd);
    if (titleText.isEmpty()) {
        return TRUE;
    }

    ScreenSource source;
    source.kind = ScreenSource::Kind::Window;
    source.nativeHandle = reinterpret_cast<quintptr>(hwnd);
    source.id = windowId(exe, classText, titleText);
    source.label = exe.isEmpty() ? titleText : QStringLiteral("%1 — %2").arg(titleText, exe);
    list->append(source);
    return TRUE;
}

struct MonitorEnumContext {
    QList<ScreenSource>* list = nullptr;
    int index = 0;
};

BOOL CALLBACK enumMonitorProc(HMONITOR monitor, HDC, LPRECT, LPARAM param)
{
    auto* ctx = reinterpret_cast<MonitorEnumContext*>(param);
    MONITORINFOEXW info{};
    info.cbSize = sizeof(info);
    if (!GetMonitorInfoW(monitor, &info)) {
        return TRUE;
    }

    const int width = info.rcMonitor.right - info.rcMonitor.left;
    const int height = info.rcMonitor.bottom - info.rcMonitor.top;
    const QString device = QString::fromWCharArray(info.szDevice);
    const bool primary = (info.dwFlags & MONITORINFOF_PRIMARY) != 0;

    ScreenSource source;
    source.kind = ScreenSource::Kind::Display;
    source.nativeHandle = reinterpret_cast<quintptr>(monitor);
    source.id = QStringLiteral("display:%1").arg(device);
    source.label = QStringLiteral("%1×%2  (%3,%4)")
                       .arg(width)
                       .arg(height)
                       .arg(info.rcMonitor.left)
                       .arg(info.rcMonitor.top);
    if (primary) {
        source.label += QStringLiteral("  主屏");
        ctx->list->prepend(source);
    } else {
        ctx->list->append(source);
    }
    ++ctx->index;
    return TRUE;
}

bool monitorRect(HMONITOR monitor, RECT& rect)
{
    MONITORINFO info{};
    info.cbSize = sizeof(info);
    if (!monitor || !GetMonitorInfo(monitor, &info)) {
        return false;
    }
    rect = info.rcMonitor;
    return rect.right > rect.left && rect.bottom > rect.top;
}

bool readDib(HDC dc, HBITMAP bitmap, int width, int height, cv::Mat& output)
{
    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    cv::Mat mat(height, width, CV_8UC4);
    if (GetDIBits(dc, bitmap, 0, static_cast<UINT>(height), mat.data, &info, DIB_RGB_COLORS) == 0) {
        return false;
    }
    output = std::move(mat);
    return true;
}

bool captureScreenRect(const RECT& rect, cv::Mat& output)
{
    const int width = rect.right - rect.left;
    const int height = rect.bottom - rect.top;
    if (width <= 0 || height <= 0 || width > kMaxCaptureEdge || height > kMaxCaptureEdge) {
        return false;
    }

    HDC screen = GetDC(nullptr);
    if (!screen) {
        return false;
    }
    HDC memory = CreateCompatibleDC(screen);
    HBITMAP bitmap = CreateCompatibleBitmap(screen, width, height);
    if (!memory || !bitmap) {
        if (bitmap) {
            DeleteObject(bitmap);
        }
        if (memory) {
            DeleteDC(memory);
        }
        ReleaseDC(nullptr, screen);
        return false;
    }

    HGDIOBJ previous = SelectObject(memory, bitmap);
    const BOOL copied = BitBlt(memory, 0, 0, width, height, screen, rect.left, rect.top, SRCCOPY | CAPTUREBLT);
    bool ok = false;
    if (copied) {
        ok = readDib(memory, bitmap, width, height, output);
    }
    SelectObject(memory, previous);
    DeleteObject(bitmap);
    DeleteDC(memory);
    ReleaseDC(nullptr, screen);
    return ok;
}

bool captureWindowBitBlt(HWND hwnd, cv::Mat& output)
{
    RECT frame{};
    if (FAILED(DwmGetWindowAttribute(hwnd, DWMWA_EXTENDED_FRAME_BOUNDS, &frame, sizeof(frame)))) {
        if (!GetWindowRect(hwnd, &frame)) {
            return false;
        }
    }

    RECT windowRect{};
    if (!GetWindowRect(hwnd, &windowRect)) {
        windowRect = frame;
    }
    const int width = windowRect.right - windowRect.left;
    const int height = windowRect.bottom - windowRect.top;
    if (width <= 0 || height <= 0 || width > kMaxCaptureEdge || height > kMaxCaptureEdge) {
        return captureScreenRect(frame, output);
    }

    HDC windowDc = GetDC(hwnd);
    if (!windowDc) {
        return captureScreenRect(frame, output);
    }
    HDC memory = CreateCompatibleDC(windowDc);
    HBITMAP bitmap = CreateCompatibleBitmap(windowDc, width, height);
    if (!memory || !bitmap) {
        if (bitmap) {
            DeleteObject(bitmap);
        }
        if (memory) {
            DeleteDC(memory);
        }
        ReleaseDC(hwnd, windowDc);
        return captureScreenRect(frame, output);
    }

    HGDIOBJ previous = SelectObject(memory, bitmap);
    const BOOL printed = PrintWindow(hwnd, memory, PW_RENDERFULLCONTENT);
    bool ok = false;
    if (printed) {
        ok = readDib(memory, bitmap, width, height, output);
    }
    SelectObject(memory, previous);
    DeleteObject(bitmap);
    DeleteDC(memory);
    ReleaseDC(hwnd, windowDc);
    if (ok) {
        return true;
    }
    return captureScreenRect(frame, output);
}

winrt::com_ptr<ID3D11Device> createDeviceForMonitor(HMONITOR monitor, winrt::com_ptr<ID3D11DeviceContext>& context)
{
    winrt::com_ptr<IDXGIFactory1> factory;
    winrt::check_hresult(CreateDXGIFactory1(IID_PPV_ARGS(factory.put())));

    winrt::com_ptr<IDXGIAdapter1> matched;
    for (UINT adapterIndex = 0;; ++adapterIndex) {
        winrt::com_ptr<IDXGIAdapter1> adapter;
        if (factory->EnumAdapters1(adapterIndex, adapter.put()) == DXGI_ERROR_NOT_FOUND) {
            break;
        }
        for (UINT outputIndex = 0;; ++outputIndex) {
            winrt::com_ptr<IDXGIOutput> output;
            if (adapter->EnumOutputs(outputIndex, output.put()) == DXGI_ERROR_NOT_FOUND) {
                break;
            }
            DXGI_OUTPUT_DESC desc{};
            if (SUCCEEDED(output->GetDesc(&desc)) && desc.Monitor == monitor) {
                matched = adapter;
                break;
            }
        }
        if (matched) {
            break;
        }
    }

    const D3D_FEATURE_LEVEL levels[] = {
        D3D_FEATURE_LEVEL_11_1,
        D3D_FEATURE_LEVEL_11_0,
        D3D_FEATURE_LEVEL_10_1,
        D3D_FEATURE_LEVEL_10_0,
    };
    winrt::com_ptr<ID3D11Device> device;
    HRESULT hr = E_FAIL;
    if (matched) {
        hr = D3D11CreateDevice(matched.get(),
                               D3D_DRIVER_TYPE_UNKNOWN,
                               nullptr,
                               D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                               levels,
                               ARRAYSIZE(levels),
                               D3D11_SDK_VERSION,
                               device.put(),
                               nullptr,
                               context.put());
    }
    if (FAILED(hr)) {
        device = nullptr;
        context = nullptr;
        hr = D3D11CreateDevice(nullptr,
                               D3D_DRIVER_TYPE_HARDWARE,
                               nullptr,
                               D3D11_CREATE_DEVICE_BGRA_SUPPORT,
                               levels,
                               ARRAYSIZE(levels),
                               D3D11_SDK_VERSION,
                               device.put(),
                               nullptr,
                               context.put());
    }
    winrt::check_hresult(hr);

    winrt::com_ptr<ID3D11Multithread> multithread;
    if (SUCCEEDED(device->QueryInterface(IID_PPV_ARGS(multithread.put())))) {
        multithread->SetMultithreadProtected(TRUE);
    }
    return device;
}

using Direct3DDevice = winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice;

Direct3DDevice wrapDevice(ID3D11Device* device)
{
    winrt::com_ptr<IDXGIDevice> dxgiDevice;
    winrt::check_hresult(device->QueryInterface(IID_PPV_ARGS(dxgiDevice.put())));
    winrt::com_ptr<::IInspectable> inspectable;
    winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgiDevice.get(), inspectable.put()));
    return inspectable.as<Direct3DDevice>();
}

} // namespace

struct ScreenCaptureThread::WgcImpl {
    explicit WgcImpl(ScreenCaptureThread* owner)
        : owner(owner)
    {
    }

    ~WgcImpl()
    {
        stop();
    }

    bool start(const ScreenSource& source, bool captureCursor, QString& error)
    {
        namespace wgc = winrt::Windows::Graphics::Capture;
        HMONITOR monitor = nullptr;
        if (source.kind == ScreenSource::Kind::Display) {
            monitor = reinterpret_cast<HMONITOR>(source.nativeHandle);
        } else {
            monitor = MonitorFromWindow(reinterpret_cast<HWND>(source.nativeHandle), MONITOR_DEFAULTTONEAREST);
        }
        if (!monitor) {
            error = QStringLiteral("无法确定捕获源所在的显示器");
            return false;
        }

        device = createDeviceForMonitor(monitor, context);
        winrtDevice = wrapDevice(device.get());

        auto factory = winrt::get_activation_factory<wgc::GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
        const auto iid = winrt::guid_of<wgc::IGraphicsCaptureItem>();
        if (source.kind == ScreenSource::Kind::Display) {
            winrt::check_hresult(factory->CreateForMonitor(monitor, iid, reinterpret_cast<void**>(winrt::put_abi(item))));
        } else {
            auto* hwnd = reinterpret_cast<HWND>(source.nativeHandle);
            if (!IsWindow(hwnd)) {
                error = QStringLiteral("窗口已关闭");
                return false;
            }
            winrt::check_hresult(factory->CreateForWindow(hwnd, iid, reinterpret_cast<void**>(winrt::put_abi(item))));
        }

        poolSize = item.Size();
        if (poolSize.Width <= 0 || poolSize.Height <= 0) {
            error = QStringLiteral("捕获源尺寸无效");
            return false;
        }

        framePool = wgc::Direct3D11CaptureFramePool::CreateFreeThreaded(
            winrtDevice,
            winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
            2,
            poolSize);
        session = framePool.CreateCaptureSession(item);
        if (auto session2 = session.try_as<wgc::IGraphicsCaptureSession2>()) {
            session2.IsCursorCaptureEnabled(captureCursor);
        }
        if (auto session3 = session.try_as<wgc::IGraphicsCaptureSession3>()) {
            try {
                session3.IsBorderRequired(false);
            } catch (const winrt::hresult_error&) {
                // Windows 10 不允许关掉捕获黄边
            }
        }

        accepting.store(true);
        frameToken = framePool.FrameArrived([this](wgc::Direct3D11CaptureFramePool const& sender, winrt::Windows::Foundation::IInspectable const&) {
            onFrame(sender);
        });
        closedToken = item.Closed([this](wgc::GraphicsCaptureItem const&, winrt::Windows::Foundation::IInspectable const&) {
            if (owner && !owner->m_abort.exchange(true)) {
                emit owner->errorOccurred(QStringLiteral("捕获源已关闭"));
            }
        });
        session.StartCapture();
        return true;
    }

    void stop()
    {
        accepting.store(false);
        if (framePool && frameToken.value != 0) {
            framePool.FrameArrived(frameToken);
            frameToken = {};
        }
        if (item && closedToken.value != 0) {
            item.Closed(closedToken);
            closedToken = {};
        }
        if (session) {
            session.Close();
            session = nullptr;
        }
        if (framePool) {
            framePool.Close();
            framePool = nullptr;
        }
        item = nullptr;
        {
            std::lock_guard<std::mutex> lock(d3dMutex);
            staging = nullptr;
            stageWidth = 0;
            stageHeight = 0;
            context = nullptr;
            device = nullptr;
        }
        winrtDevice = nullptr;
    }

    void onFrame(const winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool& sender)
    {
        if (!accepting.load() || !owner || owner->m_abort.load()) {
            return;
        }

        const int fps = std::clamp(owner->m_fps.load(), 1, 60);
        const auto now = std::chrono::steady_clock::now();
        const auto minInterval = std::chrono::milliseconds(1000 / fps);
        if (lastEmit.time_since_epoch().count() != 0 && now - lastEmit < minInterval) {
            if (auto skipped = sender.TryGetNextFrame()) {
                skipped.Close();
            }
            return;
        }

        try {
            auto frame = sender.TryGetNextFrame();
            if (!frame) {
                return;
            }
            const auto content = frame.ContentSize();
            if (content.Width > 0 && content.Height > 0
                && (content.Width != poolSize.Width || content.Height != poolSize.Height)) {
                poolSize = content;
                frame.Close();
                sender.Recreate(winrtDevice,
                                winrt::Windows::Graphics::DirectX::DirectXPixelFormat::B8G8R8A8UIntNormalized,
                                2,
                                poolSize);
                return;
            }

            auto surface = frame.Surface();
            winrt::com_ptr<::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess> access;
            winrt::check_hresult(static_cast<::IUnknown*>(winrt::get_abi(surface))
                                     ->QueryInterface(__uuidof(::Windows::Graphics::DirectX::Direct3D11::IDirect3DDxgiInterfaceAccess),
                                                      access.put_void()));
            winrt::com_ptr<ID3D11Texture2D> texture;
            winrt::check_hresult(access->GetInterface(__uuidof(ID3D11Texture2D), texture.put_void()));

            cv::Mat mat;
            {
                std::lock_guard<std::mutex> lock(d3dMutex);
                if (!accepting.load() || !device || !context) {
                    return;
                }
                mat = copyTexture(texture.get(), content.Width, content.Height);
            }
            frame.Close();
            if (!mat.empty()) {
                lastEmit = now;
                owner->publishFrame(std::move(mat));
            }
        } catch (const winrt::hresult_error& error) {
            qWarning() << "Screen Capture frame:" << hresultMessage(error);
        } catch (const std::exception& error) {
            qWarning() << "Screen Capture frame:" << error.what();
        }
    }

    cv::Mat copyTexture(ID3D11Texture2D* source, int contentWidth, int contentHeight)
    {
        D3D11_TEXTURE2D_DESC desc{};
        source->GetDesc(&desc);
        const int width = std::clamp(contentWidth > 0 ? contentWidth : static_cast<int>(desc.Width), 0, static_cast<int>(desc.Width));
        const int height = std::clamp(contentHeight > 0 ? contentHeight : static_cast<int>(desc.Height), 0, static_cast<int>(desc.Height));
        if (width <= 0 || height <= 0) {
            return {};
        }

        if (!staging || stageWidth != static_cast<int>(desc.Width) || stageHeight != static_cast<int>(desc.Height)) {
            staging = nullptr;
            D3D11_TEXTURE2D_DESC stage = desc;
            stage.Usage = D3D11_USAGE_STAGING;
            stage.BindFlags = 0;
            stage.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            stage.MiscFlags = 0;
            stage.MipLevels = 1;
            stage.ArraySize = 1;
            stage.SampleDesc.Count = 1;
            stage.SampleDesc.Quality = 0;
            winrt::check_hresult(device->CreateTexture2D(&stage, nullptr, staging.put()));
            stageWidth = static_cast<int>(desc.Width);
            stageHeight = static_cast<int>(desc.Height);
        }

        context->CopyResource(staging.get(), source);
        D3D11_MAPPED_SUBRESOURCE mapped{};
        winrt::check_hresult(context->Map(staging.get(), 0, D3D11_MAP_READ, 0, &mapped));
        cv::Mat mat(height, width, CV_8UC4);
        const auto* rows = static_cast<const uint8_t*>(mapped.pData);
        for (int y = 0; y < height; ++y) {
            std::memcpy(mat.ptr(y), rows + static_cast<size_t>(y) * mapped.RowPitch, static_cast<size_t>(width) * 4);
        }
        context->Unmap(staging.get(), 0);
        return mat;
    }

    ScreenCaptureThread* owner = nullptr;
    std::atomic<bool> accepting{false};
    std::mutex d3dMutex;
    winrt::com_ptr<ID3D11Device> device;
    winrt::com_ptr<ID3D11DeviceContext> context;
    winrt::com_ptr<ID3D11Texture2D> staging;
    int stageWidth = 0;
    int stageHeight = 0;
    Direct3DDevice winrtDevice{nullptr};
    winrt::Windows::Graphics::Capture::GraphicsCaptureItem item{nullptr};
    winrt::Windows::Graphics::Capture::Direct3D11CaptureFramePool framePool{nullptr};
    winrt::Windows::Graphics::Capture::GraphicsCaptureSession session{nullptr};
    winrt::Windows::Graphics::SizeInt32 poolSize{};
    winrt::event_token frameToken{};
    winrt::event_token closedToken{};
    std::chrono::steady_clock::time_point lastEmit{};
};

ScreenCaptureThread::ScreenCaptureThread(QObject* parent)
    : QThread(parent)
{
    qRegisterMetaType<cv::Mat>("cv::Mat");
}

ScreenCaptureThread::~ScreenCaptureThread()
{
    stop();
    if (isRunning() && !wait(8000)) {
        qWarning() << "ScreenCaptureThread: did not stop in time, terminating";
        terminate();
        wait();
    }
}

QList<ScreenSource> ScreenCaptureThread::listDisplays()
{
    QList<ScreenSource> list;
    MonitorEnumContext ctx{&list, 0};
    EnumDisplayMonitors(nullptr, nullptr, enumMonitorProc, reinterpret_cast<LPARAM>(&ctx));
    for (int i = 0; i < list.size(); ++i) {
        list[i].label = QStringLiteral("显示器 %1  ").arg(i + 1) + list[i].label;
    }
    return list;
}

QList<ScreenSource> ScreenCaptureThread::listWindows()
{
    QList<ScreenSource> list;
    EnumWindows(enumWindowProc, reinterpret_cast<LPARAM>(&list));
    std::sort(list.begin(), list.end(), [](const ScreenSource& a, const ScreenSource& b) {
        return a.label.localeAwareCompare(b.label) < 0;
    });
    return list;
}

bool ScreenCaptureThread::resolve(ScreenSource& source)
{
    if (source.kind == ScreenSource::Kind::Display) {
        const QList<ScreenSource> displays = listDisplays();
        for (const ScreenSource& display : displays) {
            if (display.id == source.id) {
                source = display;
                return source.nativeHandle != 0;
            }
        }
        return false;
    }

    if (source.nativeHandle != 0 && IsWindow(reinterpret_cast<HWND>(source.nativeHandle))) {
        const QList<ScreenSource> windows = listWindows();
        for (const ScreenSource& window : windows) {
            if (window.nativeHandle == source.nativeHandle) {
                source.id = window.id;
                source.label = window.label;
                return true;
            }
        }
        return true;
    }

    const QList<ScreenSource> windows = listWindows();
    for (const ScreenSource& window : windows) {
        if (window.id == source.id) {
            source = window;
            return true;
        }
    }

    const QString prefix = windowPrefix(source.id);
    if (!prefix.isEmpty()) {
        const ScreenSource* matched = nullptr;
        int count = 0;
        for (const ScreenSource& window : windows) {
            if (window.id.startsWith(prefix)) {
                matched = &window;
                ++count;
            }
        }
        if (count == 1 && matched) {
            source = *matched;
            return true;
        }
    }
    return false;
}

void ScreenCaptureThread::startCapture(const ScreenSource& source, bool captureCursor, int fps)
{
    if (isRunning()) {
        stop();
        if (!wait(8000)) {
            qWarning() << "ScreenCaptureThread: previous session still running";
            terminate();
            wait();
        }
    }

    {
        QMutexLocker locker(&m_mutex);
        m_source = source;
        m_captureCursor.store(captureCursor);
        m_fps.store(std::clamp(fps, 1, 60));
        m_abort.store(false);
    }
    start();
}

void ScreenCaptureThread::setFps(int fps)
{
    m_fps.store(std::clamp(fps, 1, 60));
}

void ScreenCaptureThread::stop()
{
    m_abort.store(true);
}

void ScreenCaptureThread::publishFrame(cv::Mat frame)
{
    if (!frame.empty() && !m_abort.load()) {
        emit frameAvailable(frame);
    }
}

bool ScreenCaptureThread::startWgc(const ScreenSource& source, bool captureCursor, QString& error)
{
    stopWgc();
    if (!winrt::Windows::Graphics::Capture::GraphicsCaptureSession::IsSupported()) {
        error = QStringLiteral("系统不支持 Windows Graphics Capture");
        return false;
    }
    try {
        auto impl = std::make_unique<WgcImpl>(this);
        if (!impl->start(source, captureCursor, error)) {
            return false;
        }
        m_wgc = std::move(impl);
        return true;
    } catch (const winrt::hresult_error& errorCode) {
        error = hresultMessage(errorCode);
        m_wgc.reset();
        return false;
    } catch (const std::exception& exception) {
        error = QString::fromUtf8(exception.what());
        m_wgc.reset();
        return false;
    }
}

void ScreenCaptureThread::stopWgc()
{
    m_wgc.reset();
}

void ScreenCaptureThread::fallbackLoop(const ScreenSource& source)
{
    while (!m_abort.load()) {
        const auto started = std::chrono::steady_clock::now();
        cv::Mat frame;
        bool ok = false;
        if (source.kind == ScreenSource::Kind::Window) {
            auto* hwnd = reinterpret_cast<HWND>(source.nativeHandle);
            if (!IsWindow(hwnd)) {
                emit errorOccurred(QStringLiteral("窗口已关闭"));
                break;
            }
            ok = captureWindowBitBlt(hwnd, frame);
        } else {
            RECT rect{};
            if (!monitorRect(reinterpret_cast<HMONITOR>(source.nativeHandle), rect)) {
                emit errorOccurred(QStringLiteral("显示器无效"));
                break;
            }
            ok = captureScreenRect(rect, frame);
        }
        if (ok) {
            publishFrame(std::move(frame));
        }

        const int fps = std::clamp(m_fps.load(), 1, 60);
        const auto budget = std::chrono::milliseconds(1000 / fps);
        const auto spent = std::chrono::steady_clock::now() - started;
        if (spent < budget) {
            QThread::msleep(static_cast<unsigned long>(std::chrono::duration_cast<std::chrono::milliseconds>(budget - spent).count()));
        }
    }
}

void ScreenCaptureThread::run()
{
    m_abort.store(false);
    ScreenSource source;
    bool captureCursor = true;
    {
        QMutexLocker locker(&m_mutex);
        source = m_source;
        captureCursor = m_captureCursor.load();
    }
    if (!resolve(source)) {
        emit errorOccurred(QStringLiteral("找不到捕获源"));
        emit captureStateChanged(false, QString());
        return;
    }

    bool ownApartment = false;
    try {
        winrt::init_apartment(winrt::apartment_type::multi_threaded);
        ownApartment = true;
    } catch (const winrt::hresult_error& error) {
        if (error.code() != static_cast<winrt::hresult>(RPC_E_CHANGED_MODE)) {
            emit errorOccurred(hresultMessage(error));
            emit captureStateChanged(false, QString());
            return;
        }
    }

    QString wgcError;
    const bool wgc = startWgc(source, captureCursor, wgcError);
    if (wgc) {
        emit captureStateChanged(true, QStringLiteral("Graphics Capture"));
        while (!m_abort.load()) {
            QThread::msleep(30);
        }
        stopWgc();
    } else {
        qWarning() << "Screen Capture: Graphics Capture failed, using BitBlt:" << wgcError;
        emit captureStateChanged(true, QStringLiteral("BitBlt"));
        fallbackLoop(source);
    }

    emit captureStateChanged(false, QString());
    if (ownApartment) {
        winrt::uninit_apartment();
    }
}
