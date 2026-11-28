// ============================================================================
//  PluginDefinition —— Control 指令片段插件定义（src/Clips/Control/）
//
//  【2026-11-15 结构对齐（用户定案「片段实现与节点同构」）】文件命名/组织
//  对齐节点插件基准（PluginDefinition.hpp/.cpp + 类型模型独立头）；类名
//  统一 Plugin。业务接口 = BaseClass 的 ClipPlugInterface
// （clipType/createModel/createPropertySection），Q_PLUGIN_METADATA 供
//  QPluginLoader 识别——编译进独立 control.plugin（磁盘驱动）。
//  与旧 TriggerClipModel（src/Nodes/TimeLineNode/，节点体系内嵌静态注册）
//  的差异：本插件走磁盘插件体系（bin/plugins/TimeLine/control.plugin），
//  类型表纯磁盘驱动。
//  专属编辑区 = 同目录 ControlClipSection（OSCMessageListWidget + 测试
//  发送按钮，include 只进 cpp）。Control 不绑定素材（asset_id 恒空——
//  工厂不查 Asset，与 Text/Live 的素材校验不同）。结构详见
//  Video/PluginDefinition.hpp。
// ============================================================================

#pragma once

#include <QObject>
#include <QWidget>

#include "AbstractClipInterface.h"

// ---- 插件元数据宏区（布局对齐节点 PLUGIN_*；详见 Video/PluginDefinition.hpp）----
#define CLIP_TYPE "Control"
#define CLIP_DESCRIBE "指令片段（触发语义）"

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

    /// 片段属性面板专属编辑区工厂：Control 无素材（asset_id 恒空），
    /// 只校验片段上下文存在即建区（与 Text/Live 的素材校验不同）。
    /// 【P3-B 协议类型化 2026-11-17】ctx = ms::ClipSectionContext*（原
    /// void* + static_cast 还原退役）。
    QWidget* createPropertySection(ms::ClipSectionContext* ctx,
                                   QWidget* parent) override;

    // ---- 类型显示元数据（P2-B 下放 2026-11-17）----
    /// 页 3 页签名（原宿主 kindPageName 表下放）
    QString clipDisplayName() const override { return tr("指令"); }
    /// 无媒体绑定语义：指令表在「指令」专属页编辑（媒体区隐藏 +
    /// 绑定行说明文案，原宿主硬编码文案下放）
    bool hasMediaBinding() const override { return false; }
    QString bindingHint() const override {
        return tr("（指令片段无需媒体文件）");
    }

    // ---- 动画页暴露面（P4-A schema 化 2026-11-17）----
    /// 指令片段无画布呈现（无 transform 语义）——动画页整页不加（Doc.md
    /// 「实现期二选一，默认隐藏」定案；占位版待 P4 后续切片）
    ClipTweenExposure tweenExposure() const override { return {false, {}}; }
};
