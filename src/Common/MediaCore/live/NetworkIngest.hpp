// ============================================================================
//  NetworkIngest —— 网络流采集器（ILiveIngest 注册表 Network 条目实现）
//
//  定位（2026-11-21 组播转发定案）：display 端收 editor LiveHub 的
//  MPEG-TS over UDP 组播流（多 display 共享一路入网带宽——NDI 单播
//  ×N 的替代）；URL 通吃 udp://（组播/单播）/ rtsp:// / srt://。
//
//  管线：avformat 打开 URL（低延迟组合拳：NOBUFFER + probesize 32k +
//  analyzeduration 0 + 3s 超时断流检测）→ H.264 软解（硬解后续可加）
//  → sws YUV→RGBA → LiveFrame 打进程对齐接收戳 → sink。
//
//  打戳语义：display 端接收/解码完成时刻（processAlignedNowUs，含
//  SNTP offset）——与帧同步排程（LiveMediaSource pts+target_latency）
//  无缝衔接：多 display 同戳同延迟 → 跨机帧同步。
//
//  自愈：av_read_frame 失败/超时 → 关闭重开（500ms 退避）；源离线
//  占位语义与 NdiIngest 一致。线程骨架继承 IngestBase（start/stop/
//  诊断/异常壳全继承）。
// ============================================================================

#pragma once

#include "live/IngestBase.hpp"

namespace ms {

class NetworkIngest final : public IngestBase {
public:
    /// \param url 流地址（udp://239.x.x.x:port 组播 / rtsp:// / srt://）
    /// \param latency_mode 0=最低（NOBUFFER 直通）1=同步（稍大缓冲）
    NetworkIngest(std::string url, int latency_mode, FrameSink sink)
        : IngestBase(std::move(url), latency_mode, std::move(sink)) {}

protected:
    void recvLoop() override;
    bool onStart() override; // avformat_network_init（进程内一次）
    const char* logTag() const noexcept override { return "net"; }
};

} // namespace ms
