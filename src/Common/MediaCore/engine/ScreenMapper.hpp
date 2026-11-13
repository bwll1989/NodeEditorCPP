// ============================================================================
//  ScreenMapper.hpp —— 虚拟画布 → 本机输出映射 + 绘制列表解算
//
//  设计依据：《media-server-sync-design.md》§5.2 / §5.3 / §4.4
//    * ScreenMapper 职责（§5.2）：把虚拟画布坐标映射到本机物理输出分辨率——
//      本 screen 画布矩形 (sx,sy,sw,sh) × 本机输出 (ow,oh)：
//        拉伸模式  s_x = ow/sw, s_y = oh/sh          （LED 拼接屏像素对位）
//        等比模式  s = min(ow/sw, oh/sh)，居中留白    （Editor 预览窗）
//      任意画布点 (cx,cy) → 本机 ((cx-sx)*s_x + pad_x, (cy-sy)*s_y + pad_y)
//    * 绘制列表解算（§5.3 TimelinePlayer）：给定 session_time，对本机全部
//      screen 解出活跃 clip → 素材内部时间（TimelineEngine.evaluate，两端
//      同源纯函数）→ 本机坐标绘制矩形 → 交给 MediaScheduler 取帧 / Renderer
//      合成；渲染时用 viewport/scissor 限定本 screen 区域，跨屏素材自然
//      连续拼接（clip 全图按本 screen 映射绘制，出屏部分由 scissor 裁掉）。
//
//  纯函数组件：只读 Scene + 几何计算，无 IO 无时钟无媒体——单元测试无需
//  网络与文件；同一 (scene, t, 输出尺寸) 恒得同一列表（跨端一致根基）。
//
//  可见性过滤：与素材依赖计算（model/AssetDeps.hpp clipCoversScreen）同
//  语义——轴对齐开区间（贴边共线不算）、旋转外接圆保守扩展（宁多画不漏画），
//  完全不相交的 clip 不产生绘制项（省一次 produce 驱动与纹理绑定）。
// ============================================================================

#pragma once

#include <cmath>
#include <string>
#include <vector>

#include "engine/TimelineEngine.hpp"
#include "model/AssetDeps.hpp"
#include "model/SceneModel.hpp"

namespace ms {

// ---------------------------------------------------------------------------
// 映射模式（§5.2 的两种选项）
// ---------------------------------------------------------------------------
enum class FitMode {
    Stretch = 0, ///< 拉伸铺满：s_x=ow/sw、s_y=oh/sh——LED 拼接屏各屏独立
                 ///< 分辨率时的像素级对位（多屏拼接默认）
    Fit     = 1, ///< 等比适配：s=min(ow/sw, oh/sh)，居中留白——Editor 预览窗
};

// ---------------------------------------------------------------------------
// ScreenTransform —— 单个 screen 的画布→本机坐标【通用 2D 仿射】（每帧构造）
//
//   ox = a*cx + b*cy + c
//   oy = d*cx + e*cy + f
//
// 轴对齐屏：b=d=0，退化为原 (scale, offset) 线性映射（mapX/mapY 与
// 历史语义逐位一致——既有测试与快速路径的根基）；
// 旋转屏：屏是画布上绕中心顺时针转 rotation 度的【取景窗】，内容方向
// 跟随画布——屏中心映到输出中心、屏局部轴经逆旋转对齐输出轴，缩放取
// 等比 min（拉伸语义对旋转屏无定义）并居中（可能留白）。
// ---------------------------------------------------------------------------
struct ScreenTransform {
    double a = 1.0, b = 0.0, c = 0.0; ///< 输出 x = a*cx + b*cy + c
    double d = 0.0, e = 1.0, f = 0.0; ///< 输出 y = d*cx + e*cy + f
    bool   rotated = false;           ///< 屏旋转标记（true 时 DrawItem 走仿射路径）

