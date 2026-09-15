#pragma once

/**
 * @file ISFRenderer.hpp
 * @brief VVISF 封装：在 ImageGpuUpload 共享 GL 上下文中加载 / 渲染 ISF
 *
 * ============================================================================
 * 线程与上下文约束（与 ViosoWarper 相同）
 * ============================================================================
 * - ensureReady / render / destroy 必须在 ImageGpuUpload::runGl 回调内调用
 * - render() 返回的 GpuTextureHandle 在 runGl 内不要挂 makeTextureLifetime；
 *   成功离开 runGl 后再由 DataModel 调用 adoptTextureLifetime
 *
 * ============================================================================
 * 坐标系 / 音频
 * ============================================================================
 * - 图像输入：flipped=true（工程顶原点 → ISF 采样）
 * - 音频输入：1×W float 行纹理，setBufferForAudioInputKey（一般不翻 V）
 * - 输出：blit 后由 DataModel 像素翻 V
 */

#include "GpuTextureHandle.h"

#include <QHash>
#include <QString>
#include <QVariant>
#include <QVariantMap>
#include <QVector>

#include <memory>

class QOpenGLFunctions;

namespace Nodes
{

/**
 * @brief 从 ISF JSON INPUTS 解析出的端口/控件描述
 */
struct ISFPortDesc
{
    QString name;    ///< ATTR NAME（接线 / 存盘键）
    QString caption; ///< LABEL，空则回退为 name

    enum class Kind {
        Image,       ///< TYPE: image → ImageData
        Float,       ///< TYPE: float
        Bool,        ///< TYPE: bool
        Long,        ///< TYPE: long
        Event,       ///< TYPE: event（单帧脉冲）
        Color,       ///< TYPE: color
        Point2D,     ///< TYPE: point2D
        Audio,       ///< TYPE: audio → AudioData（节点内打波形纹理）
        AudioFFT,    ///< TYPE: audioFFT → AudioData（节点内 FFT 后打频谱纹理）
        Unsupported  ///< cube 等暂不建口
    } kind = Kind::Unsupported;

    QVariant defaultValue;

    double minValue = 0.0;
    double maxValue = 1.0;
    bool hasMin = false;
    bool hasMax = false;

    double minX = 0.0;
    double minY = 0.0;
    double maxX = 1.0;
    double maxY = 1.0;
    bool hasPointMin = false;
    bool hasPointMax = false;

    /**
     * audio / audioFFT 的 MAX：期望纹理宽度（采样数或 FFT bin 数）。
     * ISF 里该 MAX 是 long；未声明时用默认 512。
     */
    int audioMaxSamples = 512;
    bool hasAudioMax = false;

    QStringList longLabels;
    QVector<int> longValues;

    bool isImage() const { return kind == Kind::Image; }
    bool isAudioWave() const { return kind == Kind::Audio; }
    bool isAudioFft() const { return kind == Kind::AudioFFT; }
    bool isAudioPort() const { return isAudioWave() || isAudioFft(); }

    bool isVariable() const
    {
        return kind == Kind::Float || kind == Kind::Bool || kind == Kind::Long
               || kind == Kind::Event || kind == Kind::Color || kind == Kind::Point2D;
    }
};

/** 一行音频纹理的 CPU 载荷（值域 0–1，宽 = values.size()） */
struct ISFAudioRow
{
    QVector<float> values;
};

/**
 * @brief 单节点一份的 ISF 渲染器（VVGL 弱上下文 + ISFScene）
 */
class ISFRenderer
{
public:
    ISFRenderer();
    ~ISFRenderer();

    ISFRenderer(const ISFRenderer&) = delete;
    ISFRenderer& operator=(const ISFRenderer&) = delete;

    void destroy(QOpenGLFunctions* f);

    bool ensureReady(QOpenGLFunctions* f,
                     const QString& absoluteFsPath,
                     bool forceReload = false);

    QVector<ISFPortDesc> portDescriptors() const { return m_ports; }
    QString loadedPath() const { return m_loadedPath; }
    bool isReady() const { return m_ready; }
    QString lastError() const { return m_lastError; }

    /**
     * @brief 渲染一帧（须在 runGl 内）
     * @param images       image ATTR → 外部纹理
     * @param audioRows    audio/audioFFT ATTR → 已 cook 的 0–1 行数据
     * @param scalarValues 标量 ATTR
     * @param renderTime   秒；<0 用 scene 时钟
     * @param outW/outH    <=0 从输入图推断，再不行 1280×720
     */
    NodeDataTypes::GpuTextureHandle render(
        QOpenGLFunctions* f,
        const QHash<QString, NodeDataTypes::GpuTextureHandle>& images,
        const QHash<QString, ISFAudioRow>& audioRows,
        const QVariantMap& scalarValues,
        double renderTime,
        int outW,
        int outH);

private:
    bool bootstrapGl(QOpenGLFunctions* f);
    bool loadFile(const QString& absoluteFsPath);
    void refreshPortCache();

    struct Impl;
    std::unique_ptr<Impl> m_impl;

    QString m_loadedPath;
    QVector<ISFPortDesc> m_ports;
    /** 上一帧上传的音频行纹理，保留到下一帧再删（scene 仍可能持有 GLBuffer 包装） */
    QVector<NodeDataTypes::GpuTextureHandle> m_ownedAudioTextures;
    bool m_ready = false;
    bool m_glReady = false;
    QString m_lastError;
};

bool parseISFPortsFromFile(const QString& absoluteFsPath,
                           QVector<ISFPortDesc>& ports,
                           QString* error = nullptr);

} // namespace Nodes
