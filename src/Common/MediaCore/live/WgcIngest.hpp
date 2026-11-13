// ============================================================================
//  WgcIngest —— 本机窗口/显示器捕获（Windows.Graphics.Capture；注册表
//  Window/Monitor 两条目共用本实现）
//
//  技术路线（2026-11-08 定案）：
//    * WGC CreateFreeThreaded 帧池（无 DispatcherQueue 依赖，直接在工作
//      线程消费）+ TryGetNextFrame 轮询（~60fps 节流，与 SpoutIngest
//      同构的"最新帧"语义）；
//    * 帧面 surface 经 IDirect3DDxgiInterfaceAccess 取 ID3D11Texture2D
//      → DxTextureReadback 统一读回（与 SpoutIngest 共享路径）；
//    * 目标定位：asset.live_source_name 携带 "[hwnd:HEX]" / "[hmon:HEX]"
//      后缀（list 枚举时生成——UI 契约见 live/LiveIngest.hpp）；解析失败
//      / 目标关闭 = 离线占位 + 发现循环重试；
//    * 自愈：item.Closed 事件 → 重连；内容尺寸变化 → 帧池 Recreate。
//
//  依赖：C++/WinRT（Windows SDK 自带头，10.0.17763+；CreateFreeThreaded
//  需 18362+）；链接 windowsapp + dwmapi（枚举过滤 cloaked 窗口）。
//
//  线程生命周期/诊断/兜底壳：继承 IngestBase（2026-11-09 复用整理）。
// ============================================================================

#pragma once

#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "live/IngestBase.hpp"

namespace ms {

class WgcIngest : public IngestBase {
public:
    /// 捕获目标类别（注册表 Window/Monitor 两条目的实现分派）
    enum class Kind { Window, Monitor };

    WgcIngest(Kind kind, std::string source_ref, int latency_mode,
              FrameSink sink);
    ~WgcIngest() override;

    WgcIngest(const WgcIngest&)            = delete;
    WgcIngest& operator=(const WgcIngest&) = delete;

private:
    void        recvLoop() override;
    bool        onStart() override; // GraphicsCaptureSession::IsSupported 门控
    const char* logTag() const noexcept override { return "wgc"; }

    /// 可中断等待：50ms 短拍轮询 running_（2026-11-10）——stop() join 接收
    /// 线程，长睡眠（500ms 重试窗口）会把 join 卡满整段；析构可能在渲染
    /// 线程（shared_ptr 最后持有者），短拍把析构卡顿上界压到 ~50ms
    void sleepInterruptible(std::uint32_t total_ms) {
        using namespace std::chrono;
        for (std::uint32_t slept = 0; slept < total_ms; slept += 50) {
            if (!running_.load(std::memory_order_acquire)) return;
            std::this_thread::sleep_for(milliseconds(50));
        }
    }

    // ---- 配置（start 期固定；线程只读）----
    Kind kind_; ///< 捕获目标类别（定位后缀 hwnd/hmon 解析与 Create 分派）
};

/// WGC 系统支持探测（GraphicsCaptureSession::IsSupported 包装——单测/
/// UI 门控用；老系统无此 API 返回 false）
bool wgcProbeSupported();

/// 窗口/显示器枚举（注册表 list 包装；即时返回，wait_ms 兼容签名）
std::vector<IngestSource> listWgcWindowSources(std::uint32_t wait_ms);
std::vector<IngestSource> listWgcMonitorSources(std::uint32_t wait_ms);

/// 工厂（注册表 create 包装；Window/Monitor 各一）
std::unique_ptr<ILiveIngest> makeWgcWindow(std::string source_name,
                                           int        latency_mode,
                                           ILiveIngest::FrameSink sink);
std::unique_ptr<ILiveIngest> makeWgcMonitor(std::string source_name,
                                            int        latency_mode,
                                            ILiveIngest::FrameSink sink);

} // namespace ms
