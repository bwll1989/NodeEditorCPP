// ============================================================================
//  CanvasGeometry.hpp —— 画布几何纯函数（编辑器画布共享，core 层无 Qt 依赖）
//
//  职责：
//    把 SceneCanvasWidget 中"旋转感知"的几何计算（命中判定 / 角点缩放 /
//    角点位置 / 旋转拖柄 / 等比缩放）提炼为无状态纯函数——编辑器 UI 与
//    core_tests 共用同一实现，保证"测试验证的数学"与"画布实际执行的
//    行为"逐位一致。
//
//  坐标与旋转约定（与渲染端 SoftRenderer/ScreenMapper 全链路统一）：
//    * 画布坐标系：y 向下（屏幕约定）；正旋转角 = 顺时针（度）；
//    * R(θ)  = [[cosθ, -sinθ], [sinθ, cosθ]]      —— 画布域 → 局部域用 R(-θ)；
//    * 矩形语义：x/y 为**未旋转**外接框左上角，矩形绕自身中心 (x+w/2, y+h/2)
//      旋转 rotation 度（与 SceneModel::Screen / ClipTransform 相同语义）。
//
//  角点编码（corner，0..3）：以**未旋转矩形的局部角**为准——
//      0 = 左上 (-w/2,-h/2)   1 = 右上 (+w/2,-h/2)
//      2 = 右下 (+w/2,+h/2)   3 = 左下 (-w/2,+h/2)
//    旋转非 0 时局部角在画布上的位置随之转动（如 90° 时局部"左上"出现在
//    画布右上）；手柄按画布可见角点绘制，命中后把"用户拖的可见角"对应回
//    局部角编码传入 resizeRectAnchored 即可——函数内部自洽处理旋转。
//
//  角点缩放算法（锚定对角，旋转对象在初始局部坐标系解算）：
//      C0 = 原始中心；A = 原始矩形对角点（锚，画布坐标，缩放中不动）；
//      A_local   = R(-θ)·(A − C0)    —— 锚点的初始局部坐标；
//      cur_local = R(-θ)·(P − C0)    —— 当前鼠标点的初始局部坐标；
//      w_new = max(|cur_local.x − A_local.x|, min_w)
//      h_new = max(|cur_local.y − A_local.y|, min_h)
//      center_local = (cur_local + A_local) / 2
//      C_new = C0 + R(θ)·center_local
//      x/y   = C_new − (w_new/2, h_new/2)
//    性质（core_tests 逐条验证）：
//      * 锚定对角在缩放全程保持画布位置不动（旋转对象亦然）；
//      * rotation=0 时退化为标准锚定缩放（对角不动、x/y/w/h 直接求解）；
//      * 鼠标越过锚点时矩形翻转到另一侧（宽高取绝对值的自然语义）；
//      * min_w/min_h 钳制避免 0/负尺寸（screen 有 R4 校验 w/h>0；clip 的
//        transform.w/h ≤ 0 会触发 auto-size 回退语义，故显式 resize 必须
//        恒写正值）。
// ============================================================================

#pragma once

#include <cmath>

#include "model/SceneModel.hpp" // Transform::anchorUV（同层纯 C++，无 Qt）

