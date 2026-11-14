// ============================================================================
//  AudioClipModel —— 音频媒体片段模型（src/Clips/Audio/）
//
//  【2026-11-15 结构对齐（用户定案「片段实现与节点同构」）】自
//  AudioClipPlugin.hpp 单头拆出——文件结构对齐节点插件基准。
//
//  类型专属模型：紫色块。Player 仅消费音频的时间信息（§4.4，不出声），
//  画布无几何呈现，片段只承载时间区间（入出点后续扩展）。
// ============================================================================

#pragma once

#include "SceneClipModel.hpp"

class AudioClipModel : public SceneClipModel {
    Q_OBJECT
public:
    AudioClipModel(qint64 startUs, QObject* parent = nullptr)
        : SceneClipModel(startUs, "Audio", {}, parent) {
        RESIZEABLE  = true;
        EMBEDWIDGET = false;
        SHOWBORDER  = true;
        setColor(QColor(0x9B, 0x6F, 0xD1)); // 紫（音频类型色）
    }
};
