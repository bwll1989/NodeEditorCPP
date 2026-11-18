#include "CefD3dTextureBridge.h"

#include "Common/DataTypes/ImageGpuUpload.h"

#include <QOpenGLFunctions>

#include <windows.h>
#include <d3d11.h>
#include <d3d11_1.h>

#include <cstring>
#include <string>

using namespace NodeDataTypes;

namespace Nodes {

namespace {

// ---- WGL_NV_DX_interop2 手动声明（SDK 无 wglext.h，规避额外依赖）----
using PFNWGLDXOPENDEVICENV_T = void*(WINAPI*)(void* dxDevice);
using PFNWGLDXCLOSEDEVICENV_T = BOOL(WINAPI*)(void* hDxInteropDevice);
using PFNWGLDXREGISTEROBJECTNV_T =
    HANDLE(WINAPI*)(void* hDxInteropDevice, void* dxObject, GLuint name, GLenum type, GLenum access);
using PFNWGLDXUNREGISTEROBJECTNV_T = BOOL(WINAPI*)(void* hDxInteropDevice, void* hObject);
using PFNWGLDXLOCKOBJECTSNV_T = BOOL(WINAPI*)(void* hDxInteropDevice, GLint count, void** hObjects);
using PFNWGLDXUNLOCKOBJECTSNV_T = BOOL(WINAPI*)(void* hDxInteropDevice, GLint count, void** hObjects);
using PFNWGLGETEXTENSIONSSTRINGARB_T = const char*(WINAPI*)(HDC hdc);

constexpr GLuint kWglAccessReadOnlyNv = 0x00000000;

/// 进程级探测缓存：-1 未探测，0 不支持，1 支持
std::atomic<int> s_probeResult{-1};

template <typename T>
T wglProc(const char* name) {
    return reinterpret_cast<T>(wglGetProcAddress(name));
}

}  // namespace

// =============================================================================
// Impl
// =============================================================================
struct CefD3dTextureBridge::Impl {
    ID3D11Device* d3dDevice = nullptr;
    ID3D11Device1* d3dDevice1 = nullptr;
    ID3D11DeviceContext* d3dContext = nullptr;
    void* dxInterop = nullptr;  ///< wglDXOpenDeviceNV 结果

    ID3D11Texture2D* copyTex = nullptr;  ///< CopyResource 目标（D3D 侧，经 interop 映射 GL）
    void* interopObject = nullptr;       ///< wglDXRegisterObjectNV 句柄
    GLuint interopGlTex = 0;             ///< copyTex 的 GL 名
    GLuint fbo = 0;                      ///< 绑定 interopGlTex，glCopyTexSubImage2D 源
    int texW = 0;
    int texH = 0;

    PFNWGLDXLOCKOBJECTSNV_T dxLock = nullptr;
    PFNWGLDXUNLOCKOBJECTSNV_T dxUnlock = nullptr;

    ~Impl() { /* 清理走 releaseGlResources/releaseD3dResources，由桥析构驱动 */ }

    void releaseGlResources(QOpenGLFunctions* f) {
        if (interopObject && dxInterop) {
            dxUnlock(dxInterop, 1, &interopObject);  // 幂等尝试，保证后续注销可进行
        }
        if (interopObject && dxInterop) {
            PFNWGLDXUNREGISTEROBJECTNV_T dxUnregister =
                reinterpret_cast<PFNWGLDXUNREGISTEROBJECTNV_T>(wglGetProcAddress("wglDXUnregisterObjectNV"));
            if (dxUnregister)
                dxUnregister(dxInterop, interopObject);
            interopObject = nullptr;
        }
        if (interopGlTex && f) {
            f->glDeleteTextures(1, &interopGlTex);
            interopGlTex = 0;
        }
        if (fbo && f) {
            f->glDeleteFramebuffers(1, &fbo);
            fbo = 0;
        }
    }

    void releaseD3dResources() {
        if (copyTex) {
            copyTex->Release();
            copyTex = nullptr;
        }
        if (dxInterop) {
            PFNWGLDXCLOSEDEVICENV_T dxClose =
                reinterpret_cast<PFNWGLDXCLOSEDEVICENV_T>(wglGetProcAddress("wglDXCloseDeviceNV"));
            if (dxClose)
                dxClose(dxInterop);
            dxInterop = nullptr;
        }
        if (d3dContext) {
            d3dContext->Release();
            d3dContext = nullptr;
        }
        if (d3dDevice1) {
            d3dDevice1->Release();
            d3dDevice1 = nullptr;
        }
        if (d3dDevice) {
            d3dDevice->Release();
            d3dDevice = nullptr;
        }
    }
};

CefD3dTextureBridge::~CefD3dTextureBridge() {
    if (!m_impl)
        return;
    // GUI 线程（节点 stop/dtor 路径）：interop 注销须在 GL 上下文内
    Impl* impl = m_impl;
    ImageGpuUpload::instance().runGl([impl](QOpenGLFunctions* f) {
        impl->releaseGlResources(f);
    });
    impl->releaseD3dResources();
    delete impl;
    m_impl = nullptr;
}

bool CefD3dTextureBridge::isSupported() {
    int cached = s_probeResult.load(std::memory_order_relaxed);
    if (cached >= 0)
        return cached == 1;

    // ① D3D11 设备 + BGRA 支持可创建
    ID3D11Device* dev = nullptr;
    ID3D11DeviceContext* ctx = nullptr;
    D3D_FEATURE_LEVEL fl = D3D_FEATURE_LEVEL_11_0;
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                   D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0,
                                   D3D11_SDK_VERSION, &dev, &fl, &ctx);
    if (SUCCEEDED(hr)) {
        dev->Release();
        ctx->Release();
    }

