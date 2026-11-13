// ============================================================================
//  SceneAssembler.hpp —— standalone 时间轴装配器（编排意图 → SceneModel）
//
//  定位（display×MMC 合并方案 v2.2，2026-11-11）：
//    M2 网页（缩水版 Editor）提交的编排意图（片段清单 + 基本变换 + 淡入
//    淡出）在此装配成渲染引擎直接消费的 Scene。与 editor 全功能编辑链的
//    差异（MVP 定案）：
//      * 素材 = 本机文件（assets_root 相对路径），探测后直接入表——
//        无网络分发（standalone 素材本地就绪）；
//      * 变换 = 静态 transform（位置/缩放/层级/不透明度）——无 tween
//        动画编排（淡入淡出除外，v2.2 定案进第一版）；
//      * 淡入淡出 = 首/尾 Opacity+AudioGain 双通道同步打 key（音频视频
//        同淡；图片无音频，gain 通道无害）。
//    装配产物为不可变快照（shared_ptr<const Scene>）——交给
//    TimelineRegistry 登记，NodeController 切换时整体替换场景槽。
//
//  探测口径（与 editor 导入链同源，2026-11-11 音频时长联动修复对齐）：
//    * Video/Image：FfmpegMediaSource::prepare（容器头探测，不解码帧）——
//      图片本质"单帧媒体"，同一解码路径产出宽高；
//    * Audio：FfmpegMediaSource::probeDurationUs（静态容器探测——音频无
//      视频流，prepare 必失败，勿复用实例路径）；
//    * Text：无文件（text_content 随场景走），宽高走 TextRasterizer::measure
//      （与编辑器导入/两端源装配三处测量同源）。
//
//  Screen 绑定：装配产物恒含一个覆盖全画布的 screen（player_id = 本机
//  生效 id）——渲染端 composeFrame 的绘制项按"clip 矩形 ∩ 本机 screen
//  矩形"解算，无 screen = items 恒 0 黑屏；输出光栅尺寸在 GL 后端下
//  跟随窗口客户区（显示分辨率模型 2026-11-10），screen 矩形只管取景。
// ============================================================================

#pragma once

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

#include "model/SceneModel.hpp"

namespace ms::standalone {

// ---------------------------------------------------------------------------
// 编排意图（M2 网页 JSON 的直接映射；M1 由 main 的演示/CLI 构造）
// ---------------------------------------------------------------------------

/// 单片段
struct ClipSpec {
    /// 素材文件（相对 assets_root，'/' 分隔）；Text 片段留空、填 text
    std::string file;
    /// Text 片段内容（file 为空时生效；样式默认雅黑/48px/白字）
    std::string text;

    std::int64_t start_us    = 0; ///< 时间轴起点
    std::int64_t duration_us = 0; ///< 播放时长；0 = 自动（视频/音频用探测
                                  ///< 时长；图片恒 0 → 必须显式给出）
    std::int64_t in_us       = 0; ///< 素材内部入点

    // ---- 基本变换（静态 transform；MVP 定案：位置/缩放/层级/不透明度）----
    // 坐标一律画布绝对像素（2026-11-11 用户定案：渲染点对点，勿用归一化
    // 0~1——面板与文档同域，所见即所得）
    double x = 0.0, y = 0.0;      ///< 画布位置（左上角，px）
    double w = 0.0, h = 0.0;      ///< 绘制尺寸（px）；0 = 素材自然尺寸
    double opacity = 1.0;         ///< 静态不透明度（无淡入淡出时生效）
    int    z       = 0;           ///< 轨内叠放层级 [0,999]（跨轨层级=轨道
                                  ///< 次序；装配端钳制防穿轨，见 assemble）

    // ---- 旋转 / 裁剪（2026-11-11 补齐；与 editor Transform v3 同语义）------
    double rotation = 0.0;        ///< 旋转角度（度，顺时针，绕锚点=默认中心）
    /// 裁剪（画布 px，相对未裁切绘制矩形边缘向内收；被裁区域不显示、
    /// 内容不缩放——Transform::crop 同语义，负值装配端钳 0）
    double crop_l = 0.0, crop_r = 0.0, crop_t = 0.0, crop_b = 0.0;

