// ============================================================================
//  PluginDefinition —— 3D 模型片段插件定义（src/Clips/Mesh/，阶段 3A）
//
//  结构对齐片段插件基准（详见 Video/PluginDefinition.hpp 头注释）：实现
//  BaseClass 的 ClipPlugInterface（clipType/createModel/createPropertySection），
//  Q_PLUGIN_METADATA(IID ClipPlugInterface_iid) 供 QPluginLoader 识别——
//  编译进独立 mesh.plugin（bin/plugins/TimeLine/，磁盘驱动：删 .plugin =
//  类型不存在，零内置兜底）。
//
//  媒体绑定语义：hasMediaBinding() = true（默认）——属性面板页 1 媒体区
//  显示，可在面板导入/换绑 .glb 等模型素材。无专属编辑区（默认 nullptr）
//  ——银幕侧投影参数（placement/绑定）在屏幕属性对话框「3D mapping」页
//  编辑（ScenePropertyDialog 链），片段面板不重复。
// ============================================================================

#pragma once

#include <QObject>

#include "AbstractClipInterface.h"

// ---- 插件元数据宏区（布局对齐节点 PLUGIN_* 宏区；详见 Video 同区注释）----
// CLIP_TYPE = 类型串协议键：工厂表键/场景存储/typeForKind 映射键，与目录
// 名一致。CLIP_DESCRIBE = 人类可读描述。
#define CLIP_TYPE "Mesh"
#define CLIP_DESCRIBE "3D 模型片段（投影表面素材载体）"

class Plugin : public QObject, public ClipPlugInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ClipPlugInterface_iid)
    Q_INTERFACES(ClipPlugInterface)
public:
    Plugin();

    /// 类型串（返回 CLIP_TYPE；Add Track 菜单/场景存储/工厂表键）
    QString clipType() const override;

    /// 片段模型工厂（parent 由适配器随后 setParent）
    AbstractClipModel* createModel(qint64 startUs) override;

    // ---- 类型显示元数据（P2-B 下放 2026-11-17）----
    /// 页 3 页签名（原宿主 kindPageName 表下放）
    QString clipDisplayName() const override { return QStringLiteral("模型"); }

    // ---- 动画页暴露面（P4-A schema 化 2026-11-17）----
    /// 模型片段无画布内容呈现（画布隐形，同 Control/Artnet/Mapping 口径
    /// ——模型经银幕 3D 视口呈现）——动画页整页不加（transform 语义不在
    /// 片段上，placement 在银幕侧）。
    ClipTweenExposure tweenExposure() const override { return {false, {}}; }
};
