// =============================================================================
// cefbox_test — P0 冒烟测试
//
// 验收（CefRefactorPlan.md P0）：
// 1. CefRuntime 初始化 + FlowCefHelper 子进程树拉起
// 2. 本地动画 HTML 加载 → OnPaint BGRA 帧计数（>100 帧）
// 3. close → OnBeforeClose → CefShutdown → 进程自然退出（零挂起、零残留进程）
// 返回码：0=PASS，非 0=FAIL
// =============================================================================
#include "CefOsrBrowser.h"
#include "CefRuntime.h"

#include <windows.h>

#include <atomic>
#include <chrono>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

namespace fs = std::filesystem;

namespace {

// 持续动画页面：CSS 动画 + rAF 计数，驱动合成器连续出帧
const char kTestHtml[] = R"HTML(<!doctype html>
<html><head><meta charset="utf-8"><style>
body{margin:0;background:#102030;overflow:hidden}
#box{position:absolute;width:120px;height:120px;border-radius:60px;background:#ff8800;
     animation:spin 2s linear infinite}
@keyframes spin{from{transform:translate(0px,200px) rotate(0deg)}
                to{transform:translate(1100px,200px) rotate(720deg)}}
#hud{position:absolute;left:12px;top:12px;color:#8ff;font:24px monospace}
</style></head><body>
<div id="box"></div><div id="hud">frame 0</div>
<script>let f=0;function tick(){f++;document.getElementById('hud').textContent='frame '+f;
requestAnimationFrame(tick)}requestAnimationFrame(tick);</script>
</body></html>)HTML";

std::string wideToUtf8(const std::wstring& w) {
    if (w.empty())
        return std::string();
    int n = WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), nullptr, 0, nullptr, nullptr);
    std::string out((size_t)n, '\0');
    if (n > 0)
        WideCharToMultiByte(CP_UTF8, 0, w.data(), (int)w.size(), out.data(), n, nullptr, nullptr);
    return out;
}

// 本地路径 → file:// URL（仅做基本转义，测试路径无特殊字符场景够用）
std::string pathToFileUrl(const fs::path& p) {
    std::string s = wideToUtf8(p.wstring());
    std::string out = "file:///";
    for (char c : s) {
        if (c == '\\')
            out += '/';
        else if (c == ' ')
            out += "%20";
        else
            out += c;
    }
    return out;
}

void pumpForMs(int ms) {
    auto t0 = std::chrono::steady_clock::now();
    MSG msg;
    while (std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - t0)
               .count() < ms) {
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
}

}  // namespace

int main() {
    wchar_t tempDirW[MAX_PATH];
    GetTempPathW(MAX_PATH, tempDirW);
    fs::path tempDir(tempDirW);

    fs::path htmlPath = tempDir / L"cefbox_test.html";
    {
        std::ofstream f(htmlPath, std::ios::binary);
        f << kTestHtml;
    }
    fs::path cacheDir = tempDir / ("cefbox_test_cache_" + std::to_string(GetCurrentProcessId()));

    cefbox::CefRuntime::Settings rs;
    rs.cacheDir = wideToUtf8(cacheDir.wstring());
    if (!cefbox::CefRuntime::instance().initialize(rs)) {
        std::fprintf(stderr, "[test] CefRuntime initialize FAILED\n");
        return 2;
    }
    std::fprintf(stderr, "[test] runtime initialized\n");

    std::atomic<int> frames{0};
    std::atomic<bool> loaded{false};
    int firstW = 0, firstH = 0;
    long long lastPaintMs = 0;
    auto t0 = std::chrono::steady_clock::now();

    auto browser = cefbox::CefOsrBrowser::create(
        1280, 720, pathToFileUrl(htmlPath),
        [&](const void* bgra, int w, int h) {
            int n = frames.fetch_add(1) + 1;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now() - t0)
                          .count();
            if (n == 1) {
                firstW = w;
                firstH = h;
                std::fprintf(stderr, "[frame #1] %dx%d @%lldms\n", w, h, (long long)ms);
            } else if (n % 60 == 0) {
                std::fprintf(stderr, "[frame #%d] %dx%d @%lldms\n", n, w, h, (long long)ms);
            }
            lastPaintMs = ms;
            (void)bgra;
        },
        [&](bool ok) {
            loaded = ok;
            auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(
                          std::chrono::steady_clock::now() - t0)
                          .count();
            std::fprintf(stderr, "[load] %s @%lldms\n", ok ? "ok" : "fail", (long long)ms);
        });

    if (!browser) {
        std::fprintf(stderr, "[test] browser create FAILED\n");
        return 3;
    }

    pumpForMs(10000);  // 跑 10 秒

    const int frameCount = frames.load();
    std::fprintf(stderr, "[result] frames=%d loaded=%d size=%dx%d lastPaint@%lldms\n",
                 frameCount, (int)loaded.load(), firstW, firstH, lastPaintMs);

    // 优雅关闭链：close → 等 OnBeforeClose → CefShutdown → 进程自然结束
    browser->close();
    auto tClose = std::chrono::steady_clock::now();
    while (!browser->isClosed() &&
           std::chrono::duration_cast<std::chrono::milliseconds>(
               std::chrono::steady_clock::now() - tClose)
                   .count() < 5000) {
        MSG msg;
        while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
            TranslateMessage(&msg);
            DispatchMessageW(&msg);
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(2));
    }
    std::fprintf(stderr, "[test] browser closed=%d\n", (int)browser->isClosed());

    cefbox::CefRuntime::instance().shutdown();
    std::fprintf(stderr, "[test] runtime shutdown done\n");

    const bool pass = frameCount > 100 && loaded.load() && browser->isClosed();
    std::fprintf(stderr, pass ? "[PASS]\n" : "[FAIL]\n");
    return pass ? 0 : 1;
}
