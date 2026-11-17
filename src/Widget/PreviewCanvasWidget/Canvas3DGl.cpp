// ============================================================================
//  Canvas3DGl.cpp —— 3D 视口贴图层实现（shader + 单应解算 + 纹理缓存直传）
// ============================================================================

#include "Canvas3DGl.hpp"

#include <QPointF>

#include <cmath>
#include <cstdio>
#include <cstring>

#include "GlLoader.hpp"

namespace ms {

namespace {

// ---------------------------------------------------------------------------
// 着色器（GLSL 330 core）
// ---------------------------------------------------------------------------

/// 顶点着色器：预投影四边形 + 透视 w。
/// a_pos.xy = 单应分子（屏幕逻辑坐标 × W），a_pos.z = 分母 W（透视分量）
/// ——clip = (N·kx − W, −N·ky + W, 0, W)，光栅端 /W 插值逐点复现单应映射
///（细节见头文件"数学"节）。u_scale=(2/W_l, −2/H_l)：逻辑坐标→NDC 系数，
/// y 取负 = Qt 顶向下 → GL 底向上翻转（与 GlRenderer 顶点构造同一语义）。
constexpr char kVertexShader[] = R"(
#version 330 core
layout(location = 0) in vec3 a_pos;   // xy=单应分子（逻辑坐标×W） z=分母 W
layout(location = 1) in vec2 a_uv;    // 帧纹理坐标（单位方）
uniform vec2 u_scale;                 // 逻辑尺寸→NDC：(2/W, -2/H)
out vec2 v_uv;
void main() {
    vec2 ndc = a_pos.xy * u_scale + vec2(-1.0, 1.0) * a_pos.z;
    gl_Position = vec4(ndc, 0.0, a_pos.z);
    v_uv = a_uv;
}
)";

/// 片元着色器：纹理采样 + 不透明度。uv 由光栅透视校正插值（四角恒在
/// [0,1] 域内，无需出界 discard——CLAMP_TO_EDGE 兜底插值浮点抖动）。
/// out.a = c.a·opa（glBlendFuncSeparate 的 A 链——与 GlRenderer /
/// SoftRenderer 同公式，Qt 标注层叠加语义一致）。
constexpr char kFragmentShader[] = R"(
#version 330 core
in vec2 v_uv;
uniform sampler2D u_tex;
uniform float u_opacity;
out vec4 fragColor;
void main() {
    vec4 c = texture(u_tex, v_uv);
    fragColor = vec4(c.rgb, c.a * u_opacity);
}
)";

/// 着色器编译（失败打 infoLog；同 GlRenderer 惯例）
bool compileShader(unsigned int type, const char* src, unsigned int& out) {
    out = gl::glCreateShader(type);
    gl::glShaderSource(out, 1, &src, nullptr);
    gl::glCompileShader(out);
    int ok = 0;
    gl::glGetShaderiv(out, gl::GL_COMPILE_STATUS, &ok);
    if (!ok) {
        char log[1024] = "";
        gl::glGetShaderInfoLog(out, sizeof(log), nullptr, log);
        std::fprintf(stderr, "[canvas3d-gl] 着色器编译失败: %s\n", log);
        std::fflush(stderr);
        return false;
    }
    return true;
}

} // namespace

