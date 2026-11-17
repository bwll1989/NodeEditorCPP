// ============================================================================
//  PluginDefinition —— Mapping 映射片段插件定义（src/Clips/Mapping/）
//
//  【2026-11-15 片段移植】自节点体系 src/Nodes/TimeLineNode/mappingclipmodel
// （NodeClipDelegateModel 帧域 + currentData 拉取驱动 + 控件内 QJSEngine）
//  移植为磁盘插件（SceneClipModel μs 域 + onPlayheadAdvanced 推送驱动 +
//  模型自带 QJSEngine）。文件组织对齐 Control 插件基准
// （PluginDefinition.hpp/.cpp + 类型模型独立头；类名统一 Plugin）。业务
//  接口 = BaseClass 的 ClipPlugInterface（clipType/createModel/
//  createPropertySection），Q_PLUGIN_METADATA 供 QPluginLoader 识别——
//  编译进独立 mapping.plugin（磁盘驱动）。
//
//  语义（与节点体系同源）：片段携带一张 OSC 消息表（地址 + 类型 + 表达式
//  原文），播放头在片段区间内的每一帧对表达式求值（$percent = 区间内归一
//  化进度、$frame = 区间内相对帧号；Math 全局可用）并逐条重发——驱动外部
//  状态（映射 = "把时间轴进度映射到任意 OSC 地址"）。配置存
//  ms::Clip.clip_data（JSON {"messages":[...]}，OSCMessageListWidget
//  save/load 兼容——save 把求值结果覆写为表达式原文，持久化即表达式文本，
//  求值只在发送时发生）。无素材绑定（asset_id 恒空）、无画布呈现（与
//  Control 同口径）。结构详见 ../Video/PluginDefinition.hpp。
// ============================================================================

#pragma once

#include <QObject>
#include <QWidget>

#include "AbstractClipInterface.h"

// ---- 插件元数据宏区（布局对齐节点 PLUGIN_*；详见 Video/PluginDefinition.hpp）----
#define CLIP_TYPE "Mapping"
#define CLIP_DESCRIBE "映射片段（OSC 消息表逐帧求值重发）"

class Plugin : public QObject, public ClipPlugInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ClipPlugInterface_iid)
    Q_INTERFACES(ClipPlugInterface)
public:
    Plugin();

    /// 类型串（返回 CLIP_TYPE；Add Item 菜单/场景存储/工厂表键）
    QString clipType() const override;

    /// 片段模型工厂（parent 由适配器随后 setParent）
    AbstractClipModel* createModel(qint64 startUs) override;

    /// 片段属性面板专属编辑区工厂：Mapping 无素材（asset_id 恒空），
    /// 只校验片段上下文存在即建区（与 Control 同口径）。
    QWidget* createPropertySection(ms::ClipSectionContext* ctx,
                                   QWidget* parent) override;

    // ---- 类型显示元数据（P2-B 下放 2026-11-17）----
    /// 页 3 页签名（原宿主 kindPageName 表漏本类型落兜底「属性」——下放纠偏）
    QString clipDisplayName() const override { return QStringLiteral("映射"); }
    /// 无媒体绑定语义：消息表在专属页编辑（媒体区隐藏 + 绑定行说明文案）
    bool hasMediaBinding() const override { return false; }
    QString bindingHint() const override {
        return QStringLiteral("（映射片段无需媒体文件）");
    }
};
