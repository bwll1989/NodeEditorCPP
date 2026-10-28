/**
 * @file MpcdiWarper.cpp
 * @brief 用 Scalable MPCDI 库读校准，再用全屏 shader 做 2D warp + alpha blend
 *
 * 语义按 VESA MPCDI 2.0 的 2D 档（读盘仍是 Scalable 1.0 库，缺的 2.0 字段当可选）：
 * - PFM 第一个像素是光栅左上（§3.6.1）。1.0 库按经典 PFM 自底向上读入，
 *   这一行落在内存高地址，上传后在纹理顶部。
 * - R/G 是归一化 UV（§3.6.2）。V=1 为内容顶部。节点上的「翻转 Y」打开时，
 *   采样源图用 1-V，用来把画面顶部对上内容顶部。
 * - 多数样本在 0..1 时按归一化 UV；多数远大于 1 时按 buffer 像素。
 *   越界、负数、NaN 输出黑色。
 * - alpha PNG 顶行在前，上传时翻成与变形网格同一朝向。
 *   乘之前按 gammaEmbedded 还原成线性。画面 RGB 先按 <color> decode 进线性，
 *   融合后再按 encode 回去。没有这些标签时伽马为 1，等于不转换。beta 不参与。
 */

#include "MpcdiWarper.hpp"

#include "ImageGpuUpload.h"

#include <QFileInfo>
#include <QOpenGLFunctions>
#include <QOpenGLShader>
#include <QOpenGLShaderProgram>

#include <algorithm>
#include <cmath>
#include <cstring>
#include <memory>
#include <utility>

#include "mpcdiAlphaMap.h"
#include "mpcdiBetaMap.h"
#include "mpcdiBuffer.h"
#include "mpcdiDisplay.h"
#include "mpcdiErrorHelper.h"
#include "mpcdiFileSet.h"
#include "mpcdiGeometryWarpFile.h"
#include "mpcdiProfile.h"
#include "mpcdiReader.h"
#include "mpcdiRegion.h"

namespace Nodes
{
namespace
{

#ifndef GL_RGB32F
#define GL_RGB32F 0x8815
#endif

constexpr int kMaxRegions = 64;
constexpr float kNormalizedLimit = 2.5f;

const char kVertShader[] = R"(
attribute vec2 vertex;
attribute vec2 texCoord;
varying vec2 vTexCoord;
void main() {
    gl_Position = vec4(vertex, 0.0, 1.0);
    vTexCoord = texCoord;
}
)";

const char kFragWarp[] = R"(
uniform sampler2D uSource;
uniform sampler2D uWarp;
uniform sampler2D uAlpha;
uniform sampler2D uBeta;
uniform vec2 uWarpScale;
uniform float uUseAlpha;
uniform float uUseBeta;
uniform float uFlipY;
uniform float uFlipWarpY;
uniform float uRegionLocal;
uniform vec4 uRegionRect;
uniform float uBlend;
uniform float uBlackLevel;
uniform float uBounds;
uniform float uDecodeGamma;
uniform float uEncodeGamma;
uniform float uAlphaGamma;
varying vec2 vTexCoord;
void main() {
    vec2 warpUv = vTexCoord;
    if (uFlipWarpY > 0.5) {
        warpUv.y = 1.0 - warpUv.y;
    }
    vec2 uv = texture2D(uWarp, warpUv).rg / uWarpScale;
    bool mapped = uv.x >= -0.001 && uv.y >= -0.001 && uv.x <= 1.001 && uv.y <= 1.001;
    if (!mapped) {
        gl_FragColor = uBounds > 0.5 ? vec4(0.25, 0.15, 0.0, 1.0) : vec4(0.0);
        return;
    }
    if (uRegionLocal > 0.5 && uRegionRect.z > 0.0001 && uRegionRect.w > 0.0001) {
        uv = (uv - uRegionRect.xy) / uRegionRect.zw;
    }
    if (uFlipY > 0.5) {
        uv.y = 1.0 - uv.y;
    }
    vec4 color = texture2D(uSource, clamp(uv, 0.0, 1.0));
    color.rgb = pow(max(color.rgb, 0.0), vec3(uDecodeGamma));
    float blendMask = 1.0;
    if (uBlend > 0.5 && uUseAlpha > 0.5) {
        blendMask = max(texture2D(uAlpha, vTexCoord).r, 0.0);
        blendMask = pow(blendMask, uAlphaGamma);
    }
    float black = 0.0;
    if (uBlackLevel > 0.5 && uUseBeta > 0.5) {
        black = texture2D(uBeta, vTexCoord).r;
    }
    color.rgb = blendMask * color.rgb * (1.0 - black) + black;
    color.a *= blendMask;
    color.rgb = pow(max(color.rgb, 0.0), vec3(1.0 / uEncodeGamma));
    if (uBounds > 0.5) {
        float e = 0.004;
        if (vTexCoord.x < e || vTexCoord.x > 1.0 - e || vTexCoord.y < e || vTexCoord.y > 1.0 - e) {
            color.rgb = mix(color.rgb, vec3(0.0, 1.0, 0.5), 0.8);
        }
    }
    gl_FragColor = color;
}
)";