// ---------------------------------------------------------------------------
// ensure —— 惰性初始化（GL 函数表 + 程序 + VAO/VBO；上下文 current 时调用）
// ---------------------------------------------------------------------------
bool Canvas3DGl::ensure() {
    if (failed_) return false;
    if (inited_) return true;

    // GL 函数表（Qt 上下文 current，wglGetProcAddress 可用；进程级一次）
    static bool s_loaded = false;
    if (!s_loaded) {
        if (!gl::loadGl()) {
            std::fprintf(stderr, "[canvas3d-gl] GL 函数表加载失败——3D 贴图"
                                 "回退 QPainter（会话内永久）\n");
            std::fflush(stderr);
            failed_ = true;
            return false;
        }
        s_loaded = true;
    }

    // ---- 程序 --------------------------------------------------------------
    unsigned int vs = 0, fs = 0;
    if (!compileShader(gl::GL_VERTEX_SHADER, kVertexShader, vs)) {
        failed_ = true;
        return false;
    }
    if (!compileShader(gl::GL_FRAGMENT_SHADER, kFragmentShader, fs)) {
        gl::glDeleteShader(vs);
        failed_ = true;
        return false;
    }
    program_ = gl::glCreateProgram();
    gl::glAttachShader(program_, vs);
    gl::glAttachShader(program_, fs);
    gl::glLinkProgram(program_);
    gl::glDeleteShader(vs); // 链接后着色器对象可删
    gl::glDeleteShader(fs);
    int ok = 0;
    gl::glGetProgramiv(program_, gl::GL_LINK_STATUS, &ok);
    if (!ok) {
        char log[1024] = "";
        gl::glGetProgramInfoLog(program_, sizeof(log), nullptr, log);
        std::fprintf(stderr, "[canvas3d-gl] 程序链接失败: %s\n", log);
        std::fflush(stderr);
        failed_ = true;
        return false;
    }
    // uniform location 一次性缓存（链接后固定；热路径零查找）
    loc_scale_    = gl::glGetUniformLocation(program_, "u_scale");
    loc_opacity_  = gl::glGetUniformLocation(program_, "u_opacity");
    loc_tex_      = gl::glGetUniformLocation(program_, "u_tex");
    gl::glUseProgram(program_);
    gl::glUniform1i(loc_tex_, 0); // 采样器绑定单元 0（一次性）
    gl::glUseProgram(0);

    // ---- VAO/VBO ------------------------------------------------------------
    // attrib 配置每 draw 幂等重配（同上下文 QPainter 会改写 VAO 内 attrib
    // 状态且 bind 不恢复——GlRenderer::drawQuad 二次 paint 黑屏教训）
    gl::glGenVertexArrays(1, &vao_);
    gl::glGenBuffers(1, &vbo_);

    inited_ = true;
    return true;
}

