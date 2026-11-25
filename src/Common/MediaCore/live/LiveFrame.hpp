// ============================================================================
//  LiveFrame —— 实时源统一帧快照（Live Hub 数据平面唯一载荷）
//
//  贯通链路（2026-11-06 实时源定稿；2026-11-25 组播回归）：
//      Ingest（NdiIngest 收外部源）→ LiveHub 帧缓冲（Feed）
//        ├─ 预览直用：LiveMediaSource(Hub 模式) 进程内取帧（零拷贝优势）
//        └─ 组播再发布：MpegTsSender Feed → H.264 → MPEG-TS → UDP 组播
//           （multicastUrlFor(asset_id) 确定性地址）
//      display 端：LiveMediaSource(Network 模式) 收组播流
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

    // 【帧同步 2026-11-25 语义定稿】应显时刻戳（进程级对齐时钟，Unix 纪
    // 元 μs）= 帧就绪时刻 + 延迟预算（如 120ms——预算在打戳端统一打进
    // pts，用户定案"editor 端捕获时就把 120ms 加上"）：帧出生即定显示
    // 时刻，网络波动只影响到达早晚、不改显示节拍（早到等 / 晚到顺延）。
    // display 按 "now ≥ pts" 到点回放，排程独立于播放头（暂停继续流动）。
    // 打戳落点按链路段分工：editor 捕获类源在捕获端打（捕获+预算）；
    // display 组播收流在解码出画点打（解码+预算，组播同流同达 → 多端
    // 同步）；editor 预览（Hub 模式）用捕获戳 + 60ms 链路补偿。
    std::int64_t              pts_us = 0;
};

using LiveFramePtr = std::shared_ptr<const LiveFrame>;

} // namespace ms
