/**
 * @file ViosoWarper.cpp
 * @brief ViosoWarper 实现：VIOSO OpenGL 路径离屏 Warp+Blend
 */

#include "ViosoWarper.hpp"

#include "ImageGpuUpload.h"

#include <QByteArray>
#include <QFileInfo>
#include <QOpenGLFunctions>

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>

#include "VIOSOWarpBlend.h"

namespace Nodes
{
namespace
{

/** @brief 转为 VIOSO C API 可用的本地编码绝对路径 */
QByteArray toApiPath(const QString& path)
{
    return QFileInfo(path).absoluteFilePath().toLocal8Bit();
}

/**
 * @brief 分配空 RGBA8 2D 纹理（暂不挂 RAII lifetime）
 * @note 须在已 makeCurrent 的共享上下文中调用。
 *       成功返回前再 `adoptTextureLifetime`；失败路径用 `deleteRawTexture`，
 *       切勿在 runGl 内让带 makeTextureLifetime 的句柄析构（会 destroyTexture 再抢锁）。
 */
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
    // 与工程内其它 GPU pass 一致：内部 RGBA8，外部格式用 GL_BGRA
    f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
    f->glBindTexture(GL_TEXTURE_2D, 0);

    handle.textureId = texId;
    handle.width = width;
    handle.height = height;
    handle.internalFormat = GL_RGBA8;
    return handle;
}

/** @brief 当前上下文中删除尚未挂 lifetime 的纹理 */
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

/** @brief 成功离开 runGl 前挂上 RAII，供外侧正常析构 */
void adoptTextureLifetime(NodeDataTypes::GpuTextureHandle& handle)
{
    if (!handle.valid() || handle.lifetime) {
        return;
    }
    handle.lifetime = NodeDataTypes::makeTextureLifetime(handle.textureId).lifetime;
}

} // namespace

ViosoWarper::~ViosoWarper()
{
    if (!m_warper) {
        return;
    }
    // 尽量在共享上下文中 Destroy（OpenGL warper 可能持有 GPU 资源）
    NodeDataTypes::ImageGpuUpload::instance().runGl([this](QOpenGLFunctions* f) {
        destroy(f);
    });
}

void ViosoWarper::destroy(QOpenGLFunctions* /*f*/)
{
    if (m_warper) {
        VWB_Destroy(static_cast<VWB_Warper*>(m_warper));
        m_warper = nullptr;
    }
    m_ready = false;
    m_loadedPath.clear();
    m_loadedIndex = -1;
    m_loadedCryptoKey.clear();
    m_outWidth = 0;
    m_outHeight = 0;
}

bool ViosoWarper::parseCryptoKey(const QString& text, QByteArray& outKey16, QString* error)
{
    outKey16.clear();
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return true; // 关闭加密
    }

    // 去掉常见分隔符后若恰好 32 位十六进制 → 按 hex 解析为 16 字节
    QString hex = trimmed;
    hex.remove(QLatin1Char(' '));
    hex.remove(QLatin1Char('-'));
    hex.remove(QLatin1Char(':'));

    if (hex.size() == 32) {
        bool allHex = true;
        for (const QChar c : hex) {
            const ushort u = c.toLower().unicode();
            if (!((u >= '0' && u <= '9') || (u >= 'a' && u <= 'f'))) {
                allHex = false;
                break;
            }
        }
        if (allHex) {
            outKey16 = QByteArray::fromHex(hex.toLatin1());
            if (outKey16.size() != 16) {
                if (error) {
                    *error = QStringLiteral("failed to decode Passkey hex");
                }
                return false;
            }
            return true;
        }
    }

    // 明文：UTF-8 截断/零填充到 16 字节
    const QByteArray utf8 = trimmed.toUtf8();
    outKey16 = QByteArray(16, '\0');
    std::memcpy(outKey16.data(), utf8.constData(),
                static_cast<size_t>(qMin(16, utf8.size())));
    return true;
}

bool ViosoWarper::applyProcessCryptoKey(const QByteArray& cryptoKey16, QString* error)
{
    // VWB_setCryptoKey 只保存指针，必须用进程内稳定缓冲
    static std::array<uint8_t, 16> s_keyBytes{};

    if (cryptoKey16.isEmpty()) {
        const VWB_ERROR err = VWB_setCryptoKey(nullptr);
        if (err != VWB_ERROR_NONE) {
            if (error) {
                *error = QStringLiteral("VWB_setCryptoKey(nullptr) failed (%1)").arg(static_cast<int>(err));
            }
            return false;
        }
        return true;
    }

    if (cryptoKey16.size() != 16) {
        if (error) {
            *error = QStringLiteral("crypto key must be 16 bytes, got %1").arg(cryptoKey16.size());
        }
        return false;
    }

    std::memcpy(s_keyBytes.data(), cryptoKey16.constData(), 16);
    const VWB_ERROR err = VWB_setCryptoKey(s_keyBytes.data());
    if (err != VWB_ERROR_NONE) {
        if (error) {
            *error = QStringLiteral("VWB_setCryptoKey failed (%1)").arg(static_cast<int>(err));
        }
        return false;
    }
    return true;
}

