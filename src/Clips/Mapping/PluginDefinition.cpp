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

QWidget* Plugin::createPropertySection(ms::ClipSectionContext* ctx,
                                       QWidget* parent) {
    if (!ctx || !ctx->clip) return nullptr; // ctx 已类型化（P3-B），直用成员
    return new ms::MappingClipSection(*ctx, parent);
}
