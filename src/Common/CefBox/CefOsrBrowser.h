#pragma once
// =============================================================================
// CefOsrBrowser — 无窗口（OSR）浏览器
//
// - 帧输出：OnPaint → BGRA8（预乘 alpha）软件位图 → FrameCallback
//   （P2 升级 OnAcceleratedPaint 共享纹理时在此类内切换，对节点层透明）
// - 弹窗策略：全部阻断（OnBeforePopup 返回 true）
// - 线程约定：全部方法在 CEF 主线程（= CefRuntime::initialize() 线程）调用。
//   external_message_pump=true 时 CEF 的 UI 线程即主线程，回调天然同步。
// =============================================================================
#include "CefBoxExport.h"

#include "include/cef_client.h"

#include <atomic>
#include <functional>
#include <string>

namespace cefbox {

class CEFBOX_EXPORT CefOsrBrowser : public CefClient,
                                    public CefLifeSpanHandler,
                                    public CefRenderHandler,
                                    public CefLoadHandler,
                                    public CefDisplayHandler {
public:
    /// BGRA8 预乘 alpha 帧回调；buffer 生命周期仅限回调内（需要保留须拷出）。
    using FrameCallback = std::function<void(const void* bgra, int width, int height)>;
    /// 加载结果回调（ok=false 见 qDebug 日志）。
    using LoadCallback = std::function<void(bool ok)>;

    /// 创建 OSR 浏览器并加载 url（UTF-8）。runtime 必须已 initialize；主线程调用。
    static CefRefPtr<CefOsrBrowser> create(int width, int height,
                                           const std::string& urlUtf8,
                                           FrameCallback onFrame,
                                           LoadCallback onLoad = {});

    /// 尚未被 OnBeforeClose 关闭的浏览器数（CefRuntime::shutdown 排空等待用）
    static int aliveCount();

    /// 导航到新 URL（UTF-8）
    void loadUrl(const std::string& urlUtf8);
    /// 重载当前页（等价 CDP Page.reload）
    void reloadPage();
    /// 在主 frame 执行 JS（UTF-8）
    void executeJavaScript(const std::string& jsUtf8);
    /// 改变视图尺寸（触发 WasResized → 新帧）
    void resize(int width, int height);
    /// 优雅关闭（等 OnBeforeClose 后宿主即可丢弃引用）
    void close();
    /// 摘除全部回调（节点析构路径用——保证此后 OnPaint 不再触达节点）
    void clearCallbacks();
    bool isClosed() const { return m_closed; }
    int width() const { return m_width; }
    int height() const { return m_height; }
    /// 页面标题（OnTitleChange 累积，UTF-8；主 frame 才更新）
    const std::string& title() const { return m_title; }
    /// 当前主 frame URL（UTF-8；浏览器未就绪返回空）
    std::string currentUrl() const;

    // ---- CefClient ----
    CefRefPtr<CefLifeSpanHandler> GetLifeSpanHandler() override { return this; }
    CefRefPtr<CefRenderHandler> GetRenderHandler() override { return this; }
    CefRefPtr<CefLoadHandler> GetLoadHandler() override { return this; }
    CefRefPtr<CefDisplayHandler> GetDisplayHandler() override { return this; }

    // ---- CefLifeSpanHandler ----
    void OnAfterCreated(CefRefPtr<CefBrowser> browser) override;
    bool OnBeforePopup(CefRefPtr<CefBrowser> browser,
                       CefRefPtr<CefFrame> frame,
                       int popup_id,
                       const CefString& target_url,
                       const CefString& target_frame_name,
                       WindowOpenDisposition target_disposition,
                       bool user_gesture,
                       const CefPopupFeatures& popupFeatures,
                       CefWindowInfo& windowInfo,
                       CefRefPtr<CefClient>& client,
                       CefBrowserSettings& settings,
                       CefRefPtr<CefDictionaryValue>& extra_info,
                       bool* no_javascript_access) override;
    void OnBeforeClose(CefRefPtr<CefBrowser> browser) override;

    // ---- CefRenderHandler ----
    void GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) override;
    void OnPaint(CefRefPtr<CefBrowser> browser,
                 PaintElementType type,
                 const RectList& dirtyRects,
                 const void* buffer,
                 int width,
                 int height) override;

    // ---- CefLoadHandler ----
    void OnLoadEnd(CefRefPtr<CefBrowser> browser,
                   CefRefPtr<CefFrame> frame,
                   int httpStatusCode) override;
    void OnLoadError(CefRefPtr<CefBrowser> browser,
                     CefRefPtr<CefFrame> frame,
                     ErrorCode errorCode,
                     const CefString& errorText,
                     const CefString& failedUrl) override;

    // ---- CefDisplayHandler ----
    void OnTitleChange(CefRefPtr<CefBrowser> browser,
                       const CefString& title) override;

private:
    CefOsrBrowser(int width, int height, FrameCallback onFrame, LoadCallback onLoad);
    ~CefOsrBrowser() override;

    CefRefPtr<CefBrowser> m_browser;
    FrameCallback m_onFrame;
    LoadCallback m_onLoad;
    int m_width;
    int m_height;
    bool m_closed = false;
    std::string m_title;

    static std::atomic<int> s_aliveCount;

    IMPLEMENT_REFCOUNTING(CefOsrBrowser);
    DISALLOW_COPY_AND_ASSIGN(CefOsrBrowser);
};

}  // namespace cefbox
