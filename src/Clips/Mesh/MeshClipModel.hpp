// ============================================================================
//  MeshClipModel —— 3D 模型片段模型（src/Clips/Mesh/，阶段 3A 2026-11-18）
//
//  类型专属模型（默认色 + 类型标志，继承 BaseClass.dll 的 SceneClipModel）：
//    * 青绿块（对齐视口素模线框色系 0x3A8A7C/0x50E0C0）、可拖拽移动/缩放；
//    * RESIZEABLE=true（模型无固有时长——素材 duration_us 恒 0，拖入保持
//      默认长度，用户按演出编排拖右缘改）；EMBEDWIDGET=false（属性走双击
//      对话框）；SHOWBORDER=false。
//
//  语义定位：模型素材（.obj/.glb/.gltf/.fbx/.dae/.ply/.stl）的时间轴载体
//  ——拖入即导入 Asset(kind=Mesh) + 建片段 + asset_id 绑定（与 Video/Image
//  同链）。消费端 = 银幕投影表面（Screen.model_asset_id + placement，
//  屏幕属性「3D mapping」页绑定；3B display 投影渲染管线消费）。片段自身
//  画布隐形（trackInvisibleOnCanvas 同 Audio/Control/Artnet/Mapping 口径
//  ——模型经银幕 3D 视口呈现，不作为画布内容块出现）。
// ============================================================================

#pragma once

#include "SceneClipModel.hpp"

class MeshClipModel : public SceneClipModel {
    Q_OBJECT
public:
    MeshClipModel(qint64 startUs, QObject* parent = nullptr)
        : SceneClipModel(startUs, "Mesh", {}, parent) {
        RESIZEABLE  = true;   // 模型无固有时长——按演出编排拖右缘改
        EMBEDWIDGET = false;  // 不内嵌控件编辑器（属性走双击对话框）
        SHOWBORDER  = false;
        setColor(QColor(0x2E, 0xB8, 0x9A)); // 青绿（3D 模型类型色）
    }
};