// ---------------------------------------------------------------------------
// drawClipFace —— 缓存上传 + 单应解算 + 绘制（一气呵成；上下文 current）
// ---------------------------------------------------------------------------
bool Canvas3DGl::drawClipFace(const std::string& key, const TexData& d,
                              const double sx[4], const double sy[4],
                              float opacity) {
    if (failed_ || !ensure()) return false;
    if (lw_ <= 0 || lh_ <= 0) return false;
    if (d.width <= 0 || d.height <= 0) return false;
    const bool is_dxt = d.dxt != nullptr; // DXT 优先（HAP 零解压直传）
    if (!is_dxt && d.rgba == nullptr) return false; // 双域皆空 → 占位

    // ---- 纹理确保（存储命中复用；尺寸/格式变化重建）------------------------
    // 容量逐出（>64 项清最久未用）：clip 删除后缓存残留纹理的 GPU 泄漏防御
    if (cache_.size() > 64) {
        auto oldest = cache_.end();
        for (auto it = cache_.begin(); it != cache_.end(); ++it) {
            if (it->first == key) continue; // 本帧仍要用，不逐出
            if (oldest == cache_.end() ||
                it->second.last_used < oldest->second.last_used)
                oldest = it;
        }
        if (oldest != cache_.end()) {
            if (oldest->second.tex != 0)
                gl::glDeleteTextures(1, &oldest->second.tex);
            cache_.erase(oldest);
        }
    }
    TexSlot& s = cache_[key];
    s.last_used = frame_no_;
    const unsigned int fmt = is_dxt ? d.dxt_format : 0;
    unsigned int tex = s.tex;
    bool rebuilt = false; // 重建帧已随 TexImage2D 带首帧数据（DXT 路径）
    if (s.tex == 0 || s.w != d.width || s.h != d.height || s.fmt != fmt) {
        // （重）建存储：DXT 压缩存储与 RGBA 可变存储互斥不可原地切换，
        // 对象级重建最简（尺寸随片段定，播放中恒定——低频路径）
        if (s.tex != 0) gl::glDeleteTextures(1, &s.tex);
        s.tex = 0;
        gl::glGenTextures(1, &s.tex);
        gl::glBindTexture(gl::GL_TEXTURE_2D, s.tex);
        // 采样参数（双线性 + 边缘钳制；与 GlRenderer 帧纹理同语义）
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MIN_FILTER,
                            gl::GL_LINEAR);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_MAG_FILTER,
                            gl::GL_LINEAR);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_S,
                            gl::GL_CLAMP_TO_EDGE);
        gl::glTexParameteri(gl::GL_TEXTURE_2D, gl::GL_TEXTURE_WRAP_T,
                            gl::GL_CLAMP_TO_EDGE);
        if (is_dxt) {
            const int blocks_x = (d.width + 3) / 4;
            const int blocks_y = (d.height + 3) / 4;
            const int bytes_per_block =
                fmt == gl::GL_COMPRESSED_RGB_S3TC_DXT1 ? 8 : 16;
            const int image_bytes =
                blocks_x * blocks_y * bytes_per_block;
            // S3TC 运行时探测（首个 DXT 上传）：隔离错误队列后查错——
            // INVALID_ENUM/VALUE = 驱动不支持，dxt_dead_ 永久跳过
            //（对齐 GlRenderer CpuDxt 段语义；桌面 GL S3TC 标配）
            const bool probing = !dxt_probed_;
            if (probing) {
                while (gl::glGetError() != gl::GL_NO_ERROR) {}
            }
            gl::glCompressedTexImage2D(gl::GL_TEXTURE_2D, 0, fmt,
                                       d.width, d.height, 0,
                                       image_bytes, d.dxt);
            if (probing) {
                dxt_probed_ = true;
                const unsigned int err = gl::glGetError();
                if (err != gl::GL_NO_ERROR) {
                    dxt_dead_ = true;
                    std::fprintf(stderr,
                                 "[canvas3d-gl] DXT 直传失败(err=0x%X "
                                 "%dx%d fmt=0x%X)——S3TC 不支持？"
                                 "回退 QPainter\n",
                                 err, d.width, d.height, fmt);
                    std::fflush(stderr);
                    return false;
                }
            }
        } else {
            gl::glTexImage2D(gl::GL_TEXTURE_2D, 0, gl::GL_RGBA,
                             d.width, d.height, 0,
                             gl::GL_RGBA, gl::GL_UNSIGNED_BYTE, nullptr);
        }
        s.w   = d.width;
        s.h   = d.height;
        s.fmt = fmt;
        rebuilt = true;
    }
    tex = s.tex;

    // ---- 内容上传（每帧全量；视频帧逐帧变化本就必须，图片与 QPainter
    //      路径逐帧 raster 上传同成本——内容哈希脏检查留待后续）------------
    if (!(rebuilt && is_dxt)) { // DXT 重建帧已随 TexImage2D 带数据
        gl::glActiveTexture(gl::GL_TEXTURE0);
        gl::glBindTexture(gl::GL_TEXTURE_2D, tex);
        if (is_dxt) {
            if (dxt_dead_) return false; // 已判死：快速失败（日志已打）
            // 压缩块几何：4×4 像素/块；imageSize 必须 = 块行数×块列数×
            // 块字节【精确匹配】——错传像素行数（d.height）时 GL 判
            // INVALID_VALUE 整次拒绝更新（首版实证：HAP 首帧后画面冻结）
            const int blocks_x = (d.width + 3) / 4;
            const int blocks_y = (d.height + 3) / 4;
            const int bytes_per_block =
                fmt == gl::GL_COMPRESSED_RGB_S3TC_DXT1 ? 8 : 16;
            gl::glCompressedTexSubImage2D(gl::GL_TEXTURE_2D, 0, 0, 0,
                                          d.width, d.height, fmt,
                                          blocks_x * blocks_y * bytes_per_block,
                                          d.dxt);
            // 【首帧冻结防御】错误粘滞排查：上传被拒时清错误位并回退
            //（否则错误粘滞污染后续 glGetError 探测）
            const unsigned int err = gl::glGetError();
            if (err != gl::GL_NO_ERROR) {
                static bool s_subimg_logged = false;
                if (!s_subimg_logged) {
                    s_subimg_logged = true;
                    // 一次性日志：参数错而非 S3TC 缺失——修数据源而非判死
                    std::fprintf(stderr,
                                 "[canvas3d-gl] DXT SubImage 被拒(err=0x%X "
                                 "%dx%d fmt=0x%X)——检查块几何\n",
                                 err, d.width, d.height, fmt);
                    std::fflush(stderr);
                }
            }
        } else {
            // 行距解耦（QImage bytesPerLine 允许 ≠ width*4；当前恒等防御性设置）
            gl::glPixelStorei(gl::GL_UNPACK_ROW_LENGTH,
                              static_cast<int>(d.stride / 4));
            gl::glTexSubImage2D(gl::GL_TEXTURE_2D, 0, 0, 0,
                                d.width, d.height,
                                gl::GL_RGBA, gl::GL_UNSIGNED_BYTE, d.rgba);
            gl::glPixelStorei(gl::GL_UNPACK_ROW_LENGTH, 0);
        }
    }

    // ---- 单应解算：单位方 → 屏幕四角（TL TR BR BL ↔ uv 同序）--------------
    double h[8] = {};
    const double dst[8] = {sx[0], sy[0], sx[1], sy[1],
                           sx[2], sy[2], sx[3], sy[3]};
    if (!solveHomography(dst, h)) return false; // 退化四边形 → 占位

    float verts[6 * 5]; // 6 顶点 × {Nx, Ny, W, u, v}
    static constexpr int    kIdx[6] = {0, 1, 2, 0, 2, 3}; // TL TR BR / TL BR BL
    static constexpr double kUv[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    for (int k = 0; k < 6; ++k) {
        const int    i = kIdx[k];
        const double u = kUv[i][0], v = kUv[i][1];
        const double w = h[6] * u + h[7] * v + 1.0;
        if (!(w > 1e-6) || !std::isfinite(w))
            return false; // 翻转/贴焦平面 → 回退 QPainter（宁缺勿错）
        const double nx = h[0] * u + h[1] * v + h[2];
        const double ny = h[3] * u + h[4] * v + h[5];
        if (!std::isfinite(nx) || !std::isfinite(ny)) return false;
        verts[k * 5 + 0] = static_cast<float>(nx);
        verts[k * 5 + 1] = static_cast<float>(ny);
        verts[k * 5 + 2] = static_cast<float>(w);
        verts[k * 5 + 3] = static_cast<float>(u);
        verts[k * 5 + 4] = static_cast<float>(v);
    }

    // ---- 状态与绘制（每 draw 幂等重配；零依赖残留状态）---------------------
    gl::glViewport(0, 0,
                   static_cast<int>(static_cast<float>(lw_) * dpr_),
                   static_cast<int>(static_cast<float>(lh_) * dpr_));
    gl::glDisable(gl::GL_DEPTH_TEST);
    gl::glDisable(gl::GL_CULL_FACE);
    gl::glDisable(gl::GL_SCISSOR_TEST);
    gl::glDisable(gl::GL_STENCIL_TEST);
    gl::glColorMask(1, 1, 1, 1); // GL_TRUE（GlLoader 常量表无此枚举）
    gl::glEnable(gl::GL_BLEND);
    gl::glBlendFuncSeparate(gl::GL_SRC_ALPHA, gl::GL_ONE_MINUS_SRC_ALPHA,
                            gl::GL_ONE, gl::GL_ONE_MINUS_SRC_ALPHA);
    gl::glBindVertexArray(vao_);
    gl::glBindBuffer(gl::GL_ARRAY_BUFFER, vbo_);
    gl::glEnableVertexAttribArray(0); // a_pos（3 float，偏移 0）
    gl::glVertexAttribPointer(0, 3, gl::GL_FLOAT, gl::GL_FALSE,
                              5 * sizeof(float),
                              reinterpret_cast<const void*>(0));
    gl::glEnableVertexAttribArray(1); // a_uv（2 float，偏移 12 字节）
    gl::glVertexAttribPointer(1, 2, gl::GL_FLOAT, gl::GL_FALSE,
                              5 * sizeof(float),
                              reinterpret_cast<const void*>(12));
    gl::glBufferData(gl::GL_ARRAY_BUFFER, sizeof(verts), verts,
                     gl::GL_DYNAMIC_DRAW); // 孤儿式重分配（驱动端异步安全）
    gl::glUseProgram(program_);
    gl::glUniform2f(loc_scale_,
                    2.0f / static_cast<float>(lw_),
                    -2.0f / static_cast<float>(lh_));
    gl::glUniform1f(loc_opacity_, opacity);
    gl::glDrawArrays(gl::GL_TRIANGLES, 0, 6);
    // 解绑（QPainter 接管落在缺省 VAO——attrib 改写不触及本类对象）
    gl::glBindVertexArray(0);
    gl::glBindTexture(gl::GL_TEXTURE_2D, 0);
    gl::glUseProgram(0);
    return true;
}

