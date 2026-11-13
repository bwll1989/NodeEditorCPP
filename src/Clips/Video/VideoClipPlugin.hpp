// ============================================================================
//  VideoClipPlugin —— 视频媒体插件（时间轴片段类型；纯磁盘 .plugin 形态）
//
//  插件化约定（第四迭代引入；2026-11-13 片段与内核彻底解耦——用户定案）：
//    * 一种媒体类型 = 一个插件目录（src/Clips/<Type>/）= XxxClipModel +
//      XxxClipPlugin 两个类（类型专属编辑区随目录自选附加）；
//    * XxxClipModel：类型专属模型（默认色 / 可缩放标志 / 业务镜像字段），
//      后续类型特有编辑能力（视频入出点、音量等）加在模型上；
//    * XxxClipPlugin：ClipPlugInterface 实现（clipType 类型串 + 工厂 +
//      可选 createPropertySection 属性面板专属区），Q_PLUGIN_METADATA 供
//      QPluginLoader 识别，编译进独立 .plugin 动态库；
//    * 加载路径（WorkScenePluginLoader::loadPlugins）：纯磁盘驱动——
//      扫描 exe 同级 plugins/TimeLine/*.plugin（构建直接产出到该目录），
//      没有 .plugin 文件 = 类型不存在
//      （内置兜底已删：Add Track 菜单不露项，内核不含任何片段实现）；
//    * 右键 "Add Track" 菜单、片段模型工厂、类型表全部来自插件表——
//      新增媒体类型 = 新增一个插件目录（构建脚本 foreach 自动发现）。
// ============================================================================

#ifndef VIDEOCLIPPLUGIN_HPP
#define VIDEOCLIPPLUGIN_HPP

#include <QObject>

#include "SceneClipModel.hpp"
#include "AbstractClipInterface.h"

// ---------------------------------------------------------------------------
// 视频片段模型：橙色块（与编辑器既有配色一致）、可拖拽移动/缩放
// ---------------------------------------------------------------------------
class VideoClipModel : public SceneClipModel {
    Q_OBJECT
public:
    VideoClipModel(qint64 startUs, QObject* parent = nullptr)
        : SceneClipModel(startUs, "Video", {}, parent) {
        RESIZEABLE  = true;   // 时间轴上可拖右缘改时长
        EMBEDWIDGET = false;  // 不内嵌控件编辑器（属性走双击对话框）
        SHOWBORDER  = false;
        setColor(QColor(0xE6, 0x8A, 0x33)); // 橙（视频类型色）
    }
};

// ---------------------------------------------------------------------------
// 视频插件：type = "Video"。Q_PLUGIN_METADATA 供 QPluginLoader 按
// ClipPlugInterface_iid 识别（本类只存在于 video.plugin 一个二进制）。
// ---------------------------------------------------------------------------
class VideoClipPlugin : public QObject, public ClipPlugInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ClipPlugInterface_iid)
    Q_INTERFACES(ClipPlugInterface)
public:
    explicit VideoClipPlugin(QObject* parent = nullptr) : QObject(parent) {}

    QString clipType() const override { return "Video"; }

    AbstractClipModel* createModel(qint64 startUs) override {
        return new VideoClipModel(startUs); // parent 由适配器随后 setParent
    }
};

#endif // VIDEOCLIPPLUGIN_HPP
