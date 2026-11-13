#include "mpcdi_parse.hpp"

// ============================================================================
// alphaMap（PNG）解码 —— 对齐 bake.js readPngRgba / identityRgba。
// 解码引擎 = stb_image.h（公共域单头；alpha 遮罩是低频数据，解码 + 自写双线性
// 降采样即可，不必引入 libpng+zlib 组合）。
// ============================================================================

// stb_image 在 /W4 下有海量内部警告，降级到最低告警级再包含（不影响我们代码）
#pragma warning(push, 0)
#define STB_IMAGE_IMPLEMENTATION
#define STBI_ONLY_PNG                 // MPCDI alphaMap 只可能是 PNG
#include "stb_image.h"
#pragma warning(pop)

#include <cmath>
#include <cstring>

namespace ms {

namespace {

/**
 * RGBA8 双线性降采样（对齐 bake.js readPngRgba 的 canvas 缩放语义：
 * 中心对齐 + 边缘钳制；alpha 是低频遮罩，顶点网格下精度足够）。
 */
void downscale_rgba(const std::uint8_t* src, int w, int h,
                    int nw, int nh, std::vector<std::uint8_t>& out) {
    out.resize(static_cast<size_t>(nw) * nh * 4);
    auto clampf = [](float x, float hi) {
        return (std::min)((std::max)(x, 0.0f), hi);
    };
    for (int y = 0; y < nh; ++y) {
        for (int x = 0; x < nw; ++x) {
            const float sx = clampf((x + 0.5f) * w / nw - 0.5f, w - 1.001f);
            const float sy = clampf((y + 0.5f) * h / nh - 0.5f, h - 1.001f);
            const int x0 = static_cast<int>(sx);
            const int y0 = static_cast<int>(sy);
            const int x1 = (std::min)(x0 + 1, w - 1);
            const int y1 = (std::min)(y0 + 1, h - 1);
            const float tx = sx - x0;
            const float ty = sy - y0;
            const size_t o = (static_cast<size_t>(y) * nw + x) * 4;
            for (int c = 0; c < 4; ++c) {
                const float a = src[(static_cast<size_t>(y0) * w + x0) * 4 + c];
                const float b = src[(static_cast<size_t>(y0) * w + x1) * 4 + c];
                const float d = src[(static_cast<size_t>(y1) * w + x0) * 4 + c];
                const float e = src[(static_cast<size_t>(y1) * w + x1) * 4 + c];
                const float val = (a * (1 - tx) + b * tx) * (1 - ty) +
                                  (d * (1 - tx) + e * tx) * ty;
                out[o + c] = static_cast<std::uint8_t>(val + 0.5f);
            }
        }
    }
}

}  // namespace

// ---------------------------------------------------------------------------
// PNG 解码 + 限边降采样
// ---------------------------------------------------------------------------
bool decode_alpha_png(const std::vector<std::uint8_t>& png_bytes, int max_side,
                      AlphaImage& out, std::string& err) {
    out = AlphaImage{};
    int w = 0;
    int h = 0;
    int comp = 0;
    // 第 4 参 = 4：强制输出 RGBA（灰度/索引 PNG 统一展开，对齐 canvas drawImage 行为）
    stbi_uc* data = stbi_load_from_memory(png_bytes.data(),
                                          static_cast<int>(png_bytes.size()),
                                          &w, &h, &comp, 4);
    if (!data) {
        err = std::string("alpha png decode failed: ") +
              (stbi_failure_reason() ? stbi_failure_reason() : "unknown");
        return false;
    }

    // 限边降采样（保持纵横比，只缩不放大；对齐 bake.js maxMask 512）
    int nw = w;
    int nh = h;
    if (max_side > 0 && (w > max_side || h > max_side)) {
        if (w >= h) {
            nw = max_side;
            nh = (std::max)(1, static_cast<int>(std::lround(
                    static_cast<double>(h) * max_side / w)));
        } else {
            nh = max_side;
            nw = (std::max)(1, static_cast<int>(std::lround(
                    static_cast<double>(w) * max_side / h)));
        }
    }

    out.width = nw;
    out.height = nh;
    if (nw == w && nh == h) {
        out.rgba.assign(data, data + static_cast<size_t>(w) * h * 4);
    } else {
        downscale_rgba(data, w, h, nw, nh, out.rgba);
    }
    stbi_image_free(data);
    err.clear();
    return true;
}

// ---------------------------------------------------------------------------
// 无 alphaMap 占位：8x8 全 1（白色），对齐 bake.js identityRgba(8,8,[255,255,255])
// ---------------------------------------------------------------------------
AlphaImage identity_alpha() {
    AlphaImage a;
    a.width = 8;
    a.height = 8;
    a.rgba.assign(static_cast<size_t>(64) * 4, 255);
    return a;
}

// ---------------------------------------------------------------------------
// PNG 头尺寸提取（IHDR；metadata "blend size" 用，不解码整图）。
// PNG 签名 8 字节 + IHDR 块：长度(4) + "IHDR"(4) + 宽(4 BE) + 高(4 BE)。
// ---------------------------------------------------------------------------
bool png_header_size(const std::vector<std::uint8_t>& png_bytes, int& w, int& h) {
    w = h = 0;
    // 签名 + IHDR 头最小长度：8 签名 + 4 长度 + 4 "IHDR" + 4 宽 + 4 高
    if (png_bytes.size() < 24) return false;
    static const std::uint8_t sig[8] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1A, '\n'};
    if (std::memcmp(png_bytes.data(), sig, 8) != 0) return false;
    if (std::memcmp(png_bytes.data() + 12, "IHDR", 4) != 0) return false;
    const auto be32 = [&png_bytes](size_t off) -> std::uint32_t {
        return (std::uint32_t(png_bytes[off]) << 24) | (std::uint32_t(png_bytes[off + 1]) << 16) |
               (std::uint32_t(png_bytes[off + 2]) << 8) | std::uint32_t(png_bytes[off + 3]);
    };
    const std::uint32_t iw = be32(16);
    const std::uint32_t ih = be32(20);
    if (iw == 0 || iw > 0x7FFFFFFFu || ih == 0 || ih > 0x7FFFFFFFu) return false;
    w = static_cast<int>(iw);
    h = static_cast<int>(ih);
    return true;
}

}  // namespace ms
