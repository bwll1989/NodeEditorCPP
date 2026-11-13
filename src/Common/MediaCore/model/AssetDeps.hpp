// ============================================================================
//  AssetDeps.hpp —— 素材依赖计算（设计补丁：实现计划 P2 §Distributor）
//
//  设计依据：《media-server-sync-design.md》§5.5 + 实现计划 P2：
//    "素材依赖计算按 clip 绘制矩形 ∩ screen 矩形，覆盖跨屏素材"
//
//  动机：三屏拼接时，一条跨屏大素材（如 1920×540 全画布视频）会被多个
//  Player 同时需要；而只画在 screen-2 上的小图片，player-1/3 拉了也用
//  不上。Player 收到场景后按【本机绑定 screen 的矩形】过滤素材表——
//  只拉取绘制矩形与本机 screen 相交的素材，节省带宽与磁盘。
//
//  保守原则（宁多拉、不漏拉）：
//    * 旋转 clip 不做精确多边形求交——用外接圆扩展 AABB（旋转不改变
//      外接圆），保证任何实际覆盖本屏的 clip 都不会因旋转被漏判；
//    * 轴对齐情形用开区间 AABB：仅边缘相触不算相交——三屏无缝拼接是
//      常态（相邻 clip 贴边共线），若相触算相交则相邻屏互相拉取素材；
//    * 找不到本机 screen（未绑定/拼错 id）→ 依赖为空（不拉素材），
//      渲染层按占位处理并打诊断日志。
//
//  纯函数组件：只读 Scene，无 IO 无时钟——单元测试无需网络与文件。
// ============================================================================

#pragma once

#include <algorithm>
#include <cmath>
#include <string>
#include <vector>

#include "model/SceneModel.hpp"

