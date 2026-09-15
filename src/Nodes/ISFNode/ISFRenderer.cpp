/**
 * @file ISFRenderer.cpp
 * @brief VVISF / VVGL 与工程 GpuTextureHandle 的桥接实现
 *
 * 数据路径：
 *   标量 QVariant → ISFVal → setValueForInputNamed
 *   图像 GpuTextureHandle → CreateFromExistingGLTexture(flipped) → setBuffer / setFilterInput
 *   音频行 float[] → RGBA32F 1×W 纹理 → setBufferForAudioInputKey
 *   createAndRenderABuffer → FBO blit → 工程自管 RGBA8 纹理
 */

#include "ISFRenderer.hpp"

#include "ImageGpuUpload.h"

// GLEW 必须先于会拉入 gl.h 的 Qt OpenGL 头，避免符号冲突
#include <GL/glew.h>

#include <VVGL.hpp>
#include <VVISF.hpp>

#include <QColor>
#include <QFileInfo>
#include <QOpenGLContext>
#include <QOpenGLExtraFunctions>
#include <QOpenGLFunctions>

#include <algorithm>
#include <exception>
#include <utility>

namespace Nodes
{
namespace
{

// ---------------------------------------------------------------------------
// INPUT 描述解析
// ---------------------------------------------------------------------------

ISFPortDesc::Kind kindFromValType(VVISF::ISFValType t)
{
    switch (t) {
    case VVISF::ISFValType_Image:
        return ISFPortDesc::Kind::Image;
    case VVISF::ISFValType_Float:
        return ISFPortDesc::Kind::Float;
    case VVISF::ISFValType_Bool:
        return ISFPortDesc::Kind::Bool;
    case VVISF::ISFValType_Long:
        return ISFPortDesc::Kind::Long;
    case VVISF::ISFValType_Event:
        return ISFPortDesc::Kind::Event;
    case VVISF::ISFValType_Color:
        return ISFPortDesc::Kind::Color;
    case VVISF::ISFValType_Point2D:
        return ISFPortDesc::Kind::Point2D;
    case VVISF::ISFValType_Audio:
        return ISFPortDesc::Kind::Audio;
    case VVISF::ISFValType_AudioFFT:
        return ISFPortDesc::Kind::AudioFFT;
    default:
        // cube 等 → Unsupported，上层不建口
        return ISFPortDesc::Kind::Unsupported;
    }
}

/** ISFVal → 面板 / 存盘用的 QVariant */
QVariant isfValToVariant(VVISF::ISFVal v)
{
    if (v.isFloatVal()) {
        return v.getDoubleVal();
    }
    if (v.isBoolVal() || v.isEventVal()) {
        return v.getBoolVal();
    }
    if (v.isLongVal()) {
        return v.getLongVal();
    }
    if (v.isPoint2DVal()) {
        return QVariantList{v.getPointValByIndex(0), v.getPointValByIndex(1)};
    }
    if (v.isColorVal()) {
        return QVariantList{v.getColorValByChannel(0), v.getColorValByChannel(1),
                            v.getColorValByChannel(2), v.getColorValByChannel(3)};
    }
    return {};
}

/** 保证 min <= max，避免拖拽控件 range 非法 */
void normalizeRange(double& lo, double& hi)
{
    if (lo > hi) {
        std::swap(lo, hi);
    }
}

ISFPortDesc makePortDesc(const VVISF::ISFAttrRef& attr)
{
    ISFPortDesc d;
    if (!attr) {
        return d;
    }

    d.name = QString::fromStdString(attr->name());
    const QString label = QString::fromStdString(attr->label()).trimmed();
    d.caption = label.isEmpty() ? d.name : label;
    d.kind = kindFromValType(attr->type());

    if (!attr->defaultVal().isNullVal()) {
        d.defaultValue = isfValToVariant(attr->defaultVal());
    }

    if (d.kind == ISFPortDesc::Kind::Point2D) {
        // Point2D 的 MIN/MAX 也是 Point2D；必须用 getPointValByIndex
        if (!attr->minVal().isNullVal() && attr->minVal().isPoint2DVal()) {
            d.hasPointMin = true;
            d.minX = attr->minVal().getPointValByIndex(0);
            d.minY = attr->minVal().getPointValByIndex(1);
        }
        if (!attr->maxVal().isNullVal() && attr->maxVal().isPoint2DVal()) {
            d.hasPointMax = true;
            d.maxX = attr->maxVal().getPointValByIndex(0);
            d.maxY = attr->maxVal().getPointValByIndex(1);
        }
        if (d.hasPointMin && d.hasPointMax) {
            normalizeRange(d.minX, d.maxX);
            normalizeRange(d.minY, d.maxY);
        }
    } else if (d.kind == ISFPortDesc::Kind::Audio || d.kind == ISFPortDesc::Kind::AudioFFT) {
        // audio / audioFFT 的 MAX 是 long：期望纹理宽度（采样数或 bin 数）
        d.audioMaxSamples = (d.kind == ISFPortDesc::Kind::Audio) ? 512 : 512;
        if (!attr->maxVal().isNullVal()) {
            int w = 0;
            if (attr->maxVal().isLongVal()) {
                w = static_cast<int>(attr->maxVal().getLongVal());
            } else {
                w = static_cast<int>(attr->maxVal().getDoubleVal());
            }
            if (w > 0) {
                d.hasAudioMax = true;
                d.audioMaxSamples = std::clamp(w, 1, 8192);
            }
        }
    } else if (d.kind == ISFPortDesc::Kind::Float || d.kind == ISFPortDesc::Kind::Long) {
        // Color 等类型的 MIN/MAX 若误走 getDoubleVal 会得到 0；仅 float/long 需要标量 range
        if (!attr->minVal().isNullVal()) {
            d.hasMin = true;
            d.minValue = attr->minVal().getDoubleVal();
        }
        if (!attr->maxVal().isNullVal()) {
            d.hasMax = true;
            d.maxValue = attr->maxVal().getDoubleVal();
        }
        if (d.hasMin && d.hasMax) {
            normalizeRange(d.minValue, d.maxValue);
        }
    }

    if (d.kind == ISFPortDesc::Kind::Long) {
        for (const auto& s : attr->labelArray()) {
            d.longLabels.push_back(QString::fromStdString(s));
        }
        for (int32_t v : attr->valArray()) {
            d.longValues.push_back(v);
        }
    }

    // 文件未写 DEFAULT 时的兜底，保证面板 / 存盘键始终有效
    if (!d.defaultValue.isValid()) {
        switch (d.kind) {
        case ISFPortDesc::Kind::Float:
            d.defaultValue = 0.0;
            break;
        case ISFPortDesc::Kind::Bool:
        case ISFPortDesc::Kind::Event:
            d.defaultValue = false;
            break;
        case ISFPortDesc::Kind::Long:
            d.defaultValue = d.longValues.isEmpty() ? 0 : d.longValues.first();
            break;
        case ISFPortDesc::Kind::Point2D:
            d.defaultValue = QVariantList{0.0, 0.0};
            break;
        case ISFPortDesc::Kind::Color:
            d.defaultValue = QVariantList{1.0, 1.0, 1.0, 1.0};
            break;
        default:
            break;
        }
    }
    return d;
}

QVector<ISFPortDesc> portsFromDoc(const VVISF::ISFDocRef& doc)
{
    QVector<ISFPortDesc> out;
    if (!doc) {
        return out;
    }
    for (const auto& attr : doc->inputs()) {
        ISFPortDesc d = makePortDesc(attr);
        if (d.name.isEmpty() || d.kind == ISFPortDesc::Kind::Unsupported) {
            continue;
        }
        out.push_back(std::move(d));
    }
    return out;
}

// ---------------------------------------------------------------------------
// 纹理工具
// ---------------------------------------------------------------------------

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

/**
 * 上传一行 0–1 float 为 RGBA32F 纹理（高=1）。
 * 各通道同值，兼容 shader 读 .r / .a / 亮度。
 * 所有权在调用方：渲染结束后须 deleteRawTexture。
 */
NodeDataTypes::GpuTextureHandle uploadAudioRowTexture(QOpenGLFunctions* f,
                                                      const QVector<float>& row)
{
    NodeDataTypes::GpuTextureHandle handle;
    const int width = row.size();
    if (!f || width <= 0) {
        return handle;
    }

    QVector<float> rgba(width * 4);
    for (int i = 0; i < width; ++i) {
        const float v = std::max(0.0f, std::min(1.0f, row[i]));
        rgba[i * 4 + 0] = v;
        rgba[i * 4 + 1] = v;
        rgba[i * 4 + 2] = v;
        rgba[i * 4 + 3] = 1.0f;
    }

    unsigned int texId = 0;
    f->glGenTextures(1, &texId);
    f->glBindTexture(GL_TEXTURE_2D, texId);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, 1, 0, GL_RGBA, GL_FLOAT, rgba.constData());
    f->glBindTexture(GL_TEXTURE_2D, 0);