bool ViosoWarper::queryChannelCount(const QString& vwfPath, int& channelCount, QString* error)
{
    channelCount = 0;
    const QByteArray pathBytes = toApiPath(vwfPath);
    if (pathBytes.isEmpty() || !QFileInfo::exists(QString::fromLocal8Bit(pathBytes))) {
        if (error) {
            *error = QStringLiteral("vwf file not found: %1").arg(vwfPath);
        }
        return false;
    }

    VWB_uint count = 0;
    const VWB_ERROR err = VWB_vwfInfoC(pathBytes.constData(), nullptr, &count);
    if (err != VWB_ERROR_NONE && err != VWB_ERROR_FALSE) {
        if (error) {
            *error = QStringLiteral("VWB_vwfInfoC failed (%1)").arg(static_cast<int>(err));
        }
        return false;
    }
    if (count == 0) {
        if (error) {
            *error = QStringLiteral("vwf contains no projector mappings");
        }
        return false;
    }

    channelCount = static_cast<int>(count);
    return true;
}

bool ViosoWarper::queryChannelSize(const QString& vwfPath,
                                   int projectorIndex,
                                   int& outW,
                                   int& outH,
                                   QString* error)
{
    outW = 0;
    outH = 0;
    const QByteArray pathBytes = toApiPath(vwfPath);

    VWB_uint count = 0;
    VWB_ERROR err = VWB_vwfInfoC(pathBytes.constData(), nullptr, &count);
    if (err != VWB_ERROR_NONE && err != VWB_ERROR_FALSE) {
        if (error) {
            *error = QStringLiteral("VWB_vwfInfoC failed (%1)").arg(static_cast<int>(err));
        }
        return false;
    }
    if (count == 0) {
        if (error) {
            *error = QStringLiteral("vwf contains no projector mappings");
        }
        return false;
    }

    std::vector<VWB_WarpBlendHeader> headers(count);
    err = VWB_vwfInfoC(pathBytes.constData(), headers.data(), &count);
    if (err != VWB_ERROR_NONE) {
        if (error) {
            *error = QStringLiteral("VWB_vwfInfoC read headers failed (%1)").arg(static_cast<int>(err));
        }
        return false;
    }

    const int index = projectorIndex < 0 ? 0 : projectorIndex;
    if (static_cast<VWB_uint>(index) >= count) {
        if (error) {
            *error = QStringLiteral("projectorIndex %1 out of range (count=%2)")
                         .arg(index)
                         .arg(count);
        }
        return false;
    }

    outW = headers[static_cast<size_t>(index)].header.width;
    outH = headers[static_cast<size_t>(index)].header.height;
    if (outW <= 0 || outH <= 0) {
        if (error) {
            *error = QStringLiteral("invalid warp map size in vwf header");
        }
        return false;
    }
    return true;
}

bool ViosoWarper::queryOutputSize(const QString& vwfPath, int projectorIndex, int& outW, int& outH)
{
    QString error;
    if (!queryChannelSize(vwfPath, projectorIndex, outW, outH, &error)) {
        m_lastError = error;
        return false;
    }
    return true;
}

bool ViosoWarper::createWarper(QOpenGLFunctions* /*f*/,
                               const QString& vwfPath,
                               int projectorIndex,
                               const QByteArray& cryptoKey16)
{
    m_lastError.clear();

    int outW = 0;
    int outH = 0;
    if (!queryOutputSize(vwfPath, projectorIndex, outW, outH)) {
        return false;
    }

    QString keyError;
    if (!applyProcessCryptoKey(cryptoKey16, &keyError)) {
        m_lastError = keyError;
        return false;
    }

    VWB_Warper* warper = nullptr;
    // OpenGL：pDxDevice = nullptr；不依赖 ini，创建后再改 warper 字段
    const VWB_ERROR createErr = VWB_CreateA(nullptr, nullptr, "channel 1", &warper, 1, nullptr);
    if (createErr != VWB_ERROR_NONE || !warper) {
        m_lastError = QStringLiteral("VWB_CreateA failed (%1)").arg(static_cast<int>(createErr));
        return false;
    }

    const QByteArray pathBytes = toApiPath(vwfPath);
    std::memset(warper->calibFile, 0, sizeof(warper->calibFile));
    const int copyLen = qMin(pathBytes.size(), static_cast<int>(sizeof(warper->calibFile) - 1));
    if (copyLen > 0) {
        std::memcpy(warper->calibFile, pathBytes.constData(), static_cast<size_t>(copyLen));
    }
    warper->calibIndex = projectorIndex < 0 ? 0 : projectorIndex;
    // 2D 静态图 / 视频墙纸路径：不需要动态视锥与 AutoView
    warper->bAutoView = false;
    warper->bTurnWithView = false;
    warper->bUseGL110 = false; // 使用现代 GLSL 路径

    // Init 会把 .vwf 上传到 GPU；要求当前线程 GL 上下文已 current
    const VWB_ERROR initErr = VWB_Init(warper);
    if (initErr != VWB_ERROR_NONE) {
        m_lastError = QStringLiteral("VWB_Init failed (%1)").arg(static_cast<int>(initErr));
        VWB_Destroy(warper);
        return false;
    }

    m_warper = warper;
    m_outWidth = outW;
    m_outHeight = outH;
    m_loadedPath = QFileInfo(vwfPath).absoluteFilePath();
    m_loadedIndex = projectorIndex < 0 ? 0 : projectorIndex;
    m_loadedCryptoKey = cryptoKey16;
    m_ready = true;
    return true;
}

