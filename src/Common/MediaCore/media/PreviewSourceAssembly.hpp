// ============================================================================
//  PreviewSourceAssembly —— 编辑器/预览端媒体源装配出口（归口收编 2026-11-20）
//
//  背景：PreviewRenderer（Flow 主程序）此前直接 include FfmpegMediaSource.
//  hpp 等三个具体源头来注册 MediaFactory creator——违反归口铁律（主程序
//  严禁 include FfmpegMediaSource.hpp：跨 DLL DATA 导出无自动 thunk 的
//  跨界构造陷阱；且具体源头的构造知识不该外泄到视图层）。
//
//  收编后：四类源的装配知识（构造参数/硬解配置/空根语义/注册条件）归
//  MediaCore 单点维护，视图层只调本函数。display 端 RenderThread 的
//  同源注册（两端同源渲染铁律）后续可复用同一语义口径。
// ============================================================================

#pragma once

#include <string>

#include "engine/MediaFactory.hpp"
#include "media/HwAccel.hpp"

namespace ms {

/// 注册预览/编辑器端全部媒体源 creator（Video/Image/Text/Live）。
/// @param assets_root 素材根目录，可为空串——FfmpegMediaSource 对空根有
///        明确语义（full = asset.path 直通，绝对路径寻址，见其 openInput
///        空根分支；2026-11-12 实证：空根挡注册 = 视频/图片全部占位降级）。
/// @param hw 硬解配置（cuda 0 拷贝意图；gpu_delivery_ok 协商旗标由调用方
///        预置，解码端失败自愈回退见 PreviewRenderer 侧注释）。
void registerPreviewMediaSources(MediaFactory& factory,
                                 const std::string& assets_root,
                                 const HwAccelConfig& hw);

} // namespace ms
