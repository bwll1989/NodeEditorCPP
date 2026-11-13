// ============================================================================
//  Camera3D.hpp —— 3D 视口正交轨道相机（SceneCanvasWidget 专用，header-only）
//
//  【画布 3D 化 2026-11-05】Blender 式视口的相机数学。世界坐标与画布域
//  一一对应（无独立坐标系换算成本）：
//    * X = 画布 x（向右增长）
//    * Y = 高度（片段 transform.z，画布单位；0 = 地面 = 画布平面）
//    * Z = 画布 y（向下增长——与画布同向，地面即"画布躺平"）
//  【命名分工（2026-11-05 轴球文案修正）】本结构内部把高度记作 Y（GL
//  惯例 Y-up）；用户可见文案一律按属性面板语义：Z=高度（"Z 位置"）、
//  Y=画布 y——轴球 gizmo 标签/视角提示以属性语义为准（见
//  SceneCanvasWidget::drawAxisGizmo），勿把本结构成员名当用户文案。
//  视觉语义：画布矩形躺在地面（h=0），片段按 transform.z 悬浮其上——
//  用户在 3D 视图里直接看到"Z 位置"的空间含义。
//
//  【为什么是正交投影】透视投影下水平面矩形投影后是任意四边形，QPainter
//  贴纹理要走 projective 变换（质量/裁剪坑多）；正交投影保持平行性，
//  旋转矩形投影后仍是【平行四边形】——QTransform 仿射三对应点即可精确
//  贴纹理。这是"QPainter 复用 + 零 GL 管线重写"3D 方案的数学基石
//  （编辑器视口不需纵深，等价 Blender 正交视图 Numpad-5 形态）。
//
//  相机模型（轨道相机 orbit）：
//    * yaw   方位角 φ（度）：相机绕竖直轴 Y 公转；0 = 相机在画布 +y 侧
//      （画布"正面"），此时画布 x 向屏幕右、画布 y 向屏幕下——与 2D
//      画布逐点同向（无镜像），俯仰降低即"侧过来看"；
//    * pitch 俯仰角 θ（度）：8（近地平线）~ 89（正俯视）；缺省 35°；
//    * scale 视口比例（控件像素/画布单位）——正交无"距离"概念，滚轮
//      dolly 直接改 scale（等价 2D 的 fitScale×view_zoom）；
//    * (tx,tz) 注视点（画布域；恒在地面 h=0）——orbit 旋转中心 + pan 目标。
//
//  基向量（屏幕右 R / 屏幕上 U / 视线 F——{R,U,−F} 构成右手系，即
//  R×U = −F，与 OpenGL 相机惯例一致；三基向量在下方公式中自洽）：
//    F = (−sinθ·sinφ, −cosθ, −sinθ·cosφ)   视线（相机 → 注视点）
//    R = ( cosφ, 0, −sinφ )                屏幕右（恒水平）
//    U = ( −sinφ·cosθ, sinθ, −cosφ·cosθ )  屏幕上
//  φ=0 自检：相机在 +y 侧俯视，F=(0,−cosθ,−sinθ)；R=(1,0,0)——top-down
//  时画布 x 向屏幕右、y 向屏幕下，与 2D 画布完全一致。
//  投影：sx = cx + dot(P−T,R)·scale；sy = cy − dot(P−T,U)·scale。
//  高度只进 U 方程（R 恒水平）→ 悬浮片段在屏幕上抬升 h·sinθ·scale 像素。
//
//  反投影（拾取/拖拽）：控件点 → 视线射线 → 高度 h 水平面交点 → 画布域。
//  planeRayCoeffs 把交点拆成 (tx+A, tz+B)——A/B 不含注视点，pan 锚定
//  （"光标下的世界点保持不动"）因此可闭式解：tx' = 世界x − A(wx')。
//  交互数学（拖动增量/缩放/旋转方向角）全部复用画布域 2D 纯函数
//  （CanvasGeometry.hpp）——本类只负责"屏幕 ⇄ 画布平面"双向换算。
// ============================================================================

#pragma once

#include <QPointF>

#include <cmath>

