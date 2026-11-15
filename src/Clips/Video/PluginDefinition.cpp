// ============================================================================
//  PluginDefinition.cpp —— 视频片段插件实现（type = "Video"）
// ============================================================================

#include "PluginDefinition.hpp"

#include "VideoClipModel.hpp"

Plugin::Plugin() = default; // 无参：QPluginLoader 实例化（对齐节点构造签名）

QString Plugin::clipType() const { return CLIP_TYPE; }

AbstractClipModel* Plugin::createModel(qint64 startUs) {
    return new VideoClipModel(startUs); // parent 由适配器随后 setParent
}
