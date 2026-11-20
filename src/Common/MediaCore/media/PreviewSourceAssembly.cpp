// ============================================================================
//  PreviewSourceAssembly 实现 —— 见头文件注释（归口收编 2026-11-20）
// ============================================================================

#include "PreviewSourceAssembly.hpp"

#include "media/FfmpegMediaSource.hpp"
#include "media/LiveMediaSource.hpp"
#include "media/TextMediaSource.hpp"

namespace ms {

void registerPreviewMediaSources(MediaFactory& factory,
                                 const std::string& assets_root,
                                 const HwAccelConfig& hw) {
    // ---- Video/Image（FFmpeg 解码；空根也注册——语义见头文件）-------------
    // creator 值拷贝根目录 + 硬解配置：生命周期与工厂一致，避免悬垂。
    {
        const std::string root = assets_root;
        factory.registerCreator(
            AssetKind::Video, [root, hw](const Asset&) {
                return std::make_unique<FfmpegMediaSource>(root,
                                                           AssetKind::Video,
                                                           hw);
            });
        factory.registerCreator(
            AssetKind::Image, [root](const Asset&) {
                return std::make_unique<FfmpegMediaSource>(root,
                                                           AssetKind::Image);
            });
    }
    // ---- Text（无条件：文本随场景 pb 分发，无文件、不依赖 assets_root）---
    // 与 TextMediaSource 在 display 端 RenderThread 的注册同源。
    factory.registerCreator(AssetKind::Text, [](const Asset&) {
        return std::make_unique<TextMediaSource>();
    });
    // ---- Live（无条件：NDI 实时源与 Text 同款"无文件素材"）----------------
    // 与 display 端 RenderThread 注册同源——两端同源渲染铁律。
    factory.registerCreator(AssetKind::Live, [](const Asset&) {
        return std::make_unique<LiveMediaSource>();
    });
}

} // namespace ms
