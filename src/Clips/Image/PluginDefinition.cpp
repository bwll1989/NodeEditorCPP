// ============================================================================
//  PluginDefinition.cpp —— 图片片段插件实现（type = "Image"）
// ============================================================================

#include "PluginDefinition.hpp"

#include "ImageClipModel.hpp"

Plugin::Plugin(QObject* parent) : QObject(parent) {}

QString Plugin::clipType() const { return "Image"; }

AbstractClipModel* Plugin::createModel(qint64 startUs) {
    return new ImageClipModel(startUs);
}
