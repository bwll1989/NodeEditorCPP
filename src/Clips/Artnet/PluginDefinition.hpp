// ============================================================================
//  PluginDefinition —— Artnet 灯光片段插件定义（src/Clips/Artnet/）
//
//  【2026-11-15 片段移植】自节点体系 src/Nodes/TimeLineNode/artnetclipmodel
//  （NodeClipDelegateModel 帧域 + currentData 拉取驱动）移植为磁盘插件
//  （SceneClipModel μs 域；播放驱动 2026-11-22 起由 PlaybackSendScheduler
//  独立线程直读工作副本快照，模型侧 onPlayheadAdvanced 推送钩子已于
//  2026-11-27 退役删除）。文件组织对齐
//  Control 插件基准（PluginDefinition.hpp/.cpp + 类型模型独立头；类名统一
//  Plugin）。业务接口 = BaseClass 的 ClipPlugInterface
// （clipType/createModel/createPropertySection），Q_PLUGIN_METADATA 供
//  QPluginLoader 识别——编译进独立 artnet.plugin（磁盘驱动）。
//
//  语义（与节点体系同源）：片段引用一段 DMX 视频（宽 512 像素 × 高 = 域数，
//  FFV1 编码；灰度直读 / 旧 RGBA 取 R 通道），播放头进入片段区间后逐帧
//  解码，把每一行（universe）的 512 字节 DMX 写入 ArtnetService 状态表
// （owner = 本片段），由服务内置 50Hz 节拍合成广播（host/universe 可配，
//  默认广播 192.168.1.255、起始域 0、subnet/net 恒 0——Timeline 简化）。
//  配置存 ms::Clip.clip_data（JSON，随场景 pb 分发）：{"file":媒体库相对
//  路径,"targetHost":"...","startUniverse":N}。无素材绑定（asset_id 恒空）、
//  无画布呈现（与 Control 同口径）。结构详见 ../Video/PluginDefinition.hpp。
// ============================================================================

#pragma once

#include <QObject>
#include <QWidget>

#include "AbstractClipInterface.h"

// ---- 插件元数据宏区（布局对齐节点 PLUGIN_*；详见 Video/PluginDefinition.hpp）----
#define CLIP_TYPE "Artnet"
#define CLIP_DESCRIBE "灯光片段（DMX 视频逐帧回放）"

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

    /// 片段属性面板专属编辑区工厂：Artnet 无素材（asset_id 恒空），
    /// 只校验片段上下文存在即建区（与 Control 同口径）。
    QWidget* createPropertySection(ms::ClipSectionContext* ctx,
                                   QWidget* parent) override;

    // ---- 类型显示元数据（P2-B 下放 2026-11-17）----
    /// 页 3 页签名（原宿主 kindPageName 表漏本类型落兜底「属性」——
    /// 表漂移实证，P2-B 下放后由本声明纠回）
    QString clipDisplayName() const override { return tr("灯光"); }
    /// 无媒体绑定语义：DMX 视频在专属页选择（媒体区隐藏 + 绑定行说明）
    bool hasMediaBinding() const override { return false; }
    QString bindingHint() const override {
        return tr("（灯光片段：DMX 视频在专属页选择）");
    }

    // ---- 动画页暴露面（P4-A schema 化 2026-11-17）----
    /// 灯光片段无画布呈现（无 transform 语义）——动画页整页不加（与
    /// Control 同口径；Doc.md「实现期二选一，默认隐藏」定案）
    ClipTweenExposure tweenExposure() const override { return {false, {}}; }
};