bool copyMapToRgba(mpcdi::DataMap* map, std::vector<unsigned char>& dst, int& width, int& height)
{
    if (!map) {
        return false;
    }
    const int w = static_cast<int>(map->GetSizeX());
    const int h = static_cast<int>(map->GetSizeY());
    const int depth = static_cast<int>(map->GetComponentDepth());
    std::vector<unsigned char>* src = map->GetData();
    const size_t need = static_cast<size_t>(w) * static_cast<size_t>(h) * static_cast<size_t>(depth);
    if (w <= 0 || h <= 0 || depth <= 0 || !src || src->size() < need) {
        return false;
    }
    width = w;
    height = h;
    dst.resize(static_cast<size_t>(w) * static_cast<size_t>(h) * 4);
    for (int y = 0; y < h; ++y) {
        const int srcY = h - 1 - y;
        for (int x = 0; x < w; ++x) {
            const size_t si = (static_cast<size_t>(srcY) * static_cast<size_t>(w) + static_cast<size_t>(x))
                              * static_cast<size_t>(depth);
            const unsigned char value = (*src)[si];
            const size_t di = (static_cast<size_t>(y) * static_cast<size_t>(w) + static_cast<size_t>(x)) * 4;
            dst[di + 0] = value;
            dst[di + 1] = value;
            dst[di + 2] = value;
            dst[di + 3] = value;
        }
    }
    return true;
}

QString profileLabel(mpcdi::ProfileType type)
{
    switch (type) {
    case mpcdi::ProfileType2d: return QStringLiteral("2D");
    case mpcdi::ProfileType3d: return QStringLiteral("3D");
    case mpcdi::ProfileTypea3: return QStringLiteral("Advanced 3D");
    case mpcdi::ProfileTypesl: return QStringLiteral("Shader Lamp");
    }
    return QStringLiteral("MPCDI");
}

QString readErrorText(mpcdi::MPCDI_Error err)
{
    const QString detail = QString::fromStdString(mpcdi::ErrorHelper::GetLastError(false)).trimmed();
    QString text = QStringLiteral("MPCDI read failed (%1)").arg(static_cast<int>(err));
    if (!detail.isEmpty()) {
        text += QStringLiteral(": ") + detail;
    }
    return text;
}

bool looksLike2DWarp(bool is2dProfile, float maxAbs, float bufferW, float bufferH)
{
    if (is2dProfile) {
        return true;
    }
    if (maxAbs <= kNormalizedLimit) {
        return true;
    }
    const float span = std::max(bufferW, bufferH);
    return span > 1.f && maxAbs <= span * kNormalizedLimit;
}

NodeDataTypes::GpuTextureHandle allocateRgbaTexture(QOpenGLFunctions* f, int width, int height)
{
    NodeDataTypes::GpuTextureHandle handle;
    if (!f || width <= 0 || height <= 0) {
        return handle;
    }

    unsigned int texId = 0;
    f->glGenTextures(1, &texId);
    f->glBindTexture(GL_TEXTURE_2D, texId);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
    f->glBindTexture(GL_TEXTURE_2D, 0);

    handle.textureId = texId;
    handle.width = width;
    handle.height = height;
    handle.internalFormat = GL_RGBA8;
    return handle;
}