    // ---- 淡入淡出（v2.2 定案进第一版；>0 生效）-----------------------------
    /// 淡入/淡出时长（us）：首/尾 Opacity+AudioGain 双通道同步打 key
    /// ——音频视频同淡；时长自动钳制防重叠（in + out ≤ 片段时长）
    double fade_in_us  = 0;
    double fade_out_us = 0;
};

/// 单轨（片段列表；MVP 不做轨道类型约束——混合轨道合法）
struct TrackSpec {
    std::string name;                 ///< 轨道名（诊断/网页显示）
    std::vector<ClipSpec> clips;
};

/// 整个时间轴
struct SceneSpec {
    double canvas_w = 1920.0;         ///< 画布宽（虚拟画布像素）
    double canvas_h = 1080.0;         ///< 画布高
    std::vector<TrackSpec> tracks;
};

// ---------------------------------------------------------------------------
// 装配器
// ---------------------------------------------------------------------------
class SceneAssembler {
public:
    /// \param assets_root 素材根目录（UTF-8；探测与渲染共用同一寻址约定：
    /// Asset.path 为相对路径，运行时拼 assets_root）
    explicit SceneAssembler(std::string assets_root);

    /// 装配场景。
    /// \param player_id 本机生效 id（装配产物的 Screen.player_id——渲染端
    /// 选屏依据；main 传 PlayerRuntime::effectiveId()）
    /// \param err 失败原因（部分片段失败不阻断——跳过并追加说明；
    /// 返回 nullptr = 全部片段失败/无片段）
    std::shared_ptr<const Scene> assemble(const SceneSpec& spec,
                                          const std::string& player_id,
                                          std::string& err) const;

private:
    /// 单素材探测（文件 → Asset 元数据填充；kind 按扩展名推断）。
    /// \return false = 文件不存在/类型不识别/探测失败（err 说明）
    bool probeAsset(const std::string& file, Asset& out,
                    std::string& err) const;

public:
    /// 工程场景 → standalone 可渲染场景（静态；.show 工程加载产物经此
    /// 归一化）。screens 整体替换为"本机单屏全画布"——渲染端绘制项按
    /// "clip 矩形 ∩ 本机 screen 矩形"解算（composeFrame 按
    /// screen.player_id 过滤），而工程 screens 绑定的是 editor 端登记的
    /// display id，standalone 单机语义下整体换绑本机生效 id，否则 items
    /// 恒 0 黑屏。多屏工程布局（分屏取景）在单机全画布语义下不适用
    /// （M2 网页编排与 MPCDI 取景再扩展）。
    static std::shared_ptr<const Scene> adoptForStandalone(
        const Scene& src, const std::string& player_id);

    /// 单文件探测（编辑器 import_media 钩子共用，2026-11-12 动作联动）：
    /// 与成员 probeAsset 同口径但独立于 assets_root——full_utf8 即寻址
    /// 路径（编辑器空素材根语义：Asset.path = 绝对路径）。
    /// 【为何是本类的静态成员】FfmpegMediaSource.hpp 含
    /// atomic<shared_ptr>（C++20）——C++17 编译的宿主 TU（Flow 主程序）
    /// 不能直接 include，探测体必须住在 MediaCore（C++20）内。
    /// \param kind 由调用方按扩展名推断的素材种类（本函数不做类型裁决；
    ///        支持探 Text：读文件内容 + measure 回填自然尺寸）
    /// \param out 成功时填充 kind/path/size_bytes/duration/width/height
    ///        （path = full_utf8；id 不在此生成——调用方入表前自定）
    /// \return false = 文件不存在/探测失败（err 说明）
    static bool probeFile(const std::string& full_utf8, AssetKind kind,
                          Asset& out, std::string& err);

private:
    std::string assets_root_;
};

} // namespace ms::standalone
