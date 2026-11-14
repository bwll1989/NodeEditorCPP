// ============================================================================
//  PluginDefinition.cpp —— 实况源片段插件实现（type = "Live"）
// ============================================================================

#include "PluginDefinition.hpp"

#include "LiveClipModel.hpp"

#include "LiveClipSection.hpp"          // 同目录：实况源专属编辑区
#include "model/ClipSectionContext.hpp" // ms::ClipSectionContext（MediaCore）

Plugin::Plugin(QObject* parent) : QObject(parent) {}

QString Plugin::clipType() const { return "Live"; }

AbstractClipModel* Plugin::createModel(qint64 startUs) {
    return new LiveClipModel(startUs);
}

QWidget* Plugin::createPropertySection(void* ctx, QWidget* parent) {
    auto* c = static_cast<ms::ClipSectionContext*>(ctx);
    if (!c || !c->clip || !c->scene) return nullptr;
    const ms::Asset* a = c->scene->findAsset(c->clip->asset_id);
    if (!a || a->kind != ms::AssetKind::Live) return nullptr; // 未绑定实况素材
    return new ms::LiveClipSection(*c, parent);
}