void deleteRawTexture(QOpenGLFunctions* f, NodeDataTypes::GpuTextureHandle& handle)
{
    if (!handle.valid()) {
        handle = {};
        return;
    }
    const unsigned int id = handle.textureId;
    handle = {};
    if (f && id != 0) {
        f->glDeleteTextures(1, &id);
    }
}

void adoptTextureLifetime(NodeDataTypes::GpuTextureHandle& handle)
{
    if (!handle.valid() || handle.lifetime) {
        return;
    }
    handle.lifetime = NodeDataTypes::makeTextureLifetime(handle.textureId).lifetime;
}

unsigned int uploadRgba(QOpenGLFunctions* f, int width, int height, const unsigned char* pixels)
{
    unsigned int texId = 0;
    f->glGenTextures(1, &texId);
    f->glBindTexture(GL_TEXTURE_2D, texId);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, pixels);
    f->glBindTexture(GL_TEXTURE_2D, 0);
    return texId;
}

unsigned int uploadWarp(QOpenGLFunctions* f, int width, int height, const float* rgb)
{
    unsigned int texId = 0;
    f->glGenTextures(1, &texId);
    f->glBindTexture(GL_TEXTURE_2D, texId);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, width, height, 0, GL_RGB, GL_FLOAT, rgb);
    f->glBindTexture(GL_TEXTURE_2D, 0);
    return texId;
}

QOpenGLShaderProgram& warpProgram()
{
    static QOpenGLShaderProgram* program = nullptr;
    if (!program) {
        program = new QOpenGLShaderProgram();
        program->addShaderFromSourceCode(QOpenGLShader::Vertex, kVertShader);
        program->addShaderFromSourceCode(QOpenGLShader::Fragment, kFragWarp);
        program->link();
    }
    return *program;
}

} // namespace

MpcdiWarpSet::~MpcdiWarpSet()
{
    releaseGpu();
}

void MpcdiWarpSet::releaseGpu()
{
    auto drop = [](unsigned int& id) {
        if (id == 0) {
            return;
        }
        const unsigned int tex = id;
        id = 0;
        NodeDataTypes::ImageGpuUpload::instance().destroyTexture(tex);
    };
    for (auto& region : m_regions) {
        drop(region.warpTex);
        drop(region.alphaTex);
        drop(region.betaTex);
    }
    m_gpuReady = false;
    m_gpuDirty = true;
}

void MpcdiWarpSet::invalidate()
{
    releaseGpu();
    m_regions.clear();
    m_bufferIds.clear();
    m_bufferId.clear();
    m_summary.clear();
    m_cpuReady = false;
    m_path.clear();
    m_gpuDirty = true;
    m_gpuReady = false;
}

bool MpcdiWarpSet::matches(const QString& mpcdiPath, const QString& bufferId) const
{
    if (!m_cpuReady || m_path != mpcdiPath) {
        return false;
    }
    if (bufferId.isEmpty()) {
        return m_bufferId == m_bufferIds.value(0);
    }
    return bufferId == m_bufferId;
}

int MpcdiWarpSet::regionCount() const
{
    return static_cast<int>(m_regions.size());
}

QString MpcdiWarpSet::regionId(int index) const
{
    if (index < 0 || index >= regionCount()) {
        return {};
    }
    return m_regions[static_cast<size_t>(index)].id;
}

int MpcdiWarpSet::outputWidth(int index) const
{
    if (index < 0 || index >= regionCount()) {
        return 0;
    }
    return m_regions[static_cast<size_t>(index)].outW;
}

int MpcdiWarpSet::outputHeight(int index) const
{
    if (index < 0 || index >= regionCount()) {
        return 0;
    }
    return m_regions[static_cast<size_t>(index)].outH;
}

