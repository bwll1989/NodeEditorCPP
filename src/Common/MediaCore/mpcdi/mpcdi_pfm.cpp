#include "mpcdi_parse.hpp"

// ============================================================================
// PFM（geometryWarpFile）解析 + warp 坐标归一化 —— 对齐 bake.js
// parsePFM / resizeUv / looksAbsolute / normalizeUv。
//
// PFM 格式要点（Portable FloatMap，MPCDI 2D profile 的 warp 数据）：
//   行1 "PF"（3 通道 u,v,disparity）或 "Pf"（1 通道 u）；
//   行2 "<width> <height>"（# 开头为注释行，跳过）；
//   行3 scale factor——**符号即字节序**：负数 = 小端（x86 常见），正数 = 大端；
//   之后 = float 二进制数据，**行序 bottom-up**（文件第 0 行 = 视觉最底行），
//         解析后统一翻转为视觉顶起（y=0 为顶），与屏幕坐标方向一致。
//
// NaN 语义：warp 数据用 NaN 标记"该屏幕像素无有效映射"（网格外区域），
//   bakes.js 全程让 NaN 自然传播，最终 uv<0 判黑。C++ 侧保持同样传播路径，
//   唯一要小心的是 clamp 不能用 std::min/max 吞掉 NaN（x86 下 std::max(NaN,0)
//   会按参数顺序返回非 NaN），必须先判 isfinite。
// ============================================================================

#include <cmath>
#include <cstring>

