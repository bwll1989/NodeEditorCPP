// ============================================================================
//  TextMediaSource —— 文本媒体源（IMediaSource 实现；Phase 4 收官）
//
//  定位（§4.7 扩展点："新增媒体类型 = 1 个 IMediaSource 实现 + 1 行注册"）：
//    * 文本帧在 prepare() 一次渲染为 RGBA 常驻缓冲（TextRasterizer 两端
//      同源光栅化），produce(media_us) 任意时刻恒命中（与图片单帧媒体
//      同语义——【首次 produce(t>0) 直达】窗口必须通过，单图 seek 陷阱
//      的文本侧对应物：文本无时间轴，根本不走 seek 分支）；
//    * 帧尺寸 = TextRasterizer 排版自然尺寸；asset.width/height > 0
//      （编辑器测量回填）时与光栅化结果一致性由"同一 TextRasterizer
//      实现"保证——两端逐位一致（两端同源渲染铁律）；
//    * prepare 无文件 IO（文本随场景 pb 分发，AssetDeps 已排除文本），
//      assets_root 为空时照常工作（与视频/图片源的根目录依赖解耦）。
//
//  线程契约：与 FfmpegMediaSource 一致——prepare/produce 在解码驱动
//  线程（MediaScheduler::ensureSource/produce），frameView 在渲染线程。
//  文本帧缓冲发布后不变（单帧媒体），frameView 的 keep 留空：实例由
//  调用侧 shared_ptr 保活到 draw 结束，缓冲无悬垂窗口（无"解码线程
//  发布新帧覆盖旧缓冲"的并发源——文本帧静态）。
// ============================================================================

#pragma once

#include <cstdint>
#include <vector>

#include "engine/MediaFactory.hpp"

namespace ms {

class TextMediaSource : public IMediaSource {
public:
    AssetKind kind() const noexcept override { return AssetKind::Text; }

    /// 渲染文本帧（TextRasterizer 两端同源；无可用字体 → 失败可重试——
    /// 调用方 MediaScheduler 的 5s 限频重试语义与视频源一致）
    bool prepare(const Asset& asset) override;

    /// 任意时刻恒有帧（文本静态；media_us 仅占位）
    bool produce(std::int64_t media_us) override;

    /// 帧视图（CpuRgba 域；keep 留空——帧缓冲为实例常驻成员，源实例
    /// 经调度器 shared_ptr 保活，静态帧无覆盖风险）
    bool frameView(FrameView& out) const override;

    void release() override {
        rgba_.clear();
        width_ = height_ = 0;
    }

    // ---- 诊断 ----
    int width() const noexcept { return width_; }
    int height() const noexcept { return height_; }

private:
    std::vector<std::uint8_t> rgba_; ///< 常驻 RGBA 帧（prepare 期渲染）
    int width_  = 0;                 ///< 帧宽（px）
    int height_ = 0;                 ///< 帧高（px）
};

} // namespace ms
