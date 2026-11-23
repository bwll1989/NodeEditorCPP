// ============================================================================
//  DxTextureReadback —— DX11 纹理 → CPU RGBA 帧读回工具（Ingest 层共享）
//
//  用户定案（2026-11-08 注册表化重构）：SpoutIngest / WgcIngest 共享同一
//  "GPU 纹理 → LiveFrame"读回路径——本类把"staging 纹理生命周期 + 尺寸
//  变化重建 + BGRA/R8G8B8A8 换序分派"收敛为一处。
//
//  用法（接收线程内）：
//      DxTextureReadback rb;                 // 线程私有（无内部锁）
//      auto frame = rb.read(context, src_tex); // 失败/无变化返回 nullptr
//
//  线程契约：单线程使用（staging 生命周期挂本对象）；read() 在设备
//  纹理尺寸变化时自动重建 staging。
// ============================================================================

#pragma once

#include <cstdint>

#include <d3d11.h>
#include <tmmintrin.h> // SSSE3 pshufb（BGRA→RGBA 批量换序）

#include "clock/ClockProvider.hpp"
#include "live/LiveFrame.hpp"
#include "live/LiveFramePool.hpp"

namespace ms {

class DxTextureReadback {
public:
    DxTextureReadback()              = default;
    ~DxTextureReadback() { release(); }

    DxTextureReadback(const DxTextureReadback&)            = delete;
    DxTextureReadback& operator=(const DxTextureReadback&) = delete;