    /// 由 screen 画布矩形 + 本机输出分辨率构造映射
    static ScreenTransform make(const Screen& scr, int out_w, int out_h,
                                FitMode mode = FitMode::Stretch) {
        ScreenTransform t;
        if (scr.w <= 0 || scr.h <= 0 || out_w <= 0 || out_h <= 0) return t; // 退化

        // 旋转屏：等比缩放 + 居中；屏中心 → 输出中心，屏局部轴逆旋转
        // 对齐输出轴（内容方向跟随画布——取景窗语义，见结构注释）
        if (scr.rotation != 0.0) {
            const double s = out_w / scr.w < out_h / scr.h
                                 ? out_w / scr.w : out_h / scr.h;
            const double rad  = scr.rotation * 3.14159265358979323846 / 180.0;
            const double ct   = std::cos(rad), st = std::sin(rad);
            // 屏中心（画布坐标）
            const double cx = scr.x + scr.w / 2.0;
            const double cy = scr.y + scr.h / 2.0;
            // o = s·R(-θ)·(p - C) + (ow/2, oh/2)，R(-θ)=[[ct,st],[-st,ct]]
            // 展开：A = s·R(-θ)，T = (ow/2,oh/2) - A·C
            t.a = s * ct;
            t.b = s * st;
            t.d = -s * st;
            t.e = s * ct;
            t.c = out_w / 2.0 - (t.a * cx + t.b * cy);
            t.f = out_h / 2.0 - (t.d * cx + t.e * cy);
            t.rotated = true;
            return t;
        }

        if (mode == FitMode::Fit) {
            // 等比：统一缩放比取两轴较小者，屏幕完整可见、居中留白
            const double s = out_w / scr.w < out_h / scr.h
                                 ? out_w / scr.w : out_h / scr.h;
            t.a = t.e = s;
            const double pad_x = (static_cast<double>(out_w) - scr.w * s) / 2.0;
            const double pad_y = (static_cast<double>(out_h) - scr.h * s) / 2.0;
            t.c = pad_x - scr.x * s;
            t.f = pad_y - scr.y * s;
        } else {
            // 拉伸：各轴独立缩放，铺满输出（无留白）
            t.a = static_cast<double>(out_w) / scr.w;
            t.e = static_cast<double>(out_h) / scr.h;
            t.c = -scr.x * t.a;
            t.f = -scr.y * t.e;
        }
        return t;
    }

    /// 画布点 → 本机坐标（通用：含旋转）
    void mapPoint(double cx, double cy, double& ox, double& oy) const noexcept {
        ox = a * cx + b * cy + c;
        oy = d * cx + e * cy + f;
    }

    /// 画布点 → 本机坐标 X（仅轴对齐屏 b==0 时与 mapPoint 一致——快速路径）
    double mapX(double canvas_x) const noexcept { return canvas_x * a + c; }
    /// 画布点 → 本机坐标 Y（同上，快速路径用）
    double mapY(double canvas_y) const noexcept { return canvas_y * e + f; }

    /// 画布长度（宽/高）→ 本机长度（只乘缩放，无平移；仅轴对齐屏有效）
    double mapSizeX(double canvas_w) const noexcept { return canvas_w * a; }
    double mapSizeY(double canvas_h) const noexcept { return canvas_h * e; }

    /// 逆仿射（本机坐标 → 画布坐标）：软渲染逆采样与命中测试用
    /// 预条件：矩阵部分非退化（make 产物 s>0 恒成立；异常输入由调用方防御）
    ScreenTransform inverse() const noexcept {
        ScreenTransform r;
        const double det = a * e - b * d;
        if (det == 0.0) return r; // 退化：恒等（调用方数据损坏时安全兜底）
        r.a =  e / det; r.b = -b / det;
        r.d = -d / det; r.e =  a / det;
        r.c = -(r.a * c + r.b * f);
        r.f = -(r.d * c + r.e * f);
        return r;
    }
};

// ---------------------------------------------------------------------------
// DrawItem —— 本帧单个绘制项（Renderer 的最小输入单元）
//
// 坐标语义：x/y/w/h 为【本机输出像素】矩形（画布矩形经所属 screen 映射）；
//   跨屏素材在本 screen 可能部分出屏（负坐标/超出 ow·oh）——属预期，
//   由 Renderer 的 scissor 裁剪（§5.2"跨屏素材自然连续拼接"）。
//   【Phase 1 前提】当前软渲染实现为"单 Player 单屏"——每端只解算本机
//   绑定的一个 screen，DrawItem 矩形即全画布输出范围，scissor 退化为
//   画布边界裁剪（SoftRenderer::draw 内实现）。多屏一卡的 GL 后端阶段
//   才需要真正的 viewport/scissor 切换；契约字段保留（screen 指针）。
// rotation/opacity/z 为画布属性透传（缩放不改变角度；拉伸模式下的旋转
//   畸变由 Renderer 阶段的矩阵组合精确处理：M_local = Scale × Canvas）。
// ---------------------------------------------------------------------------
struct DrawItem {
    const Clip*   clip   = nullptr; ///< 片段（借用指针，指向 Scene 内部）
    const Asset*  asset  = nullptr; ///< 素材元数据（取帧/类型判断用，可空）
    const Screen* screen = nullptr; ///< 所属屏幕（viewport/scissor 限定用）

    std::int64_t media_us = 0; ///< 素材内部时间（TimelineEngine 解算）

    double x = 0.0, y = 0.0;       ///< 本机像素矩形左上角（轴对齐时为目标
                                   ///< 矩形；有旋转时为四角变换后的外接 AABB）
    double w = 0.0, h = 0.0;       ///< 本机像素矩形尺寸
    double rotation = 0.0;         ///< 旋转角（度，顺时针，绕矩形中心）
    double opacity  = 1.0;         ///< 不透明度 [0,1]
    int    z        = 0;           ///< 叠放层级（小先绘、大后绘）

