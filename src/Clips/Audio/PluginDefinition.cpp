// ============================================================================
//  PluginDefinition.cpp —— 音频片段插件实现（type = "Audio"）
// ============================================================================

#include "PluginDefinition.hpp"

#include "AudioClipModel.hpp"

Plugin::Plugin() = default; // 无参：QPluginLoader 实例化（对齐节点构造签名）

QString Plugin::clipType() const { return CLIP_TYPE; }

AbstractClipModel* Plugin::createModel(qint64 startUs) {
    return new AudioClipModel(startUs);
}
