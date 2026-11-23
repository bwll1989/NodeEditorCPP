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

    // Show 全局透视仪（单一透视）：屏姿态单应与素材 3D 投影共用同一条
    // 投影链（提前一次求值，循环内复用）
    double ef = 0.0, evx = 0.0, evy = 0.0;
    scene.effectivePersp(ef, evx, evy);
    const ScreenTransform st =
        ScreenTransform::make(scr, out_w, out_h, mode, ef, evx, evy);

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
        if (!clipCoversScreen(judge, scr, ef, evx, evy)) continue;

        // ---- 组装绘制项 ----
        DrawItem di;
        di.clip     = &c;
        di.asset    = asset;
        di.screen   = &scr;
        di.media_us = ac.media_us;      // 素材内部时间（in_us + 播放头偏移）
        di.rotation = judge.rotation;   // 画布属性透传
        di.opacity  = judge.opacity;
        // 颜色合成（2026-11-20）：片段级 × 屏幕级（Display 输出校色）——
        // 增益相乘、gamma 相乘（pow(c,1/g1) 后再 pow(·,1/g2) ≡
        // pow(c,1/(g1·g2))，忽略中间 clamp 的微差）。合成后下游单一来源，
        // 渲染端零感知两级来源；两级全默认时渲染端走原快速路径逐位退化。
        for (int i = 0; i < 3; ++i) di.rgb_gain[i] = judge.rgb_gain[i] * scr.rgb_gain[i];
        di.gamma    = judge.gamma * scr.gamma;
        di.z        = judge.z;
        // 锚点 + 四角位移透传（2026-11-09；corner 全 0 时渲染端走原快速路径）
        di.anchor   = judge.anchor;
        for (int i = 0; i < 8; ++i) di.corner[i] = judge.corner[i];
        // 裁切采样窗（v3）：几何全按裁后可见矩形解算（下游零感知），
        // uv 窗在此一处换算——渲染端只消费归一化窗
        di.crop_u0 = cu0; di.crop_v0 = cv0;
        di.crop_u1 = cu1; di.crop_v1 = cv1;

        if (st.rotated || st.posed || judge.rotation != 0.0 ||
            judge.cornerActive() || judge.pose3DActive()) {
            // ---- 变形路径：本机矩形不再是轴对齐目标 ----
            // （clip 旋转 / 屏旋转 / 屏姿态 / 四角位移 / 俯仰偏航任一触发）
            // 1) 目标四角（画布域）：2D = 绕【锚点】R(θ) + 位移（唯一真相
            //    cg::quadCornersWithAnchor）；俯仰/偏航非 0 →
            //    cg::quadCorners3DP（3D 旋转 + 透视投影 + 滚转，无姿态时
            //    逐位退化 2D 公式）。
            //    外接 AABB：SoftRenderer 只在 AABB 内逐像素逆采样（位移
            //    后的角参与 AABB——变形出界部分不丢）。
            //    【裁切】四角几何用【裁后可见矩形】（judge.x/y/w/h 已是可见
            //    矩形）——uv 窗把帧纹理对应区域映射进来（di.crop_u/v）。
            double min_x = 0.0, min_y = 0.0, max_x = 0.0, max_y = 0.0;
            double wx[4], wy[4];
            if (judge.pose3DActive()) {
                // 3D 位姿（素材 3D 化阶段②）：cg::quadCorners3DP 统一投影
                // ——几何深度 pos_z + Show 级透视参数（【单一透视
                // 2026-11-18】Scene::effectivePersp——persp_* 未配置时
                // 画布派生默认 F=2×对角线/vp=画布中心，全场共享同一投影
                // 仪；画布无效才退片段隐含焦距）+ 俯仰/偏航/滚转/锚点/
                // 角位移。q4 = 每次投影权重 s=F/(F−Z)（渲染端顶点
                // (uv·q, q) + FS 采样 uv/q 精确复现平面单应）。
                // ef/evx/evy 已在函数头求值（屏姿态单应共用）
                canvas_geometry::quadCorners3DP(
                    judge.x, judge.y, judge.w, judge.h, judge.pos_z,
                    judge.rotation, judge.rot_x, judge.rot_y, judge.anchor,
                    judge.corner, ef, evx,
                    evy, wx, wy, di.quad_q);
                di.pose3d = true;
                for (int i = 0; i < 4; ++i) { // CornerIdx 序预投影四角
                    di.quad[i][0] = wx[i];
                    di.quad[i][1] = wy[i];
                }
            } else {
                // 2D 变形（clip 旋转 / 屏旋转 / 四角位移）：四角唯一真相 =
                // cg::quadCornersWithAnchor（CornerIdx 序）。
                canvas_geometry::quadCornersWithAnchor(judge.x, judge.y,
                                                       judge.w, judge.h,
                                                       judge.rotation,
                                                       judge.anchor,
                                                       judge.corner, wx, wy);
            }
            // 【贴图语义定案 2026-11-23 用户拍板：corner 拖角 = 双线性，
            // 无透视】拖角把素材拉成任意四边形时，内容随四条边【线性】
            // 拉伸（bilinear 曲面）——非平面单应（单应含透视收敛：对边
            // 不平行时内容近大远小，用户不要）。渲染端消费 di.quad 四角
            // 细分 N×N 网格逐格仿射逼近双线性——无透视收敛、无两三角
            // 方案的对角折痕（细分≥16 时残差 <0.1px）。历史教训：
            // v1/v2（5edbc057/120f2b6f）曾把该路径修成单应 q 权重——数学
            // 自洽但语义超纲（用户实测"拖角有透视感/被复杂化"），整链
            // 撤回；最初"折线"bug 的正确解法就是细分，不是单应。
            // 3D 姿态分支（pose3d）不受影响：quadCorners3DP 的 s 权重
            // 保底语义照旧（冻结兼容层，勿扩；角位移已烘焙进预投影四角）。
            // 【触发面收紧 2026-11-23 审查】双线性仅 corner 拖角需要：
            // 纯 clip 旋转/屏旋转（无角位移）= 平行四边形，渲染端 4 顶点
            // 仿射路径（di.rotation+di.corner 重算四角）数学精确且省
            // 2112 顶点细分 + 42KB 顶点上传——bilinear 收紧到
            // cornerActive()（pose3d 优先级不变：两者同开走 pose3d）。
            if (!judge.pose3DActive() && judge.cornerActive()) {
                di.bilinear = true; // corner 拖角：渲染端走双线性细分
                for (int i = 0; i < 4; ++i) { // CornerIdx 序四角
                    di.quad[i][0] = wx[i];
                    di.quad[i][1] = wy[i];
                }
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

            // 2) output→canvas 逆映射（屏旋转仿射还原；屏姿态走单应——
            //    inv_map 恒等占位防仿射误用，渲染端按 screen_posed 分派；
            //    预条件 st 可逆——make 产物缩放恒正，数据损坏时安全兜底）
            if (st.posed) {
                di.screen_posed = true;
                for (int i = 0; i < 9; ++i) di.screen_inv_h[i] = st.h_o2c[i];
                for (int i = 0; i < 9; ++i) di.screen_h[i] = st.h_c2o[i];
                // inv_map 恒等占位（渲染端 screen_posed 分派后不消费）
            } else {
                const ScreenTransform inv = st.inverse();
                di.inv_map[0] = inv.a;
                di.inv_map[1] = inv.b;
                di.inv_map[2] = inv.c;
                di.inv_map[3] = inv.d;
                di.inv_map[4] = inv.e;
                di.inv_map[5] = inv.f;
            }
            di.affine   = true;

            // 3) clip 有效矩形（画布域，未旋转框架；裁切后 = 可见矩形）
            //    ——渲染器绕其中心做 clip 旋转的逆变换后按轴对齐矩形采样
            //    帧（uv 采样窗 = di.crop_u/v）
            di.canvas_x = judge.x;
            di.canvas_y = judge.y;
            di.canvas_w = judge.w;
            di.canvas_h = judge.h;
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
