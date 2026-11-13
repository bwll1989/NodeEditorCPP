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
};

using LiveFramePtr = std::shared_ptr<const LiveFrame>;

} // namespace ms
