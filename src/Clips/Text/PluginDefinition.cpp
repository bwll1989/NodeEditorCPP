// ============================================================================
//  PluginDefinition.cpp —— 文本片段插件实现（type = "Text"）
// ============================================================================

#include "PluginDefinition.hpp"

#include "TextClipModel.hpp"

#include "TextClipSection.hpp"          // 同目录：文本专属编辑区（UI 扩展点）
#include "model/ClipSectionContext.hpp" // ms::ClipSectionContext（MediaCore）

Plugin::Plugin(QObject* parent) : QObject(parent) {}

QString Plugin::clipType() const { return "Text"; }

AbstractClipModel* Plugin::createModel(qint64 startUs) {
    return new TextClipModel(startUs);
}

QWidget* Plugin::createPropertySection(void* ctx, QWidget* parent) {
    auto* c = static_cast<ms::ClipSectionContext*>(ctx);
    if (!c || !c->clip || !c->scene) return nullptr;
    const ms::Asset* a = c->scene->findAsset(c->clip->asset_id);
    if (!a || a->kind != ms::AssetKind::Text) return nullptr; // 未绑定文本素材
    return new ms::TextClipSection(*c, parent);
}
