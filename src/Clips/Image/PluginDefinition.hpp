// ============================================================================
//  PluginDefinition —— 静态图片媒体片段插件定义（src/Clips/Image/）
//
//  【2026-11-15 结构对齐（用户定案「片段实现与节点同构」）】文件命名/组织
//  对齐节点插件基准（PluginDefinition.hpp/.cpp + 类型模型独立头）；类名
//  统一 Plugin（每插件一份，各自 DLL 内不冲突）。业务接口 = BaseClass 的
//  ClipPlugInterface（clipType/createModel），Q_PLUGIN_METADATA 供
//  QPluginLoader 识别——编译进独立 image.plugin（bin/plugins/TimeLine/，
//  磁盘驱动：删 .plugin = 类型不存在）。结构详见 Video/PluginDefinition.hpp。
// ============================================================================

#pragma once

#include <QObject>

#include "AbstractClipInterface.h"

class Plugin : public QObject, public ClipPlugInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ClipPlugInterface_iid)
    Q_INTERFACES(ClipPlugInterface)
public:
    explicit Plugin(QObject* parent = nullptr);

    /// 类型串（Add Track 菜单/场景存储/工厂表键，与目录名一致）
    QString clipType() const override;

    /// 片段模型工厂（parent 由适配器随后 setParent）
    AbstractClipModel* createModel(qint64 startUs) override;
};