    /// 源纹理 → RGBA LiveFrame（新对象；读回失败返回 nullptr）。
    /// BGRA → swap 换序；R8G8B8A8 → 直拷；其余格式告警丢弃。
    LiveFramePtr read(ID3D11DeviceContext* context, ID3D11Texture2D* src) {
        if (context == nullptr || src == nullptr) return nullptr;

        D3D11_TEXTURE2D_DESC td{};
        src->GetDesc(&td);

        // ---- staging 尺寸/格式变化 → 重建 ----
        if (staging_ != nullptr &&
            (td.Width != w_ || td.Height != h_ || td.Format != fmt_)) {
            release();
        }
        if (staging_ == nullptr) {
            D3D11_TEXTURE2D_DESC sd = td;
            sd.Usage          = D3D11_USAGE_STAGING;
            sd.BindFlags      = 0;
            sd.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
            sd.MiscFlags      = 0;
            // 【2026-11-10 生命周期修正】旧版 device_of 内部 Release 后返回
            // 指针再 CreateTexture2D = "Release 后使用"UB（WGC 场景 recvLoop
            // 局部 ComPtr 恰好持设备侥幸不炸；语义上引用计数已归还，开
            // D3D 调试层/换驱动立即翻车）。改为调用方用毕立即 Release。
            ID3D11Device* dev = device_of(context); // GetDevice 内 AddRef
            const HRESULT hr =
                dev != nullptr
                    ? dev->CreateTexture2D(&sd, nullptr, &staging_)
                    : E_FAIL;
            if (dev != nullptr) dev->Release(); // 用毕归还（staging_ 已持引用）
            if (FAILED(hr)) return nullptr;
            w_   = td.Width;
            h_   = td.Height;
            fmt_ = td.Format;
        }

        context->CopyResource(staging_, src);

        D3D11_MAPPED_SUBRESOURCE mapped{};
        if (FAILED(context->Map(staging_, 0, D3D11_MAP_READ, 0, &mapped)))
            return nullptr;

        // ---- 换序拷贝（Spout 默认 BGRA；WGC 固定 BGRA；RGBA 直拷）----
        const bool swap_rb = td.Format == DXGI_FORMAT_B8G8R8A8_UNORM;
        if (!swap_rb && td.Format != DXGI_FORMAT_R8G8B8A8_UNORM) {
            context->Unmap(staging_, 0);
            return nullptr; // 未知格式（10/16bit 源）——调用方按无帧处理
        }
        // 池化帧（rgba 缓冲复用——消逐帧 8MB 分配+缺页；LiveFramePool.hpp）
        auto frame = pool_->acquire();
        frame->w = static_cast<int>(td.Width);
        frame->h = static_cast<int>(td.Height);
        frame->rgba.resize(static_cast<std::size_t>(td.Width) * td.Height * 4);
        // 帧同步戳：GPU 读回（内容已就绪）的接收时刻，Spout/WGC 共用。
        frame->pts_us = processAlignedNowUs();
        const auto* src_px = static_cast<const std::uint8_t*>(mapped.pData);
        auto* dst = frame->rgba.data();
        const std::uint32_t dst_stride =
            static_cast<std::uint32_t>(td.Width) * 4;
        // 换序拷贝：pshufb 批量（4 像素/拍，原逐像素标量是 WGC/Spout
        // 路径同款热点）；alpha 语义保留（0→FF，非 0 原样）。
        // w%4!=0 的非常规宽 → 整行标量回退（防御，视频分辨率恒 4 对齐）。
        const bool simd = (td.Width % 4) == 0;
        const __m128i shuf  = _mm_setr_epi8(2, 1, 0, 3, 6, 5, 4, 7,
                                            10, 9, 8, 11, 14, 13, 12, 15);
        const __m128i amask = _mm_set1_epi32(static_cast<int>(0xFF000000u));
        const __m128i zero  = _mm_setzero_si128();
        for (UINT y = 0; y < td.Height; ++y) {
            const std::uint8_t* s =
                src_px + static_cast<std::size_t>(y) * mapped.RowPitch;
            std::uint8_t* d = dst + static_cast<std::size_t>(y) * dst_stride;
            if (simd) {
                const auto*  sv = reinterpret_cast<const __m128i*>(s);
                auto*        dv = reinterpret_cast<__m128i*>(d);
                for (UINT i = 0; i < td.Width; i += 4, ++sv, ++dv) {
                    __m128i v = _mm_loadu_si128(sv);
                    if (swap_rb) v = _mm_shuffle_epi8(v, shuf);
                    // alpha==0 → 置 FF（原逐像素语义：s[3] ? s[3] : 0xFF）
                    const __m128i a =
                        _mm_srli_epi32(v, 24); // alpha 移到每通道低 8 位
                    const __m128i fix = _mm_and_si128(
                        _mm_cmpeq_epi32(a, zero), amask);
                    _mm_storeu_si128(dv, _mm_or_si128(v, fix));
                }
            } else {
                for (UINT x = 0; x < td.Width; ++x) {
                    d[x * 4 + 0] = swap_rb ? s[x * 4 + 2] : s[x * 4 + 0]; // R
                    d[x * 4 + 1] = s[x * 4 + 1];                          // G
                    d[x * 4 + 2] = swap_rb ? s[x * 4 + 0] : s[x * 4 + 2]; // B
                    d[x * 4 + 3] = s[x * 4 + 3] ? s[x * 4 + 3] : 0xFF;    // A
                }
            }
        }
        context->Unmap(staging_, 0);
        return LiveFramePtr(std::move(frame));
    }

    /// 释放 staging（重连/析构）
    void release() {
        if (staging_ != nullptr) {
            staging_->Release();
            staging_ = nullptr;
        }
        w_ = h_ = 0;
    }

private:
    /// 从 immediate context 反查设备（CreateTexture2D 需要）
    /// 契约：返回指针带一次引用（GetDevice AddRef），调用方用毕必须 Release
    static ID3D11Device* device_of(ID3D11DeviceContext* context) {
        ID3D11Device* device = nullptr;
        context->GetDevice(&device);
        return device;
    }

    ID3D11Texture2D* staging_ = nullptr;
    UINT             w_ = 0, h_ = 0;
    DXGI_FORMAT      fmt_ = DXGI_FORMAT_UNKNOWN;

    /// 逐帧缓冲池（readback 实例私有——同一源分辨率恒定，rgba 恒命中；
    /// deleter 归池自持生命周期，rb 析构后仍在外的帧归还依然安全）
    LiveFramePool::Ptr pool_ = LiveFramePool::create();
};

} // namespace ms
