// ============================================================================
//  LiveFrame —— 实时源统一帧快照（Live Hub 数据平面唯一载荷）
//
//  贯通链路（2026-11-06 实时源定稿）：
//      Ingest（NdiIngest 收外部源）→ LiveHub 帧缓冲（Feed）
//        ├─ 预览直用：LiveMediaSource(Hub 模式) 进程内取帧（零拷贝优势）
//        └─ NDI 再发布：sender 线程 Feed → NDIlib_send（ms-live/<proj>/<id>）
//      display 端：LiveMediaSource(NDI 模式) 收再发布流
//
//  不可变语义：发布后的 LiveFrame 内容永不修改——所有消费方经
//  shared_ptr<const LiveFrame> 持有，新帧 = 新对象（与 FrameView.keep
//  同一保活模式，无锁竞争、无悬垂窗口）。
// ============================================================================

#pragma once

#include <cstdint>
#include <memory>
#include <vector>

namespace ms {

struct LiveFrame {
    int                       w = 0; ///< 帧宽（px）
    int                       h = 0; ///< 帧高（px）
    std::vector<std::uint8_t> rgba;  ///< RGBA 像素（行主序，stride = w*4）

    // 【帧同步 2026-11-20】到达时刻戳（进程级对齐时钟，Unix 纪元 μs）：
    // Ingest 接收线程产出帧时打（收端接收时刻语义，含采集+传输管道延迟
    // ——多端同步只要求同戳同延迟，绝对值不要求精确）。display 端按
    // "pts + target_latency" 排程显示，排程独立于播放头（暂停继续流动）。
    std::int64_t              pts_us = 0;
};

using LiveFramePtr = std::shared_ptr<const LiveFrame>;

} // namespace ms