    // ② 离屏 GL 上下文（须 desktop GL）暴露 WGL_NV_DX_interop2
    bool hasExt = false;
    if (SUCCEEDED(hr)) {
        hasExt = ImageGpuUpload::instance().runGl([](QOpenGLFunctions*) {
            using GetExtT = PFNWGLGETEXTENSIONSSTRINGARB_T;
            auto getExt = reinterpret_cast<GetExtT>(wglGetProcAddress("wglGetExtensionsStringARB"));
            if (!getExt)
                return false;
            HDC hdc = wglGetCurrentDC();
            const char* ext = hdc ? getExt(hdc) : nullptr;
            return ext && std::strstr(ext, "WGL_NV_DX_interop2") != nullptr;
        });
    }

    s_probeResult.store(hasExt ? 1 : 0, std::memory_order_relaxed);
    return hasExt;
}

bool CefD3dTextureBridge::init() {
    if (m_impl)
        return true;

    m_impl = new Impl();

    // D3D11 设备（BGRA_SUPPORT）
    D3D_FEATURE_LEVEL fl = D3D_FEATURE_LEVEL_11_0;
    HRESULT hr = D3D11CreateDevice(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr,
                                   D3D11_CREATE_DEVICE_BGRA_SUPPORT, nullptr, 0,
                                   D3D11_SDK_VERSION, &m_impl->d3dDevice, &fl,
                                   &m_impl->d3dContext);
    if (FAILED(hr)) {
        m_impl->releaseD3dResources();
        delete m_impl;
        m_impl = nullptr;
        return false;
    }
    if (FAILED(m_impl->d3dDevice->QueryInterface(IID_ID3D11Device1,
                                                 reinterpret_cast<void**>(&m_impl->d3dDevice1)))) {
        m_impl->releaseD3dResources();
        delete m_impl;
        m_impl = nullptr;
        return false;
    }

    bool ok = ImageGpuUpload::instance().runGl([this](QOpenGLFunctions* f) {
        auto dxOpen = reinterpret_cast<PFNWGLDXOPENDEVICENV_T>(wglGetProcAddress("wglDXOpenDeviceNV"));
        auto dxRegister =
            reinterpret_cast<PFNWGLDXREGISTEROBJECTNV_T>(wglGetProcAddress("wglDXRegisterObjectNV"));
        auto dxLock = reinterpret_cast<PFNWGLDXLOCKOBJECTSNV_T>(wglGetProcAddress("wglDXLockObjectsNV"));
        auto dxUnlock =
            reinterpret_cast<PFNWGLDXUNLOCKOBJECTSNV_T>(wglGetProcAddress("wglDXUnlockObjectsNV"));
        if (!dxOpen || !dxRegister || !dxLock || !dxUnlock)
            return false;
        m_impl->dxLock = dxLock;
        m_impl->dxUnlock = dxUnlock;

        m_impl->dxInterop = dxOpen(m_impl->d3dDevice);
        if (!m_impl->dxInterop)
            return false;

        // interop GL 纹理：先建占位 storage + FBO（验证 GL 环境）。
        // 此时 copyTex 尚未创建，不可注册——真正注册在 pushFrame 的
        // 尺寸重建分支（有 D3D 对象且带真实尺寸）。
        f->glGenTextures(1, &m_impl->interopGlTex);
        f->glBindTexture(GL_TEXTURE_2D, m_impl->interopGlTex);
        f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 4, 4, 0, GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        f->glBindTexture(GL_TEXTURE_2D, 0);

        // FBO：占位验证（首个有效帧注册后重挂真实纹理）
        f->glGenFramebuffers(1, &m_impl->fbo);
        f->glBindFramebuffer(GL_FRAMEBUFFER, m_impl->fbo);
        f->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                  m_impl->interopGlTex, 0);
        const GLenum status = f->glCheckFramebufferStatus(GL_FRAMEBUFFER);
        f->glBindFramebuffer(GL_FRAMEBUFFER, 0);
        return status == GL_FRAMEBUFFER_COMPLETE;
    });

    if (!ok) {
        ImageGpuUpload::instance().runGl([this](QOpenGLFunctions* f) {
            m_impl->releaseGlResources(f);
        });
        m_impl->releaseD3dResources();
        delete m_impl;
        m_impl = nullptr;
        return false;
    }
    return true;
}

