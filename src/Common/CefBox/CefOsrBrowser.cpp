#include "CefOsrBrowser.h"

#include "include/cef_browser.h"
#include "include/cef_frame.h"
#include "include/wrapper/cef_helpers.h"

#include <cstdio>

namespace cefbox {

namespace {
// OSR 视图尺寸下限：CEF 要求 GetViewRect 返回正值，否则浏览器创建/合成异常
constexpr int kMinViewSize = 1;
}  // namespace

std::atomic<int> CefOsrBrowser::s_aliveCount{0};

int CefOsrBrowser::aliveCount() {
    return s_aliveCount.load(std::memory_order_relaxed);
}

CefRefPtr<CefOsrBrowser> CefOsrBrowser::create(int width, int height,
                                               const std::string& urlUtf8,
                                               FrameCallback onFrame,
                                               LoadCallback onLoad) {
    CEF_REQUIRE_UI_THREAD();
    if (width < 1 || height < 1 || !onFrame)
        return nullptr;

    CefRefPtr<CefOsrBrowser> browser =
        new CefOsrBrowser(width, height, std::move(onFrame), std::move(onLoad));

    CefWindowInfo windowInfo;
    windowInfo.SetAsWindowless(kNullWindowHandle);

    CefBrowserSettings browserSettings;  // 全部继承 CefSettings 全局值
    if (!CefBrowserHost::CreateBrowser(windowInfo, browser.get(), urlUtf8,
                                       browserSettings, nullptr, nullptr)) {
        return nullptr;
    }
    s_aliveCount.fetch_add(1, std::memory_order_relaxed);
    return browser;
}

CefOsrBrowser::CefOsrBrowser(int width, int height, FrameCallback onFrame, LoadCallback onLoad)
    : m_onFrame(std::move(onFrame)),
      m_onLoad(std::move(onLoad)),
      m_width(width > 0 ? width : kMinViewSize),
      m_height(height > 0 ? height : kMinViewSize) {}

CefOsrBrowser::~CefOsrBrowser() = default;

void CefOsrBrowser::loadUrl(const std::string& urlUtf8) {
    if (!m_browser)
        return;
    m_browser->GetMainFrame()->LoadURL(urlUtf8);
}

void CefOsrBrowser::reloadPage() {
    if (!m_browser)
        return;
    m_browser->Reload();
}

void CefOsrBrowser::executeJavaScript(const std::string& jsUtf8) {
    if (!m_browser)
        return;
    CefRefPtr<CefFrame> frame = m_browser->GetMainFrame();
    if (frame)
        frame->ExecuteJavaScript(jsUtf8, frame->GetURL(), 0);
}

void CefOsrBrowser::resize(int width, int height) {
    m_width = width > 0 ? width : kMinViewSize;
    m_height = height > 0 ? height : kMinViewSize;
    if (m_browser)
        m_browser->GetHost()->WasResized();
}

void CefOsrBrowser::close() {
    if (!m_browser)
        return;
    m_browser->GetHost()->CloseBrowser(false);  // 不强制；页面 confirm 等已被 OSR 抑制
}

void CefOsrBrowser::clearCallbacks() {
    m_onFrame = nullptr;
    m_onLoad = nullptr;
}

std::string CefOsrBrowser::currentUrl() const {
    if (!m_browser)
        return std::string();
    CefRefPtr<CefFrame> frame = m_browser->GetMainFrame();
    return frame ? frame->GetURL().ToString() : std::string();
}

// ---- CefLifeSpanHandler ----

void CefOsrBrowser::OnAfterCreated(CefRefPtr<CefBrowser> browser) {
    m_browser = browser;
}

bool CefOsrBrowser::OnBeforePopup(CefRefPtr<CefBrowser> browser,
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
                                  bool* no_javascript_access) {
    // 弹窗一律阻断（Web Render 节点无 UI 承载弹窗）
    std::fprintf(stderr, "[CefOsrBrowser] popup blocked: %ls\n",
                 CefString(target_url).ToWString().c_str());
    return true;
}

void CefOsrBrowser::OnBeforeClose(CefRefPtr<CefBrowser> browser) {
    m_browser = nullptr;  // OnBeforeClose 后宿主不得再访问 browser
    m_closed = true;
    s_aliveCount.fetch_sub(1, std::memory_order_relaxed);
}

// ---- CefRenderHandler ----

void CefOsrBrowser::GetViewRect(CefRefPtr<CefBrowser> browser, CefRect& rect) {
    rect = CefRect(0, 0, m_width, m_height);
}

void CefOsrBrowser::OnPaint(CefRefPtr<CefBrowser> browser,
                            PaintElementType type,
                            const RectList& dirtyRects,
                            const void* buffer,
                            int width,
                            int height) {
    if (type != PET_VIEW || !buffer || width < 1 || height < 1)
        return;  // 弹窗层已阻断，忽略 PET_POPUP 兜底
    if (m_onFrame)
        m_onFrame(buffer, width, height);
}

// ---- CefLoadHandler ----

void CefOsrBrowser::OnLoadEnd(CefRefPtr<CefBrowser> browser,
                              CefRefPtr<CefFrame> frame,
                              int httpStatusCode) {
    if (!frame->IsMain())
        return;
    if (m_onLoad)
        m_onLoad(true);
}

void CefOsrBrowser::OnLoadError(CefRefPtr<CefBrowser> browser,
                                CefRefPtr<CefFrame> frame,
                                ErrorCode errorCode,
                                const CefString& errorText,
                                const CefString& failedUrl) {
    if (errorCode == ERR_ABORTED)  // 主动导航中止不算失败
        return;
    if (!frame->IsMain())
        return;
    std::fprintf(stderr, "[CefOsrBrowser] load error %d: %ls (%ls)\n", (int)errorCode,
                 CefString(errorText).ToWString().c_str(),
                 CefString(failedUrl).ToWString().c_str());
    if (m_onLoad)
        m_onLoad(false);
}

void CefOsrBrowser::OnTitleChange(CefRefPtr<CefBrowser> browser, const CefString& title) {
    m_title = title.ToString();
}

}  // namespace cefbox
