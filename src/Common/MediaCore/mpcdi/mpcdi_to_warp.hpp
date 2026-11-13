#pragma once

// ============================================================================
// MpcdiGeometry → WarpSpec(mode=2) 转换（M4 收编自 MMC 的投递适配层）。
//
// MMC 模型：region mesh 逐个直接画视频纹理（每 region 独立 VAO + alpha
// 纹理）；MediaServer 模型：两段式呈现（FBO 合成 → 单个输出网格重映射），
// 因此必须把多个 region 的 mesh 拼接成一张规则大网格：
//
//   * 统一 cell 密度：(sw, sh) = 各 region mesh 尺寸的最大值（钳
//     [2,256]）；密度不足的 region 顶点场重采样补齐（pos/alpha 双线性
//     ——几何与羽化渐变连续；uv 最近邻——warp 场不做混合插值，无效区
//     边界保持锐利）。
//   * 布局：大网格 cols = geo.cols * sw，rows = geo.rows * sh；region
//     按 (cell_col, cell_row) 落位；空 cell 与无效顶点 pos=uv=(-1,-1)、
//     alpha=0（渲染端 discard → 露黑底，对齐 MPCDI "NaN 渲染黑"）。
//   * uv 语义：RegionVertex.uv 是 buffer 归一化坐标（视觉 y 向下）——
//     与 WarpSpec 的"视觉 y 向下"注入约定一致，GL 纹理 v 轴翻转由渲染
//     端 buildWarpMesh 统一处理（此处不翻）。
//
// 转换为纯函数（无 GL / 无线程），输入输出均可单测对拍。
// ============================================================================

#include "engine/IComposeRenderer.hpp"

#include "region_data.hpp"

namespace ms {

/// MpcdiGeometry → WarpSpec(mode=2)。失败（空 regions / 全空网格）返回
/// inactive 的 WarpSpec（mode=0）。
WarpSpec mpcdi_to_warp(const MpcdiGeometry& geo);

}  // namespace ms