// ---------------------------------------------------------------------------
// solveHomography —— 4 点 DLT：单位方 (0,0)(1,0)(1,1)(0,1) → dst 四角。
// x = (h0·u + h1·v + h2) / (h6·u + h7·v + 1)，y 同理。8 未知数 8 方程，
// 部分主元高斯消元；dst 退化（共线/重复）时主元消失 → false。
// ---------------------------------------------------------------------------
bool Canvas3DGl::solveHomography(const double dst[8], double h[8]) {
    double A[8][9] = {};
    static constexpr double kUv[4][2] = {{0, 0}, {1, 0}, {1, 1}, {0, 1}};
    for (int i = 0; i < 4; ++i) {
        const double u = kUv[i][0], v = kUv[i][1];
        const double x = dst[i * 2 + 0], y = dst[i * 2 + 1];
        // x·(h6·u + h7·v + 1) = h0·u + h1·v + h2
        double* r1   = A[i * 2 + 0];
        r1[0] = u;     r1[1] = v;     r1[2] = 1.0;
        r1[6] = -x * u; r1[7] = -x * v; r1[8] = x;
        // y·(h6·u + h7·v + 1) = h3·u + h4·v + h5
        double* r2   = A[i * 2 + 1];
        r2[3] = u;     r2[4] = v;     r2[5] = 1.0;
        r2[6] = -y * u; r2[7] = -y * v; r2[8] = y;
    }
    // 前向消元（部分主元）
    for (int col = 0; col < 8; ++col) {
        int    piv  = col;
        double best = std::fabs(A[col][col]);
        for (int r = col + 1; r < 8; ++r) {
            const double a = std::fabs(A[r][col]);
            if (a > best) {
                best = a;
                piv  = r;
            }
        }
        if (best < 1e-12) return false; // 奇异 = 退化四边形
        if (piv != col) {
            for (int c = col; c <= 8; ++c) std::swap(A[col][c], A[piv][c]);
        }
        for (int r = col + 1; r < 8; ++r) {
            const double f = A[r][col] / A[col][col];
            if (f == 0.0) continue;
            for (int c = col; c <= 8; ++c) A[r][c] -= f * A[col][c];
        }
    }
    // 回代
    for (int r = 7; r >= 0; --r) {
        double s = A[r][8];
        for (int c = r + 1; c < 8; ++c) s -= A[r][c] * h[c];
        h[r] = s / A[r][r];
        if (!std::isfinite(h[r])) return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// shutdown —— GL 资源释放（owner 于上下文 current 时调用；幂等）
// ---------------------------------------------------------------------------
void Canvas3DGl::shutdown() {
    if (!inited_) return;
    for (auto& kv : cache_) {
        if (kv.second.tex != 0) gl::glDeleteTextures(1, &kv.second.tex);
    }
    cache_.clear();
    if (vbo_)     gl::glDeleteBuffers(1, &vbo_);
    if (vao_)     gl::glDeleteVertexArrays(1, &vao_);
    if (program_) gl::glDeleteProgram(program_);
    vbo_ = 0;
    vao_ = 0;
    program_ = 0;
    inited_  = false;
    // dxt_probed_/dxt_dead_/failed_ 保留：上下文重建后结论仍可信（同进程
    // 同驱动）；失败会话不反复重试刷日志
}

} // namespace ms
