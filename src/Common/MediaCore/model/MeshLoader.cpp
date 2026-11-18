// ============================================================================
//  MeshLoader.cpp —— Assimp 后端实现（见头文件语义约定）
// ============================================================================

#include "model/MeshLoader.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdio>
#include <system_error>

#include <assimp/DefaultIOStream.h>
#include <assimp/IOStream.hpp>
#include <assimp/IOSystem.hpp>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>

namespace ms {

namespace {

// ---------------------------------------------------------------------------
// MeshIOStream —— 只读 FILE* 流（Assimp::IOStream 最小实现）
//
// Assimp 自带 DefaultIOStream 构造 protected（只能经其 DefaultIOSystem
// 创建）——自定义 IOSystem 配套自写流包装：构造接管 FILE*，析构 fclose。
// aiOrigin 值与 stdio SEEK_SET/CUR/END（0/1/2）一致，直接透传。
// ---------------------------------------------------------------------------
class MeshIOStream final : public Assimp::IOStream {
public:
    explicit MeshIOStream(std::FILE* fp) : fp_(fp) {}
    ~MeshIOStream() override {
        if (fp_) std::fclose(fp_);
    }
    size_t Read(void* pvBuffer, size_t pSize, size_t pCount) override {
        return std::fread(pvBuffer, pSize, pCount, fp_);
    }
    size_t Write(const void*, size_t, size_t) override { return 0; } // 只读加载器
    aiReturn Seek(size_t pOffset, aiOrigin pOrigin) override {
        return std::fseek(fp_, (long)pOffset, (int)pOrigin) == 0
                   ? aiReturn_SUCCESS : aiReturn_FAILURE;
    }
    size_t Tell() const override { return (size_t)std::ftell(fp_); }
    size_t FileSize() const override {
        const long cur = std::ftell(fp_);
        std::fseek(fp_, 0, SEEK_END);
        const long end = std::ftell(fp_);
        std::fseek(fp_, cur, SEEK_SET);
        return (size_t)end;
    }
    void Flush() override { std::fflush(fp_); }

private:
    std::FILE* fp_;
};

// ---------------------------------------------------------------------------
// U8IOSystem —— UTF-8 窄串路径 IO（中文路径铁律）
//
// Assimp 默认 IOSystem 直接 fopen(const char*)——MSVC 按 ACP 解码，
// UTF-8 中文路径直接打不开。这里全部经 ms::u8Path() 落盘。
// ---------------------------------------------------------------------------
class U8IOSystem final : public Assimp::IOSystem {
public:
    bool Exists(const char* pFile) const override {
        std::error_code ec;
        return std::filesystem::exists(u8Path(pFile), ec);
    }
    char getOsSeparator() const noexcept override { return '/'; }

    void Close(Assimp::IOStream* pFile) override { delete pFile; }

