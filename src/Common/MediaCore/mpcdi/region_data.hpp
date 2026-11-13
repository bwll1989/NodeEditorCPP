#pragma once

#include <cstdint>
#include <vector>

namespace ms {

/**
 * MPCDI 几何数据（ms 渲染输入）。
 * 由 mpcdi 解析模块（src/mpcdi/）从 .mpcdi 包构造，语义见 docs/phase1-mpcdi-render.md。
 *
 * 顶点坐标语义（以 v26 实证为准，2026-10-29 用 test1014 真实包数据验证）：
 *  - geometryWarpFile 的 PFM 内容 = buffer 归一化坐标（该屏幕像素应采样视频的位置），
 *    NaN 表示该屏幕位置无有效映射（渲染为黑）；
 *  - 顶点域 = 屏幕（cell 内）网格：pos 烘焙为整幅画布的归一化坐标，
 *    uv = PFM 内容（buffer 坐标，直接采样视频纹理）。
 */

/** 单 region 顶点：画布归一化位置 + buffer 归一化 UV。 */
struct RegionVertex {
    float pos_x = 0.0f, pos_y = 0.0f;  // 画布归一化坐标（cell 内屏幕网格 + cell 偏移烘焙）
    float uv_x = 0.0f, uv_y = 0.0f;    // buffer 归一化坐标（PFM 内容；无效映射为 -1，渲染黑）
};

struct RegionMesh {
    std::vector<RegionVertex> vertices;
    std::vector<uint32_t> indices;     // 网格三角化索引（(W-1)*(H-1)*6 个）

    // 网格尺寸（顶点按 W x H 行主序排布；等于 PFM 降采样后的尺寸）
    int mesh_w = 0;
    int mesh_h = 0;

    // alpha 数据（RGBA8；无 alphaMap 时为 8x8 全 1 占位，对齐 bake.js identityRgba）
    std::vector<uint8_t> alpha_rgba;
    int alpha_width = 0;
    int alpha_height = 0;
    float alpha_gamma = 0.0f;          // XML gammaEmbedded（配合 MpcdiGeometry.alpha_linearize）

    // region 在画布中的 cell 位置（computeLayout mosaic 行列）
    int cell_col = 0;
    int cell_row = 0;
};

struct MpcdiGeometry {
    int cols = 0, rows = 0;            // mosaic 布局
    int cell_w = 0, cell_h = 0;        // 单屏分辨率（region XML x/yResolution，缺失退 alpha 尺寸）
    bool alpha_linearize = false;      // v26 alpha_linearize 开关：true 时 shader 内 pow(alpha, gamma)
    bool bounds_overlay = false;       // viewer "Region bounds overlay"：region 边缘绿带 + 无效区琥珀
    std::vector<RegionMesh> regions;
};

}  // namespace ms