bool ViosoWarper::ensureReady(QOpenGLFunctions* f,
                              const QString& vwfPath,
                              int projectorIndex,
                              const QByteArray& cryptoKey16)
{
    const QString absPath = QFileInfo(vwfPath).absoluteFilePath();
    const int index = projectorIndex < 0 ? 0 : projectorIndex;

    if (m_ready && m_warper && m_loadedPath == absPath && m_loadedIndex == index
        && m_loadedCryptoKey == cryptoKey16) {
        return true;
    }

    destroy(f);

    if (absPath.isEmpty() || !QFileInfo::exists(absPath)) {
        m_lastError = QStringLiteral("vwf file not found: %1").arg(vwfPath);
        return false;
    }

    if (!cryptoKey16.isEmpty() && cryptoKey16.size() != 16) {
        m_lastError = QStringLiteral("crypto key must be 16 bytes, got %1").arg(cryptoKey16.size());
        return false;
    }

    return createWarper(f, absPath, index, cryptoKey16);
}

NodeDataTypes::GpuTextureHandle ViosoWarper::render(QOpenGLFunctions* f,
                                                    const NodeDataTypes::GpuTextureHandle& src)
{
    if (!f || !m_ready || !m_warper || !src.valid()) {
        return {};
    }

    auto* warper = static_cast<VWB_Warper*>(m_warper);

    // 官方示例要求每帧（或渲染前）调用 getViewProj 更新内部数据；
    // 2D 内容路径眼点/朝向置零即可。
    VWB_float eye[3] = {0.f, 0.f, 0.f};
    VWB_float rot[3] = {0.f, 0.f, 0.f};
    VWB_float view[16] = {};
    VWB_float proj[16] = {};
    VWB_getViewProj(warper, eye, rot, view, proj);

    NodeDataTypes::GpuTextureHandle out = allocateRgbaTexture(f, m_outWidth, m_outHeight);
    if (!out.valid()) {
        m_lastError = QStringLiteral("failed to allocate output texture");
        return {};
    }

    GLint prevFbo = 0;
    f->glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prevFbo);

    unsigned int fbo = 0;
    f->glGenFramebuffers(1, &fbo);
    f->glBindFramebuffer(GL_FRAMEBUFFER, fbo);
    f->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, out.textureId, 0);

    const GLenum status = f->glCheckFramebufferStatus(GL_FRAMEBUFFER);
    if (status != GL_FRAMEBUFFER_COMPLETE) {
        m_lastError = QStringLiteral("incomplete FBO (%1)").arg(static_cast<int>(status));
        f->glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFbo));
        f->glDeleteFramebuffers(1, &fbo);
        deleteRawTexture(f, out);
        return {};
    }

    // 必须先设好输出 viewport；不要交给 VWB_STATEMASK_VIEWPORT
    f->glViewport(0, 0, m_outWidth, m_outHeight);
    f->glDisable(GL_DEPTH_TEST);
    f->glDisable(GL_BLEND);

    // 切勿加 VWB_STATEMASK_VIEWPORT：
    // GLWarpBlend::Render 会把 viewport 改成「输入纹理」宽高，全屏四边形只画在 FBO 一角。
    const VWB_uint stateMask = static_cast<VWB_uint>(
        VWB_STATEMASK_PIXEL_SHADER | VWB_STATEMASK_SHADER_RESOURCE
        | VWB_STATEMASK_CLEARBACKBUFFER);

    // OpenGL：src 为 GLint 纹理 ID
    const VWB_ERROR renderErr = VWB_render(
        warper,
        reinterpret_cast<VWB_param>(static_cast<intptr_t>(src.textureId)),
        stateMask);

    f->glBindFramebuffer(GL_FRAMEBUFFER, static_cast<GLuint>(prevFbo));
    f->glDeleteFramebuffers(1, &fbo);
    // VIOSO 可能留下 program / 纹理绑定，恢复干净状态供后续 pass 使用
    f->glUseProgram(0);
    f->glBindTexture(GL_TEXTURE_2D, 0);

    if (renderErr != VWB_ERROR_NONE) {
        m_lastError = QStringLiteral("VWB_render failed (%1)").arg(static_cast<int>(renderErr));
        deleteRawTexture(f, out);
        return {};
    }

    m_lastError.clear();
    // 成功后再挂 RAII；V 轴翻转仍在 runGl 外侧做
    adoptTextureLifetime(out);
    return out;
}

} // namespace Nodes