namespace ms {

namespace {

/** 逐行读取（\n 结尾，剥 \r），从 pos 推进；文件尾返回空串。 */
std::string read_line(const std::vector<std::uint8_t>& b, size_t& pos) {
    std::string line;
    while (pos < b.size()) {
        const char ch = static_cast<char>(b[pos++]);
        if (ch == '\n') break;
        if (ch != '\r') line.push_back(ch);
    }
    return line;
}

}  // namespace

// ---------------------------------------------------------------------------
// parsePFM
// ---------------------------------------------------------------------------
bool parse_pfm(const std::vector<std::uint8_t>& bytes, PfmData& out, std::string& err) {
    out = PfmData{};
    if (bytes.size() < 16) {
        err = "pfm too small";
        return false;
    }

    size_t pos = 0;

    // 行1：magic
    const std::string magic = read_line(bytes, pos);
    if (magic == "PF") {
        out.channels = 3;
    } else if (magic == "Pf") {
        out.channels = 1;
    } else {
        err = "not a PFM (magic=" + magic + ")";
        return false;
    }

    // 行2：尺寸（# 注释行跳过，对齐 bake.js）
    std::string dims = read_line(bytes, pos);
    while (!dims.empty() && dims[0] == '#') dims = read_line(bytes, pos);
    int w = 0;
    int h = 0;
    {
        if (std::sscanf(dims.c_str(), "%d %d", &w, &h) != 2 || w <= 0 || h <= 0) {
            err = "pfm bad dims line: " + dims;
            return false;
        }
    }
    // 行3：scale factor（符号定字节序）
    const std::string scale_line = read_line(bytes, pos);
    char* end = nullptr;
    const double scale = std::strtod(scale_line.c_str(), &end);
    if (!end || end == scale_line.c_str()) {
        err = "pfm bad scale line: " + scale_line;
        return false;
    }
    const bool little_endian = scale < 0;

    // 数据区（字节序按 scale 符号解释）
    const size_t count = static_cast<size_t>(w) * h * out.channels;
    if (bytes.size() - pos < count * 4) {
        err = "pfm data truncated";
        return false;
    }

    // 先按文件行序读入（bottom-up），再翻转为视觉顶起
    std::vector<float> raw(count);
    const std::uint8_t* p = bytes.data() + pos;
    for (size_t i = 0; i < count; ++i) {
        std::uint32_t bits;
        if (little_endian) {
            std::memcpy(&bits, p + i * 4, 4);
        } else {
            const std::uint8_t* q = p + i * 4;
            bits = (std::uint32_t(q[0]) << 24) | (std::uint32_t(q[1]) << 16) |
                   (std::uint32_t(q[2]) << 8) | std::uint32_t(q[3]);
        }
        std::memcpy(&raw[i], &bits, 4);   // IEEE754 float 位模式
    }

    out.width = w;
    out.height = h;
    out.data.resize(count);
    const size_t row = static_cast<size_t>(w) * out.channels;
    for (int y = 0; y < h; ++y) {
        // 文件第 (h-1-y) 行 = 视觉第 y 行（bottom-up 翻转）
        std::memcpy(&out.data[static_cast<size_t>(y) * row],
                    &raw[static_cast<size_t>(h - 1 - y) * row], row * sizeof(float));
    }
    err.clear();
    return true;
}

// ---------------------------------------------------------------------------
// resizeUv（双线性降采样，对齐 bake.js）
// ---------------------------------------------------------------------------
void resize_uv(const PfmData& pfm, int max_side, PfmResized& out) {
    const int width = pfm.width;
    const int height = pfm.height;
    const int channels = pfm.channels;

    // 目标尺寸（保持纵横比，只缩不放大）
    int nw = width;
    int nh = height;
    if (max_side > 0 && (width > max_side || height > max_side)) {
        if (width >= height) {
            nw = max_side;
            nh = (std::max)(1, static_cast<int>(std::lround(
                    static_cast<double>(height) * max_side / width)));
        } else {
            nh = max_side;
            nw = (std::max)(1, static_cast<int>(std::lround(
                    static_cast<double>(width) * max_side / height)));
        }
    }

    // 单通道 PFM：v 复制 u（对齐 bake.js at() 的 channels<2 分支）
    auto fetch = [&](int ix, int iy, float& u, float& v) {
        const size_t i = (static_cast<size_t>(iy) * width + ix) * channels;
        u = pfm.data[i];
        v = channels >= 2 ? pfm.data[i + 1] : pfm.data[i];
    };

    // 双线性采样：坐标钳制到 [0, w-1.001]（保证 x1=x0+1 不越界，对齐 bake.js）
    auto sample = [&](float x, float y, float& u, float& v) {
        x = (std::min)((std::max)(x, 0.0f), static_cast<float>(width) - 1.001f);
        y = (std::min)((std::max)(y, 0.0f), static_cast<float>(height) - 1.001f);
        const int x0 = static_cast<int>(x);
        const int y0 = static_cast<int>(y);
        const int x1 = (std::min)(x0 + 1, width - 1);
        const int y1 = (std::min)(y0 + 1, height - 1);
        const float tx = x - x0;
        const float ty = y - y0;
        float a0, a1, b0, b1, c0, c1, d0, d1;
        fetch(x0, y0, a0, a1);
        fetch(x1, y0, b0, b1);
        fetch(x0, y1, c0, c1);
        fetch(x1, y1, d0, d1);
        u = (a0 * (1 - tx) + b0 * tx) * (1 - ty) + (c0 * (1 - tx) + d0 * tx) * ty;
        v = (a1 * (1 - tx) + b1 * tx) * (1 - ty) + (c1 * (1 - tx) + d1 * tx) * ty;
        // NaN 数据在乘加中自然传播（与 JS 一致）；坐标非法时手工置 NaN 保险
        if (!(x0 >= 0) || !(y0 >= 0)) { u = std::nanf(""); v = std::nanf(""); }
    };

    out.width = nw;
    out.height = nh;
    out.uvs.resize(static_cast<size_t>(nw) * nh * 2);
    for (int y = 0; y < nh; ++y) {
        for (int x = 0; x < nw; ++x) {
            // 中心对齐（texel 中心映射回源坐标，对齐 bake.js）
            const float sx = (x + 0.5f) * width / nw - 0.5f;
            const float sy = (y + 0.5f) * height / nh - 0.5f;
            float u = 0.0f;
            float v = 0.0f;
            sample(sx, sy, u, v);
            const size_t o = (static_cast<size_t>(y) * nw + x) * 2;
            out.uvs[o] = u;
            out.uvs[o + 1] = v;
        }
    }
}

// ---------------------------------------------------------------------------
// looksAbsolute（warp 值域是否为绝对像素坐标）
// ---------------------------------------------------------------------------
bool looks_absolute(const PfmResized& r) {
    float max_u = -std::numeric_limits<float>::infinity();
    float max_v = -std::numeric_limits<float>::infinity();
    float min_u = std::numeric_limits<float>::infinity();
    float min_v = std::numeric_limits<float>::infinity();
    bool has_finite = false;
    for (size_t i = 0; i + 1 < r.uvs.size(); i += 2) {
        const float u = r.uvs[i];
        const float v = r.uvs[i + 1];
        if (!std::isfinite(u) || !std::isfinite(v)) continue;   // 跳过 NaN（对齐 JS）
        has_finite = true;
        max_u = (std::max)(max_u, u);
        max_v = (std::max)(max_v, v);
        min_u = (std::min)(min_u, u);
        min_v = (std::min)(min_v, v);
    }
    if (!has_finite) return false;   // 全 NaN → 非绝对（对齐 JS 的 !isFinite(maxU)）
    // 越出 [-0.001, 1.001] 任一方向即视为绝对像素坐标
    return !(max_u <= 1.001f && max_v <= 1.001f && min_u >= -0.001f && min_v >= -0.001f);
}

// ---------------------------------------------------------------------------
// normalizeUv（四分支归一化，逐条对齐 bake.js）
// ---------------------------------------------------------------------------
void normalize_uv(float u, float v, float abs_w, float abs_h, float out[2]) {
    // NaN / INF → 无效映射（渲染黑）
    if (!std::isfinite(u) || !std::isfinite(v)) {
        out[0] = -1.0f;
        out[1] = -1.0f;
        return;
    }
    // 分支 1：已在归一化域（含 5% 容差）→ clamp
    if (u >= -0.05f && u <= 1.05f && v >= -0.05f && v <= 1.05f) {
        out[0] = (std::min)((std::max)(u, 0.0f), 1.0f);
        out[1] = (std::min)((std::max)(v, 0.0f), 1.0f);
        return;
    }
    // 分支 2：绝对像素坐标（超界且已知绝对尺寸）→ 除尺寸
    if (abs_w > 1.0f && abs_h > 1.0f &&
        (u > 1.05f || v > 1.05f || u < -0.05f || v < -0.05f)) {
        out[0] = u / abs_w;
        out[1] = v / abs_h;
        return;
    }
    // 分支 3：[-1,1] 双极坐标 → 映射到 [0,1]
    if (u >= -1.05f && u <= 1.05f && v >= -1.05f && v <= 1.05f) {
        out[0] = (u + 1.0f) * 0.5f;
        out[1] = (v + 1.0f) * 0.5f;
        return;
    }
    // 分支 4：兜底（有绝对尺寸则除，否则原样）
    if (abs_w > 1.0f && abs_h > 1.0f) {
        out[0] = u / abs_w;
        out[1] = v / abs_h;
        return;
    }
    out[0] = u;
    out[1] = v;
}

// ---------------------------------------------------------------------------
// compute_warp_stats（对齐 viewer computeWarpStats；metadata 面板用）
// 全分辨率统计（不降采样）：NaN 单独计数不参与值域；全部有限值落在 [0,1]
// 容差内 → normalized，否则 absolute (pixels)。PFM 通道布局：
// "PF"=3（u,v,disparity）、"Pf"=1（仅 u，此时 v 按 0 处理）。
// ---------------------------------------------------------------------------
bool compute_warp_stats(const std::vector<std::uint8_t>& pfm_bytes, MpcdiWarpStats& out,
                        std::string& err) {
    PfmData pfm;
    if (!parse_pfm(pfm_bytes, pfm, err)) {
        return false;   // err 已由 parse_pfm 填写
    }
    out = MpcdiWarpStats{};
    out.width = pfm.width;
    out.height = pfm.height;
    out.total_texels = static_cast<long long>(pfm.width) * pfm.height;

    float min_u = 0.0f, max_u = 0.0f, min_v = 0.0f, max_v = 0.0f;
    bool any = false;
    const int ch = pfm.channels >= 3 ? 3 : 1;
    for (size_t i = 0; i < pfm.data.size() / static_cast<size_t>(pfm.channels); ++i) {
        // 注意：pfm.data 是 channels 交错存储（u,v,disparity 或仅 u）
        const float u = pfm.data[i * pfm.channels];
        const float v = pfm.channels >= 3 ? pfm.data[i * pfm.channels + 1] : 0.0f;
        if (!std::isfinite(u) || !std::isfinite(v)) {
            ++out.nan_count;
            continue;
        }
        if (!any || u < min_u) min_u = u;
        if (!any || u > max_u) max_u = u;
        if (!any || v < min_v) min_v = v;
        if (!any || v > max_v) max_v = v;
        any = true;
    }
    out.min_u = min_u;
    out.max_u = max_u;
    out.min_v = min_v;
    out.max_v = max_v;
    // 对齐 viewer：maxX<=1.001 && maxY<=1.001 && minX>=-0.001 && minY>=-0.001
    out.looks_normalized = max_u <= 1.001f && max_v <= 1.001f &&
                           min_u >= -0.001f && min_v >= -0.001f;
    err.clear();
    return true;
}

}  // namespace ms
