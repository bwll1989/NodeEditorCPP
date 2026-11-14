// ============================================================================
//  PluginDefinition.cpp —— Control 指令片段插件实现（type = "Control"）
// ============================================================================

#include "PluginDefinition.hpp"

#include "ControlClipModel.hpp"

#include "ControlClipSection.hpp"       // 同目录：指令编辑区（UI 扩展点）
#include "model/ClipSectionContext.hpp" // ms::ClipSectionContext（MediaCore）

Plugin::Plugin(QObject* parent) : QObject(parent) {}

QString Plugin::clipType() const { return "Control"; }

AbstractClipModel* Plugin::createModel(qint64 startUs) {
    return new ControlClipModel(startUs);
}

QWidget* Plugin::createPropertySection(void* ctx, QWidget* parent) {
    auto* c = static_cast<ms::ClipSectionContext*>(ctx);
    if (!c || !c->clip) return nullptr;
    return new ms::ControlClipSection(*c, parent);
}
