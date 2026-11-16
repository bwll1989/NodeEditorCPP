// ============================================================================
//  PluginDefinition.cpp —— Mapping 映射片段插件实现（type = "Mapping"）
// ============================================================================

#include "PluginDefinition.hpp"

#include "MappingClipModel.hpp"

#include "MappingClipSection.hpp"       // 同目录：消息表编辑区（UI 扩展点）
#include "model/ClipSectionContext.hpp" // ms::ClipSectionContext（MediaCore）

Plugin::Plugin() = default; // 无参：QPluginLoader 实例化（对齐节点构造签名）

QString Plugin::clipType() const { return CLIP_TYPE; }

AbstractClipModel* Plugin::createModel(qint64 startUs) {
    return new MappingClipModel(startUs);
}

QWidget* Plugin::createPropertySection(void* ctx, QWidget* parent) {
    auto* c = static_cast<ms::ClipSectionContext*>(ctx);
    if (!c || !c->clip) return nullptr;
    return new ms::MappingClipSection(*c, parent);
}
