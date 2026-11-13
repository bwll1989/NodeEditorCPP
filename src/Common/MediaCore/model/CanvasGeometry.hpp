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
//   俯仰 Rx(p)：y' = y·cos(p)，z' = −y·sin(p)（正角=顶边 y<0 → z'>0 远离）；
//   偏航 Ry(w)：x'' = x·cos(w) + z'·sin(w)，z'' = −x·sin(w) + z'·cos(w)
//              （正角=右边缘 x>0 → z''>0 远离）；
//   透视：s = f/(f + z'')（f = 2×max(w,h)——对象长边 2 倍焦距，四角深度
//         ±对角线半长时缩放比 ~[0.74,1.54]，姿态感明显且不过曝）；
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

/// 三轴旋转四角（画布域，输出序 = CornerIdx [LT,RT,RB,LB]——与
/// quadCornersWithAnchor 同约定；消费方 GL/Soft 顶点序经 kVtxToCorner 映射）
/// \param pitch_deg rot_x（俯仰，绕画布 X 轴；正=顶边后仰）
/// \param yaw_deg   rot_y（偏航，绕画布 Y 轴；正=右边缘远离）
inline void quadCorners3D(double x, double y, double w, double h,
                          double roll_deg, double pitch_deg, double yaw_deg,
                          int anchor, const double* corner8, double* ox,
                          double* oy) noexcept {
    if (pitch_deg == 0.0 && yaw_deg == 0.0) {
        // 快路径：无俯仰偏航 → 历史 2D 公式逐位直通（含滚转/锚点/位移）
        quadCornersWithAnchor(x, y, w, h, roll_deg, anchor, corner8, ox, oy);
        return;
    }
    double au = 0.5, av = 0.5;
    Transform::anchorUV(anchor, au, av);
    const double acx = x + au * w; // 锚点（画布域）
    const double acy = y + av * h;
    const double f   = perspectiveFocal(w, h);

    // 3D 旋转系数（俯仰 Rx 后接偏航 Ry；角无需逐点重算）
    const double pr = pitch_deg * kDegToRad;
    const double cp = std::cos(pr), sp = std::sin(pr);
    const double yw = yaw_deg * kDegToRad;
    const double cy = std::cos(yw), sy = std::sin(yw);
    const double rad = roll_deg * kDegToRad;
    const double ct = std::cos(rad), st = std::sin(rad);

    for (int i = 0; i < 4; ++i) {
        const double vx = x + ((i == 1 || i == 2) ? w : 0.0); // 顺时针生成
        const double vy = y + ((i >= 2) ? h : 0.0);
        const double lx = vx - acx, ly = vy - acy; // 局部（z=0 平面）
        // Rx(pitch)：y' = y·cos(p)，z' = −y·sin(p)（正=顶边远离）
        const double y1 = ly * cp;
        const double z1 = -ly * sp;
        // Ry(yaw)：x'' = x·cos + z'·sin；z'' = −x·sin + z'·cos（正=右缘远离）
        const double x2 = lx * cy + z1 * sy;
        const double z2 = -lx * sy + z1 * cy;
        // 透视投影：s = f/(f+z)（z>0 远离→缩小；f/2 余量下恒正——防御
        // 钳正防异常数据翻转；越过焦平面时宁缩勿翻）
        double s = f / (f + z2);
        if (!(s > 0.0)) s = 1e-6;
        const double px = x2 * s, py = y1 * s;
        // 滚转（画布面内 2D，绕锚点）+ 角位移（沿画布轴）
        ox[i] = acx + ct * px - st * py + (corner8 ? corner8[i * 2 + 0] : 0.0);
        oy[i] = acy + st * px + ct * py + (corner8 ? corner8[i * 2 + 1] : 0.0);
    }
}

/// 缩放结果（新外接框左上角 + 尺寸；rotation 不变由调用方保留）
struct ResizeResult {
    double x = 0.0;
    double y = 0.0;
    double w = 0.0;
    double h = 0.0;
};

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