    Assimp::IOStream* Open(const char* pFile, const char* pMode) override {
        std::FILE* fp = std::fopen(u8Path(pFile).string().c_str(), pMode);
        if (!fp) return nullptr;
        return new MeshIOStream(fp);
    }
};

// 小写扩展名提取（"dir/model.GLB" → "glb"；无后缀 = 空串）
std::string lowerExt(const std::string& path) {
    const size_t dot = path.find_last_of('.');
    if (dot == std::string::npos) return {};
    std::string ext = path.substr(dot + 1);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return ext;
}

} // namespace

bool loadMesh(const std::string& utf8_path, MeshData& out, std::string* err) {
    out = MeshData{};
    auto fail = [&](std::string msg) {
        if (err) *err = std::move(msg);
        return false;
    };

    // ---- 文件预检（懒加载发生在 UI 线程——超大文件直接拒绝，防卡死）----
    std::error_code ec;
    const auto fsize = std::filesystem::file_size(u8Path(utf8_path), ec);
    if (ec) return fail("cannot stat file");
    if (fsize == 0) return fail("empty file");
    if (fsize > 256ull * 1024 * 1024) return fail("file too large (>256MB)");

    // ---- Assimp 导入（层级烘焙 + 三角化；多子网格合并为单网格）----------
    Assimp::Importer imp;
    // 所有权契约【2026-11-18 崩溃实证】：5.4.3 的 SetIOHandler 会接管
    // handler 所有权（析构时 delete）——绝不可传静态/栈对象指针，否则
    // 对非堆内存执行 delete → 堆损坏（0xc0000374，时崩时不崩，LFH 随机
    // 化掩盖）。U8IOSystem 无状态，每次 new 成本可忽略。
    imp.SetIOHandler(new U8IOSystem());
    const unsigned flags = aiProcess_Triangulate         // 多边形 → 三角
                           | aiProcess_PreTransformVertices; // 节点层级烘焙
    const aiScene* scene = imp.ReadFile(utf8_path, flags);
    if (!scene || !scene->HasMeshes()) {
        return fail(std::string("assimp: ") +
                    (scene ? "no meshes in scene" : imp.GetErrorString()));
    }

    // UV v 方向归一（输出统一 v 向下 = 图像坐标；见头文件 UV 约定）：
    // OBJ/FBX/COLLADA 导出 v 向上 → 加载时翻 v=1−v；glTF 原生 v 向下 → 原样；
    // PLY/STL 常无 UV（缺失顶点 = (0,0)），翻不翻无观测。
    const std::string ext = lowerExt(utf8_path);
    const bool flip_v = (ext == "obj" || ext == "fbx" || ext == "dae");

    // ---- 顶点收集（先量后装；超大网格防御）----
    size_t v_total = 0, i_total = 0;
    for (unsigned m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh* mesh = scene->mMeshes[m];
        v_total += mesh->mNumVertices;
        i_total += mesh->mNumFaces * 3ull;
    }
    if (v_total > 10'000'000) return fail("too many vertices (>10M)");

    out.pos.reserve(v_total * 3);
    out.uv.reserve(v_total * 2);
    out.indices.reserve(i_total);

    for (unsigned m = 0; m < scene->mNumMeshes; ++m) {
        const aiMesh* mesh = scene->mMeshes[m];
        const aiVector3D* uvs =
            mesh->HasTextureCoords(0) ? mesh->mTextureCoords[0] : nullptr;
        const uint32_t base = (uint32_t)(out.pos.size() / 3);
        for (unsigned v = 0; v < mesh->mNumVertices; ++v) {
            const aiVector3D& p = mesh->mVertices[v];
            out.pos.insert(out.pos.end(), {p.x, p.y, p.z});
            if (uvs) {
                const float tu = uvs[v].x;
                const float tv = flip_v ? 1.0f - uvs[v].y : uvs[v].y;
                out.uv.insert(out.uv.end(), {tu, tv});
            } else {
                out.uv.insert(out.uv.end(), {0.0f, 0.0f});
            }
        }
        for (unsigned f = 0; f < mesh->mNumFaces; ++f) {
            const aiFace& face = mesh->mFaces[f];
            if (face.mNumIndices != 3) continue; // 三角化后仍非 3（点/线残留）
            for (unsigned k = 0; k < 3; ++k)
                out.indices.push_back(base + face.mIndices[k]);
        }
    }

    if (!out.valid()) {
        out = MeshData{};
        return fail("no valid triangles after triangulation");
    }

    // ---- 轴变换 Y-up→Z-up： (x,y,z) → (−x, z, y)（刚体旋转 det=+1）----
    // ---- 同时求包围盒（变换后）----
    double bb_min[3] = {1e300, 1e300, 1e300};
    double bb_max[3] = {-1e300, -1e300, -1e300};
    for (size_t i = 0; i < out.pos.size(); i += 3) {
        const float x = -out.pos[i + 0];
        const float y =  out.pos[i + 2]; // Assimp z → Flow y（画布 y）
        const float z =  out.pos[i + 1]; // Assimp y → Flow z（高度）
        out.pos[i + 0] = x;
        out.pos[i + 1] = y;
        out.pos[i + 2] = z;
        bb_min[0] = std::fmin(bb_min[0], x); bb_max[0] = std::fmax(bb_max[0], x);
        bb_min[1] = std::fmin(bb_min[1], y); bb_max[1] = std::fmax(bb_max[1], y);
        bb_min[2] = std::fmin(bb_min[2], z); bb_max[2] = std::fmax(bb_max[2], z);
    }

    // ---- 落地归一化：XY 中心 → 原点、底面贴 z=0 ------------------------
    const double cx = (bb_min[0] + bb_max[0]) * 0.5;
    const double cy = (bb_min[1] + bb_max[1]) * 0.5;
    const double dz = bb_min[2]; // 抬到 z=0 地面
    for (size_t i = 0; i < out.pos.size(); i += 3) {
        out.pos[i + 0] = (float)(out.pos[i + 0] - cx);
        out.pos[i + 1] = (float)(out.pos[i + 1] - cy);
        out.pos[i + 2] = (float)(out.pos[i + 2] - dz);
    }

    // 包围盒记录为【归一化后】舞台域值（placement 消费方直接可用）
    out.min[0] = bb_min[0] - cx; out.max[0] = bb_max[0] - cx;
    out.min[1] = bb_min[1] - cy; out.max[1] = bb_max[1] - cy;
    out.min[2] = 0.0;            out.max[2] = bb_max[2] - dz;

    // ---- 尺度归一化【2026-11-18 米制资产实测修正】------------------------
    // 模型文件单位五花八门（米制 DCC 导出为主），而舞台单位=画布 px——
    // 2m 立方体归一后仅 ±1 单位 ≈ 视口里一个点（用户实测"只有一个点"）。
    // "1:1 单位语义"对米制资产完全不可用，改为：最大 XY 边归一到
    // kModelTargetSize（与典型屏幕同量级），placement scale 仍可再调。
    // 两端共用本加载器，display 3B 自动一致。
    constexpr double kModelTargetSize = 600.0;
    const double ex = out.max[0] - out.min[0];
    const double ey = out.max[1] - out.min[1];
    const double max_edge = std::max(ex, ey);
    if (max_edge > 1e-9 && std::abs(max_edge - kModelTargetSize) > 1e-6) {
        const float k = (float)(kModelTargetSize / max_edge);
        for (size_t i = 0; i < out.pos.size(); i += 3) {
            out.pos[i + 0] *= k;
            out.pos[i + 1] *= k;
            out.pos[i + 2] *= k;
        }
        out.min[0] *= k; out.max[0] *= k;
        out.min[1] *= k; out.max[1] *= k;
        out.min[2] *= k; out.max[2] *= k;
    }
    return true;
}

} // namespace ms
