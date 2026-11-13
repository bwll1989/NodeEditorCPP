// ============================================================================
//  AudioClipPlugin —— 音频媒体插件（时间轴片段类型）
//  结构说明见 VideoClipPlugin.hpp 头注释（一种媒体 = 一个插件文件）。
//  说明：Player 仅消费音频的时间信息（§4.4，不出声），画布无几何呈现，
//  片段只承载时间区间（入出点后续扩展）。
// ============================================================================

#ifndef AUDIOCLIPPLUGIN_HPP
#define AUDIOCLIPPLUGIN_HPP

#include <QObject>

#include "SceneClipModel.hpp"
#include "AbstractClipInterface.h"

// ---------------------------------------------------------------------------
// 音频片段模型：紫色块
// ---------------------------------------------------------------------------
class AudioClipModel : public SceneClipModel {
    Q_OBJECT
public:
    AudioClipModel(qint64 startUs, QObject* parent = nullptr)
        : SceneClipModel(startUs, "Audio", {}, parent) {
        RESIZEABLE  = true;
        EMBEDWIDGET = false;
        SHOWBORDER  = true;
        setColor(QColor(0x9B, 0x6F, 0xD1)); // 紫（音频类型色）
    }
};

// ---------------------------------------------------------------------------
// 音频插件：注册进类型表（type = "Audio"）
// ---------------------------------------------------------------------------
class AudioClipPlugin : public QObject, public ClipPlugInterface {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID ClipPlugInterface_iid) // QPluginLoader 按 IID 识别
    Q_INTERFACES(ClipPlugInterface)
public:
    explicit AudioClipPlugin(QObject* parent = nullptr) : QObject(parent) {}

    QString clipType() const override { return "Audio"; }

    AbstractClipModel* createModel(qint64 startUs) override {
        return new AudioClipModel(startUs);
    }
};

#endif // AUDIOCLIPPLUGIN_HPP