    // ---- 旋转渲染路径（ScreenMapper 提供几何，SoftRenderer 按帧尺寸合成）----
    // affine=false：轴对齐快速路径——x/y/w/h 即目标矩形，按 (px-左上)×比例
    //   直接采样（与历史实现逐像素一致，rotation 必为 0）；
    // affine=true ：x/y/w/h 为四角变换后的外接 AABB（仅作裁剪范围），
    //   每像素先经 inv_map 逆仿射还原画布点，再绕 clip 中心逆旋转落到
    //   未旋转矩形内采样帧像素（帧采样仿射由渲染器按实际帧尺寸合成）。
    bool   affine = false;                  ///< true = 走逆仿射旋转采样
    double inv_map[6] = {1, 0, 0, 0, 1, 0}; ///< output→canvas 逆仿射系数
    double canvas_x = 0.0, canvas_y = 0.0;  ///< clip 有效矩形左上（画布域，
                                            ///< 未旋转框架——affine 路径用）
    double canvas_w = 0.0, canvas_h = 0.0;  ///< clip 有效尺寸（画布域）
    double screen_rotation = 0.0;           ///< 所属屏旋转角（度；标注/GL 后端用）

    // ---- 锚点 + 四角点位移（2026-11-09，画布域；两端同源几何）-------------
    // anchor：Transform 透传（0=中心默认——历史"绕中心"行为逐位一致）；
    // corner：四角位移 px，序 = Transform::corner 同构（LT/RT/RB/LB × dx,dy），
    // 作用在旋转后的画布角上（屏幕空间拖角语义）；全 0 时渲染端走原快速
    // 路径（逐位不变）。
    int    anchor     = 0;   ///< 旋转/缩放枢轴（0=中心 1=左上 2=右上 3=右下 4=左下）
    double corner[8]  = {0, 0, 0, 0, 0, 0, 0, 0}; ///< 四角位移（画布域 px）

    // ---- 姿态（v3）+ 裁切 uv 窗（2026-11-09 v3）---------------------------
    // rot_x/rot_y：俯仰/偏航（度；非 0 时 ScreenMapper 经
    // cg::quadCorners3D 预投影四角到 quad[4][2]（画布域 CornerIdx 序），
    // 渲染端直接消费——跳过内联 2D 角生成）；
    // crop_u/v 窗口：裁切后帧纹理的采样窗（归一化 [0,1]，u1>u0、v1>v0），
    // 全 1 = 未裁切（渲染端各路径逐位退回全幅采样）。
    bool   pose3d    = false; ///< 俯仰/偏航非 0（渲染端 quad 预投影判定）
    double quad[4][2] = {{0, 0}, {0, 0}, {0, 0}, {0, 0}}; ///< 画布域四角
                                                           ///<（CornerIdx 序；
                                                           ///< pose3d 时有效）
    double crop_u0 = 0.0, crop_v0 = 0.0; ///< 裁切采样窗左上（归一化）
    double crop_u1 = 1.0, crop_v1 = 1.0; ///< 裁切采样窗右下（1,1=未裁切）

    /// 任一角点位移非 0（渲染端快速路径判定；与 Transform::cornerActive
    /// 同义——DrawItem 侧独立副本，NaN 视作非 0 走保守路径）
    bool cornerActive() const noexcept {
        for (int i = 0; i < 8; ++i)
            if (corner[i] != 0.0) return true;
        return false;
    }
};

/// 解算某 Player 本帧的绘制列表（§5.2 映射 × §5.3 活跃解算，纯函数）
///
/// \param scene     完整场景
/// \param player_id 本机 Player 标识（匹配 Screen.player_id）
/// \param out_w/out_h 本机输出分辨率（像素）
/// \param t_us      播放头（通常为 session_time）
/// \param mode      画布→本机映射模式（默认拉伸，拼接屏常态）
/// \return 绘制项列表；同 screen 内保持 evaluate 的 z 升序稳定序
std::vector<DrawItem> buildDrawList(const Scene& scene,
                                    const std::string& player_id,
                                    int out_w, int out_h,
                                    std::int64_t t_us,
                                    FitMode mode = FitMode::Stretch);

/// 预览解算：整个虚拟画布视作一块屏（Editor 预览所见即所得）
///
/// 与 buildDrawList 的差别：不经 player_id 匹配 screen——多屏布局拼合
/// 为一块全画布输出（scr-1/scr-2 各占半幅的画面在同一预览帧上连续可见），
/// 片段只画一次（display 端跨屏片段各屏各画一次的本机语义在此不适用）。
/// 可见性过滤/有效尺寸/z 序与正式渲染同源（两端同帧同结论）。
///
/// \param preview_scr 调用方持有的虚拟屏（DrawItem.screen 借用其地址，
///        须在返回列表使用期间存活；通常 x=0,y=0,w=canvas_w,h=canvas_h）
/// \param out_w/out_h 预览输出分辨率（建议=画布尺寸 1:1，widget 端再缩放）
std::vector<DrawItem> buildPreviewDrawList(const Scene& scene,
                                           const Screen& preview_scr,
                                           int out_w, int out_h,
                                           std::int64_t t_us);

} // namespace ms
