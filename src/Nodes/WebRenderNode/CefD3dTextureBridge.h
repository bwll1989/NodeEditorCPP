#pragma once

/**
 * @file CefD3dTextureBridge.h
 * @brief CEF 共享纹理（OnAcceleratedPaint）→ GL 纹理桥 — P2
 *
 * 链路（全部 GUI 线程，GL 侧经 ImageGpuUpload::runGl 持离屏共享上下文）：
 *   CEF NT handle → ID3D11Device1::OpenSharedResource1 → CopyResource 到自有
 *   D3D 纹理 → wglDX interop Lock → FBO 拷入普通 GL 纹理 → makeTextureLifetime
 *   → ImageFrame::fromTexture。
 *
 * 设计要点：
 *   - CEF 156 共享纹理无 keyed mutex；句柄仅回调内有效，须逐帧重开+拷贝。
 *   - 下游拿到的是普通 GL 纹理（拷贝产物），与 interop 对象解耦——consumer
 *     在共享组任意上下文 bind 均合法，无 interop 所有权纠纷。
 *   - 探测失败（无 D3D11 / 无 WGL_NV_DX_interop2 / ANGLE 上下文）自动降级
 *     CPU OnPaint 路径（节点侧负责）。
 *   - 行序与 CPU 路径一致：D3D 布局 == OnPaint 位图布局（首行=顶），不做翻转。
 */

#include "Common/CefBox/CefOsrBrowser.h"
#include "Common/DataTypes/GpuTextureHandle.h"

namespace Nodes {

class CefD3dTextureBridge
{
public:
    CefD3dTextureBridge() = default;
    ~CefD3dTextureBridge();

    CefD3dTextureBridge(const CefD3dTextureBridge&) = delete;
    CefD3dTextureBridge& operator=(const CefD3dTextureBridge&) = delete;

    /// 进程级探测（结果缓存）：D3D11 设备 + WGL_NV_DX_interop2 扩展可用性。
    /// 须在 ImageGpuUpload::warmup() 之后（需要离屏 GL 上下文）。
    static bool isSupported();

    /// 创建 D3D 设备 / interop 设备 / interop 纹理 / FBO。须先 isSupported()。
    bool init();

    /// 单帧：open shared → D3D copy → interop lock → GL 拷贝 → unlock。
    /// 失败返回 invalid handle（调用方应降级 CPU 路径）。
    NodeDataTypes::GpuTextureHandle pushFrame(const cefbox::SharedFrameInfo& info);

private:
    struct Impl;
    /// 注意：用原始指针而非 unique_ptr——本类被 header-only 的
    /// WebRenderDataModel 持有，其 inline 析构无法见到 Impl 完整定义。
    Impl* m_impl = nullptr;
};

} // namespace Nodes
