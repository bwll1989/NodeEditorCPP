// ============================================================================
//  PluginDefinition.cpp —— 音频片段插件实现（type = "Audio"）
// ============================================================================

#include "PluginDefinition.hpp"

#include "AudioClipModel.hpp"

Plugin::Plugin(QObject* parent) : QObject(parent) {}

QString Plugin::clipType() const { return "Audio"; }

AbstractClipModel* Plugin::createModel(qint64 startUs) {
    return new AudioClipModel(startUs);
}
