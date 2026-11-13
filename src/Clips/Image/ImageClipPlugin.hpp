// ============================================================================
//  ImageClipPlugin —— 静态图片媒体插件（时间轴片段类型）
//  结构说明见 VideoClipPlugin.hpp 头注释（一种媒体 = 一个插件文件；
//  磁盘 .plugin 优先 + 进程内兜底，双形态见 VideoClipPlugin 插件类注释）。
// ============================================================================

#ifndef IMAGECLIPPLUGIN_HPP
#define IMAGECLIPPLUGIN_HPP

#include <QObject>

#include "SceneClipModel.hpp"
#include "AbstractClipInterface.h"

// ---------------------------------------------------------------------------
// 图片片段模型：青色块；图片无固有播放时长，时间轴上默认可自由拉伸
// ---------------------------------------------------------------------------
class ImageClipModel : public SceneClipModel {
    Q_OBJECT
public:
    ImageClipModel(qint64 startUs, QObject* parent = nullptr)
        : SceneClipModel(startUs, "Image", {}, parent) {
        RESIZEABLE  = true;
        EMBEDWIDGET = false;
        SHOWBORDER  = false;
        setColor(QColor(0x33, 0xB2, 0xA6)); // 青（图片类型色）
    }
};

// ---------------------------------------------------------------------------
// 图片插件：注册进类型表（type = "Image"）
// ---------------------------------------------------------------------------
class ImageClipPlugin : public QObject, public ClipPlugInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ClipPlugInterface_iid) // QPluginLoader 按 IID 识别
    Q_INTERFACES(ClipPlugInterface)
public:
    explicit ImageClipPlugin(QObject* parent = nullptr) : QObject(parent) {}

    QString clipType() const override { return "Image"; }

    AbstractClipModel* createModel(qint64 startUs) override {
        return new ImageClipModel(startUs);
    }
};

#endif // IMAGECLIPPLUGIN_HPP
