// ============================================================================
//  PluginDefinition.cpp —— 视频片段插件实现（type = "Video"）
// ============================================================================

#include "PluginDefinition.hpp"

#include "VideoClipModel.hpp"

Plugin::Plugin(QObject* parent) : QObject(parent) {}

QString Plugin::clipType() const { return "Video"; }

AbstractClipModel* Plugin::createModel(qint64 startUs) {
    return new VideoClipModel(startUs); // parent 由适配器随后 setParent
}
