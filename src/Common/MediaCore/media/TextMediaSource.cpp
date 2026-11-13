// ============================================================================
//  TextMediaSource 实现 —— 文本帧渲染（prepare 一次光栅化，全程复用）
// ============================================================================

#include "media/TextMediaSource.hpp"

#include "media/TextRasterizer.hpp"

namespace ms {

// ---------------------------------------------------------------------------
// prepare：TextPayload → 光栅化帧（自然尺寸；asset.width/height 仅诊断，
// 帧尺寸以排版结果为准——与编辑器测量回填同源同规则）
// ---------------------------------------------------------------------------
bool TextMediaSource::prepare(const Asset& asset) {
    // 非文本素材（装配错位防御）：立即失败（调度器 5s 限频警告）
    if (asset.kind != AssetKind::Text) return false;

    const TextStyle st = TextRasterizer::styleFromAsset(asset);
    // 内容为空：仍渲染（空帧 = 占位白板）而非失败——空文本片段合法
    // （用户可能先建片段后输入），保持"片段在即渲染"语义
    // 光栅化失败（系统缺字体）不在此处打印：调度器每帧重试会放大成
    // 每帧一条的刷屏，失败告警统一走调度器的 5s 限频通道
    const bool ok = TextRasterizer::render(st, rgba_, width_, height_);
    if (!ok) {
        width_ = height_ = 0;
        return false;
    }
    return true;
}

bool TextMediaSource::produce(std::int64_t media_us) {
    (void)media_us; // 文本无时间轴：任意时刻恒命中（与图片单帧快速路径
                    // 同语义，但无 seek 分支——文本帧 prepare 期已就绪）
    return width_ > 0 && height_ > 0 && !rgba_.empty();
}

bool TextMediaSource::frameView(FrameView& out) const {
    if (width_ <= 0 || height_ <= 0 || rgba_.empty()) return false;
    out.width  = width_;
    out.height = height_;
    out.stride = static_cast<std::uint32_t>(width_) * 4;
    out.rgba   = rgba_.data();
    out.keep   = nullptr; // 帧缓冲为实例成员：源实例由调度器 shared_ptr
                          // 保活（find 返回值存活域覆盖 draw），静态帧
                          // 无解码线程覆盖风险
    out.domain = FrameDomain::CpuRgba;
    out.native = nullptr;
    return true;
}

} // namespace ms
