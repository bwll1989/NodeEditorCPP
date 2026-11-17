// ============================================================================
//  PluginDefinition.cpp —— Control 指令片段插件实现（type = "Control"）
// ============================================================================

#include "PluginDefinition.hpp"

#include "ControlClipModel.hpp"

#include "ControlClipSection.hpp"       // 同目录：指令编辑区（UI 扩展点）
#include "model/ClipSectionContext.hpp" // ms::ClipSectionContext（MediaCore）

Plugin::Plugin() = default; // 无参：QPluginLoader 实例化（对齐节点构造签名）

QString Plugin::clipType() const { return CLIP_TYPE; }

AbstractClipModel* Plugin::createModel(qint64 startUs) {
    return new ControlClipModel(startUs);
}

QWidget* Plugin::createPropertySection(ms::ClipSectionContext* ctx,
                                       QWidget* parent) {
    if (!ctx || !ctx->clip) return nullptr; // ctx 已类型化（P3-B），直用成员
    return new ms::ControlClipSection(*ctx, parent);
}