    handle.textureId = texId;
    handle.width = width;
    handle.height = 1;
    handle.internalFormat = GL_RGBA32F;
    return handle;
}

/**
 * 把工程纹理包装成 VVGL::GLBuffer，不转移所有权（deleter=null）。
 * @param flipped true：告诉 VVISF 采样时按顶原点解释（对齐工程 Mat）
 */
VVGL::GLBufferRef wrapExternalTexture(const NodeDataTypes::GpuTextureHandle& src, bool flipped)
{
    if (!src.valid()) {
        return nullptr;
    }
    const VVGL::Size size(static_cast<double>(src.width), static_cast<double>(src.height));
    const VVGL::Rect imgRect(0., 0., size.width, size.height);
    return VVGL::CreateFromExistingGLTexture(
        static_cast<int32_t>(src.textureId),
        VVGL::GLBuffer::Target_2D,
        VVGL::GLBuffer::IF_RGBA8,
        VVGL::GLBuffer::PF_BGRA,
        VVGL::GLBuffer::PT_UByte,
        size,
        flipped,
        imgRect,
        nullptr,
        nullptr);
}

/** 从 QVariantList 取第 i 个 double，越界返回 fallback */
double listAt(const QVariantList& list, int i, double fallback)
{
    return i < list.size() ? list.at(i).toDouble() : fallback;
}

/**
 * 面板 / VariableData 的 QVariant → VVISF::ISFVal
 * Color 兼容 0–1 与 0–255 两种写法。
 */
VVISF::ISFVal variantToISFVal(ISFPortDesc::Kind kind, const QVariant& v)
{
    using namespace VVISF;
    switch (kind) {
    case ISFPortDesc::Kind::Float:
        return ISFFloatVal(v.toDouble());
    case ISFPortDesc::Kind::Bool:
        return ISFBoolVal(v.toBool());
    case ISFPortDesc::Kind::Long:
        return ISFLongVal(static_cast<int32_t>(v.toInt()));
    case ISFPortDesc::Kind::Event:
        return ISFEventVal(v.toBool());
    case ISFPortDesc::Kind::Point2D: {
        const QVariantList list = v.toList();
        return ISFPoint2DVal(listAt(list, 0, 0.0), listAt(list, 1, 0.0));
    }
    case ISFPortDesc::Kind::Color: {
        if (v.typeId() == QMetaType::QColor) {
            const QColor c = v.value<QColor>();
            return ISFColorVal(c.redF(), c.greenF(), c.blueF(), c.alphaF());
        }
        const QVariantList list = v.toList();
        double r = listAt(list, 0, 0.0);
        double g = listAt(list, 1, 0.0);
        double b = listAt(list, 2, 0.0);
        double a = listAt(list, 3, 1.0);
        if (r > 1.0 || g > 1.0 || b > 1.0 || a > 1.0) {
            return ISFColorVal(r / 255.0, g / 255.0, b / 255.0, a > 1.0 ? a / 255.0 : a);
        }
        return ISFColorVal(r, g, b, a);
    }
    default:
        return ISFNullVal();
    }
}

/** getTypeString() 非 const，故参数不可为 const 引用 */
QString formatIsfErr(VVISF::ISFErr& e)
{
    QString msg = QString::fromStdString(e.getTypeString() + ": " + e.general + " / " + e.specific);
    for (const auto& kv : e.details) {
        msg += QStringLiteral("\n%1: %2")
                   .arg(QString::fromStdString(kv.first), QString::fromStdString(kv.second));
    }
    return msg;
}

} // namespace

