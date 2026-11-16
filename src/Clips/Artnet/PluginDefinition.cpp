// ============================================================================
//  PluginDefinition.cpp —— Artnet 灯光片段插件实现（type = "Artnet"）
// ============================================================================

#include "PluginDefinition.hpp"

#include "ArtnetClipModel.hpp"

#include "ArtnetClipSection.hpp"        // 同目录：DMX 配置区（UI 扩展点）
#include "model/ClipSectionContext.hpp" // ms::ClipSectionContext（MediaCore）

Plugin::Plugin() = default; // 无参：QPluginLoader 实例化（对齐节点构造签名）

QString Plugin::clipType() const { return CLIP_TYPE; }

AbstractClipModel* Plugin::createModel(qint64 startUs) {
    return new ArtnetClipModel(startUs);
}

QWidget* Plugin::createPropertySection(void* ctx, QWidget* parent) {
    auto* c = static_cast<ms::ClipSectionContext*>(ctx);
    if (!c || !c->clip) return nullptr;
    return new ms::ArtnetClipSection(*c, parent);
}