bool MpcdiWarpSet::load(const QString& mpcdiPath, const QString& bufferId, QString* error, bool force)
{
    const QString abs = QFileInfo(mpcdiPath).absoluteFilePath();
    if (!force && matches(abs, bufferId)) {
        if (error) {
            error->clear();
        }
        m_lastError.clear();
        return true;
    }

    auto fail = [&](const QString& message) {
        m_lastError = message;
        if (error) {
            *error = message;
        }
        releaseGpu();
        m_regions.clear();
        m_bufferIds.clear();
        m_summary.clear();
        m_cpuReady = false;
        m_path.clear();
        m_bufferId.clear();
        m_gpuDirty = true;
        m_gpuReady = false;
        return false;
    };

    if (abs.isEmpty() || !QFileInfo::exists(abs)) {
        return fail(QStringLiteral("mpcdi file not found"));
    }

    const QByteArray localPath = abs.toLocal8Bit();
    std::unique_ptr<mpcdi::Profile> profile(new mpcdi::Profile());
    mpcdi::Reader* reader = mpcdi::Reader::CreateReader();
    reader->SetCheckVersionSupported(false);
    reader->SetDoProfileValidation(false);
    const mpcdi::MPCDI_Error err = reader->Read(std::string(localPath.constData()), profile.get());
    delete reader;
    if (MPCDI_FAILED(err) || !profile->GetDisplay()) {
        return fail(readErrorText(err));
    }

    mpcdi::Display* display = profile->GetDisplay();
    const std::vector<std::string> bufferNames = display->GetBufferNames();
    if (bufferNames.empty()) {
        return fail(QStringLiteral("MPCDI has no buffer"));
    }

    QStringList ids;
    ids.reserve(static_cast<int>(bufferNames.size()));
    for (const std::string& name : bufferNames) {
        ids.push_back(QString::fromUtf8(name.c_str()));
    }

    QString chosen = bufferId.trimmed();
    if (!ids.contains(chosen)) {
        chosen = ids.front();
    }
    mpcdi::Buffer* buffer = display->GetBuffer(chosen.toStdString());
    if (!buffer) {
        return fail(QStringLiteral("buffer not found: %1").arg(chosen));
    }

    const std::vector<std::string> regionNames = buffer->GetRegionNames();
    if (regionNames.empty()) {
        return fail(QStringLiteral("buffer \"%1\" has no region").arg(chosen));
    }
    if (static_cast<int>(regionNames.size()) > kMaxRegions) {
        return fail(QStringLiteral("too many regions (%1), max %2")
                        .arg(regionNames.size())
                        .arg(kMaxRegions));
    }

    const bool is2d = profile->GetProfileType() == mpcdi::ProfileType2d;
    const float bufferW = static_cast<float>(buffer->GetXresolution());
    const float bufferH = static_cast<float>(buffer->GetYresolution());

    std::vector<Region> built;
    built.reserve(regionNames.size());
    for (const std::string& regionName : regionNames) {
        mpcdi::Region* region = buffer->GetRegion(regionName);
        mpcdi::FileSet* files = region ? region->GetFileSet() : nullptr;
        mpcdi::GeometryWarpFile* warp = files ? files->GetGeometryWarpFile() : nullptr;
        if (!region || !warp || warp->GetSizeX() == 0 || warp->GetSizeY() == 0 || !warp->GetData()) {
            return fail(QStringLiteral("region \"%1\" has no geometry warp")
                            .arg(QString::fromUtf8(regionName.c_str())));
        }

        const unsigned int warpCount = warp->GetSizeX() * warp->GetSizeY();
        float maxAbs = 0.f;
        int finiteCount = 0;
        int largeCount = 0;
        const mpcdi::NODE* nodes = warp->GetData();
        for (unsigned int i = 0; i < warpCount; ++i) {
            const float r = nodes[i].r;
            const float g = nodes[i].g;
            if (!std::isfinite(r) || !std::isfinite(g)) {
                continue;
            }
            ++finiteCount;
            const float sample = std::max(std::fabs(r), std::fabs(g));
            maxAbs = std::max(maxAbs, sample);
            if (sample > kNormalizedLimit) {
                ++largeCount;
            }
        }
        if (!looksLike2DWarp(is2d, maxAbs, bufferW, bufferH)) {
            return fail(QStringLiteral(
                "profile %1 stores 3D surface coordinates, not a 2D image warp. "
                "Export an MPCDI 2D media file.")
                            .arg(profileLabel(profile->GetProfileType())));
        }

        Region cpu;
        cpu.id = QString::fromUtf8(regionName.c_str());
        cpu.outW = region->GetXresolution();
        cpu.outH = region->GetYresolution();
        if (cpu.outW <= 0) {
            cpu.outW = static_cast<int>(warp->GetSizeX());
        }
        if (cpu.outH <= 0) {
            cpu.outH = static_cast<int>(warp->GetSizeY());
        }
        cpu.warpW = static_cast<int>(warp->GetSizeX());
        cpu.warpH = static_cast<int>(warp->GetSizeY());
        const bool pixelSpace = finiteCount > 0 && largeCount * 2 > finiteCount;
        if (pixelSpace) {
            cpu.warpScaleX = bufferW > 1.f ? bufferW : static_cast<float>(cpu.outW);
            cpu.warpScaleY = bufferH > 1.f ? bufferH : static_cast<float>(cpu.outH);
        }
        cpu.decodeGamma = region->GetDecodeGamma() > 0.f ? region->GetDecodeGamma() : 1.f;
        cpu.encodeGamma = region->GetEncodeGamma() > 0.f ? region->GetEncodeGamma() : 1.f;
        cpu.pixelScaleX = bufferW > 1.f ? bufferW : static_cast<float>(std::max(cpu.outW, 1));
        cpu.pixelScaleY = bufferH > 1.f ? bufferH : static_cast<float>(std::max(cpu.outH, 1));
        cpu.rectX = region->GetX();
        cpu.rectY = region->GetY();
        cpu.rectW = region->GetXsize() > 0.f ? region->GetXsize() : 1.f;
        cpu.rectH = region->GetYsize() > 0.f ? region->GetYsize() : 1.f;
        cpu.warp.resize(static_cast<size_t>(warpCount) * 3);
        std::memcpy(cpu.warp.data(), nodes, cpu.warp.size() * sizeof(float));

        if (mpcdi::AlphaMap* alpha = files->GetAlphaMap()) {
            const double embedded = alpha->GetGammaEmbedded();
            cpu.alphaGamma = embedded > 0.0 ? static_cast<float>(embedded) : 1.f;
            copyMapToRgba(alpha, cpu.alpha, cpu.alphaW, cpu.alphaH);
        }
        if (mpcdi::BetaMap* beta = files->GetBetaMap()) {
            copyMapToRgba(beta, cpu.beta, cpu.betaW, cpu.betaH);
        }
        built.push_back(std::move(cpu));
    }

    releaseGpu();
    m_regions.swap(built);
    m_bufferIds = ids;
    m_bufferId = chosen;
    m_path = abs;
    m_cpuReady = true;
    m_gpuDirty = true;
    m_gpuReady = false;
    m_summary = QStringLiteral("%1 · %2 · %3 region%4")
                    .arg(profileLabel(profile->GetProfileType()),
                         chosen,
                         QString::number(m_regions.size()),
                         m_regions.size() == 1 ? QString() : QStringLiteral("s"));
    m_lastError.clear();
    if (error) {
        error->clear();
    }
    return true;
}

