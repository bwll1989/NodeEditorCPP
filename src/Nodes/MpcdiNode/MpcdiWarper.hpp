#pragma once

/**
 * @file MpcdiWarper.hpp
 * @brief 读取 MPCDI，并在当前 OpenGL 上下文里按 region 做 Warp + Blend
 *
 * 线程约束与 ViosoWarper 相同：upload / render 必须在 ImageGpuUpload::runGl() 内调用。
 * 失败路径不得在 runGl 内析构带 RAII lifetime 的 GpuTextureHandle。
 */

#include "GpuTextureHandle.h"

#include <QString>
#include <QStringList>

#include <vector>

class QOpenGLFunctions;

namespace Nodes
{

class MpcdiWarpSet
{
public:
    MpcdiWarpSet() = default;
    ~MpcdiWarpSet();

    MpcdiWarpSet(const MpcdiWarpSet&) = delete;
    MpcdiWarpSet& operator=(const MpcdiWarpSet&) = delete;

    /** @brief 在 GL 线程之外释放已上传的纹理（走 ImageGpuUpload::destroyTexture） */
    void releaseGpu();

    /**
     * @brief 解析 `.mpcdi`，抽出所选 buffer 的每个 region
     * @param bufferId 空则用文件里的第一个 buffer
     * @param force    true 时忽略缓存重新读盘
     */
    bool load(const QString& mpcdiPath, const QString& bufferId, QString* error, bool force = false);

    void invalidate();

    bool isCpuReady() const { return m_cpuReady; }
    bool matches(const QString& mpcdiPath, const QString& bufferId) const;

    QStringList bufferIds() const { return m_bufferIds; }
    QString loadedBufferId() const { return m_bufferId; }
    QString summary() const { return m_summary; }
    int regionCount() const;
    QString regionId(int index) const;
    int outputWidth(int index) const;
    int outputHeight(int index) const;
    QString lastError() const { return m_lastError; }

    /** @brief 把 warp / alpha 上传到当前上下文。须在 runGl 内调用。 */
    bool upload(QOpenGLFunctions* f);

    /**
     * @brief 将源纹理按该 region 变形并乘融合遮罩
     * 返回的纹理尚未做显示管线 V 翻转。
     * coordMode：0 自动，1 强制归一化，2 强制像素。
     */
    struct RenderOptions
    {
        bool flipY = true;
        bool flipWarpY = false;
        bool regionLocal = false;
        bool blend = true;
        bool linearizeAlpha = true;
        bool blackLevel = true;
        bool bounds = false;
        int coordMode = 0;
    };

    NodeDataTypes::GpuTextureHandle render(QOpenGLFunctions* f,
                                           int regionIndex,
                                           const NodeDataTypes::GpuTextureHandle& src,
                                           const RenderOptions& options);

private:
    struct Region
    {
        QString id;
        int outW = 0;
        int outH = 0;
        int warpW = 0;
        int warpH = 0;
        float warpScaleX = 1.f;
        float warpScaleY = 1.f;
        float pixelScaleX = 1.f;
        float pixelScaleY = 1.f;
        float rectX = 0.f;
        float rectY = 0.f;
        float rectW = 1.f;
        float rectH = 1.f;
        float decodeGamma = 1.f;
        float encodeGamma = 1.f;
        float alphaGamma = 1.f;
        std::vector<float> warp; ///< RGB32F，y=0 为纹理底行
        int alphaW = 0;
        int alphaH = 0;
        std::vector<unsigned char> alpha; ///< RGBA8，y=0 为纹理底行；空=不融合
        int betaW = 0;
        int betaH = 0;
        std::vector<unsigned char> beta;
        unsigned int warpTex = 0;
        unsigned int alphaTex = 0;
        unsigned int betaTex = 0;
    };

    void deleteGpuTextures(QOpenGLFunctions* f);

    QString m_path;
    QString m_bufferId;
    QStringList m_bufferIds;
    QString m_summary;
    QString m_lastError;
    std::vector<Region> m_regions;
    bool m_cpuReady = false;
    bool m_gpuDirty = true;
    bool m_gpuReady = false;
};

} // namespace Nodes
