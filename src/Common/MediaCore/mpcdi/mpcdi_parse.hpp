#pragma once

// ============================================================================
// MPCDI 包解析（.mpcdi zip 包 → MpcdiGeometry）——M3 顶点网格渲染的数据入口。
//
// 语义逐条对齐 v26 浏览器端烘焙器 web/mpcdi_bake.js（黄金参考，变形效果经
// v24~v26 多轮实测校准）。迁移映射：
//   loadZip/findInZip      → mpcdi_zip.cpp（minizip 只读解包 + 容错查找）
//   parseMpcdiXml          → mpcdi_xml.cpp（tinyxml2）
//   parsePFM/resizeUv      → mpcdi_pfm.cpp（双字节序 / bottom-up / NaN / 双线性降采样）
//   looksAbsolute/normalizeUv → mpcdi_pfm.cpp
//   readPngRgba/identityRgba  → mpcdi_alpha.cpp（stb_image）
//   computeLayout/bake 总装   → mpcdi_build.cpp（顶点网格替代 shader 烘焙）
//
// zip 组件来自 MPCDI 官方参考实现（Scalable Display）的 ThirdParty：
// minizip + zlib 1.2.7 + tinyxml2，纯 C/C++ 源码、许可无忧、Android NDK 可直接编译。
// ============================================================================

