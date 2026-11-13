// ============================================================================
//  SoftRenderer.cpp —— 软件合成实现（最近邻采样 + alpha 混合 + scissor 裁剪）
// ============================================================================

#include "engine/SoftRenderer.hpp"

#include <algorithm>
#include <cmath>

namespace ms {

// ---------------------------------------------------------------------------
// draw —— 把素材帧按 DrawItem 合成到画布
//
// 双路径：
//   * 轴对齐（affine=false，历史语义）：目标矩形内逐像素 (px-左上)×比例
//     采样帧——与既有测试逐像素一致；
//   * 旋转（affine=true）：x/y/w/h 为外接 AABB，每像素经 inv_map 逆仿射
//     还原画布点，再绕 clip 中心逆旋转（R(-rot)）落到未旋转矩形框架内
//     采样帧像素；逆映射落在 clip 矩形外的像素不写（旋转角外露底色）。
//     复合仿射每 DrawItem 预合成一次，逐像素仅 6 乘 2 加。
// ---------------------------------------------------------------------------
bool SoftRenderer::draw(const DrawItem& di, const FrameView& frame) {
    if (!frame.valid() || width_ <= 0 || height_ <= 0) return false;
    // 【入口防御】NaN/Inf 矩形拒绘——double→int 转换的 UB 源（ScreenMapper
    // 正常有界，此处兜底异常变换数据，如素材 w=0 除零、场景数据损坏）
    if (!std::isfinite(di.x) || !std::isfinite(di.y) ||
        !std::isfinite(di.w) || !std::isfinite(di.h))
        return false;
    if (di.w <= 0 || di.h <= 0) return false; // 退化矩形

    // ---- scissor：目标矩形（或 AABB）裁剪到画布范围（等价 GL scissor；
    //      跨屏素材在本屏的出屏部分在此被裁掉）----
    // 【double 域先钳制再 cast】直接 cast 越界大值（如 1e300）是未定义
    // 行为；先 clamp 到 [0, 画布宽/高]，结果必然可安全转 int。
    // 与原语义等价：矩形完全出画布时钳制后 x0>=x1（或 y0>=y1）仍拒绘。
    const int x0 = static_cast<int>(std::floor(
        std::clamp(di.x, 0.0, static_cast<double>(width_))));
    const int y0 = static_cast<int>(std::floor(
        std::clamp(di.y, 0.0, static_cast<double>(height_))));
    const int x1 = static_cast<int>(std::ceil(
        std::clamp(di.x + di.w, 0.0, static_cast<double>(width_))));
    const int y1 = static_cast<int>(std::ceil(
        std::clamp(di.y + di.h, 0.0, static_cast<double>(height_))));
    if (x0 >= x1 || y0 >= y1) return false; // 与画布不相交：零写入

    // 整体不透明度（DrawItem 透传；与像素 alpha 相乘）
    const double opacity = std::clamp(di.opacity, 0.0, 1.0);
    if (opacity <= 0.0) return true; // 全透明：无视觉变化，但矩形合法（已绘制）

    // 整体不透明度的 0~255 整数因子（+0.5 四舍五入）
    const std::uint32_t clip_a =
        static_cast<std::uint32_t>(opacity * 255.0 + 0.5);

    if (!di.affine) {
        // ================================================================
        // 轴对齐快速路径（原实现，逐像素一致）
        // ================================================================
        // 采样窗（v3 裁切）：di.x/y/w/h 已是裁后可见矩形——帧采样窗 =
        // [u0·fw, u1·fw]×[v0·fh, v1·fh]。未裁切（u0=0/u1=1）时与历史
        // 公式 sx=(px+0.5−x)·su 逐位一致。
        const double fu0 = di.crop_u0 * frame.width;
        const double fv0 = di.crop_v0 * frame.height;
        // 采样步长：目标像素 → 帧像素（1:1 时 = 1，无重采样畸变）
        const double su = (di.crop_u1 - di.crop_u0) * frame.width  / di.w;
        const double sv = (di.crop_v1 - di.crop_v0) * frame.height / di.h;

        // 逐目标像素：最近邻采样 + alpha 混合
        for (int py = y0; py < y1; ++py) {
            // 目标像素中心 py+0.5 → 帧行号（窗原点偏移 + 钳到帧内）
            int sy = static_cast<int>(fv0 + (py + 0.5 - di.y) * sv);
            sy = std::clamp(sy, 0, frame.height - 1);
            const std::uint8_t* src_row = frame.rgba +
                static_cast<std::size_t>(sy) * frame.stride;

            std::uint8_t* dst_row = canvas_.data() +
                static_cast<std::size_t>(py) * width_ * 4;

            for (int px = x0; px < x1; ++px) {
                int sx = static_cast<int>(fu0 + (px + 0.5 - di.x) * su);
                sx = std::clamp(sx, 0, frame.width - 1);
                const std::uint8_t* s = src_row + static_cast<std::size_t>(sx) * 4;
                blendPixel(dst_row, px, s, clip_a);
            }
        }
        return true;
    }

    // ====================================================================
    // 旋转仿射路径：output 像素 → 画布点 → clip 局部（逆旋转）→ 帧坐标
    // ====================================================================
    // 防御：inv_map/画布矩形异常拒绘（数据损坏时宁缺勿错）
    for (int i = 0; i < 6; ++i)
        if (!std::isfinite(di.inv_map[i])) return false;
    if (!std::isfinite(di.canvas_x) || !std::isfinite(di.canvas_y) ||
        !std::isfinite(di.canvas_w) || !std::isfinite(di.canvas_h) ||
        !std::isfinite(di.rotation) ||
        di.canvas_w <= 0.0 || di.canvas_h <= 0.0)
        return false;

    // ---- 四角点位移分流（2026-11-09 corner pin）：任一角位移非 0 →
    //      目标不再是仿射像（四边形两三角）——走 drawQuadSampling 专用
    //      内核（三角分解与 GL 条带同构，对拍逐像素可比）；全 0 → 原单
    //      仿射路径（历史行为逐位不变）----
    if (di.cornerActive()) {
        drawQuadSampling(di, frame, x0, y0, x1, y1, clip_a);
        return true;
    }

    // 帧采样仿射 M_cf（画布→帧）：s = S_f·R(-rot)·(p - A) + half_frame
    //   R(-rot)=[[ct,st],[-st,ct]]（rot 为顺时针角，y 向下坐标系）；
    //   S_f=diag(fw/cw, fh/ch)（画布长度→帧像素长度）
    //   A = 锚点（2026-11-09；Transform.anchor 指定枢轴，默认中心与历史
    //   "绕中心"逐位一致——ScreenMapper/GlRenderer/画布同口径）
    constexpr double kPi = 3.14159265358979323846;
    const double rad = di.rotation * kPi / 180.0;
    const double ct  = std::cos(rad), stn = std::sin(rad);
    // 裁切采样窗（v3）：可见矩形 [canvas_*] 映射到帧窗 [u0·fw..u1·fw]
    // ——比例按窗宽、平移按窗中心。未裁切窗=全帧 → 与历史 half_frame
    // 公式逐位一致（0.5·fw 与 fw/2 同为精确运算）。
    const double fu0 = di.crop_u0 * frame.width;
    const double fv0 = di.crop_v0 * frame.height;
    const double su  = (di.crop_u1 - di.crop_u0) * frame.width  / di.canvas_w;
    const double sv  = (di.crop_v1 - di.crop_v0) * frame.height / di.canvas_h;
    double au = 0.5, av = 0.5;
    Transform::anchorUV(di.anchor, au, av);
    const double acx = di.canvas_x + au * di.canvas_w; // 锚点（画布域）
    const double acy = di.canvas_y + av * di.canvas_h;
    const double m_a =  su * ct,  m_b = su * stn;
    const double m_d = -sv * stn, m_e = sv * ct;
    const double m_c = (di.crop_u0 + di.crop_u1) * 0.5 * frame.width
                         - (m_a * acx + m_b * acy);
    const double m_f = (di.crop_v0 + di.crop_v1) * 0.5 * frame.height
                         - (m_d * acx + m_e * acy);

    // 复合 output→frame：M = M_cf ∘ inv_map（先逆映射到画布，再采帧）
    const double A = m_a * di.inv_map[0] + m_b * di.inv_map[3];
    const double B = m_a * di.inv_map[1] + m_b * di.inv_map[4];
    const double C = m_a * di.inv_map[2] + m_b * di.inv_map[5] + m_c;
    const double D = m_d * di.inv_map[0] + m_e * di.inv_map[3];
    const double E = m_d * di.inv_map[1] + m_e * di.inv_map[4];
    const double F = m_d * di.inv_map[2] + m_e * di.inv_map[5] + m_f;

    // 逐目标像素：逆仿射采样 + 出 clip 矩形不写 + alpha 混合
    for (int py = y0; py < y1; ++py) {
        std::uint8_t* dst_row = canvas_.data() +
            static_cast<std::size_t>(py) * width_ * 4;
        const double pcy = py + 0.5;
        for (int px = x0; px < x1; ++px) {
            const double pcx = px + 0.5;
            // 帧采样坐标（同时等价于 clip 局部坐标 × 采样比例）
            const double fx = A * pcx + B * pcy + C;
            const double fy = D * pcx + E * pcy + F;
            // 落在采样窗（= 裁后可见内容帧域）外 → 不写，露画布底色。
            // 判窗坐标等价判局部坐标（线性对应）；半像素容差内钳入，
            // 避免浮点边界把边界行误判出界（未裁切窗=全帧 → 与历史
            // fx∈[-0.5, fw] 判定逐位一致）。
            if (fx < fu0 - 0.5 || fy < fv0 - 0.5 ||
                fx > fu0 + (di.crop_u1 - di.crop_u0) * frame.width ||
                fy > fv0 + (di.crop_v1 - di.crop_v0) * frame.height)
                continue;
            const int sx = std::clamp(
                static_cast<int>(fx), 0, frame.width - 1);
            const int sy = std::clamp(
                static_cast<int>(fy), 0, frame.height - 1);
            const std::uint8_t* s = frame.rgba +
                static_cast<std::size_t>(sy) * frame.stride +
                static_cast<std::size_t>(sx) * 4;
            blendPixel(dst_row, px, s, clip_a);
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// drawQuadSampling —— 四边形两三角形仿射采样（corner pin / 透视路径内核）
//
// 目标四边形（canvas 域）顶点：
//   * 2D（pose3d=false）：未旋转角绕【锚点】R(θ) + Transform::corner 位移；
//   * 3D（pose3d=true，v3 俯仰/偏航）：ScreenMapper 已用
//     cg::quadCorners3D 预投影填 di.quad（画布域 CornerIdx 序——滚转/
//     锚点/角位移/透视缩放全部烘焙在内），此处仅顶点序重排。
// 顶点序 [LT, RT, LB, RB]（i bit0=右→u、bit1=下→v——与 GlRenderer
// drawQuad 的 GL 顶点序逐一同构——三角分解 {0,1,2}+{1,2,3}、对角线
// LB→RT 一致，保证 GL 对拍逐像素可比：同一像素必落同一三角形、同一仿射）。
// uv 分配与顶点序直通（v3 起为裁切采样窗）：P0↔(u0·fw, v0·fh)
// P1↔(u1·fw, v0·fh) P2↔(u0·fw, v1·fh) P3↔(u1·fw, v1·fh)——未裁切
// 窗=全帧时与历史直通表逐位一致。
// 每三角形求 canvas→frame 仿射（3 点对解 6 参），逐像素先 inv_map 到
// canvas 再判重心归属。零面积三角形防御跳过（退化四边形宁缺勿错）。
// 【TODO 透视校正】两三角形仿射 = 分片线性近似，大幅 corner pin 时帧
// 纹理非严格透视（GL 同为仿射条带）——未来 q 坐标插值升级两端同步。
// ---------------------------------------------------------------------------
void SoftRenderer::drawQuadSampling(const DrawItem& di, const FrameView& frame,
                                    int x0, int y0, int x1, int y1,
                                    std::uint32_t clip_a) {
    // output→canvas 逆仿射（ScreenMapper 提供；防御已在 draw 入口完成）
    const double ia = di.inv_map[0], ib = di.inv_map[1],
                 ic = di.inv_map[2], id = di.inv_map[3],
                 ie = di.inv_map[4], iff = di.inv_map[5];
    const double det0 = ia * ie - ib * id;
    if (!std::isfinite(det0) || std::fabs(det0) < 1e-12) return;

    // 目标四角（canvas 域）：顶点序 [LT, RT, LB, RB]（i bit0=右→u、
    // bit1=下→v——与 GlRenderer verts 逐一同构，条带三角 {0,1,2}+
    // {1,2,3} 对角线 LB→RT 同一条）。
    static constexpr int kVtxToCorner[4] = {0, 1, 3, 2}; // 顶点 i → CornerIdx
    double P[4][2];
    if (di.pose3d) {
        // 3D 透视（v3）：预投影四角直取（CornerIdx → 顶点序重排）。
        for (int i = 0; i < 4; ++i) {
            P[i][0] = di.quad[kVtxToCorner[i]][0];
            P[i][1] = di.quad[kVtxToCorner[i]][1];
        }
    } else {
        // 2D 路径（原实现）：旋转：未旋转角 → 相对【锚点】偏移 →
        // R(θ) → + 锚点（p' = R·(p−A)+A，锚点在旋转中不动；anchor=
        // 中心时与历史"绕中心"逐位一致）+ 角位移。
        constexpr double kPi = 3.14159265358979323846;
        const double rad = di.rotation * kPi / 180.0;
        const double ct  = std::cos(rad), stn = std::sin(rad);
        double au = 0.5, av = 0.5;
        Transform::anchorUV(di.anchor, au, av);
        const double acx = di.canvas_x + au * di.canvas_w;
        const double acy = di.canvas_y + av * di.canvas_h;
        for (int i = 0; i < 4; ++i) {
            const double vx = di.canvas_x + ((i & 1) ? di.canvas_w : 0.0);
            const double vy = di.canvas_y + ((i & 2) ? di.canvas_h : 0.0);
            const double rx = vx - acx, ry = vy - acy; // 相对锚点偏移
            P[i][0] = acx + ct * rx - stn * ry
                        + di.corner[kVtxToCorner[i] * 2 + 0];
            P[i][1] = acy + stn * rx + ct * ry
                        + di.corner[kVtxToCorner[i] * 2 + 1];
        }
    }
    // 帧空间对应点（uv 采样窗与顶点序直通：P0↔窗左上、P1↔右上、
    // P2↔左下、P3↔右下；未裁切窗=全帧 → 历史直通表逐位一致）
    const double U[4] = {di.crop_u0 * frame.width, di.crop_u1 * frame.width,
                         di.crop_u0 * frame.width, di.crop_u1 * frame.width};
    const double V[4] = {di.crop_v0 * frame.height, di.crop_v0 * frame.height,
                         di.crop_v1 * frame.height, di.crop_v1 * frame.height};

    // 两个三角形（与 GL 条带同构）：{0,1,2} 与 {1,2,3}
    for (int t = 0; t < 2; ++t) {
        const int i0 = t, i1 = t + 1, i2 = t + 2;
        // canvas→frame 仿射（3 点对解 6 参：U = a·X + b·Y + c）
        const double x0c = P[i0][0], y0c = P[i0][1];
        const double x1c = P[i1][0], y1c = P[i1][1];
        const double x2c = P[i2][0], y2c = P[i2][1];
        const double det = (x1c - x0c) * (y2c - y0c) -
                           (x2c - x0c) * (y1c - y0c);
        if (!std::isfinite(det) || std::fabs(det) < 1e-12) continue; // 零面积
        const double ta = ((U[i1]-U[i0]) * (y2c-y0c) -
                           (U[i2]-U[i0]) * (y1c-y0c)) / det;
        const double tb = ((U[i2]-U[i0]) * (x1c-x0c) -
                           (U[i1]-U[i0]) * (x2c-x0c)) / det;
        const double tc = U[i0] - ta * x0c - tb * y0c;
        const double td = ((V[i1]-V[i0]) * (y2c-y0c) -
                           (V[i2]-V[i0]) * (y1c-y0c)) / det;
        const double te = ((V[i2]-V[i0]) * (x1c-x0c) -
                           (V[i1]-V[i0]) * (x2c-x0c)) / det;
        const double tf = V[i0] - td * x0c - te * y0c;
        // 三角形有向面积（canvas 域，符号任一——重心判定用同构符号）
        const double area2 =
            (x1c - x0c) * (y2c - y0c) - (x2c - x0c) * (y1c - y0c);
        const double inv_area = 1.0 / area2;

        for (int py = y0; py < y1; ++py) {
            std::uint8_t* dst_row = canvas_.data() +
                static_cast<std::size_t>(py) * width_ * 4;
            const double pcy = py + 0.5;
            for (int px = x0; px < x1; ++px) {
                const double pcx = px + 0.5;
                // output → canvas（与单仿射路径同一逆映射）
                const double cxp = ia * pcx + ib * pcy + ic;
                const double cyp = id * pcx + ie * pcy + iff;
                // 重心坐标判定（canvas 域，容差 1e-9 相对面积）
                const double w0 = ((x1c - cxp) * (y2c - cyp) -
                                   (x2c - cxp) * (y1c - cyp)) * inv_area;
                const double w1 = ((x2c - cxp) * (y0c - cyp) -
                                   (x0c - cxp) * (y2c - cyp)) * inv_area;
                const double w2 = 1.0 - w0 - w1;
                if (w0 < -1e-9 || w1 < -1e-9 || w2 < -1e-9)
                    continue; // 不在本三角形（另一三角形负责或出界）
                // canvas → frame 采样坐标（凸组合保证在帧矩形内）
                const double fx = ta * cxp + tb * cyp + tc;
                const double fy = td * cxp + te * cyp + tf;
                const int sx = std::clamp(
                    static_cast<int>(fx), 0, frame.width - 1);
                const int sy = std::clamp(
                    static_cast<int>(fy), 0, frame.height - 1);
                const std::uint8_t* s = frame.rgba +
                    static_cast<std::size_t>(sy) * frame.stride +
                    static_cast<std::size_t>(sx) * 4;
                blendPixel(dst_row, px, s, clip_a);
            }
        }
    }
}

// ---------------------------------------------------------------------------
// blendPixel —— 单像素 alpha 混合（三路径单源；语义与历史内联逐位一致）
// ---------------------------------------------------------------------------
void SoftRenderer::blendPixel(std::uint8_t* dst_row, int px,
                              const std::uint8_t* s,
                              std::uint32_t clip_a) noexcept {
    std::uint8_t* d = dst_row + static_cast<std::size_t>(px) * 4;
    // alpha = 像素 alpha × clip opacity（精确 ÷255 四舍五入；
    // 不能用 >>8 近似——255*255>>8=254，全不透明像素误入混合
    // 分支造成每帧 1/255 亮度衰减，纯色帧合成会肉眼可见变暗）
    const std::uint32_t a =
        (static_cast<std::uint32_t>(s[3]) * clip_a + 127) / 255;
    if (a == 0)   return;              // 全透明：保持目标像素
    if (a >= 255) {                    // 不透明：直接覆盖
        d[0] = s[0]; d[1] = s[1]; d[2] = s[2]; d[3] = 255;
        return;
    }
    // 半透明：out = src*a + dst*(1-a)（四通道，+127 四舍五入）
    const std::uint32_t ia = 255 - a;
    d[0] = static_cast<std::uint8_t>((s[0] * a + d[0] * ia + 127) / 255);
    d[1] = static_cast<std::uint8_t>((s[1] * a + d[1] * ia + 127) / 255);
    d[2] = static_cast<std::uint8_t>((s[2] * a + d[2] * ia + 127) / 255);
    d[3] = static_cast<std::uint8_t>(a + (d[3] * ia + 127) / 255);
}

} // namespace ms
