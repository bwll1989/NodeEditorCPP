// ============================================================================
// mpcdi_to_warp —— MpcdiGeometry → WarpSpec(mode=2) 拼接转换（实现见头注释）
// ============================================================================

#include "mpcdi_to_warp.hpp"

#include <algorithm>
#include <cmath>

namespace ms {
namespace {

/** 无效顶点标记（渲染端约定：pos/uv=(-1,-1) → 顶点窗外 + uv 越界 discard）。 */
constexpr float kInvalid = -1.0f;

/** 顶点场重采样目标密度钳制（对齐 bake.js maxWarp=256 的量级保护）。 */
constexpr int kMinCell = 2;
constexpr int kMaxCell = 256;

/** 值有效（有限且非无效标记域）。 */
inline bool valid(float v) noexcept {
    return std::isfinite(v) && v != kInvalid;
}

/** alpha 场采样（双线性）：src RGBA8（取 R 通道），(tx,ty)∈[0,1] → [0,1]。 */
inline float sample_alpha_bilinear(const std::vector<std::uint8_t>& rgba,
                                   int w, int h, float tx, float ty) noexcept {
    if (w <= 0 || h <= 0 ||
        rgba.size() < static_cast<size_t>(w) * h * 4)
        return 1.0f;   // 无 alpha 场 = 全 1（identity 占位语义）
    const float fx = std::clamp(tx, 0.0f, 1.0f) * (w - 1);
    const float fy = std::clamp(ty, 0.0f, 1.0f) * (h - 1);
    const int x0 = static_cast<int>(fx), y0 = static_cast<int>(fy);
    const int x1 = std::min(x0 + 1, w - 1), y1 = std::min(y0 + 1, h - 1);
    const float s = fx - x0, t = fy - y0;
    const auto at = [&](int x, int y) {
        return static_cast<float>(
            rgba[(static_cast<size_t>(y) * w + x) * 4]);   // R 通道
    };
    const float v = at(x0, y0) * (1 - s) * (1 - t) + at(x1, y0) * s * (1 - t) +
                    at(x0, y1) * (1 - s) * t + at(x1, y1) * s * t;
    return v / 255.0f;
}

/** pos 场采样（双线性）：RegionVertex.pos 分量；任一角无效 → false（保守）。 */
inline bool sample_pos_bilinear(const RegionMesh& m, float tx, float ty,
                                float out[2]) noexcept {
    const float fx = std::clamp(tx, 0.0f, 1.0f) * (m.mesh_w - 1);
    const float fy = std::clamp(ty, 0.0f, 1.0f) * (m.mesh_h - 1);
    const int x0 = static_cast<int>(fx), y0 = static_cast<int>(fy);
    const int x1 = std::min(x0 + 1, m.mesh_w - 1);
    const int y1 = std::min(y0 + 1, m.mesh_h - 1);
    const float s = fx - x0, t = fy - y0;
    const auto at = [&](int x, int y, int c) {
        const auto& v = m.vertices[static_cast<size_t>(y) * m.mesh_w + x];
        return c == 0 ? v.pos_x : v.pos_y;
    };
    const float a0 = at(x0, y0, 0), a1 = at(x0, y0, 1);
    const float b0 = at(x1, y0, 0), b1 = at(x1, y0, 1);
    const float c0 = at(x0, y1, 0), c1 = at(x0, y1, 1);
    const float d0 = at(x1, y1, 0), d1 = at(x1, y1, 1);
    // 任一角无效（NaN / 无效标记）→ 保守整点无效（不外推）
    if (!valid(a0) || !valid(a1) || !valid(b0) || !valid(b1) ||
        !valid(c0) || !valid(c1) || !valid(d0) || !valid(d1))
        return false;
    out[0] = a0 * (1 - s) * (1 - t) + b0 * s * (1 - t) +
             c0 * (1 - s) * t + d0 * s * t;
    out[1] = a1 * (1 - s) * (1 - t) + b1 * s * (1 - t) +
             c1 * (1 - s) * t + d1 * s * t;
    return true;
}

}  // namespace

WarpSpec mpcdi_to_warp(const MpcdiGeometry& geo) {
    WarpSpec ws;
    if (geo.regions.empty() || geo.cols <= 0 || geo.rows <= 0) return ws;

    // ---- 统一 cell 密度：各 region mesh 尺寸最大值（钳 [2,256]）----
    int sw = kMinCell, sh = kMinCell;
    for (const auto& r : geo.regions) {
        sw = std::max(sw, r.mesh_w);
        sh = std::max(sh, r.mesh_h);
    }
    sw = std::clamp(sw, kMinCell, kMaxCell);
    sh = std::clamp(sh, kMinCell, kMaxCell);

    ws.mode = 2;
    ws.cols = geo.cols * sw;
    ws.rows = geo.rows * sh;
    const size_t n = static_cast<size_t>(ws.cols) * ws.rows;
    ws.pts.assign(n * 2, kInvalid);   // 空位/无效默认：pos 窗外
    ws.uvs.assign(n * 2, kInvalid);   // uv 无效（discard 黑）
    ws.alphas.assign(n, 0.0f);        // alpha 0（RGB 乘 0 黑）

    // ---- region 逐个落位（重采样到统一 cell 密度）----
    for (const auto& r : geo.regions) {
        if (r.mesh_w <= 0 || r.mesh_h <= 0) continue;
        for (int j = 0; j < sh; ++j) {
            // 归一化采样位置（cell 内）：网格顶点行主序、y 视觉向下——
            // 与 RegionVertex 行序同向（顶行 = 0）
            const float ty = sh > 1 ? static_cast<float>(j) / (sh - 1)
                                    : 0.0f;
            for (int i = 0; i < sw; ++i) {
                const float tx = sw > 1 ? static_cast<float>(i) / (sw - 1)
                                        : 0.0f;
                const size_t g = (static_cast<size_t>(r.cell_row) * sh + j) *
                                     ws.cols +
                                 static_cast<size_t>(r.cell_col) * sw + i;
                float p[2] = {kInvalid, kInvalid};
                const bool pos_ok = sample_pos_bilinear(r, tx, ty, p);
                if (pos_ok) {
                    ws.pts[g * 2] = p[0];
                    ws.pts[g * 2 + 1] = p[1];
                }
                // uv：最近邻查表（无效区锐利边界）
                {
                    const int x = std::clamp(
                        static_cast<int>(tx * (r.mesh_w - 1) + 0.5f), 0,
                        r.mesh_w - 1);
                    const int y = std::clamp(
                        static_cast<int>(ty * (r.mesh_h - 1) + 0.5f), 0,
                        r.mesh_h - 1);
                    const auto& v =
                        r.vertices[static_cast<size_t>(y) * r.mesh_w + x];
                    if (valid(v.uv_x) && valid(v.uv_y) &&
                        v.uv_x != kInvalid && v.uv_y != kInvalid) {
                        ws.uvs[g * 2] = v.uv_x;
                        ws.uvs[g * 2 + 1] = v.uv_y;
                    }
                }
                ws.alphas[g] = sample_alpha_bilinear(
                    r.alpha_rgba, r.alpha_width, r.alpha_height, tx, ty);
            }
        }
    }
    return ws;
}

}  // namespace ms
