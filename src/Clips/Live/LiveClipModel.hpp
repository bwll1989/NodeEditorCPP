// ============================================================================
//  LiveClipModel —— 实时源媒体片段模型（src/Clips/Live/）
//
//  【2026-11-15 结构对齐（用户定案「片段实现与节点同构」）】自
//  LiveClipPlugin.hpp 单头拆出——文件结构对齐节点插件基准。
//
//  类型专属模型：青色块（与视频/图片/文本色区分）。本模型不持业务镜像
//  字段：Live 业务属性（NDI 源名/延迟）存于工作副本的 Live 素材
// （Asset.live_*，随场景 pb 分发），实况域唯一写者是实况区（直写素材），
//  模型只镜像 asset_id（与 TextClipModel 同款"无文件素材"模式）。
// ============================================================================

#pragma once

#include "SceneClipModel.hpp"

class LiveClipModel : public SceneClipModel {
    Q_OBJECT
public:
    LiveClipModel(qint64 startUs, QObject* parent = nullptr)
        : SceneClipModel(startUs, "Live", {}, parent) {
        RESIZEABLE  = true;
        EMBEDWIDGET = false;
        SHOWBORDER  = false;
        setColor(QColor(0x3C, 0xC4, 0xD9)); // 青（实时源类型色）
    }
};