void MpcdiWarpSet::deleteGpuTextures(QOpenGLFunctions* f)
{
    if (!f) {
        return;
    }
    for (auto& region : m_regions) {
        if (region.warpTex != 0) {
            f->glDeleteTextures(1, &region.warpTex);
            region.warpTex = 0;
        }
        if (region.alphaTex != 0) {
            f->glDeleteTextures(1, &region.alphaTex);
            region.alphaTex = 0;
        }
        if (region.betaTex != 0) {
            f->glDeleteTextures(1, &region.betaTex);
            region.betaTex = 0;
        }
    }
}

bool MpcdiWarpSet::upload(QOpenGLFunctions* f)
{
    if (!f || !m_cpuReady) {
        m_lastError = QStringLiteral("MPCDI is not loaded");
        return false;
    }
    if (m_gpuReady && !m_gpuDirty) {
        return true;
    }

    deleteGpuTextures(f);

    GLint prevAlign = 4;
    f->glGetIntegerv(GL_UNPACK_ALIGNMENT, &prevAlign);
    f->glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

    bool ok = true;
    for (auto& region : m_regions) {
        if (region.warp.empty()) {
            m_lastError = QStringLiteral("region \"%1\" warp is empty").arg(region.id);
            ok = false;
            break;
        }
        region.warpTex = uploadWarp(f, region.warpW, region.warpH, region.warp.data());
        if (region.warpTex == 0) {
            m_lastError = QStringLiteral("failed to upload warp for %1").arg(region.id);
            ok = false;
            break;
        }
        if (!region.alpha.empty()) {
            region.alphaTex = uploadRgba(f, region.alphaW, region.alphaH, region.alpha.data());
            if (region.alphaTex == 0) {
                m_lastError = QStringLiteral("failed to upload alpha for %1").arg(region.id);
                ok = false;
                break;
            }
        }
        if (!region.beta.empty()) {
            region.betaTex = uploadRgba(f, region.betaW, region.betaH, region.beta.data());
            if (region.betaTex == 0) {
                m_lastError = QStringLiteral("failed to upload beta for %1").arg(region.id);
                ok = false;
                break;
            }
        }
    }

    f->glPixelStorei(GL_UNPACK_ALIGNMENT, prevAlign);
    if (!ok) {
        deleteGpuTextures(f);
        m_gpuReady = false;
        m_gpuDirty = true;
        return false;
    }

    m_gpuDirty = false;
    m_gpuReady = true;
    m_lastError.clear();
    return true;
}