namespace ms {

struct Camera3D {
    // ---- 轨道参数（见文件头）------------------------------------------------
    double yaw_deg   = 0.0;   ///< 方位角（0 = 画布正面；右键拖拽 orbit）
    double pitch_deg = 35.0;  ///< 俯仰角（kPitchMin..kPitchMax；缺省 35°）
    double scale     = 1.0;   ///< 视口比例（控件像素/画布单位；resetView=fit）
    double tx        = 0.0;   ///< 注视点 x（画布域）
    double tz        = 0.0;   ///< 注视点 y（画布域）

    /// 俯仰钳制（下限 8°：正弦过小会让地面反投影发散到无穷远）
    static constexpr double kPitchMin = 8.0;
    static constexpr double kPitchMax = 89.0;
    /// 缺省俯仰（resetView 归位值；Blender 缺省视角的观感）
    static constexpr double kPitchHome = 35.0;

    // ---- 派生基向量（每次现算：6 次三角函数，paint 期可忽略）----------------
    void basis(double* rx, double* rz,          ///< 屏幕右 R（y 分量恒 0）
               double* ux, double* uy, double* uz, ///< 屏幕上 U
               double* fx, double* fy, double* fz) const { ///< 视线 F
        const double ph = yaw_deg * (3.14159265358979323846 / 180.0);
        const double th = pitch_deg * (3.14159265358979323846 / 180.0);
        const double sp = std::sin(ph), cp = std::cos(ph);
        const double st = std::sin(th), ct = std::cos(th);
        *rx = cp;       *rz = -sp;                      // R = (cosφ, 0, −sinφ)
        *ux = -sp * ct; *uy = st;   *uz = -cp * ct;     // U = (−sinφcosθ, sinθ, −cosφcosθ)
        *fx = -sp * st; *fy = -ct;  *fz = -cp * st;     // F = (−sinθsinφ, −cosθ, −sinθcosφ)
    }

    // ---- 正投影：世界 (x=画布x, h=高度, z=画布y) → 控件点 --------------------
    /// \param ccx,ccy 控件中心（注视点恒投影到控件中心）
    QPointF project(double x, double h, double z, double ccx, double ccy) const {
        double rx, rz, ux, uy, uz, fx, fy, fz;
        basis(&rx, &rz, &ux, &uy, &uz, &fx, &fy, &fz);
        const double dx = x - tx, dz = z - tz;
        const double u = dx * rx + dz * rz;             // 屏幕（右+）
        const double v = dx * ux + h * uy + dz * uz;    // 屏幕（上+）
        return QPointF(ccx + u * scale, ccy - v * scale);
    }

    /// 视深（painter's algorithm 排序键）：dot(P−T, F)，越大越远（先画）。
    /// 正交投影下同一射线上的点深度单调——遮挡关系即排序关系。
    double depthOf(double x, double h, double z) const {
        double rx, rz, ux, uy, uz, fx, fy, fz;
        basis(&rx, &rz, &ux, &uy, &uz, &fx, &fy, &fz);
        return (x - tx) * fx + h * fy + (z - tz) * fz;
    }

    /// 反投影系数：控件点 (wx,wy) 的视线与高度 h 水平面的交点 =
    /// 画布域 (tx + A, tz + B)。A/B 只依赖控件点与相机姿态（不含注视点）
    /// ——pan 锚定据此闭式解：改注视点后交点平移量 = 注视点位移。
    void planeRayCoeffs(double wx, double wy, double ccx, double ccy, double h,
                        double* A, double* B) const {
        double rx, rz, ux, uy, uz, fx, fy, fz;
        basis(&rx, &rz, &ux, &uy, &uz, &fx, &fy, &fz);
        const double u = (wx - ccx) / scale;
        const double v = (ccy - wy) / scale;
        const double q0y = v * uy;          // 射线原点高度（注视点在地面）
        const double t  = (h - q0y) / fy;   // 平面交参数（F.y=−cosθ≠0 恒成立）
        *A = u * rx + v * ux + t * fx;
        *B = u * rz + v * uz + t * fz;
    }
};

} // namespace ms
