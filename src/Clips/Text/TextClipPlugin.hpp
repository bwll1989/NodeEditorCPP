// ============================================================================
//  TextClipPlugin —— 字幕/文本媒体插件（时间轴片段类型）
//  结构说明见 VideoClipPlugin.hpp 头注释（一种媒体 = 一个插件文件）。
//  文本内容/字体/字号/颜色等业务属性存于工作副本的 Text 素材（Asset.text_*，
//  随场景 pb 分发）——编辑区 = 本目录 TextClipSection（2026-11-05 文本
//  片段完整化；2026-11-13 随片段插件化随本插件走：宿主经
//  createPropertySection 工厂加载，广播灌值经 ClipSectionInterface）。
//  本模型不持业务镜像字段：文本域唯一写者是文本区（直写素材），模型只镜像
//  asset_id（画布渲染/依赖联动）——此前骨架期的镜像字段已删除（双源易漂移）。
// ============================================================================

#ifndef TEXTCLIPPLUGIN_HPP
#define TEXTCLIPPLUGIN_HPP

#include <QObject>
#include <QWidget>

#include "SceneClipModel.hpp"
#include "AbstractClipInterface.h"

// 专属编辑区（2026-11-13 片段插件化）：随本插件分发——TextClipSection 的
// 实现（含 moc 槽符号）只在 text.plugin 内，主程序不含任何片段实现。
#include "TextClipSection.hpp"          // 同目录：文本专属编辑区（UI 扩展点）
#include "model/ClipSectionContext.hpp" // ms::ClipSectionContext（MediaCore）

// ---------------------------------------------------------------------------
// 字幕片段模型：黄色块
// ---------------------------------------------------------------------------
class TextClipModel : public SceneClipModel {
    Q_OBJECT
public:
    TextClipModel(qint64 startUs, QObject* parent = nullptr)
        : SceneClipModel(startUs, "Text", {}, parent) {
        RESIZEABLE  = true;
        EMBEDWIDGET = false;
        SHOWBORDER  = false;
        setColor(QColor(0xD9, 0xC2, 0x3C)); // 黄（文本类型色）
    }
};

// ---------------------------------------------------------------------------
// 字幕插件：注册进类型表（type = "Text"）
// ---------------------------------------------------------------------------
class TextClipPlugin : public QObject, public ClipPlugInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ClipPlugInterface_iid) // QPluginLoader 按 IID 识别
    Q_INTERFACES(ClipPlugInterface)
public:
    explicit TextClipPlugin(QObject* parent = nullptr) : QObject(parent) {}

    QString clipType() const override { return "Text"; }

    AbstractClipModel* createModel(qint64 startUs) override {
        return new TextClipModel(startUs);
    }

    /// 片段属性面板专属编辑区工厂（UI 扩展点，2026-11-13 片段插件化）：
    /// 未绑定文本素材返回 nullptr（面板只含通用区）；ctx = 宿主构造的
    /// ms::ClipSectionContext*（经 void* 穿越基类边界）。
    QWidget* createPropertySection(void* ctx, QWidget* parent) override {
        auto* c = static_cast<ms::ClipSectionContext*>(ctx);
        if (!c || !c->clip || !c->scene) return nullptr;
        const ms::Asset* a = c->scene->findAsset(c->clip->asset_id);
        if (!a || a->kind != ms::AssetKind::Text) return nullptr; // 未绑定文本素材
        return new ms::TextClipSection(*c, parent);
    }
};

#endif // TEXTCLIPPLUGIN_HPP