namespace ms {
namespace canvas_geometry {

/// 度 → 弧度（与 SceneCanvasWidget/SoftRenderer 同值同约定）
inline constexpr double kDegToRad = 3.14159265358979323846 / 180.0;

/// 片段四角（画布域）：未旋转角 → 相对【锚点】偏移 → R(θ) → + 锚点 +
/// 角点位移。**四角生成的唯一真相**（2026-11-09 X 形根治）——画布
/// clipQuadCorners/clipHit、core ScreenMapper 变形 AABB 全部转发至此，
/// 防三处手写副本顶点序漂移（此前 bit 模式 zigzag 生成 + i*2 直索引
/// 造成蝴蝶结 X 形与 RB/LB 位移互换，三处同型漏网）。
///   * 角序 = Transform::CornerIdx 顺时针 [LT,RT,RB,LB]（数据序同构，
///     消费方 drawPolygon/3D 三角剖分/AABB 全按此语义；GL 端 zigzag
///     顶点序由 kVtxToCorner 映射对齐）；
///   * 旋转中心语义 p' = R·(p − A) + A（锚点不动）；
///   * 位移沿画布 X/Y 轴不随旋转转（屏幕空间拖角语义）。
/// \param corner8 Transform::corner（8 值，可空 = 无位移）
inline void quadCornersWithAnchor(double x, double y, double w, double h,
                                  double rotation_deg, int anchor,
                                  const double* corner8, double* ox,
                                  double* oy) noexcept {
    double au = 0.5, av = 0.5;
    Transform::anchorUV(anchor, au, av);
    const double acx = x + au * w; // 锚点（画布域）
    const double acy = y + av * h;
    const double rad = rotation_deg * kDegToRad;
    const double ct = std::cos(rad), st = std::sin(rad);
    for (int i = 0; i < 4; ++i) {
        const double vx = x + ((i == 1 || i == 2) ? w : 0.0); // 顺时针生成
        const double vy = y + ((i >= 2) ? h : 0.0);
        const double rx = vx - acx, ry = vy - acy; // 相对锚点偏移
        ox[i] = acx + ct * rx - st * ry + (corner8 ? corner8[i * 2 + 0] : 0.0);
        oy[i] = acy + st * rx + ct * ry + (corner8 ? corner8[i * 2 + 1] : 0.0);
    }
}

/// 角点编码（见文件头注释）：0=左上 1=右上 2=右下 3=左下（局部/未旋转语义）
enum Corner {
    kCornerTL = 0,
    kCornerTR = 1,
    kCornerBR = 2,
    kCornerBL = 3,
};

// ---------------------------------------------------------------------------
// 三轴旋转透视四角（2026-11-09 v3，DaVinci 姿态组俯仰/偏航）
//
// 模型：图像平面绕【锚点】做 3D 旋转（俯仰 Rx → 偏航 Ry，滚转 Rz 在投影
// 后以 2D 旋转叠加），再经针孔透视投影回画布平面——四角推算后走既有
// 变形 quad 管线（GL/Soft 同构两三角仿射，片上为分片线性近似，大幅
// 姿态时帧纹理非严格透视；GL/Soft 同构故对拍一致，与 corner pin 的
// TODO 透视校正同一条升级路径）。
//
// 数学（画布域，y 向下）：
//   局部 p = 角 − 锚点（z=0 平面）；
//   俯仰 Rx(p)：y' = y·cos(p)，z' = −y·sin(p)（正角=顶边 y<0 → z'>0
//              翘起朝 Eye——站立/后仰，2026-11-18 深度轴翻转定案）；
//   偏航 Ry(w)：x'' = x·cos(w) + z'·sin(w)，z'' = −x·sin(w) + z'·cos(w)
//              （正角=右边缘 x>0 → z''<0 下沉远离）；
//   透视：s = f/(f − z'')（f = 2×max(w,h)——对象长边 2 倍焦距；深度
//         正 = 朝观察者 Eye = 放大，越焦平面宁缩勿翻）；
//   投影：p3 = (x''·s, y'·s)；
//   滚转：R(θ)·p3（绕锚点 2D 旋转，与 quadCornersWithAnchor 同式——
//         滚转=投影后画布面内旋转，DaVinci roll 语义）；
//   画布角 = 锚点 + R(θ)·p3 + 角位移（位移沿画布轴，屏幕空间拖角语义）。
//
// 逐位兼容：pitch==yaw==0 时 z'≡0 → s≡1、x''≡x、y'≡y → 退化为
// quadCornersWithAnchor 同式（历史行为零漂移）。
// ---------------------------------------------------------------------------

/// 透视焦距（对象长边 2 倍；经验值——四角最大深度≈半对角线，姿态不过畸）
inline double perspectiveFocal(double w, double h) noexcept {
    const double m = w > h ? w : h;
    return 2.0 * m;
}

// ---------------------------------------------------------------------------
// 素材 3D 化阶段①泛化投影（2026-11-17）：三轴旋转 + 几何深度 + Show 级
// 全局透视——语义链"全场唯一一次透视投影"的数学真相（路线文档三·0）。
//
// 【单一透视（2026-11-18 二次定案：对齐 Watchout Preferences→3D Perspective
// 模型）】旋转扭曲与 Z 缩放共用同一组全局透视参数（WO 文档："Images rotated
// around the X or Y axes appear distorted according to the perspective"——
// 同一 Perspective 值既控旋转透视又控 Z 缩放；消隐点全局唯一，默认舞台
// 中心）。此前的两级拆分（旋转恒对象镜头）已按用户拍板退役。
//
// 统一模型（画布域，y 向下；深度 Z 正 = 朝观察者 Eye = 舞台向上 =
// 放大——2026-11-18 深度轴翻转定案：画布躺地面、Eye 悬浮于 VP 上方
// F 高度，pos_z>0 = 面片升高靠近 Eye = 画面放大，与 3D 舞台视口高度
// 轴同向；旧"正 = 远离"与视口矛盾【面片升高成品却缩小】退役）：
//   局部 p = 角 − 锚点（z=0 地面平面）；
//   俯仰 Rx：y1 = y·cos(p)，z1 = −y·sin(p)（正角=顶边翘起朝 Eye）；
//   偏航 Ry：x2 = x·cos(w) + z1·sin(w)，z2 = −x·sin(w) + z1·cos(w)
//            （正角=右缘下沉远离；复合序 Rx→Ry，滚转最后 = 世界轴
//            Z——与既有 v3 行为一致）；
//   世界：  X = 锚x + x2，Y = 锚y + y1，Z = pos_z + z2；
//   透视：  s = F/(F − Z)——调用方经 Scene::effectivePersp 传参
//           （persp_focal>0 用户配置 / 未配置 = 画布派生 F=2×对角线、
//           vp=画布中心）；Z≥F（到达/穿越 Eye 高度平面）宁缩勿翻
//           钳 1e-6（防除零/防投影翻转）；
//   投影：  P = vp + (W − vp)·s（素材朝 Eye 升近放大、下沉/远去时
//           向消隐点收敛）；
//   滚转：  绕【投影后锚点】P_a = vp + (A − vp)·s_a 做 2D 旋转 + 角位移
//           （pos_z=0 时 s_a=1 → P_a = 锚点 = 现状逐位）。
//   q：     q_i = s_i——GL 透视校正采样因子（屏幕空间线性插值 1/w 对
//           平面片单应精确；见 GlRenderer 阶段②）。
//
// 【兼容矩阵】（单测 clip3d_math_test 钉死）：
//   pos_z=0 + 姿态=0 → 快路径 quadCornersWithAnchor（s≡1 恒等）；
//   focal<=0（画布无效防御）→ 片段隐含对象透视回退（F=2×max(w,h)、
//   vp=锚点）——数据损坏防御，正常路径调用方恒传 effectivePersp 值。
// ---------------------------------------------------------------------------

/// 三轴旋转 + 几何深度 + 全局透视四角（输出序 = CornerIdx [LT,RT,RB,LB]）
/// \param pos_z   几何深度（画布域 px，正 = 朝观察者 Eye/舞台向上 =
///                放大；Transform::pos_z 透传）
/// \param persp_focal 全局透视焦距（px；<=0 = 未配置 → 片段隐含透视防御回退）
/// \param persp_vp_x/y 全局消隐点（画布 px；focal>0 时消费）
/// \param q4      输出每角缩放因子 s（CornerIdx 序；可空 = 不需要——
///                GL 透视校正采样用，轴对齐/2D 仿射路径恒 1）
inline void quadCorners3DP(double x, double y, double w, double h,
                           double pos_z, double roll_deg, double pitch_deg,
                           double yaw_deg, int anchor, const double* corner8,
                           double persp_focal, double persp_vp_x,
                           double persp_vp_y, double* ox, double* oy,
                           double* q4) noexcept {
    if (pitch_deg == 0.0 && yaw_deg == 0.0 && pos_z == 0.0) {
        // 快路径：无俯仰偏航且无深度 → 历史 2D 公式逐位直通（含滚转/
        // 锚点/位移；透视未生效项 s≡1——pos_z=0 时锚点 s=1、投影恒等）
        quadCornersWithAnchor(x, y, w, h, roll_deg, anchor, corner8, ox, oy);
        if (q4) {
            q4[0] = q4[1] = q4[2] = q4[3] = 1.0;
        }
        return;
    }
    double au = 0.5, av = 0.5;
    Transform::anchorUV(anchor, au, av);
    const double acx = x + au * w; // 锚点（画布域）
    const double acy = y + av * h;

    // 单一透视仪（对齐 Watchout：旋转与 Z 共用一组全局参数）：调用方传
    // effectivePersp 值；focal<=0（画布无效数据损坏防御）退片段隐含透视
    const bool   global = persp_focal > 0.0;
    const double F      = global ? persp_focal : perspectiveFocal(w, h);
    const double vpx    = global ? persp_vp_x : acx;
    const double vpy    = global ? persp_vp_y : acy;

    // 3D 旋转系数（俯仰 Rx 后接偏航 Ry；角无需逐点重算）
    const double pr = pitch_deg * kDegToRad;
    const double cp = std::cos(pr), sp = std::sin(pr);
    const double yw = yaw_deg * kDegToRad;
    const double cy = std::cos(yw), sy = std::sin(yw);
    const double rad = roll_deg * kDegToRad;
    const double ct = std::cos(rad), st = std::sin(rad);

    // 滚转中心 = 投影后锚点（锚点自身深度 = pos_z；pos_z=0 → P_a = 锚点
    // = 现状"绕锚点"逐位）
    const double den_a = F - pos_z; // pos_z≥F（锚点到达/越过 Eye）钳缩
    double s_a = den_a > 1e-9 ? F / den_a : 1e-6;
    const double pax = vpx + (acx - vpx) * s_a;
    const double pay = vpy + (acy - vpy) * s_a;

    for (int i = 0; i < 4; ++i) {
        const double vx = x + ((i == 1 || i == 2) ? w : 0.0); // 顺时针生成
        const double vy = y + ((i >= 2) ? h : 0.0);
        const double lx = vx - acx, ly = vy - acy; // 局部（z=0 平面）
        // Rx(pitch)：y' = y·cos(p)，z' = −y·sin(p)（正=顶边翘起朝 Eye）
        const double y1 = ly * cp;
        const double z1 = -ly * sp;
        // Ry(yaw)：x'' = x·cos + z'·sin；z'' = −x·sin + z'·cos（正=右缘
        //          下沉远离——向消隐点收敛方向）
        const double x2 = lx * cy + z1 * sy;
        const double z2 = -lx * sy + z1 * cy;
        // 世界点（画布 XY + 深度 Z 含 pos_z + 旋转诱导 z2）→ 透视
        // s = F/(F−Z)（Z 正=朝 Eye=放大；Z≥F 越 Eye 平面宁缩勿翻）
        const double Z = pos_z + z2;
        const double den = F - Z;
        double s = den > 1e-9 ? F / den : 1e-6;
        const double X = acx + x2, Y = acy + y1;
        // 投影（向消隐点收敛）
        const double px = vpx + (X - vpx) * s;
        const double py = vpy + (Y - vpy) * s;
        // 滚转（绕投影后锚点 2D）+ 角位移（沿画布轴）
        const double rx = px - pax, ry = py - pay;
        ox[i] = pax + ct * rx - st * ry + (corner8 ? corner8[i * 2 + 0] : 0.0);
        oy[i] = pay + st * rx + ct * ry + (corner8 ? corner8[i * 2 + 1] : 0.0);
        if (q4) q4[i] = s;
    }
}

/// 三轴旋转透视四角（2026-11-20 修订定案，素材旋转唯一真相）：
/// XYZ 三轴旋转照常应用（局部 z=0 平面的面片，俯仰 Rx → 偏航 Ry，滚转
/// 最后 = 世界轴 Z），旋转完成后再【透视投影】到 XY 平面（针孔压扁，
/// 近大远小——2026-11-20 曾短暂改走正交扁平化（丢 Z 无透视），用户目验
/// 旋转观感后拍板恢复透视）。透视仪 = Scene::effectivePersp（persp_* 未
/// 配置 = 画布派生默认 F=2×对角线、vp=画布中心，全场共享同一投影仪）；
/// 深度 Z = pos_z + 旋转诱导 z2（EditorRuntime 加载归零 pos_z——透视
/// 效果仅由旋转诱导）。屏幕（Screen）斜装姿态单应同走 quadCorners3DP
/// 透视链（合成端分治：素材 judge 与屏 st 各自投影）。
/// quadCornersOrthoFlat（正交扁平化，同日废弃）已删除——勿再引用。
/// quadCorners3D 兼容转发与 quadCorners3DWorld 真 3D 世界四角均已删除
///（2026-11-20 用户授权清理：消费者全部迁 3DP+effectivePersp，归档在
/// backup-stage-paradigm-1120 可回收）。

/// 缩放结果（新外接框左上角 + 尺寸；rotation 不变由调用方保留）
struct ResizeResult {
    double x = 0.0;
    double y = 0.0;
    double w = 0.0;
    double h = 0.0;
};

// ---------------------------------------------------------------------------
// 单应（2026-11-17 阶段2 屏姿态）：4 点对解 3×3 投影映射——屏取景窗带
// 俯仰/偏航时画布↔输出的映射从仿射升级单应（透视除法保直线，clip 姿态
// 透视与屏姿态透视两级单应复合的数学根基）。
//   * 顶点约定与 quadCorners3DP 同源：单位方 (0,0)(1,0)(1,1)(0,1) 依序
//     对应 CornerIdx [LT,RT,RB,LB] 目标点；
//   * 解法 = 8×9 增广矩阵 + 部分主元消元（与编辑器 Canvas3DGl::solveHomography
//     同一实现——三处（编辑器 3D 贴图/屏映射/对拍）同一数学）；
//   * 退化（共线/重复点/近奇异）返回 false——数据损坏宁缺勿错。
// ---------------------------------------------------------------------------

/// 4 点对解单应：单位方四角 → dst 八元组（x0,y0,…,x3,y3，CornerIdx 序）
/// \param h9 输出 3×3 行主序（h9[8]=1）：单位方 (u,v) → 目标 (x,y)
inline bool solveHomography(const double dst[8], double h9[9]) noexcept {
    // 增广矩阵 A[8][9]：每点两行——
    //   x = h0·u + h1·v + h2 − h6·u·x − h7·v·x
    //   y = h3·u + h4·v + h5 − h6·u·y − h7·v·y
    static const double U[4] = {0.0, 1.0, 1.0, 0.0};
    static const double V[4] = {0.0, 0.0, 1.0, 1.0};
    double A[8][9];
    for (int i = 0; i < 4; ++i) {
        const double u = U[i], v = V[i];
        const double x = dst[i * 2 + 0], y = dst[i * 2 + 1];
        double* rx = A[i * 2 + 0];     // x 方程行
        rx[0] = u; rx[1] = v; rx[2] = 1.0;
        rx[3] = rx[4] = rx[5] = 0.0;
        rx[6] = -u * x; rx[7] = -v * x; rx[8] = x;
        double* ry = A[i * 2 + 1];     // y 方程行
        ry[0] = ry[1] = ry[2] = 0.0;
        ry[3] = u; ry[4] = v; ry[5] = 1.0;
        ry[6] = -u * y; ry[7] = -v * y; ry[8] = y;
    }
    // 部分主元消元 → 上三角
    for (int col = 0; col < 8; ++col) {
        int piv = col;
        double best = std::fabs(A[col][col]);
        for (int r = col + 1; r < 8; ++r) {
            const double m = std::fabs(A[r][col]);
            if (m > best) { best = m; piv = r; }
        }
        if (best < 1e-12) return false; // 奇异（共线/重复点）
        if (piv != col)
            for (int c = col; c < 9; ++c)
                std::swap(A[col][c], A[piv][c]);
        for (int r = col + 1; r < 8; ++r) {
            const double k = A[r][col] / A[col][col];
            if (k == 0.0) continue;
            for (int c = col; c < 9; ++c) A[r][c] -= k * A[col][c];
        }
    }
    // 回代
    double s[8] = {};
    for (int row = 7; row >= 0; --row) {
        double acc = A[row][8];
        for (int c = row + 1; c < 8; ++c) acc -= A[row][c] * s[c];
        s[row] = acc / A[row][row];
        if (!std::isfinite(s[row])) return false;
    }
    for (int i = 0; i < 8; ++i) h9[i] = s[i];
    h9[8] = 1.0;
    return true;
}

/// 单应映射点：[x; y; 1] → H·[x; y; 1]，除 w 分量（透视核心）
inline void homographyMapPoint(const double h9[9], double x, double y,
                               double* ox, double* oy) noexcept {
    const double w = h9[6] * x + h9[7] * y + h9[8];
    if (!(std::fabs(w) > 1e-12)) { *ox = x; *oy = y; return; } // 退化防御：恒等
    *ox = (h9[0] * x + h9[1] * y + h9[2]) / w;
    *oy = (h9[3] * x + h9[4] * y + h9[5]) / w;
}

/// 3×3 行主序单应逆（伴随/行列式解析逆）；奇异返回 false（调用方兜底）
inline bool homographyInverse(const double h9[9], double inv[9]) noexcept {
    const double a = h9[0], b = h9[1], c = h9[2];
    const double d = h9[3], e = h9[4], f = h9[5];
    const double g = h9[6], hh = h9[7], i = h9[8];
    const double A =  (e * i - f * hh);
    const double B = -(d * i - f * g);
    const double C =  (d * hh - e * g);
    const double D = -(b * i - c * hh);
    const double E =  (a * i - c * g);
    const double F = -(a * hh - b * g);
    const double G =  (b * f - c * e);
    const double H = -(a * f - c * d);
    const double I =  (a * e - b * d);
    const double det = a * A + b * B + c * C;
    if (!std::isfinite(det) || std::fabs(det) < 1e-12) return false;
    const double k = 1.0 / det;
    inv[0] = A * k; inv[1] = D * k; inv[2] = G * k;
    inv[3] = B * k; inv[4] = E * k; inv[5] = H * k;
    inv[6] = C * k; inv[7] = F * k; inv[8] = I * k; // 伴随 = cofactor 转置
    return true;
}

/// 矩形绕中心旋转 rotation 度后，局部角 corner 的画布坐标
/// \param x,y,w,h  未旋转外接框；\param rotation_deg 顺时针度数
inline void rotatedCorner(double x, double y, double w, double h,
                          double rotation_deg, int corner, double* ox,
                          double* oy) noexcept {
    // 局部角偏移（相对中心）：角编码 → 符号
    const double lx = (corner == kCornerTR || corner == kCornerBR) ? w / 2.0
                                                                   : -w / 2.0;
    const double ly = (corner == kCornerBR || corner == kCornerBL) ? h / 2.0
                                                                   : -h / 2.0;
    const double cx = x + w / 2.0, cy = y + h / 2.0; // 旋转中心
    if (rotation_deg == 0.0) {                       // 轴对齐快速路径
        *ox = cx + lx;
        *oy = cy + ly;
        return;
    }
    const double rad = rotation_deg * kDegToRad;
    const double ct = std::cos(rad), st = std::sin(rad);
    // R(θ)·(lx,ly) + C（顺时针；y 向下坐标系）
    *ox = cx + lx * ct - ly * st;
    *oy = cy + lx * st + ly * ct;
}

/// 画布点是否落在（可能旋转的）矩形内：相对中心逆旋转后判未旋转框
/// \note 与 SceneCanvasWidget::hitTest 的 rectHit 同数学（收敛到此处复用）
inline bool rotatedRectContains(double x, double y, double w, double h,
                                double rotation_deg, double px,
                                double py) noexcept {
    if (w <= 0.0 || h <= 0.0) return false; // 退化矩形（同 QRectF::contains 语义）
    const double cx = x + w / 2.0, cy = y + h / 2.0;
    const double dx = px - cx, dy = py - cy;
    if (rotation_deg == 0.0) // 轴对齐快速路径（与既有行为逐位一致）
        return px >= x && px <= x + w && py >= y && py <= y + h;
    const double rad = rotation_deg * kDegToRad;
    const double ct = std::cos(rad), st = std::sin(rad);
    // R(-θ) = [[ct, st], [-st, ct]]：把画布点转进初始局部坐标系
    const double lx = dx * ct + dy * st;
    const double ly = -dx * st + dy * ct;
    return std::fabs(lx) <= w / 2.0 && std::fabs(ly) <= h / 2.0;
}

/// 角点缩放（锚定对角；含旋转对象局部坐标解算——算法见文件头注释）
/// \param x,y,w,h    原始未旋转外接框（缩放基准）
/// \param rotation_deg  旋转角（度，顺时针；缩放不改变旋转）
/// \param corner     被拖拽的局部角编码（0..3）；锚定其对角
/// \param px,py      当前鼠标点（画布坐标）
/// \param min_w,min_h 最小尺寸钳制（画布单位；建议 ≥1，理由见文件头）
inline ResizeResult resizeRectAnchored(double x, double y, double w, double h,
                                       double rotation_deg, int corner,
                                       double px, double py, double min_w,
                                       double min_h) noexcept {
    const double cx0 = x + w / 2.0, cy0 = y + h / 2.0; // 原始中心 C0
    const double rad = rotation_deg * kDegToRad;
    const double ct = std::cos(rad), st = std::sin(rad);

    // 锚点 = 被拖角的对角（局部坐标符号取反）→ 初始局部坐标 A_local。
    // 局部对角偏移 = −(被拖角偏移)，直接由 corner 编码取符号。
    const double alx = (corner == kCornerTR || corner == kCornerBR) ? -w / 2.0
                                                                    : w / 2.0;
    const double aly = (corner == kCornerBR || corner == kCornerBL) ? -h / 2.0
                                                                    : h / 2.0;

    // 当前鼠标点 → 初始局部坐标：R(-θ)·(P − C0)
    const double dx = px - cx0, dy = py - cy0;
    const double plx = dx * ct + dy * st;
    const double ply = -dx * st + dy * ct;

    // 新尺寸：锚点与鼠标在初始局部坐标系的轴向距离（翻转取 abs；钳最小值）
    ResizeResult r;
    r.w = std::fabs(plx - alx);
    r.h = std::fabs(ply - aly);
    if (r.w < min_w) r.w = min_w;
    if (r.h < min_h) r.h = min_h;

    // 新中心：锚点与鼠标的局部中点转回画布域（锚定语义的自然结果——
    // 新外接框中心 = 旋转后的 (鼠标+锚)/2）
    const double mllx = (plx + alx) / 2.0, mlly = (ply + aly) / 2.0;
    const double ncx = cx0 + mllx * ct - mlly * st;
    const double ncy = cy0 + mllx * st + mlly * ct;

    r.x = ncx - r.w / 2.0;
    r.y = ncy - r.h / 2.0;
    return r;
}

/// 旋转拖柄位置：顶边中点沿"局部 -y"方向外延 offset（画布单位）后随对象
/// 旋转。编辑器把控件像素偏移换算成画布单位传入（视觉大小恒定），
/// 函数只负责旋转映射——柄画在"对象头顶"并跟随姿态转动。
inline void rotateHandlePos(double x, double y, double w, double h,
                            double rotation_deg, double offset, double* ox,
                            double* oy) noexcept {
    // 局部偏移（相对中心）：顶边中点再向上 offset
    const double lx = 0.0;
    const double ly = -h / 2.0 - offset;
    const double cx = x + w / 2.0, cy = y + h / 2.0; // 旋转中心
    if (rotation_deg == 0.0) {                       // 轴对齐快速路径
        *ox = cx + lx;
        *oy = cy + ly;
        return;
    }
    const double rad = rotation_deg * kDegToRad;
    const double ct = std::cos(rad), st = std::sin(rad);
    *ox = cx + lx * ct - ly * st; // R(θ)·(lx,ly) + C（顺时针）
    *oy = cy + lx * st + ly * ct;
}

/// 鼠标点相对矩形中心的画布方向角（度；y 向下坐标系、顺时针为正，
/// 与 rotation 同约定）。旋转拖拽解算：new_rot = orig + (ang_now − ang_start)
/// ——起点方向角作为基准相减，与手柄初始位置无关（增量语义）。
inline double angleFromCenter(double x, double y, double w, double h, double px,
                              double py) noexcept {
    const double cx = x + w / 2.0, cy = y + h / 2.0;
    return std::atan2(py - cy, px - cx) / kDegToRad;
}

/// 角点缩放的等比变体（Shift 约束）：新矩形与原矩形相似（w:h 不变），
/// 锚定对角不动，尺寸取"能包住鼠标方向"的比例（ratio = max 两轴）——
/// 等比时被拖角一般不贴鼠标（鼠标只定方向，尺寸由比例决定）。
/// 翻转语义与 resizeRectAnchored 一致：鼠标越过锚点向另一侧延伸。
inline ResizeResult resizeRectAnchoredRatio(double x, double y, double w,
                                            double h, double rotation_deg,
                                            int corner, double px, double py,
                                            double min_w, double min_h) noexcept {
    const double cx0 = x + w / 2.0, cy0 = y + h / 2.0; // 原始中心 C0
    const double rad = rotation_deg * kDegToRad;
    const double ct = std::cos(rad), st = std::sin(rad);

    // 锚点初始局部坐标（同 resizeRectAnchored）
    const double alx = (corner == kCornerTR || corner == kCornerBR) ? -w / 2.0
                                                                    : w / 2.0;
    const double aly = (corner == kCornerBR || corner == kCornerBL) ? -h / 2.0
                                                                    : h / 2.0;

    // 当前鼠标点 → 初始局部坐标：R(-θ)·(P − C0)
    const double dx = px - cx0, dy = py - cy0;
    const double plx = dx * ct + dy * st;
    const double ply = -dx * st + dy * ct;

    // 锚 → 鼠标的局部向量（决定延伸方向与比例）
    const double vx = plx - alx, vy = ply - aly;

    // 相似比例：两轴中"需求更大"的一轴定标（钳到最小尺寸对应的比例）
    double ratio = 0.0;
    if (std::fabs(vx) / w > ratio) ratio = std::fabs(vx) / w;
    if (std::fabs(vy) / h > ratio) ratio = std::fabs(vy) / h;
    if (min_w / w > ratio) ratio = min_w / w;
    if (min_h / h > ratio) ratio = min_h / h;

    ResizeResult r;
    r.w = w * ratio;
    r.h = h * ratio;

    // 等比下中心由"锚不动"直接解出：新锚局部 = 中心局部 + 锚符号偏移，
    // 与原锚画布位置相等 → 中心局部 = A_local ∓ (nw/2, nh/2)（符号随 v）
    const double sx = vx >= 0.0 ? 1.0 : -1.0;
    const double sy = vy >= 0.0 ? 1.0 : -1.0;
    const double mllx = alx + sx * r.w / 2.0;
    const double mlly = aly + sy * r.h / 2.0;
    const double ncx = cx0 + mllx * ct - mlly * st;
    const double ncy = cy0 + mllx * st + mlly * ct;

    r.x = ncx - r.w / 2.0;
    r.y = ncy - r.h / 2.0;
    return r;
}

} // namespace canvas_geometry
} // namespace ms