#include "region_data.hpp"

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace ms {

// ---------------------------------------------------------------------------
// 包内容：zip 解包后的文件字典（键 = 包内路径，正斜杠、无前导斜杠）。
// ---------------------------------------------------------------------------

struct MpcdiPackageData {
    std::map<std::string, std::vector<std::uint8_t>> files;

    bool empty() const { return files.empty(); }

    /**
     * 容错查找（对齐 bake.js findInZip）：
     *   1. 路径原样（正斜杠归一化后）；
     *   2. <buffer_id>/<rel>；
     *   3. <buffer_id>/<rel 基名>；
     *   4. 全表大小写不敏感的路径后缀 / 基名匹配。
     * 找不到返回 nullptr。
     */
    const std::vector<std::uint8_t>* find(const std::string& rel,
                                          const std::string& buffer_id) const;
};

/** 从磁盘 .mpcdi（zip）读入全部文件（minizip；DEFLATE/STORE 均支持）。 */
bool load_mpcdi_package(const std::string& utf8_path, MpcdiPackageData& out, std::string& err);

// ---------------------------------------------------------------------------
// XML 解析（mpcdi.xml）：display/buffer/region 结构 + fileset 资源路径。
// ---------------------------------------------------------------------------

struct MpcdiRegionInfo {
    std::string id;              // region id（如 "region_0"）
    std::string key;             // display/buffer/region 三段组合键（v26 选择语义）
    std::string display_id;
    std::string buffer_id;
    int x_resolution = 0;        // 单屏分辨率（XML region 属性，构造 cell 尺寸用）
    int y_resolution = 0;
    float x = 0.0f, y = 0.0f;    // region 子矩形（v26 region_local 选项用，本核心未启用）
    float x_size = 1.0f, y_size = 1.0f;
    std::string geometry_warp_file;  // PFM 路径（相对包根，可能带 buffer 目录前缀差异）
    std::string alpha_map;           // PNG 路径；空 = 无 alphaMap
    std::string beta_map;            // 路径；空 = 无 betaMap（本核心第一阶段不消费 beta）
    float alpha_gamma = 0.0f;        // XML gammaEmbedded；无 alphaMap 时无意义
};

struct MpcdiBufferInfo {
    std::string id;
    int x_resolution = 0;        // XML buffer 属性（metadata 面板显示用）
    int y_resolution = 0;
};

struct MpcdiInfo {
    std::string profile;         // "2d" 等
    std::string version;
    std::vector<MpcdiBufferInfo> buffers;  // XML buffer 结构（metadata 展示用）
    std::vector<MpcdiRegionInfo> regions;  // XML 顺序
};

/** 解析包内 mpcdi.xml（对齐 bake.js parseMpcdiXml，含 fileset 索引回填）。 */
bool parse_mpcdi_xml(const MpcdiPackageData& pkg, MpcdiInfo& out, std::string& err);

// ---------------------------------------------------------------------------
// PFM（geometryWarpFile）解析与 warp 坐标归一化。
// ---------------------------------------------------------------------------

/** 解析后的 PFM：uvs 为行主序 (y*W+x)，每个元素 (u,v)；NaN 保留（无效映射标志）。 */
struct PfmData {
    int width = 0, height = 0;
    int channels = 0;            // "PF"=3（u,v,disparity）；"Pf"=1（仅 u）
    std::vector<float> data;     // 行主序，已翻转为视觉顶起（顶行 = y 0）
};

/** 解析 PFM 字节流（对齐 bake.js parsePFM：scale 符号定字节序、bottom-up 行序翻转）。 */
bool parse_pfm(const std::vector<std::uint8_t>& bytes, PfmData& out, std::string& err);

/**
 * 双线性降采样到 maxSide 内（对齐 bake.js resizeUv：中心对齐采样、边缘钳制）。
 * 输出 uvs 尺寸 = nw*nh，NaN 自然传播（对齐 JS 行为）。
 */
struct PfmResized {
    int width = 0, height = 0;
    std::vector<float> uvs;      // (u,v) 对，行主序
};

void resize_uv(const PfmData& pfm, int max_side, PfmResized& out);

/** warp 值域是否为绝对像素坐标（对齐 bake.js looksAbsolute：跳过 NaN，越出 [0,1.001] 即真）。 */
bool looks_absolute(const PfmResized& r);

/**
 * warp 全分辨率统计（对齐 viewer computeWarpStats；metadata 面板用）。
 * NaN 单独计数不参与值域；looks_normalized = 全部有限值落在 [0,1] 容差内。
 */
struct MpcdiWarpStats {
    int width = 0, height = 0;
    float min_u = 0.0f, max_u = 0.0f;
    float min_v = 0.0f, max_v = 0.0f;
    long long nan_count = 0;
    long long total_texels = 0;
    bool looks_normalized = false;
};

bool compute_warp_stats(const std::vector<std::uint8_t>& pfm_bytes, MpcdiWarpStats& out,
                        std::string& err);

/** PNG 头部尺寸提取（IHDR 宽高，不解码整图；metadata "blend size" 用）。 */
bool png_header_size(const std::vector<std::uint8_t>& png_bytes, int& w, int& h);

/**
 * warp 坐标归一化（对齐 bake.js normalizeUv 四分支）：
 *   NaN                     → (-1,-1)（渲染黑）
 *   已在 [-0.05,1.05]       → clamp 到 [0,1]
 *   绝对像素（absW/absH>1）  → 除以 absW/absH
 *   [-1.05,1.05]            → 映射 [-1,1] → [0,1]
 *   兜底                     → 除 absW/absH（若 >1）或原样
 */
void normalize_uv(float u, float v, float abs_w, float abs_h, float out[2]);

// ---------------------------------------------------------------------------
// alpha（alphaMap PNG）解码。
// ---------------------------------------------------------------------------

/** RGBA8 图像。 */
struct AlphaImage {
    int width = 0, height = 0;
    std::vector<std::uint8_t> rgba;
};

/** 解码 PNG（stb_image）并降采样到 maxSide 内；失败返回 false。 */
bool decode_alpha_png(const std::vector<std::uint8_t>& png_bytes, int max_side,
                      AlphaImage& out, std::string& err);

/** 无 alphaMap 时的全 1 占位（对齐 bake.js identityRgba 8x8 白）。 */
AlphaImage identity_alpha();

// ---------------------------------------------------------------------------
// 布局与几何总装。
// ---------------------------------------------------------------------------

/** 布局模式（对齐 bake.js computeLayout；slots 为手动 mosaic 摆放）。 */
struct MpcdiLayoutOpt {
    std::string mode = "grid";       // grid/row/column/cols2/cols3/cols4
    int cols = 0;                    // grid 模式手动列数（0 = ceil(sqrt(n)) 自动）
    int rows = 0;                    // mosaic 模式手动行数（0 = 由 slots.size()/cols 推出）
    std::vector<std::string> slots;  // 手动 mosaic：按行优先摆放的 region key/id（空串=空 cell）
};

struct MpcdiBuildOptions {
    MpcdiLayoutOpt layout;             // 布局模式与 mosaic 摆放
    std::vector<std::string> regions;  // 参与拼接的 region（key 或 id；key 优先）
    int max_warp = 256;                // warp PFM 降采样上限（对齐 bake.js maxWarp 缺省 256）
    int max_mask = 512;                // alphaMap 降采样上限（对齐 bake.js maxMask 缺省 512）
    bool alpha_linearize = false;      // pow 线性化开关（v26 同名选项）
    bool force_absolute = false;       // 强制按绝对像素坐标解释 warp（调试用）
    bool force_normalized = false;     // 强制按归一化坐标解释 warp（调试用）
    bool flip_y = false;               // 对齐 v26 flip_y：warp 输出的 buffer uv.y 取反（源内容上下镜像）
    bool flip_screen_y = false;        // 对齐 v26 flip_screen_y：warp 查表 y 镜像（变形整体上下翻转）
    bool region_local = false;         // 对齐 v26 region_local：uv = (uv - region.xy) / region.size
    bool blend_enabled = true;         // viewer "Blend"：false 时不乘 alphaMap（用全 1 占位）
    bool bounds_overlay = false;       // viewer "Region bounds overlay"：region 边缘绿带 + 无效区琥珀高亮
};

/**
 * 总装：解析包 + 选择 region + 布局 + 生成顶点网格。
 * 失败返回 false 并填 err（包损坏 / region 不存在 / PFM 解析失败等）。
 *
 * 与 v26 的差异（有意为之）：
 *  - 无 geometryWarpFile 的 region 退化为单位网格（pos=uv 直通），v26 直接报错；
 *  - betaMap 第一阶段不消费（当前产品无 beta 数据需求，字段仍解析保留）；
 *  - 无中间画布/最大纹理 8192 缩放（顶点网格直接画窗口，无该限制）。
 */
bool build_mpcdi_geometry(const std::string& utf8_package_path,
                          const MpcdiBuildOptions& opt,
                          MpcdiGeometry& out, std::string& err);

/** 便捷重载：已持有解包数据时使用（跳过 zip 解包）。 */
bool build_mpcdi_geometry(const MpcdiPackageData& pkg,
                          const MpcdiBuildOptions& opt,
                          MpcdiGeometry& out, std::string& err);

}  // namespace ms