// ---------------------------------------------------------------------------
// ISFRenderer
// ---------------------------------------------------------------------------

struct ISFRenderer::Impl
{
    VVGL::GLContextRef ctx;   ///< 弱包装 ImageGpuUpload 当前上下文
    VVISF::ISFSceneRef scene; ///< 当前 .fs 对应的场景
};

bool parseISFPortsFromFile(const QString& absoluteFsPath,
                           QVector<ISFPortDesc>& ports,
                           QString* error)
{
    ports.clear();
    const QString path = QFileInfo(absoluteFsPath).absoluteFilePath();
    if (path.isEmpty() || !QFileInfo::exists(path)) {
        if (error) {
            *error = QStringLiteral("ISF file not found");
        }
        return false;
    }
    try {
        // 第三参 true：只解析 JSON / 元数据，不编译 GL
        auto doc = VVISF::CreateISFDocRef(path.toStdString(), nullptr, true);
        ports = portsFromDoc(doc);
        return true;
    } catch (VVISF::ISFErr& e) {
        if (error) {
            *error = formatIsfErr(e);
        }
        return false;
    } catch (const std::exception& e) {
        if (error) {
            *error = QString::fromUtf8(e.what());
        }
        return false;
    }
}

ISFRenderer::ISFRenderer()
    : m_impl(std::make_unique<Impl>())
{
}

