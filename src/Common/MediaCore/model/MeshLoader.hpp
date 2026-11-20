// ============================================================================
//  MeshLoader.hpp —— 3D 模型文件 → 舞台空间 MeshData（Assimp 后端）
//
//  3D mapping 阶段 3A（2026-11-18）：3D 模型进舞台。编辑器 3D 视口与
//  display 投影渲染两端共用本加载器——同一文件恒得同一顶点数据（跨端
//  一致根基，同 ScreenMapper 纯函数纪律）。
//
//  解析后端（2026-11-18 定案）：Assimp 5.4 静态库（3rdparty/assimp-src，
//  独立编译于 build/assimp）。格式按扩展名自动识别——主流覆盖：
//    obj / glb / gltf / fbx / dae / ply / stl（导入器按需裁编，见
//    build/assimp 配置）；其余格式 Assimp 未编译即报"不支持"。
//  前身 OBJLoader.hpp（手写 v/vt/f 最小集）由本文件取代后退役：
//    * OBJ 解析改走 Assimp（多边形三角化/负索引/索引越界语义由
//      aiProcess_Triangulate + importer 内部处理，行为对齐主流建模
//      软件导出预期）；
//    * 节点层级经 aiProcess_PreTransformVertices 烘焙为世界坐标——
//      "模型 = 一个投影表面"语义（多子网格/装配体合并为单网格）。
//
//  坐标约定（两端一致的关键，勿在消费端再变换）：
//    * Assimp 输出 Y-up 右手（主流 DCC 导出惯例）；Flow 舞台空间
//      Z-up（画布 XY 平面 + Z 高度朝 Eye）。加载时轴变换
//      (x,y,z) → (−x, z, y)——含 x 取反的刚体旋转（det=+1，非镜像；
//      标准游戏资产 Y-up→Z-up 惯例）。
//    * 归一化（同样在加载内完成）：包围盒 XY 中心平移到原点、底面
//      （min Z）贴 z=0 地面——模型"落地在画布原点"，摆位交给 Screen
//      placement（offset/scale），建模原点差异不再困扰对位。
//    * 单位 = 舞台单位 = 画布 px（1:1）；模型实际尺寸差异由 placement
//      scale 调节（ScreenPropertyDialog 暴露）。
//  UV 约定（3B 投影纹理消费前定死）：输出统一【v 向下 = 图像坐标】
//    （内容画面 QImage row0=顶，采样即图像坐标）：OBJ/FBX/COLLADA 的
//    v 向上 UV 加载时翻 v=1−v；glTF（v 向下）原样透传；无 UV 顶点 = (0,0)。
//
//  路径编码：入参恒为 UTF-8 窄串（同 Asset.path 约定）。Assimp 默认
//  IOSystem 走 fopen（MSVC 按 ACP 解码，中文路径直接挂）——实现内置
//  U8IOSystem（经 ms::u8Path 打开；中文路径铁律，见 runtime/AssetPath.hpp）。
// ============================================================================

#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "runtime/AssetPath.hpp" // u8Path：中文路径防御（铁律必经）

namespace ms {

// ---------------------------------------------------------------------------
// MeshData —— 三角网格（舞台空间顶点；加载即含轴变换+归一化）
// ---------------------------------------------------------------------------
struct MeshData {
    std::vector<float>    pos;      ///< xyz 三 float/顶点（舞台域：画布 px + Z 向上）
    std::vector<float>    uv;       ///< uv 两 float/顶点（统一 v 向下=图像坐标；
                                    ///< 缺失顶点 = (0,0)）
    std::vector<uint32_t> indices;  ///< 三角索引（3 的倍数）
    double min[3] = {0, 0, 0};      ///< 归一化后包围盒（舞台域；placement 参考）
    double max[3] = {0, 0, 0};

    bool valid() const noexcept {
        return !pos.empty() && !indices.empty() &&
               pos.size() % 3 == 0 && uv.size() == pos.size() / 3 * 2 &&
               indices.size() % 3 == 0;
    }
    size_t vertexCount() const noexcept { return pos.size() / 3; }
    size_t triangleCount() const noexcept { return indices.size() / 3; }
};

// ---------------------------------------------------------------------------
// loadMesh —— 解析模型文件填入 mesh（含轴变换 Y-up→Z-up + 落地归一化）
//
// \param utf8_path  UTF-8 窄串路径（Asset.path 原文；绝对/相对由调用方
//                   先经 assetResolvePath 解析）
// \param out        输出网格（先 clear）
// \param err        可空；失败原因（诊断用）
// \return true = 成功且网格有效（至少 1 个顶点 1 个三角）
// ---------------------------------------------------------------------------
bool loadMesh(const std::string& utf8_path, MeshData& out,
              std::string* err = nullptr);

} // namespace ms
