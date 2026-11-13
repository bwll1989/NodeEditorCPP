// ============================================================================
//  ScreenMapper.cpp —— 绘制列表解算实现（纯函数，§5.2/§5.3）
// ============================================================================

#include "engine/ScreenMapper.hpp"

#include <algorithm>

#include "model/CanvasGeometry.hpp" // quadCornersWithAnchor（四角唯一真相）

namespace ms {

// ---------------------------------------------------------------------------
// 单屏解算（buildDrawList / buildPreviewDrawList 共用内核）
//   scr 须在 items 使用期间存活（DrawItem.screen 借用其地址——
//   正式路径指向 scene.screens 元素，预览路径指向调用方持有的虚拟屏）
// ---------------------------------------------------------------------------
static void buildDrawItemsForScreen(const Scene& scene, const EvalResult& eval,
                                    const Screen& scr, int out_w, int out_h,
                                    FitMode mode, std::vector<DrawItem>& items) {
    if (scr.w <= 0 || scr.h <= 0) return; // 退化 screen 防御

    const ScreenTransform st = ScreenTransform::make(scr, out_w, out_h, mode);

    for (const auto& ac : eval.active) {
        const Clip& c = *ac.clip;
        // 【Tween】消费逐帧解算变换（evaluate 已用 tween 通道覆写静态基准；
        // 无通道时 resolved == clip.transform，行为与历史逐像素一致）
        const Transform& tf = ac.resolved;

        // ---- 有效绘制尺寸：transform.w<=0 → 沿用素材原始宽度（模型语义）；
        //      素材缺失或尺寸仍非法 → 无法解算，跳过（上层按占位处理）----
        const Asset* asset = scene.findAsset(c.asset_id);
        const double eff_w = tf.w > 0 ? tf.w : (asset ? asset->width  : 0.0);
        const double eff_h = tf.h > 0 ? tf.h : (asset ? asset->height : 0.0);
        if (eff_w <= 0 || eff_h <= 0) continue;

        // ---- 裁切（v3）：名义矩形收缩为可见矩形 + 帧采样窗（归一化）----
        // 可见宽高钳 ≥1px（Transform::visibleRect 同口径——过裁宁显示
        // 一丝勿翻转）；uv 窗 = 裁切量 / 未裁切名义尺寸（内容不缩放——
        // 裁掉的部分从采样窗移除，可见区仍铺满裁后矩形）。
        double vis_x = tf.x, vis_y = tf.y, vis_w = eff_w, vis_h = eff_h;
        double cu0 = 0.0, cv0 = 0.0, cu1 = 1.0, cv1 = 1.0;
        if (tf.cropActive()) {
            tf.visibleRect(vis_x, vis_y, vis_w, vis_h);
            cu0 = std::clamp(tf.crop[kCropL] / eff_w, 0.0, 1.0);
            cv0 = std::clamp(tf.crop[kCropT] / eff_h, 0.0, 1.0);
            cu1 = std::clamp(1.0 - tf.crop[kCropR] / eff_w, cu0, 1.0);
            cv1 = std::clamp(1.0 - tf.crop[kCropB] / eff_h, cv0, 1.0);
        }

        // ---- 可见性过滤：与素材依赖计算同语义（开区间 ∩ + 旋转外接圆
        //      保守扩展）。用【裁后可见尺寸】判定；完全不相交的 clip 不
        //      产项，省一次取帧驱动与纹理上传 ----
        Transform judge = tf;
        judge.x = vis_x;
        judge.y = vis_y;
        judge.w = vis_w;
        judge.h = vis_h;
        if (!clipCoversScreen(judge, scr)) continue;

        // ---- 组装绘制项 ----
        DrawItem di;
        di.clip     = &c;
        di.asset    = asset;
        di.screen   = &scr;
        di.media_us = ac.media_us;      // 素材内部时间（in_us + 播放头偏移）
        di.rotation = judge.rotation;   // 画布属性透传
        di.opacity  = judge.opacity;
        di.z        = judge.z;
        // 锚点 + 四角位移透传（2026-11-09；corner 全 0 时渲染端走原快速路径）
        di.anchor   = judge.anchor;
        for (int i = 0; i < 8; ++i) di.corner[i] = judge.corner[i];
        // 裁切采样窗（v3）：几何全按裁后可见矩形解算（下游零感知），
        // uv 窗在此一处换算——渲染端只消费归一化窗
        di.crop_u0 = cu0; di.crop_v0 = cv0;
        di.crop_u1 = cu1; di.crop_v1 = cv1;

        if (st.rotated || judge.rotation != 0.0 || judge.cornerActive() ||
            judge.poseActive()) {
            // ---- 变形路径：本机矩形不再是轴对齐目标 ----
            // （clip 旋转 / 屏旋转 / 四角位移 / 俯仰偏航任一触发）
            // 1) 目标四角（画布域）：2D = 绕【锚点】R(θ) + 位移（唯一真相
            //    cg::quadCornersWithAnchor）；俯仰/偏航非 0 →
            //    cg::quadCorners3D（3D 旋转 + 透视投影 + 滚转，无姿态时
            //    逐位退化 2D 公式）。
            //    外接 AABB：SoftRenderer 只在 AABB 内逐像素逆采样（位移
            //    后的角参与 AABB——变形出界部分不丢）。
            //    【裁切】四角几何用【裁后可见矩形】（judge.x/y/w/h 已是可见
            //    矩形）——uv 窗把帧纹理对应区域映射进来（di.crop_u/v）。
            double min_x = 0.0, min_y = 0.0, max_x = 0.0, max_y = 0.0;
            double wx[4], wy[4];
            if (judge.poseActive()) {
                canvas_geometry::quadCorners3D(
                    judge.x, judge.y, judge.w, judge.h, judge.rotation,
                    judge.rot_x, judge.rot_y, judge.anchor, judge.corner,
                    wx, wy);
                di.pose3d = true;
                for (int i = 0; i < 4; ++i) { // CornerIdx 序预投影四角
                    di.quad[i][0] = wx[i];
                    di.quad[i][1] = wy[i];
                }
            } else {
                canvas_geometry::quadCornersWithAnchor(judge.x, judge.y,
                                                       judge.w, judge.h,
                                                       judge.rotation,
                                                       judge.anchor,
                                                       judge.corner, wx, wy);
            }
            for (int i = 0; i < 4; ++i) {
                double ox = 0.0, oy = 0.0;
                st.mapPoint(wx[i], wy[i], ox, oy);
                if (i == 0) {
                    min_x = max_x = ox;
                    min_y = max_y = oy;
                } else {
                    min_x = std::min(min_x, ox);
                    max_x = std::max(max_x, ox);
                    min_y = std::min(min_y, oy);
                    max_y = std::max(max_y, oy);
                }
            }
            di.x = min_x;
            di.y = min_y;
            di.w = max_x - min_x;
            di.h = max_y - min_y;

            // 2) output→canvas 逆仿射（屏旋转在此还原；预条件 st 可逆——
            //    make 产物缩放恒正，数据损坏时 inverse 已安全兜底恒等）
            const ScreenTransform inv = st.inverse();
            di.affine   = true;
            di.inv_map[0] = inv.a;
            di.inv_map[1] = inv.b;
            di.inv_map[2] = inv.c;
            di.inv_map[3] = inv.d;
            di.inv_map[4] = inv.e;
            di.inv_map[5] = inv.f;

            // 3) clip 有效矩形（画布域，未旋转框架；裁切后 = 可见矩形）
            //    ——渲染器绕其中心做 clip 旋转的逆变换后按轴对齐矩形采样
            //    帧（uv 采样窗 = di.crop_u/v）
            di.canvas_x = judge.x;
            di.canvas_y = judge.y;
            di.canvas_w = judge.w;
            di.canvas_h = judge.h;
            di.screen_rotation = scr.rotation;
        } else {
            // ---- 轴对齐快速路径（裁切 = 可见矩形直绘；uv 窗渲染端处理）----
            di.x = st.mapX(judge.x);
            di.y = st.mapY(judge.y);
            di.w = st.mapSizeX(judge.w);
            di.h = st.mapSizeY(judge.h);
        }
        items.push_back(di);
    }
}

std::vector<DrawItem> buildDrawList(const Scene& scene,
                                    const std::string& player_id,
                                    int out_w, int out_h,
                                    std::int64_t t_us,
                                    FitMode mode) {
    std::vector<DrawItem> items;

    // ---- 1. 全场景活跃解算（TimelineEngine 纯函数，z 升序稳定序）----
    //      两端同源：Editor 预览与 Player 渲染对同一 (scene, t) 得同一结论
    const EvalResult eval = TimelineEngine(scene).evaluate(t_us);

    // ---- 2. 逐本机 screen 生成绘制项（跨屏素材在每个相关 screen 各产一项，
    //         各按各自映射——出屏部分由 Renderer scissor 裁掉，拼接自然连续）----
    for (const auto& scr : scene.screens) {
        if (scr.player_id != player_id) continue; // 只解算本机绑定的 screen
        buildDrawItemsForScreen(scene, eval, scr, out_w, out_h, mode, items);
    }
    return items;
}

std::vector<DrawItem> buildPreviewDrawList(const Scene& scene,
                                           const Screen& preview_scr,
                                           int out_w, int out_h,
                                           std::int64_t t_us) {
    std::vector<DrawItem> items;
    // 全画布虚拟屏：不做 player 匹配；可见性/有效尺寸/z 序同正式渲染
    const EvalResult eval = TimelineEngine(scene).evaluate(t_us);
    buildDrawItemsForScreen(scene, eval, preview_scr, out_w, out_h,
                            FitMode::Stretch, items);
    return items;
}

} // namespace ms