ISFRenderer::~ISFRenderer()
{
    // 析构也走 runGl，保证 GL 对象在正确上下文中释放
    NodeDataTypes::ImageGpuUpload::instance().runGl([this](QOpenGLFunctions* f) {
        destroy(f);
    });
}

void ISFRenderer::destroy(QOpenGLFunctions* f)
{
    for (auto& t : m_ownedAudioTextures) {
        deleteRawTexture(f, t);
    }
    m_ownedAudioTextures.clear();

    if (m_impl) {
        m_impl->scene.reset();
        // 不销毁全局 BufferPool，也不 delete 被包装的 QOpenGLContext
        m_impl->ctx.reset();
    }
    m_ready = false;
    m_glReady = false;
    m_loadedPath.clear();
    m_ports.clear();
}

bool ISFRenderer::bootstrapGl(QOpenGLFunctions* /*f*/)
{
    if (m_glReady && m_impl && m_impl->ctx) {
        return true;
    }

    QOpenGLContext* current = QOpenGLContext::currentContext();
    if (!current) {
        m_lastError = QStringLiteral("No current QOpenGLContext in runGl");
        return false;
    }

    try {
        m_impl->ctx = std::make_shared<VVGL::GLContext>(current);
        if (!VVGL::GetGlobalBufferPool()) {
            VVGL::CreateGlobalBufferPool(m_impl->ctx);
        }
        m_glReady = true;
        return true;
    } catch (const std::exception& e) {
        m_lastError = QStringLiteral("VVGL bootstrap failed: %1").arg(QString::fromUtf8(e.what()));
        m_impl->ctx.reset();
        m_glReady = false;
        return false;
    }
}

bool ISFRenderer::loadFile(const QString& absoluteFsPath)
{
    if (!m_impl || !m_impl->ctx) {
        m_lastError = QStringLiteral("GL not bootstrapped");
        return false;
    }

    try {
        m_impl->scene = VVISF::CreateISFSceneRefUsing(m_impl->ctx);
        m_impl->scene->setThrowExceptions(true);
        // useFile 第二/三参：编译 + 抛异常
        m_impl->scene->useFile(absoluteFsPath.toStdString(), true, true);
        refreshPortCache();
        m_loadedPath = absoluteFsPath;
        m_ready = true;
        m_lastError.clear();
        return true;
    } catch (VVISF::ISFErr& e) {
        m_lastError = formatIsfErr(e);
        m_impl->scene.reset();
        m_ready = false;
        return false;
    } catch (const std::exception& e) {
        m_lastError = QString::fromUtf8(e.what());
        m_impl->scene.reset();
        m_ready = false;
        return false;
    }
}

