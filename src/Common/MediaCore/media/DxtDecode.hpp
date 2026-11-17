// ============================================================================
//  DxtDecode.hpp —— 小型 CPU DXT 解码器（HAP 预览软解消费）
// ----------------------------------------------------------------------------
//  2026-11-17 收编进 MediaCore 供预览端使用：编辑器预览的 3D 视口
//（QPainter 贴图）与 SoftCompose 软合成只能消费 CPU RGBA 像素，HAP
//  CpuDxt 帧在此解压成 QImage 交付（worker 线程，≈1-3ms/帧@1080p）；
//  display 端与预览 2D GL 直通仍走压缩纹理直传不解压。
//  算法为标准 S3TC 规范（与 GPU 采样器/FFmpeg texturedsp 同一算法，整数
//  舍入路径可能差 ±1-2，对拍用容差断言而非精确相等）：
//    * DXT1（Hap1）：8B/块 = c0,c1(RGB565) + 4B 双位索引；
//      c0<=c1 时调色板第 3 项为半混色、第 4 项为黑（1 位 alpha 语义）。
//    * DXT5（Hap5/Hap7）：16B/块 = 8B alpha（a0,a1 + 16×3bit 索引）+
//      8B 颜色（恒四色插值，无黑关键帧特例）。
// ============================================================================
#pragma once

#include <cstdint>
#include <cstddef>

namespace dxtref {

inline void unpack565(std::uint16_t c, std::uint8_t out[3]) {
    const int r = (c >> 11) & 0x1F;
    const int g = (c >> 5) & 0x3F;
    const int b = c & 0x1F;
    out[0] = static_cast<std::uint8_t>((r << 3) | (r >> 2));
    out[1] = static_cast<std::uint8_t>((g << 2) | (g >> 4));
    out[2] = static_cast<std::uint8_t>((b << 3) | (b >> 2));
}

/// DXT 颜色块（8B）：dxt1Mode=true 时支持 c0<=c1 黑关键帧特例
inline void decodeColorBlock(const std::uint8_t* src, std::uint8_t dst[16 * 4],
                             bool dxt1Mode) {
    const std::uint16_t c0 = static_cast<std::uint16_t>(src[0] | (src[1] << 8));
    const std::uint16_t c1 = static_cast<std::uint16_t>(src[2] | (src[3] << 8));
    std::uint8_t p[4][3];
    unpack565(c0, p[0]);
    unpack565(c1, p[1]);
    if (dxt1Mode && c0 <= c1) {
        for (int ch = 0; ch < 3; ++ch) {
            p[2][ch] = static_cast<std::uint8_t>((p[0][ch] + p[1][ch]) / 2);
            p[3][ch] = 0;
        }
    } else {
        for (int ch = 0; ch < 3; ++ch) {
            p[2][ch] = static_cast<std::uint8_t>((p[0][ch] * 2 + p[1][ch]) / 3);
            p[3][ch] = static_cast<std::uint8_t>((p[0][ch] + p[1][ch] * 2) / 3);
        }
    }
    const std::uint32_t bits = static_cast<std::uint32_t>(src[4]) |
                               (static_cast<std::uint32_t>(src[5]) << 8) |
                               (static_cast<std::uint32_t>(src[6]) << 16) |
                               (static_cast<std::uint32_t>(src[7]) << 24);
    for (int i = 0; i < 16; ++i) {
        const int idx = (bits >> (2 * i)) & 3;
        dst[i * 4 + 0] = p[idx][0];
        dst[i * 4 + 1] = p[idx][1];
        dst[i * 4 + 2] = p[idx][2];
        dst[i * 4 + 3] = 255;
    }
}

inline void decodeDxt1Block(const std::uint8_t* src, std::uint8_t dst[16 * 4]) {
    decodeColorBlock(src, dst, true);
}

inline void decodeDxt5Block(const std::uint8_t* src, std::uint8_t dst[16 * 4]) {
    const int a0 = src[0], a1 = src[1];
    std::uint8_t ap[8];
    ap[0] = static_cast<std::uint8_t>(a0);
    ap[1] = static_cast<std::uint8_t>(a1);
    if (a0 > a1) {
        for (int i = 2; i <= 7; ++i)
            ap[i] = static_cast<std::uint8_t>(((8 - i) * a0 + (i - 1) * a1) / 7);
    } else {
        ap[2] = static_cast<std::uint8_t>((4 * a0 + a1) / 5);
        ap[3] = static_cast<std::uint8_t>((3 * a0 + 2 * a1) / 5);
        ap[4] = static_cast<std::uint8_t>((2 * a0 + 3 * a1) / 5);
        ap[5] = static_cast<std::uint8_t>((a0 + 4 * a1) / 5);
        ap[6] = 0;
        ap[7] = 255;
    }
    std::uint64_t abits = 0;
    for (int i = 0; i < 6; ++i)
        abits |= static_cast<std::uint64_t>(src[2 + i]) << (8 * i);
    std::uint8_t color[16 * 4];
    decodeColorBlock(src + 8, color, false);
    for (int i = 0; i < 16; ++i) {
        const int idx = static_cast<int>((abits >> (3 * i)) & 7);
        dst[i * 4 + 0] = color[i * 4 + 0];
        dst[i * 4 + 1] = color[i * 4 + 1];
        dst[i * 4 + 2] = color[i * 4 + 2];
        dst[i * 4 + 3] = ap[idx];
    }
}

/// 裸 DXT 数据（GL 压缩纹理排布，行主序块网格）→ RGBA（行主序 top-down）
/// \param dxt5 true=DXT5（16B/块），false=DXT1（8B/块）
/// \param stride 输出行字节数（= w*4）
inline void decodeToRgba(int width, int height, const std::uint8_t* data,
                         bool dxt5, std::uint8_t* out_rgba, std::size_t stride) {
    const int blocksX = (width + 3) / 4;
    const int blocksY = (height + 3) / 4;
    const std::size_t blockSize = dxt5 ? 16 : 8;
    for (int by = 0; by < blocksY; ++by) {
        for (int bx = 0; bx < blocksX; ++bx, data += blockSize) {
            std::uint8_t px[16 * 4];
            if (dxt5) decodeDxt5Block(data, px);
            else      decodeDxt1Block(data, px);
            for (int py = 0; py < 4; ++py) {
                const int y = by * 4 + py;
                if (y >= height) break;
                for (int pxx = 0; pxx < 4; ++pxx) {
                    const int x = bx * 4 + pxx;
                    if (x >= width) break;
                    const int pi = py * 4 + pxx;
                    std::uint8_t* row = out_rgba + static_cast<std::size_t>(y) * stride;
                    row[x * 4 + 0] = px[pi * 4 + 0];
                    row[x * 4 + 1] = px[pi * 4 + 1];
                    row[x * 4 + 2] = px[pi * 4 + 2];
                    row[x * 4 + 3] = px[pi * 4 + 3];
                }
            }
        }
    }
}

} // namespace dxtref