NodeDataTypes::GpuTextureHandle MpcdiWarpSet::render(QOpenGLFunctions* f,
                                                     int regionIndex,
                                                     const NodeDataTypes::GpuTextureHandle& src,
                                                     const RenderOptions& options)
{
    if (!f || !src.valid() || regionIndex < 0 || regionIndex >= regionCount()) {
        return {};
    }
    if (!upload(f)) {
        return {};
    }

    const Region& region = m_regions[static_cast<size_t>(regionIndex)];
    if (region.warpTex == 0 || region.outW <= 0 || region.outH <= 0) {
        m_lastError = QStringLiteral("region \"%1\" is not ready").arg(region.id);
        return {};
    }

    auto& program = warpProgram();
    if (!program.isLinked()) {
        m_lastError = QStringLiteral("warp shader link failed: %1").arg(program.log());
        return {};
    }

    NodeDataTypes::GpuTextureHandle out = allocateRgbaTexture(f, region.outW, region.outH);
    if (!out.valid()) {
        m_lastError = QStringLiteral("failed to allocate output texture");
        return {};
    }

    GLint prevFbo = 0;
    f->glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);
    GLint prevViewport[4] = {0, 0, 0, 0};
    f->glGetIntegerv(GL_VIEWPORT, prevViewport);

    unsigned int fbo = 0;
    f->glGenFramebuffers(1, &fbo);
    f->glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    f->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, out.textureId, 0);
    const unsigned int status = f->glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        m_lastError = QStringLiteral("incomplete FBO (%1)").arg(status);
        f->glBindFramebuffer(GL_FRAMEBUFFER, static_cast<unsigned int>(prevFbo));
        f->glDeleteFramebuffers(1, &fbo);
        deleteRawTexture(f, out);
        return {};
    }

    f->glViewport(0, 0, region.outW, region.outH);
    f->glDisable(GL_DEPTH_TEST);
    f->glDisable(GL_BLEND);
    f->glClearColor(0.f, 0.f, 0.f, 0.f);
    f->glClear(GL_COLOR_BUFFER_BIT);

    f->glActiveTexture(GL_TEXTURE0);
    f->glBindTexture(GL_TEXTURE_2D, src.textureId);
    f->glActiveTexture(GL_TEXTURE1);
    f->glBindTexture(GL_TEXTURE_2D, region.warpTex);
    f->glActiveTexture(GL_TEXTURE2);
    f->glBindTexture(GL_TEXTURE_2D, region.alphaTex != 0 ? region.alphaTex : region.warpTex);
    f->glActiveTexture(GL_TEXTURE3);
    f->glBindTexture(GL_TEXTURE_2D, region.betaTex != 0 ? region.betaTex : region.warpTex);

    if (!program.bind()) {
        m_lastError = QStringLiteral("failed to bind warp shader");
        f->glBindFramebuffer(GL_FRAMEBUFFER, static_cast<unsigned int>(prevFbo));
        f->glDeleteFramebuffers(1, &fbo);
        deleteRawTexture(f, out);
        return {};
    }

    float scaleX = 1.f;
    float scaleY = 1.f;
    if (options.coordMode == 2) {
        scaleX = region.pixelScaleX;
        scaleY = region.pixelScaleY;
    } else if (options.coordMode == 0) {
        scaleX = region.warpScaleX;
        scaleY = region.warpScaleY;
    }
    const float alphaGamma = options.linearizeAlpha ? region.alphaGamma : 1.f;

    program.setUniformValue("uSource", 0);
    program.setUniformValue("uWarp", 1);
    program.setUniformValue("uAlpha", 2);
    program.setUniformValue("uBeta", 3);
    program.setUniformValue("uWarpScale", scaleX, scaleY);
    program.setUniformValue("uUseAlpha", region.alphaTex != 0 ? 1.f : 0.f);
    program.setUniformValue("uUseBeta", region.betaTex != 0 ? 1.f : 0.f);
    program.setUniformValue("uFlipY", options.flipY ? 1.f : 0.f);
    program.setUniformValue("uFlipWarpY", options.flipWarpY ? 1.f : 0.f);
    program.setUniformValue("uRegionLocal", options.regionLocal ? 1.f : 0.f);
    program.setUniformValue("uRegionRect", region.rectX, region.rectY, region.rectW, region.rectH);
    program.setUniformValue("uBlend", options.blend ? 1.f : 0.f);
    program.setUniformValue("uBlackLevel", options.blackLevel ? 1.f : 0.f);
    program.setUniformValue("uBounds", options.bounds ? 1.f : 0.f);
    program.setUniformValue("uDecodeGamma", region.decodeGamma);
    program.setUniformValue("uEncodeGamma", region.encodeGamma);
    program.setUniformValue("uAlphaGamma", alphaGamma);

    static const float vertices[] = {
        -1.f, -1.f, 0.f, 0.f,
         1.f, -1.f, 1.f, 0.f,
        -1.f,  1.f, 0.f, 1.f,
         1.f,  1.f, 1.f, 1.f,
    };
    program.enableAttributeArray("vertex");
    program.enableAttributeArray("texCoord");
    program.setAttributeArray("vertex", vertices, 2, sizeof(float) * 4);
    program.setAttributeArray("texCoord", vertices + 2, 2, sizeof(float) * 4);
    f->glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    program.disableAttributeArray("vertex");
    program.disableAttributeArray("texCoord");
    program.release();

    f->glActiveTexture(GL_TEXTURE3);
    f->glBindTexture(GL_TEXTURE_2D, 0);
    f->glActiveTexture(GL_TEXTURE2);
    f->glBindTexture(GL_TEXTURE_2D, 0);
    f->glActiveTexture(GL_TEXTURE1);
    f->glBindTexture(GL_TEXTURE_2D, 0);
    f->glActiveTexture(GL_TEXTURE0);
    f->glBindTexture(GL_TEXTURE_2D, 0);
    f->glUseProgram(0);
    f->glBindFramebuffer(GL_FRAMEBUFFER, static_cast<unsigned int>(prevFbo));
    f->glViewport(prevViewport[0], prevViewport[1], prevViewport[2], prevViewport[3]);
    f->glDeleteFramebuffers(1, &fbo);

    m_lastError.clear();
    adoptTextureLifetime(out);
    return out;
}

} // namespace Nodes