namespace ms {

/// 判断 clip 的画布绘制矩形是否覆盖到 screen 矩形（开区间 AABB +
/// 旋转保守扩展）
///
/// \param clip_t clip 的变换属性（x/y/w/h 为画布坐标绘制矩形；rotation 顺时针度）
/// \param scr    目标 screen（画布坐标矩形）
/// \return true = 覆盖（含部分重叠）；false = 不覆盖（贴边共线不算）
///
/// 旋转处理：不做精确多边形裁剪（P2 无渲染管线依赖此精度）——旋转后
/// AABB 的对角线长度不变，用外接圆半径 r = √(hw²+hh²) 同时扩展两个轴，
/// 等价于"旋转后 AABB 的最大可能范围"，方向保守（多判相交）。
inline bool clipCoversScreen(const Transform& clip_t, const Screen& scr) {
    if (clip_t.w <= 0 || clip_t.h <= 0 || scr.w <= 0 || scr.h <= 0)
        return false; // 退化矩形（未设置尺寸）不参与依赖判定

    double cx = clip_t.x + clip_t.w / 2.0;
    double cy = clip_t.y + clip_t.h / 2.0;
    double hw = clip_t.w / 2.0;
    double hh = clip_t.h / 2.0;

    // ---- screen 旋转：圆-圆保守判定（宁多拉不漏拉）----
    // 旋转屏的区域是斜矩形，精确多边形裁剪超出 P2 精度需求——clip 与
    // screen 都取各自外接圆（clip 圆已含其旋转；screen 圆含屏旋转），
    // 圆心距 ≤ 半径和即判相交。保守方向安全：多拉素材只是多一次下载，
    // 漏拉则 Player 黑屏。
    if (scr.rotation != 0.0) {
        const double scx = scr.x + scr.w / 2.0;
        const double scy = scr.y + scr.h / 2.0;
        const double r_clip   = std::sqrt(hw * hw + hh * hh);
        const double r_screen = std::sqrt(scr.w * scr.w + scr.h * scr.h) / 2.0;
        const double dx = cx - scx, dy = cy - scy;
        const double rr = r_clip + r_screen;
        return dx * dx + dy * dy <= rr * rr;
    }

    if (clip_t.rotation != 0.0) {
        // 外接圆半径 = 矩形对角线一半：旋转任意角度后 AABB 半宽/半高
        // 都不会超过它——用圆近似旋转包围盒，绝无漏判
        const double r = std::sqrt(hw * hw + hh * hh);
        hw = r;
        hh = r;
    }

    // AABB 区间相交【开区间】：仅边缘相触不算（三屏无缝拼接是常态——
    // 相邻 clip 贴边共线时互不依赖；轴对齐情形开区间即精确语义，
    // "保守不漏拉"由旋转的外接圆扩展保证）
    return (cx + hw > scr.x) && (cx - hw < scr.x + scr.w) &&
           (cy + hh > scr.y) && (cy - hh < scr.y + scr.h);
}

/// 计算指定 Player（按其绑定的 screen）需要的素材 ID 集合
///
/// 遍历全部轨道片段：绘制矩形与本机任一 screen 相交 → 依赖其 asset_id。
/// \param scene     完整场景（素材表 + 时间轴 + 屏幕布局）
/// \param player_id 本机 Player 标识（与 Screen.player_id 匹配）
/// \return 依赖的素材 ID 列表（按时间轴遍历序去重；不含 Text 素材——
///          文本内容随场景分发，无文件可拉）
inline std::vector<std::string> computeAssetDeps(const Scene& scene,
                                                 const std::string& player_id) {
    std::vector<std::string> deps;
    auto push_unique = [&deps](const std::string& id) {
        if (!id.empty() &&
            std::find(deps.begin(), deps.end(), id) == deps.end())
            deps.push_back(id);
    };

    // ---- 音频素材无条件依赖（2026-11-11 音频同步修复）--------------------
    // 音频不绘制（无像素宽高）——几何判定恒 false，此前音频素材被所有
    // 显示端跳过（"素材不在本机依赖内，跳过"无限刷屏 + display 无声）。
    // 音频不属于任何 screen：只要时间轴上有片段引用它，本机在跑就拉
    //（宁多拉不漏拉；视频素材的音轨随素材文件自然到达，不受此段影响）。
    for (const auto& track : scene.timeline.tracks) {
        for (const auto& clip : track.clips) {
            if (const Asset* a = scene.findAsset(clip.asset_id);
                a && a->kind == AssetKind::Audio)
                push_unique(clip.asset_id);
        }
    }

    // 素材 id → 是否无文件素材（文本/实时源随场景 pb 分发，任何情况都不拉）
    // 【2026-11-08 Live Hub】Live 素材的源名/URL 随 pb 走——display 端
    // LiveMediaSource 直接连 editor 再发布的流，与素材文件分发通道无关。
    auto is_fileless_asset = [&scene](const Asset& a) {
        return a.kind == AssetKind::Text || a.kind == AssetKind::Live;
    };

    for (const auto& scr : scene.screens) {
        if (scr.player_id != player_id) continue; // 只看本机绑定的 screen

        for (const auto& track : scene.timeline.tracks) {
            for (const auto& clip : track.clips) {
                const Asset* asset = scene.findAsset(clip.asset_id);
                if (!asset || is_fileless_asset(*asset))
                    continue; // 未绑定/文本/实时源：无文件可拉
                if (asset->kind == AssetKind::Audio)
                    continue; // 音频已在上面的无条件段入列
                // ---- 有效绘制尺寸：transform.w<=0 → 沿用素材原始尺寸
                //（模型语义）——与 ScreenMapper::buildDrawItemsForScreen
                //【同源同规则】。若直接用原始 w/h 判定，auto-size clip
                //（w=-1 跟随素材）会被误判退化矩形 → 素材永不拉取，
                // Editor 预览正常而 Player 黑屏且无报错。
                const double eff_w = clip.transform.w > 0
                                         ? clip.transform.w
                                         : (asset ? asset->width : 0.0);
                const double eff_h = clip.transform.h > 0
                                         ? clip.transform.h
                                         : (asset ? asset->height : 0.0);
                Transform judge = clip.transform;
                judge.w = eff_w;
                judge.h = eff_h;
                if (!clipCoversScreen(judge, scr)) continue;
                // 去重后加入（跨屏共享素材只记一次）
                push_unique(clip.asset_id);
            }
        }
    }
    return deps;
}

} // namespace ms