void ISFRenderer::refreshPortCache()
{
    m_ports.clear();
    if (!m_impl || !m_impl->scene) {
        return;
    }
    m_ports = portsFromDoc(m_impl->scene->doc());
}

bool ISFRenderer::ensureReady(QOpenGLFunctions* f, const QString& absoluteFsPath, bool forceReload)
{
    const QString path = QFileInfo(absoluteFsPath).absoluteFilePath();
    if (path.isEmpty()) {
        m_lastError = QStringLiteral("Empty ISF path");
        m_ready = false;
        return false;
    }
    if (!QFileInfo::exists(path)) {
        m_lastError = QStringLiteral("ISF file not found: %1").arg(path);
        m_ready = false;
        return false;
    }

    if (!bootstrapGl(f)) {
        return false;
    }

    // 路径相同且不强制时跳过重编译（热路径）
    if (!forceReload && m_ready && m_loadedPath == path && m_impl && m_impl->scene) {
        return true;
    }

    return loadFile(path);
}

NodeDataTypes::GpuTextureHandle ISFRenderer::render(
    QOpenGLFunctions* f,
    const QHash<QString, NodeDataTypes::GpuTextureHandle>& images,
    const QHash<QString, ISFAudioRow>& audioRows,
    const QVariantMap& scalarValues,
    double renderTime,
    int outW,
    int outH)
{
    NodeDataTypes::GpuTextureHandle out;
    if (!f || !m_ready || !m_impl || !m_impl->scene) {
        m_lastError = QStringLiteral("ISF renderer not ready");
        return out;
    }

    // —— 写标量 / 向量 / 颜色 ——
    for (const ISFPortDesc& port : m_ports) {
        if (!port.isVariable() || !scalarValues.contains(port.name)) {
            continue;
        }
        try {
            m_impl->scene->setValueForInputNamed(
                variantToISFVal(port.kind, scalarValues.value(port.name)),
                port.name.toStdString());
        } catch (...) {
            // 单个 ATTR 失败不中断整帧
        }
    }

    // —— 写图像（外部纹理，keepAlive 保活到本函数返回）——
    QVector<VVGL::GLBufferRef> keepAlive;
    keepAlive.reserve(images.size() + audioRows.size());
    for (const ISFPortDesc& port : m_ports) {
        if (!port.isImage()) {
            continue;
        }
        const auto it = images.constFind(port.name);
        if (it == images.constEnd() || !it->valid()) {
            continue;
        }
        VVGL::GLBufferRef buf = wrapExternalTexture(*it, /*flipped=*/true);
        if (!buf) {
            continue;
        }
        keepAlive.push_back(buf);
        try {
            auto attr = m_impl->scene->inputNamed(port.name.toStdString());
            // Filter 类 .fs 的主输入需走 setFilterInputBuffer
            if (attr && attr->isFilterInputImage()) {
                m_impl->scene->setFilterInputBuffer(buf);
            } else {
                m_impl->scene->setBufferForInputNamed(buf, port.name.toStdString());
            }
        } catch (...) {
        }
    }

    // —— 写音频 / FFT 行纹理 ——
    // 先释放上一帧纹理；本帧纹理挂到 m_ownedAudioTextures，留到下一帧再删
    // （ISFScene 可能仍持有上一帧的 GLBuffer 包装）
    for (auto& t : m_ownedAudioTextures) {
        deleteRawTexture(f, t);
    }
    m_ownedAudioTextures.clear();

    for (const ISFPortDesc& port : m_ports) {
        if (!port.isAudioPort()) {
            continue;
        }
        const auto it = audioRows.constFind(port.name);
        if (it == audioRows.constEnd() || it->values.isEmpty()) {
            continue;
        }
        NodeDataTypes::GpuTextureHandle tex = uploadAudioRowTexture(f, it->values);
        if (!tex.valid()) {
            continue;
        }
        m_ownedAudioTextures.push_back(tex);
        VVGL::GLBufferRef buf = VVGL::CreateFromExistingGLTexture(
            static_cast<int32_t>(tex.textureId),
            VVGL::GLBuffer::Target_2D,
            VVGL::GLBuffer::IF_RGBA32F,
            VVGL::GLBuffer::PF_RGBA,
            VVGL::GLBuffer::PT_Float,
            VVGL::Size(static_cast<double>(tex.width), 1.0),
            /*flipped=*/false,
            VVGL::Rect(0., 0., static_cast<double>(tex.width), 1.0),
            nullptr,
            nullptr);
        if (!buf) {
            continue;
        }
        keepAlive.push_back(buf);
        try {
            m_impl->scene->setBufferForAudioInputKey(buf, port.name.toStdString());
        } catch (...) {
        }
    }

    // —— 输出尺寸 ——
    if (outW <= 0 || outH <= 0) {
        for (const auto& tex : images) {
            if (tex.valid()) {
                outW = tex.width;
                outH = tex.height;
                break;
            }
        }
    }
    if (outW <= 0 || outH <= 0) {
        outW = 1280;
        outH = 720;
    }

    VVGL::GLBufferRef rendered;
    try {
        const VVGL::Size size(static_cast<double>(outW), static_cast<double>(outH));
        if (renderTime >= 0.0) {
            rendered = m_impl->scene->createAndRenderABuffer(size, renderTime, nullptr, nullptr);
        } else {
            rendered = m_impl->scene->createAndRenderABuffer(size, nullptr, nullptr);
        }
    } catch (VVISF::ISFErr& e) {
        m_lastError = formatIsfErr(e);
        return {};
    } catch (const std::exception& e) {
        m_lastError = QString::fromUtf8(e.what());
        return {};
    }

    if (!rendered || rendered->name == 0) {
        m_lastError = QStringLiteral("ISF render produced empty buffer");
        return {};
    }

    // VVGL pool 可能回收 rendered；先 blit 到工程自管纹理
    out = allocateRgbaTexture(f, outW, outH);
    if (!out.valid()) {
        m_lastError = QStringLiteral("Failed to allocate ISF output texture");
        return {};
    }

    QOpenGLContext* ctx = QOpenGLContext::currentContext();
    QOpenGLExtraFunctions* xf = ctx ? ctx->extraFunctions() : nullptr;
    if (!xf) {
        deleteRawTexture(f, out);
        m_lastError = QStringLiteral("OpenGL blit requires QOpenGLExtraFunctions");
        return {};
    }

    GLuint fboSrc = 0;
    GLuint fboDst = 0;
    xf->glGenFramebuffers(1, &fboSrc);
    xf->glGenFramebuffers(1, &fboDst);
    xf->glBindFramebuffer(GL_READ_FRAMEBUFFER, fboSrc);
    xf->glFramebufferTexture2D(
        GL_READ_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, rendered->name, 0);
    xf->glBindFramebuffer(GL_DRAW_FRAMEBUFFER, fboDst);
    xf->glFramebufferTexture2D(
        GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, out.textureId, 0);

    const GLenum srcStatus = xf->glCheckFramebufferStatus(GL_READ_FRAMEBUFFER);
    const GLenum dstStatus = xf->glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER);
    if (srcStatus != GL_FRAMEBUFFER_COMPLETE || dstStatus != GL_FRAMEBUFFER_COMPLETE) {
        xf->glBindFramebuffer(GL_FRAMEBUFFER, 0);
        xf->glDeleteFramebuffers(1, &fboSrc);
        xf->glDeleteFramebuffers(1, &fboDst);
        deleteRawTexture(f, out);
        m_lastError = QStringLiteral("ISF blit FBO incomplete (src=%1 dst=%2)")
                          .arg(static_cast<int>(srcStatus))
                          .arg(static_cast<int>(dstStatus));
        return {};
    }

    xf->glBlitFramebuffer(0, 0, outW, outH, 0, 0, outW, outH, GL_COLOR_BUFFER_BIT, GL_NEAREST);
    xf->glBindFramebuffer(GL_FRAMEBUFFER, 0);
    xf->glDeleteFramebuffers(1, &fboSrc);
    xf->glDeleteFramebuffers(1, &fboDst);

    m_lastError.clear();
    return out;
}

} // namespace Nodes
