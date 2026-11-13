// ============================================================================
//  NdiIngest —— NDI 收端核心（Live Hub Ingest 层唯一 NDI 实现）
//
//  职责（§实时源方案 §Ingest 统一小接口）：
//    * 按 source_name 在 NDI 发现列表匹配源 → 建 receiver 连流；
//    * 接收线程持续拉帧（drain/直通双模式对应 latency 语义），
//      每帧经回调上抛（回调在接收线程执行——订阅方自行保证发布安全）；
//    * 断线自动重连（回到发现循环）。
//
//  与 LiveMediaSource 的关系：收端逻辑唯一实现在此——LiveMediaSource 的
//  NDI 模式内嵌一个 NdiIngest 实例（display 端复用同一收端核心，两端
//  同源渲染铁律在数据平面的落点）；Hub 的 Ingest 亦直接持有本类。
//
//  NDI 运行时引用计数（2026-11-09 修正）：**实例级持有一份**——首次
//  start() 成功时 ndiAcquire（ndi_held_ 标志防重），析构时配对 release。
//  旧版 stop() 无条件 release（正常 stop 后析构兜底再调一次 = 计数双减，
//  其他 NDI 实例的运行时可能被提前销毁）——已修。
//
//  线程生命周期/诊断/兜底壳：继承 IngestBase（2026-11-09 复用整理）。
// ============================================================================

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "live/IngestBase.hpp"

namespace ms {

class NdiIngest : public IngestBase {
public:
    using FrameSink = ILiveIngest::FrameSink; // 兼容别名（外部引用点）

    NdiIngest(std::string source_name, int latency_mode, FrameSink sink);
    ~NdiIngest() override;

    NdiIngest(const NdiIngest&)            = delete;
    NdiIngest& operator=(const NdiIngest&) = delete;

    /// 一次性源枚举（UI「刷新」按钮用）：临时发现器 wait_ms 拍取当前
    /// 发现列表并销毁——与长驻接收器互不影响。NDI 运行时初始化失败 /
    /// 超时无源返回空表（UI 显示"未发现源"）。调用同步阻塞 ≤ wait_ms。
    static std::vector<std::string> listSources(std::uint32_t wait_ms = 300);

private:
    void        recvLoop() override;
    bool        onStart() override;   ///< NDI 运行时 acquire（实例级持有）
    void        onCleanup() override; ///< recv/finder 销毁（幂等；不动计数）
    const char* logTag() const noexcept override { return "live-ingest"; }

    /// NDI 帧缓冲 → LiveFrame 快照 → 基类 emitFrame（字节序按 FourCC
    /// 分派——RGBX_RGBA 请求下 BGRX/RGBA 双布局，见实现注释）
    void publishNdiFrame(const void* ndi_video_frame);

    // ---- NDI 实例（线程私有）----
    bool  ndi_held_ = false; ///< 运行时引用已持有（析构配对 release）
    void* recv_   = nullptr; ///< NDIlib_recv_instance_t
    void* finder_ = nullptr; ///< NDIlib_find_instance_t

    // ---- 接收线程私有 ----
    std::int64_t unsupported_log_ms_ = -1; ///< 不支持格式限频日志状态
};

} // namespace ms
