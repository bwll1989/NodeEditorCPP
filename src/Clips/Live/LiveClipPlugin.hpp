// ============================================================================
//  LiveClipPlugin —— 实时源媒体插件（时间轴片段类型；2026-11-13 磁盘化）
//  结构说明见 VideoClipPlugin.hpp 头注释（一种媒体 = 一个插件文件）。
//  实时源业务属性（NDI 源名/延迟）存于工作副本的 Live 素材
//  （Asset.live_*，随场景 pb 分发）——编辑区 = 本目录 LiveClipSection
//  （随本插件走：宿主经 createPropertySection 工厂加载，广播灌值经
//  ClipSectionInterface）。
//  本模型不持业务镜像字段：Live 域唯一写者是实况区（直写素材），模型只
//  镜像 asset_id（与 TextClipPlugin 同款"无文件素材"模式）。
//  【2026-11-13 磁盘化（用户定案）】原"仅内置注册"撤销——Live 与其余
//  四类统一为磁盘 .plugin（改完重编→替换即可）；内置兜底注册同步删除
//  （Q_PLUGIN_METADATA 单份约束：磁盘形态与内置形态不可共存——删
//  live.plugin 后 Live 类型不可建，与其他类型行为一致）。
// ============================================================================

#ifndef LIVECLIPPLUGIN_HPP
#define LIVECLIPPLUGIN_HPP

#include <QObject>
#include <QWidget>

#include "SceneClipModel.hpp"
#include "AbstractClipInterface.h"
#include "LiveClipSection.hpp"          // 同目录：实况源专属编辑区
#include "model/ClipSectionContext.hpp" // ms::ClipSectionContext（MediaCore）

// ---------------------------------------------------------------------------
// 实况片段模型：青色块（与视频/图片/文本色区分）
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// 实况插件：注册进类型表（type = "Live"）
// ---------------------------------------------------------------------------
class LiveClipPlugin : public QObject, public ClipPlugInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ClipPlugInterface_iid) // QPluginLoader 按 IID 识别
    Q_INTERFACES(ClipPlugInterface)
public:
    explicit LiveClipPlugin(QObject* parent = nullptr) : QObject(parent) {}

    QString clipType() const override { return "Live"; }

    AbstractClipModel* createModel(qint64 startUs) override {
        return new LiveClipModel(startUs);
    }

    /// 片段属性面板专属编辑区工厂（UI 扩展点，2026-11-13 片段插件化）：
    /// 未绑定 Live 素材返回 nullptr（面板只含通用区）；ctx = 宿主构造的
    /// ms::ClipSectionContext*（经 void* 穿越基类边界）。
    QWidget* createPropertySection(void* ctx, QWidget* parent) override {
        auto* c = static_cast<ms::ClipSectionContext*>(ctx);
        if (!c || !c->clip || !c->scene) return nullptr;
        const ms::Asset* a = c->scene->findAsset(c->clip->asset_id);
        if (!a || a->kind != ms::AssetKind::Live) return nullptr; // 未绑定实况素材
        return new ms::LiveClipSection(*c, parent);
    }
};

#endif // LIVECLIPPLUGIN_HPP
