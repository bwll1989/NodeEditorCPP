// ============================================================================
//  PluginDefinition —— 音频媒体片段插件定义（src/Clips/Audio/）
//
//  【2026-11-15 结构对齐（用户定案「片段实现与节点同构」）】文件命名/组织
//  对齐节点插件基准（PluginDefinition.hpp/.cpp + 类型模型独立头）；类名
//  统一 Plugin（每插件一份，各自 DLL 内不冲突）。业务接口 = BaseClass 的
//  ClipPlugInterface（clipType/createModel），Q_PLUGIN_METADATA 供
//  QPluginLoader 识别——编译进独立 audio.plugin（bin/plugins/TimeLine/，
//  磁盘驱动：删 .plugin = 类型不存在）。结构详见 Video/PluginDefinition.hpp。
// ============================================================================

#pragma once

#include <QObject>

#include "AbstractClipInterface.h"

// ---- 插件元数据宏区（布局对齐节点 PLUGIN_*；详见 Video/PluginDefinition.hpp）----
#define CLIP_TYPE "Audio"
#define CLIP_DESCRIBE "音频媒体片段"

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
    /// 页 3 页签名（原宿主 kindPageName 表下放；Audio 无专属区，本名
    /// 暂不显示但声明齐备——插件声明面向未来页面）
    QString clipDisplayName() const override { return QStringLiteral("音频"); }

    // ---- 动画页暴露面（P4-A schema 化 2026-11-17）----
    /// 音频片段动画页只暴露音频增益（用户定案：画布变换/合成域属性对纯
    /// 音频无意义，不生成）；key 词汇对齐 P4 终态 FieldDef.key
    ClipTweenExposure tweenExposure() const override {
        return {true, {QStringLiteral("audio_gain")}};
    }
};
