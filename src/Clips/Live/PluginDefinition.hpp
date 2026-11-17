// ============================================================================
//  PluginDefinition —— 实时源媒体片段插件定义（src/Clips/Live/）
//
//  【2026-11-15 结构对齐（用户定案「片段实现与节点同构」）】文件命名/组织
//  对齐节点插件基准（PluginDefinition.hpp/.cpp + 类型模型独立头）；类名
//  统一 Plugin。业务接口 = BaseClass 的 ClipPlugInterface
// （clipType/createModel/createPropertySection），Q_PLUGIN_METADATA 供
//  QPluginLoader 识别——编译进独立 live.plugin（磁盘驱动）。
//  【2026-11-13 磁盘化（用户定案）】原"仅内置注册"撤销——Live 与其余五类
//  统一为磁盘 .plugin（改完重编→替换即可）；内置兜底注册同步删除。
//  专属编辑区 = 同目录 LiveClipSection（include 只进 cpp）。结构详见
//  Video/PluginDefinition.hpp。
// ============================================================================

#pragma once

#include <QObject>
#include <QWidget>

#include "AbstractClipInterface.h"

// ---- 插件元数据宏区（布局对齐节点 PLUGIN_*；详见 Video/PluginDefinition.hpp）----
#define CLIP_TYPE "Live"
#define CLIP_DESCRIBE "实时源媒体片段"

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

    /// 片段属性面板专属编辑区工厂（UI 扩展点，2026-11-13 片段插件化）：
    /// 未绑定 Live 素材返回 nullptr（面板只含通用区）；ctx = 宿主构造的
    /// ms::ClipSectionContext*（经 void* 穿越基类边界）。
    QWidget* createPropertySection(ms::ClipSectionContext* ctx,
                                   QWidget* parent) override;

    // ---- 类型显示元数据（P2-B 下放 2026-11-17）----
    /// 页 3 页签名（原宿主 kindPageName 表下放）
    QString clipDisplayName() const override { return QStringLiteral("实况"); }
    /// 无媒体绑定语义：「指定媒体文件…」与实况区源选择功能重复，绑定/
    /// 选择入口统一归实况区（2026-11-09 用户定案）。实况源名是动态值
    ///（读 Asset.live_source_name），仍归宿主解析，bindingHint 不用。
    bool hasMediaBinding() const override { return false; }
};
