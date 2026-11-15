// ============================================================================
//  PluginDefinition.cpp —— 图片片段插件实现（type = "Image"）
// ============================================================================

#include "PluginDefinition.hpp"

#include "ImageClipModel.hpp"

Plugin::Plugin() = default; // 无参：QPluginLoader 实例化（对齐节点构造签名）

QString Plugin::clipType() const { return CLIP_TYPE; }

AbstractClipModel* Plugin::createModel(qint64 startUs) {
    return new ImageClipModel(startUs);
}
