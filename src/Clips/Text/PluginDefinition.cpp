// ============================================================================
//  PluginDefinition.cpp —— 文本片段插件实现（type = "Text"）
// ============================================================================

#include "PluginDefinition.hpp"

#include "TextClipModel.hpp"

#include "TextClipSection.hpp"          // 同目录：文本专属编辑区（UI 扩展点）
#include "model/ClipSectionContext.hpp" // ms::ClipSectionContext（MediaCore）

Plugin::Plugin() = default; // 无参：QPluginLoader 实例化（对齐节点构造签名）

QString Plugin::clipType() const { return CLIP_TYPE; }

AbstractClipModel* Plugin::createModel(qint64 startUs) {
    return new TextClipModel(startUs);
}

QWidget* Plugin::createPropertySection(ms::ClipSectionContext* ctx,
                                       QWidget* parent) {
    if (!ctx || !ctx->clip || !ctx->scene) return nullptr; // ctx 已类型化（P3-B）
    const ms::Asset* a = ctx->scene->findAsset(ctx->clip->asset_id);
    if (!a || a->kind != ms::AssetKind::Text) return nullptr; // 未绑定文本素材
    return new ms::TextClipSection(*ctx, parent);
}
