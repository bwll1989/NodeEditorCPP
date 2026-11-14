// ============================================================================
//  ImageClipModel —— 静态图片媒体片段模型（src/Clips/Image/）
//
//  【2026-11-15 结构对齐（用户定案「片段实现与节点同构」）】自
//  ImageClipPlugin.hpp 单头拆出——文件结构对齐节点插件基准。
//
//  类型专属模型：青色块；图片无固有播放时长，时间轴上默认可自由拉伸。
// ============================================================================

#pragma once

#include "SceneClipModel.hpp"

class ImageClipModel : public SceneClipModel {
    Q_OBJECT
public:
    ImageClipModel(qint64 startUs, QObject* parent = nullptr)
        : SceneClipModel(startUs, "Image", {}, parent) {
        RESIZEABLE  = true;
        EMBEDWIDGET = false;
        SHOWBORDER  = false;
        setColor(QColor(0x33, 0xB2, 0xA6)); // 青（图片类型色）
    }
};
