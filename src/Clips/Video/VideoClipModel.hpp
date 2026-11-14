// ============================================================================
//  VideoClipModel —— 视频媒体片段模型（src/Clips/Video/）
//
//  【2026-11-15 结构对齐（用户定案「片段实现与节点同构」）】自
//  VideoClipPlugin.hpp 单头拆出——文件结构对齐节点插件基准
//  （PluginDefinition.hpp/.cpp + 类型模型独立头 + 独立 CMakeLists + Doc.md）。
//
//  类型专属模型（默认色 + 类型标志，继承 BaseClass.dll 的 SceneClipModel）：
//    * 橙色块（与编辑器既有配色一致）、可拖拽移动/缩放；
//    * RESIZEABLE=true（时间轴上可拖右缘改时长）、EMBEDWIDGET=false
//     （不内嵌控件编辑器，属性走双击对话框）、SHOWBORDER=false。
// ============================================================================

#pragma once

#include "SceneClipModel.hpp"

class VideoClipModel : public SceneClipModel {
    Q_OBJECT
public:
    VideoClipModel(qint64 startUs, QObject* parent = nullptr)
        : SceneClipModel(startUs, "Video", {}, parent) {
        RESIZEABLE  = true;   // 时间轴上可拖右缘改时长
        EMBEDWIDGET = false;  // 不内嵌控件编辑器（属性走双击对话框）
        SHOWBORDER  = false;
        setColor(QColor(0xE6, 0x8A, 0x33)); // 橙（视频类型色）
    }
};
