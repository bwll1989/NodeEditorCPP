// ============================================================================
//  TextClipModel —— 字幕/文本媒体片段模型（src/Clips/Text/）
//
//  【2026-11-15 结构对齐（用户定案「片段实现与节点同构」）】自
//  TextClipPlugin.hpp 单头拆出——文件结构对齐节点插件基准。
//
//  类型专属模型：黄色块。本模型不持业务镜像字段：文本内容/字体/字号/颜色
//  等业务属性存于工作副本的 Text 素材（Asset.text_*，随场景 pb 分发），
//  文本域唯一写者是文本区（直写素材），模型只镜像 asset_id（画布渲染/
//  依赖联动）——此前骨架期的镜像字段已删除（双源易漂移）。
// ============================================================================

#pragma once

#include "SceneClipModel.hpp"

class TextClipModel : public SceneClipModel {
    Q_OBJECT
public:
    TextClipModel(qint64 startUs, QObject* parent = nullptr)
        : SceneClipModel(startUs, "Text", {}, parent) {
        RESIZEABLE  = true;
        EMBEDWIDGET = false;
        SHOWBORDER  = false;
        setColor(QColor(0xD9, 0xC2, 0x3C)); // 黄（文本类型色）
    }
};
