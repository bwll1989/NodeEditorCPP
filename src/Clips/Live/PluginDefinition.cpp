// ============================================================================
//  PluginDefinition.cpp —— 实况源片段插件实现（type = "Live"）
// ============================================================================

#include "PluginDefinition.hpp"

#include "LiveClipModel.hpp"

#include "LiveClipSection.hpp"          // 同目录：实况源专属编辑区
#include "model/ClipSectionContext.hpp" // ms::ClipSectionContext（MediaCore）

Plugin::Plugin() = default; // 无参：QPluginLoader 实例化（对齐节点构造签名）

QString Plugin::clipType() const { return CLIP_TYPE; }

AbstractClipModel* Plugin::createModel(qint64 startUs) {
    return new LiveClipModel(startUs);
}

QWidget* Plugin::createPropertySection(ms::ClipSectionContext* ctx,
                                       QWidget* parent) {
    if (!ctx || !ctx->clip || !ctx->scene) return nullptr; // ctx 已类型化（P3-B）
    const ms::Asset* a = ctx->scene->findAsset(ctx->clip->asset_id);
    if (!a || a->kind != ms::AssetKind::Live) return nullptr; // 未绑定实况素材
    return new ms::LiveClipSection(*ctx, parent);
}