GpuTextureHandle CefD3dTextureBridge::pushFrame(const cefbox::SharedFrameInfo& info) {
    if (!m_impl || !info.sharedHandle || info.codedWidth < 1 || info.codedHeight < 1)
        return {};

    return ImageGpuUpload::instance().runGl([&](QOpenGLFunctions* f) -> GpuTextureHandle {
        // ① 打开 CEF 共享纹理（逐帧重开，句柄回调后失效）
        ID3D11Texture2D* srcTex = nullptr;
        HRESULT hr = m_impl->d3dDevice1->OpenSharedResource1(info.sharedHandle,
                                                             IID_ID3D11Texture2D,
                                                             reinterpret_cast<void**>(&srcTex));
        if (FAILED(hr) || !srcTex)
            return {};

        // ② 拷贝目标尺寸对齐（首帧 / 尺寸变化时重建 interop 纹理）
        D3D11_TEXTURE2D_DESC desc;
        srcTex->GetDesc(&desc);
        if (!m_impl->copyTex || (int)desc.Width != m_impl->texW || (int)desc.Height != m_impl->texH) {
            auto dxRegister =
                reinterpret_cast<PFNWGLDXREGISTEROBJECTNV_T>(wglGetProcAddress("wglDXRegisterObjectNV"));
            auto dxUnregister =
                reinterpret_cast<PFNWGLDXUNREGISTEROBJECTNV_T>(wglGetProcAddress("wglDXUnregisterObjectNV"));
            if (!dxRegister || !dxUnregister) {
                srcTex->Release();
                return {};
            }
            if (m_impl->interopObject)
                dxUnregister(m_impl->dxInterop, m_impl->interopObject);
            if (m_impl->copyTex)
                m_impl->copyTex->Release();

            D3D11_TEXTURE2D_DESC cdesc = desc;
            cdesc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
            cdesc.MiscFlags = 0;  // 拷贝目标无需再共享
            if (FAILED(m_impl->d3dDevice->CreateTexture2D(&cdesc, nullptr, &m_impl->copyTex))) {
                m_impl->copyTex = nullptr;
                m_impl->interopObject = nullptr;
                srcTex->Release();
                return {};
            }
            f->glBindTexture(GL_TEXTURE_2D, m_impl->interopGlTex);
            f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, (int)desc.Width, (int)desc.Height, 0,
                            GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
            f->glBindTexture(GL_TEXTURE_2D, 0);
            // wglDXRegisterObjectNV 返回值 = 注册句柄（Lock/Unlock/Unregister 都用它）
            void* reg = dxRegister(m_impl->dxInterop, m_impl->copyTex, m_impl->interopGlTex,
                                   GL_TEXTURE_2D, kWglAccessReadOnlyNv);
            if (!reg) {
                m_impl->copyTex->Release();
                m_impl->copyTex = nullptr;
                m_impl->interopObject = nullptr;
                srcTex->Release();
                return {};
            }
            m_impl->interopObject = reg;
            m_impl->texW = (int)desc.Width;
            m_impl->texH = (int)desc.Height;
            f->glBindFramebuffer(GL_FRAMEBUFFER, m_impl->fbo);
            f->glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                                      m_impl->interopGlTex, 0);
            f->glBindFramebuffer(GL_FRAMEBUFFER, 0);
        }

        // ③ D3D 侧拷贝（interop 对象当前未被 GL 锁定 → D3D 可写）
        m_impl->d3dContext->CopyResource(m_impl->copyTex, srcTex);
        srcTex->Release();

        // ④ GL 侧：lock → FBO 拷入普通纹理 → unlock
        if (!m_impl->dxLock(m_impl->dxInterop, 1, &m_impl->interopObject))
            return {};

        GpuTextureHandle out;
        f->glGenTextures(1, &out.textureId);
        if (out.textureId) {
            f->glBindTexture(GL_TEXTURE_2D, out.textureId);
            f->glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, m_impl->texW, m_impl->texH, 0,
                            GL_BGRA, GL_UNSIGNED_BYTE, nullptr);
            f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            f->glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            f->glBindFramebuffer(GL_FRAMEBUFFER, m_impl->fbo);
            f->glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, 0, 0, m_impl->texW, m_impl->texH);
            f->glBindFramebuffer(GL_FRAMEBUFFER, 0);
            f->glBindTexture(GL_TEXTURE_2D, 0);
            out.width = m_impl->texW;
            out.height = m_impl->texH;
            out.internalFormat = 0x8058;  // GL_RGBA8
            out.lifetime = makeTextureLifetime(out.textureId).lifetime;
        }
        m_impl->dxUnlock(m_impl->dxInterop, 1, &m_impl->interopObject);
        return out;
    });
}

} // namespace Nodes
