#include "mpcdi_parse.hpp"

// ============================================================================
// MPCDI 总装：布局计算 + 顶点网格生成 —— 对齐 bake.js computeLayout/bake 主流程，
// 但以「顶点网格」替代「shader 烘焙」（设计 docs/phase1-mpcdi-render.md）。
//
// 顶点语义（v26 实证，2026-10-29 用 test1014 真实包验证）：
//   geometryWarpFile 的 PFM 内容是 buffer 坐标（屏幕像素采样视频的位置），
//   因此顶点域取「屏幕网格」：顶点 (x,y) 的屏幕位置 sx=x/(W-1) 直接烘焙
//   cell 偏移成画布归一化 pos；PFM 值经 normalizeUv 后作为 buffer uv。
//   （设计文档 §2 早期伪代码把两者写反了，实现以本文件为准，文档已回改。）
// ============================================================================

#include <algorithm>
#include <cmath>
#include <chrono>
#include <cstdio>

namespace ms {

namespace {

/** ASCII 小写（region 匹配容错）。 */
std::string ascii_lower(std::string s) {
    for (char& c : s) {
        if (c >= 'A' && c <= 'Z') c = static_cast<char>(c - 'A' + 'a');
    }
    return s;
}

/** computeLayout（对齐 bake.js：grid/row/column/cols2~4/auto）。 */
void compute_layout(int count, const MpcdiLayoutOpt& in, int& cols, int& rows) {
    cols = 1;
    rows = 1;
    if (count <= 0) return;
    const std::string mode = in.mode.empty() ? "grid" : ascii_lower(in.mode);
    if (mode == "row") {
        cols = count;
        rows = 1;
    } else if (mode == "column") {
        cols = 1;
        rows = count;
    } else if (mode == "cols2") {
        cols = 2;
        rows = (count + 1) / 2;
    } else if (mode == "cols3") {
        cols = 3;
        rows = (count + 2) / 3;
    } else if (mode == "cols4") {
        cols = 4;
        rows = (count + 3) / 4;
    } else {
        // grid：手动列数或 ceil(sqrt(n)) 自动方阵
        cols = in.cols > 0 ? in.cols
                           : static_cast<int>(std::ceil(std::sqrt(static_cast<double>(count))));
        rows = (count + cols - 1) / cols;
    }
}

/** 标准 W x H 网格三角化索引（每 2x2 顶点两个三角形，顶点行主序）。 */
void push_grid_indices(int w, int h, std::vector<std::uint32_t>& out) {
    for (int y = 0; y + 1 < h; ++y) {
        for (int x = 0; x + 1 < w; ++x) {
            const std::uint32_t tl = static_cast<std::uint32_t>(y * w + x);       // 左上
            const std::uint32_t tr = tl + 1;                                      // 右上
            const std::uint32_t bl = tl + static_cast<std::uint32_t>(w);          // 左下
            const std::uint32_t br = bl + 1;                                      // 右下
            out.push_back(tl); out.push_back(bl); out.push_back(tr);
            out.push_back(tr); out.push_back(bl); out.push_back(br);
        }
    }
}

/** 单 region 网格构建（含 PFM 解析/归一化/alpha；is_unit=无 warp 的直通网格）。 */
bool build_region_mesh(const MpcdiPackageData& pkg,
                       const MpcdiRegionInfo& info,
                       const MpcdiBuildOptions& opt,
                       int cell_col, int cell_row, int cols, int rows,
                       const std::map<std::string, AlphaImage>& alpha_cache,
                       RegionMesh& mesh, std::string& err) {
    // ---------- alpha（cell 尺寸探测阶段已解码的复用缓存）----------
    // blend_enabled=false（viewer "Blend" 关）：跳过包内 alphaMap，用全 1 占位——
    // 网格 shader 逻辑不变（乘 1.0），无需渲染端新开关。
    AlphaImage alpha;
    if (!opt.blend_enabled) {
        alpha = identity_alpha();
    } else if (auto it = alpha_cache.find(info.id); it != alpha_cache.end()) {
        alpha = it->second;
    } else if (!info.alpha_map.empty()) {
        const auto* bytes = pkg.find(info.alpha_map, info.buffer_id);
        if (!bytes) {
            err = "alpha map not in package: " + info.alpha_map;
            return false;
        }
        if (!decode_alpha_png(*bytes, opt.max_mask, alpha, err)) {
            err = "region " + info.id + ": " + err;
            return false;
        }
    } else {
        alpha = identity_alpha();
    }
    mesh.alpha_rgba = std::move(alpha.rgba);
    mesh.alpha_width = alpha.width;
    mesh.alpha_height = alpha.height;
    // gamma：XML gammaEmbedded 缺省 2.2（对齐 bake.js）；线性化开关由全局字段控制
    mesh.alpha_gamma = info.alpha_gamma > 0.0f ? info.alpha_gamma : 2.2f;
    mesh.cell_col = cell_col;
    mesh.cell_row = cell_row;

    // ---------- warp PFM ----------
    PfmResized resized;
    if (info.geometry_warp_file.empty()) {
        // 无 warp 数据：单位网格 2x2 直通（pos=uv），普通全屏拼接语义
        mesh.mesh_w = 2;
        mesh.mesh_h = 2;
        mesh.vertices.resize(4);
        for (int y = 0; y < 2; ++y) {
            for (int x = 0; x < 2; ++x) {
                const float sx = static_cast<float>(x);
                const float sy = static_cast<float>(y);
                RegionVertex& v = mesh.vertices[static_cast<size_t>(y) * 2 + x];
                v.pos_x = (cell_col + sx) / cols;
                v.pos_y = (cell_row + sy) / rows;
                v.uv_x = sx;
                v.uv_y = sy;
            }
        }
        push_grid_indices(2, 2, mesh.indices);
        return true;
    }

    const auto* pfm_bytes = pkg.find(info.geometry_warp_file, info.buffer_id);
    if (!pfm_bytes) {
        err = "warp pfm not in package: " + info.geometry_warp_file;
        return false;
    }
    PfmData pfm;
    if (!parse_pfm(*pfm_bytes, pfm, err)) {
        err = "region " + info.id + ": " + err;
        return false;
    }
    resize_uv(pfm, opt.max_warp, resized);

    // ---------- 归一化坐标判定（对齐 bake.js bakeOneRegionAssets）----------
    bool use_absolute = looks_absolute(resized);
    if (opt.force_absolute) use_absolute = true;
    if (opt.force_normalized) use_absolute = false;
    // 绝对像素坐标的归一化分母：XML 分辨率优先，缺省退 PFM 原始尺寸
    float abs_w = 0.0f;
    float abs_h = 0.0f;
    if (use_absolute) {
        abs_w = info.x_resolution > 0 ? static_cast<float>(info.x_resolution)
                                      : static_cast<float>(pfm.width);
        abs_h = info.y_resolution > 0 ? static_cast<float>(info.y_resolution)
                                      : static_cast<float>(pfm.height);
    }

    // ---------- 顶点生成 ----------
    const int w = resized.width;
    const int h = resized.height;
    mesh.mesh_w = w;
    mesh.mesh_h = h;
    mesh.vertices.resize(static_cast<size_t>(w) * h);
    // region_local 的 rect（对齐 bake.js：原样用 XML region 属性 x/y/xSize/ySize）
    const float rl_x = info.x;
    const float rl_y = info.y;
    const float rl_w = info.x_size;
    const float rl_h = info.y_size;
    for (int y = 0; y < h; ++y) {
        for (int x = 0; x < w; ++x) {
            RegionVertex& vtx = mesh.vertices[static_cast<size_t>(y) * w + x];
            // 屏幕网格位置（W==1/H==1 时退半格，避免除零）
            const float sx = w > 1 ? static_cast<float>(x) / (w - 1) : 0.5f;
            const float sy = h > 1 ? static_cast<float>(y) / (h - 1) : 0.5f;
            // 画布归一化位置 = cell 偏移 + cell 内位置
            vtx.pos_x = (cell_col + sx) / cols;
            vtx.pos_y = (cell_row + sy) / rows;
            // buffer uv = PFM 内容归一化（NaN → (-1,-1)，shader 判负输出黑）。
            // flip_screen_y（对齐 bake.js warpLookup.y = 1-localUV.y）：查表行镜像。
            const int lookup_y = opt.flip_screen_y ? (h - 1 - y) : y;
            const size_t o = (static_cast<size_t>(lookup_y) * w + x) * 2;
            normalize_uv(resized.uvs[o], resized.uvs[o + 1], abs_w, abs_h,
                         &vtx.uv_x);
            // flip_y（对齐 bake.js uv.y = 1-uv.y）：warp 输出 uv 上下镜像。
            // region_local（对齐 bake.js）：uv = (uv - rect.xy) / rect.wh，把
            // region 局部/像素坐标换算回 0~1（size<=0 视为单位 1 防除零）。
            // 两者都在 v26 shader 的 NaN 判黑（uv<0）之后——只对有效 uv 应用，
            // 不得触碰 (-1,-1) 无效标记（翻转会伪造出合法值破坏判黑）。
            if (opt.flip_y && vtx.uv_y >= 0.0f) vtx.uv_y = 1.0f - vtx.uv_y;
            if (opt.region_local && vtx.uv_x >= 0.0f && vtx.uv_y >= 0.0f) {
                if (rl_w > 0.0f) vtx.uv_x = (vtx.uv_x - rl_x) / rl_w;
                if (rl_h > 0.0f) vtx.uv_y = (vtx.uv_y - rl_y) / rl_h;
            }
        }
    }
    push_grid_indices(w, h, mesh.indices);
    return true;
}

}  // namespace

// ---------------------------------------------------------------------------
// 总装（包路径版）
// ---------------------------------------------------------------------------
bool build_mpcdi_geometry(const std::string& utf8_package_path,
                          const MpcdiBuildOptions& opt,
                          MpcdiGeometry& out, std::string& err) {
    MpcdiPackageData pkg;
    if (!load_mpcdi_package(utf8_package_path, pkg, err)) {
        err = "load package failed: " + err;
        return false;
    }
    return build_mpcdi_geometry(pkg, opt, out, err);
}

// ---------------------------------------------------------------------------
// 总装（包数据版）
// ---------------------------------------------------------------------------
bool build_mpcdi_geometry(const MpcdiPackageData& pkg,
                          const MpcdiBuildOptions& opt,
                          MpcdiGeometry& out, std::string& err) {
    out = MpcdiGeometry{};

    // 1) XML
    MpcdiInfo info;
    if (!parse_mpcdi_xml(pkg, info, err)) {
        err = "parse xml failed: " + err;
        return false;
    }

    // 2) 选择 region（id 或 key 精确匹配，保序）
    std::vector<const MpcdiRegionInfo*> selected;
    if (opt.regions.empty()) {
        // 未指定：取第一个（对齐 bake.js 默认单 region）
        selected.push_back(&info.regions.front());
    } else {
        for (const std::string& want : opt.regions) {
            const MpcdiRegionInfo* found = nullptr;
            for (const MpcdiRegionInfo& r : info.regions) {
                if (r.id == want || r.key == want) {
                    found = &r;
                    break;
                }
            }
            if (!found) {
                err = "region not found: " + want;
                return false;
            }
            selected.push_back(found);
        }
    }

    // 3) 布局：手动 mosaic（slots）或自动 compute_layout
    int cols = 0;
    int rows = 0;
    // slot 语义：行优先摆放；元素 = key/id 或空串（空 cell）。同一 region 可摆多格。
    std::vector<std::pair<const MpcdiRegionInfo*, int>> placed;   // (region, cell idx)
    const bool mosaic = !opt.layout.slots.empty() && opt.layout.cols > 0;
    if (mosaic) {
        cols = opt.layout.cols;
        rows = opt.layout.rows > 0
            ? opt.layout.rows
            : static_cast<int>((opt.layout.slots.size() + cols - 1) / cols);  // 推出行数
        const int need = cols * rows;
        std::vector<std::string> slots = opt.layout.slots;
        while (slots.size() < static_cast<size_t>(need)) slots.emplace_back();
        for (int i = 0; i < need; ++i) {
            const std::string& s = slots[static_cast<size_t>(i)];
            if (s.empty()) continue;
            const MpcdiRegionInfo* found = nullptr;
            for (const MpcdiRegionInfo& r : info.regions) {
                if (r.id == s || r.key == s) {
                    found = &r;
                    break;
                }
            }
            if (!found) {
                err = "slot region not found: " + s;
                return false;
            }
            placed.emplace_back(found, i);
        }
        if (placed.empty()) {
            err = "mosaic has no region placed";
            return false;
        }
    } else {
        compute_layout(static_cast<int>(selected.size()), opt.layout, cols, rows);
        for (int i = 0; i < static_cast<int>(selected.size()); ++i) {
            placed.emplace_back(selected[static_cast<size_t>(i)], i);
        }
    }

    // 4) cell 尺寸 + alpha 预解码：xResolution 优先，缺失退 alpha 尺寸，兜底 1920x1080
    //    （对齐 bake.js；alpha 只解码一次，网格构建阶段复用缓存）
    int cell_w = 0;
    int cell_h = 0;
    std::map<std::string, AlphaImage> alpha_cache;
    for (const auto& [region, cell] : placed) {
        AlphaImage alpha;
        if (!region->alpha_map.empty()) {
            const auto* bytes = pkg.find(region->alpha_map, region->buffer_id);
            if (!bytes) {
                err = "alpha map not in package: " + region->alpha_map;
                return false;
            }
            std::string aerr;
            if (!decode_alpha_png(*bytes, opt.max_mask, alpha, aerr)) {
                err = "region " + region->id + ": " + aerr;
                return false;
            }
            const int aw = region->x_resolution > 0 ? region->x_resolution : alpha.width;
            const int ah = region->y_resolution > 0 ? region->y_resolution : alpha.height;
            cell_w = (std::max)(cell_w, aw);
            cell_h = (std::max)(cell_h, ah);
            alpha_cache[region->id] = std::move(alpha);
        } else {
            // 无 alphaMap：尺寸只能靠 XML 分辨率
            cell_w = (std::max)(cell_w, region->x_resolution);
            cell_h = (std::max)(cell_h, region->y_resolution);
        }
    }
    if (cell_w <= 0 || cell_h <= 0) {
        cell_w = 1920;
        cell_h = 1080;
    }

    // 5) 逐 region 生成网格（按放置顺序 = 绘制顺序）
    for (const auto& [region, cell] : placed) {
        RegionMesh mesh;
        const int cell_col = cell % cols;
        const int cell_row = cell / cols;
        if (!build_region_mesh(pkg, *region, opt, cell_col, cell_row,
                               cols, rows, alpha_cache, mesh, err)) {
            err = "build mesh failed (" + region->id + "): " + err;
            return false;
        }
        out.regions.push_back(std::move(mesh));
    }

    out.cols = cols;
    out.rows = rows;
    out.cell_w = cell_w;
    out.cell_h = cell_h;
    out.alpha_linearize = opt.alpha_linearize;
    out.bounds_overlay = opt.bounds_overlay;
    err.clear();
    return true;
}

}  // namespace ms
