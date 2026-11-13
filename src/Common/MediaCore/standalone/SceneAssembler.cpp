// ============================================================================
//  SceneAssembler.cpp —— standalone 时间轴装配器实现
//
//  探测策略（与 editor 导入链同源，无 Qt 依赖版）：
//    扩展名推断 kind → 按种类探测元数据（失败不阻断导入，与 editor
//    "元数据仅影响默认显示尺寸/时长"同一容错哲学；但本装配器对视频/
//    音频探测失败直接跳过该片段——standalone 无占位帧兜底，宁缺勿错）。
//
//  淡入淡出打 key（v2.2 定案）：
//    Opacity 与 AudioGain 双通道同形（0 → 目标值），Linear 插值。
//    ⚠️ Keyframe 聚合构造恒五字段 {t_us, v0, v1, v2, interp}——工程铁律
//    （缺 v2 会把 interp 隐转 double 塞进 v2，静默错位）。
//    键 t_us 为片段局部时间 ∈ [0, duration_us]；淡入/淡出自动钳制防重叠。
// ============================================================================

#include "standalone/SceneAssembler.hpp"

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <sstream>
#include <utility>

#include "media/FfmpegMediaSource.hpp"
#include "media/TextRasterizer.hpp"

namespace ms::standalone {

namespace {

/// 扩展名小写化（含点；无扩展名返回空）
std::string lowerExt(const std::string& file) {
    const auto pos = file.rfind('.');
    if (pos == std::string::npos) return {};
    std::string ext = file.substr(pos);
    std::transform(ext.begin(), ext.end(), ext.begin(),
                   [](unsigned char c) { return std::tolower(c); });
    return ext;
}

/// 扩展名 → 素材种类（MVP 识别集；不识别 = Unknown → 拒绝装配）
AssetKind kindFromExt(const std::string& ext) {
    static const char* kVideo[] = {".mp4", ".mov", ".mkv", ".avi",
                                   ".webm", ".m4v", ".ts", ".flv"};
    static const char* kImage[] = {".png", ".jpg", ".jpeg", ".bmp",
                                   ".gif", ".webp"};
    static const char* kAudio[] = {".wav", ".mp3", ".aac", ".flac",
                                   ".ogg", ".m4a", ".wma"};
    for (const char* e : kVideo) if (ext == e) return AssetKind::Video;
    for (const char* e : kImage) if (ext == e) return AssetKind::Image;
    for (const char* e : kAudio) if (ext == e) return AssetKind::Audio;
    return AssetKind::Unknown;
}

} // namespace

// ---------------------------------------------------------------------------
// 构造
// ---------------------------------------------------------------------------
SceneAssembler::SceneAssembler(std::string assets_root)
    : assets_root_(std::move(assets_root)) {}

// ---------------------------------------------------------------------------
// 单文件探测（静态；成员 probeAsset 与编辑器 import_media 钩子共用体）：
// full_utf8 即寻址路径（调用方已按各自素材根语义拼好）。
// ---------------------------------------------------------------------------
bool SceneAssembler::probeFile(const std::string& full_utf8, AssetKind kind,
                               Asset& out, std::string& err) {
    out = Asset{};
    out.kind = kind;
    out.path = full_utf8; // 空素材根（编辑器）= 绝对路径；standalone = 相对

    // 文件存在性 + 大小（探测前置；standalone 无网络分发，缺文件即跳过）
    // [S1] 全链 UTF-8（round7 S1-1 修复）：assets_root_ 与 file 均为
    // UTF-8 字节串（JSON 协议约定），统一先拼 UTF-8 全路径再经 char8_t
    // 语义构造 path（绕开 MSVC 窄字符默认 ACP 解码坑，与
    // FfmpegMediaSource.cpp:282 char8_t 先例同源）。原实现 fs::path /
    // std::string 混编码把中文文件名按 ACP 折叠 → exists() 失败
    //（"文件不存在"静默跳片段），且 err 分支 .string()（宽→窄 ACP 反向
    // 转换）对不可逆字节序列抛 std::system_error → 500 异常泄漏。
    const std::filesystem::path full(
        reinterpret_cast<const char8_t*>(full_utf8.c_str()));
    std::error_code ec;
    if (!std::filesystem::exists(full, ec)) {
        err = "文件不存在: " + full_utf8;
        return false;
    }
    ec.clear();
    const auto sz = std::filesystem::file_size(full, ec);
    out.size_bytes = ec ? 0 : static_cast<std::uint64_t>(sz);

    // ---- 元数据探测（与 editor 导入链同口径）------------------------------
    if (kind == AssetKind::Video || kind == AssetKind::Image) {
        // 视频与图片统一 FFmpeg 容器头探测（打开容器即返回，不解码帧）——
        // 图片本质"单帧媒体"，同一解码路径产出宽高；duration：视频取容器
        // 值、图片恒 0（任意时长显示）。
        // 【音频勿走此路】纯音频无视频流，prepare 的 av_find_best_stream
        // (VIDEO) 必失败——下方走静态 probeDurationUs（2026-11-11 修复同源）。
        // assets_root 传空：full_utf8 已是完整路径（本函数独立于实例
        // assets_root_——静态成员无该成员），path 原样寻址。
        FfmpegMediaSource probe(std::string(), kind);
        if (!probe.prepare(out)) {
            err = "媒体探测失败: " + full_utf8;
            return false;
        }
        out.width  = probe.width();
        out.height = probe.height();
        if (kind == AssetKind::Video) out.duration_us = probe.duration_us();
        probe.release();
    } else if (kind == AssetKind::Audio) {
        // 音频：静态容器探测（不建解码上下文）；UTF-8 路径直通 ffmpeg
        out.duration_us = FfmpegMediaSource::probeDurationUs(full_utf8);
    } else if (kind == AssetKind::Text) {
        // 文本文件（编辑器 .txt 导入，2026-11-12）：读内容 + measure 回填
        // 自然尺寸（与 onAddClip 默认文本素材同款默认样式/同源测量）。
        // standalone 的 kindFromExt 不产 Text（编排意图 text 随 JSON 走），
        // 此分支仅编辑器直调 probeFile 时可达。
        std::ifstream f(full, std::ios::binary);
        if (!f) {
            err = "文本读取失败: " + full_utf8;
            return false;
        }
        std::ostringstream ss;
        ss << f.rdbuf();
        out.text_content   = ss.str(); // 原样 UTF-8 字节
        out.text_font      = "MicrosoftYaHei"; // 默认字体（映射表中文兜底）
        out.text_font_size = 48;
        out.text_color     = 0xFFFFFF;
        const TextLayout lay =
            TextRasterizer::measure(TextRasterizer::styleFromAsset(out));
        out.width  = lay.width;
        out.height = lay.height;
    }
    return true;
}

// ---------------------------------------------------------------------------
// 单素材探测（文件 → Asset 元数据填充；成员版：拼 assets_root 全路径后
// 转调静态 probeFile——探测口径单一真相，编辑器钩子共用）
// ---------------------------------------------------------------------------
bool SceneAssembler::probeAsset(const std::string& file, Asset& out,
                                std::string& err) const {
    const AssetKind kind = kindFromExt(lowerExt(file));
    if (kind == AssetKind::Unknown) {
        err = "不识别的文件类型: " + file;
        return false;
    }

    std::string full_utf8; // UTF-8 全路径（err 消息 / probeDurationUs 用）
    if (assets_root_.empty()) {
        full_utf8 = file; // 无根目录：file 即可用路径（测试场景）
    } else {
        full_utf8 = assets_root_;
        if (full_utf8.back() != '/' && full_utf8.back() != '\\')
            full_utf8 += '/';
        full_utf8 += file;
    }
    if (!probeFile(full_utf8, kind, out, err)) return false;
    out.path = file; // 成员版保持相对路径语义（渲染端 FfmpegMediaSource
                     // 拼 assets_root_）；probeFile 出参为完整路径
    return true;
}

// ---------------------------------------------------------------------------
// 装配
// ---------------------------------------------------------------------------
std::shared_ptr<const Scene> SceneAssembler::assemble(
    const SceneSpec& spec, const std::string& player_id,
    std::string& err) const {

    auto scene = std::make_shared<Scene>();
    scene->id       = "sc-standalone";
    scene->version  = "1";
    scene->canvas_w = spec.canvas_w;
    scene->canvas_h = spec.canvas_h;

    // ---- 本机 screen（覆盖全画布）------------------------------------------
    // 渲染端绘制项解算 = clip 矩形 ∩ 本机 screen 矩形（composeFrame）；
    // 无 screen = items 恒 0 黑屏。输出光栅在 GL 后端下跟随窗口客户区
    // （显示分辨率模型 2026-11-10），screen 矩形只决定取景映射。
    Screen scr;
    scr.id        = "screen-1";
    scr.player_id = player_id;
    scr.x = 0.0;
    scr.y = 0.0;
    scr.w = spec.canvas_w;
    scr.h = spec.canvas_h;
    scene->screens.push_back(std::move(scr));

    // ---- 逐轨逐片段装配 -----------------------------------------------------
    std::int64_t timeline_end_us = 0;
    std::size_t  clip_total      = 0;
    int asset_seq = 0, clip_seq = 0;
    std::string skipped; // 跳过片段清单（非致命，作诊断说明回传）

    for (std::size_t ti = 0; ti < spec.tracks.size(); ++ti) {
        const TrackSpec& ts = spec.tracks[ti];
        Track tk;
        tk.id   = "t-" + std::to_string(ti + 1);
        tk.name = ts.name.empty() ? ("轨道 " + std::to_string(ti + 1))
                                  : ts.name;
        tk.kind = AssetKind::Unknown; // MVP 混合轨道（不做类型约束）

        for (const auto& cs : ts.clips) {
            std::string clip_err;

            // -- 1. 素材构造（Text 无文件；其余探测）→ 先拿全元数据再入表 --
            Asset a;
            if (!cs.file.empty()) {
                if (!probeAsset(cs.file, a, clip_err)) {
                    skipped += "  跳过片段[" + cs.file + "]: " + clip_err + "\n";
                    continue;
                }
            } else if (!cs.text.empty()) {
                a.kind         = AssetKind::Text;
                a.text_content = cs.text;
                // 宽高测量回填（与编辑器导入/源 prepare 三处同源）
                const TextLayout lay =
                    TextRasterizer::measure(TextRasterizer::styleFromAsset(a));
                a.width  = lay.width;
                a.height = lay.height;
            } else {
                skipped += "  跳过片段: file/text 均为空\n";
                continue;
            }
            a.id = "a-" + std::to_string(++asset_seq);

            // -- 2. 片段时长（0 = 自动：视频/音频用探测时长）-----------------
            std::int64_t dur = cs.duration_us;
            if (dur <= 0) dur = a.duration_us; // 图片恒 0 → 未显式即跳过
            if (dur <= 0) {
                skipped += "  跳过片段[" +
                           (cs.file.empty() ? cs.text : cs.file) +
                           "]: 时长未知（图片须显式 duration_us）\n";
                continue;
            }

            // -- 3. 片段构造（静态 transform）--------------------------------
            Clip c;
            c.id          = "c-" + std::to_string(++clip_seq);
            c.asset_id    = a.id;
            c.start_us    = cs.start_us;
            c.duration_us = dur;
            c.in_us       = cs.in_us;
            c.transform.x       = cs.x;
            c.transform.y       = cs.y;
            c.transform.w       = cs.w;
            c.transform.h       = cs.h;
            c.transform.opacity = cs.opacity;
            c.transform.rotation = cs.rotation; // 度，顺时针，绕锚点（引擎 v3 已消费）
            c.transform.crop[kCropL] = (std::max)(cs.crop_l, 0.0); // 裁剪透传
            c.transform.crop[kCropR] = (std::max)(cs.crop_r, 0.0); //（负值钳 0，
            c.transform.crop[kCropT] = (std::max)(cs.crop_t, 0.0); //  过裁由
            c.transform.crop[kCropB] = (std::max)(cs.crop_b, 0.0); //  visibleRect 收口）
            // 轨道次序 = 视觉层级（NLE 惯例，2026-11-11 定案）：
            // 面板上面的轨盖住下面的轨（上层=叠加层）。引擎按 z 升序绘制，
            // 故 ti 越小（越靠上）z 越大；×1000 留轨道内 cs.z 微调空间。
            // cs.z 钳 [0,999]：文档字段语义 = "轨内层级"，越界钳制防穿透
            // 相邻轨道的千位基数（跨轨层次由轨道次序/▲▼ 决定，面板提示同源）。
            c.transform.z       = static_cast<int>(spec.tracks.size() - 1 - ti)
                                      * 1000
                                  + std::clamp(cs.z, 0, 999);
            c.audio_gain        = 1.0;

            // -- 4. 淡入淡出（首/尾 Opacity+AudioGain 双通道同步打 key）------
            // 钳制：in/out 不重叠且不超片段时长（in + out ≤ dur）
            const double fade_in =
                std::clamp(cs.fade_in_us, 0.0, static_cast<double>(dur));
            const double fade_out =
                std::clamp(cs.fade_out_us, 0.0,
                           static_cast<double>(dur) - fade_in);

            if (fade_in > 0.0 || fade_out > 0.0) {
                const double head = cs.opacity; // 淡入目标 = 静态不透明度
                const auto   fi   = static_cast<std::int64_t>(fade_in);
                const auto   fo   = static_cast<std::int64_t>(fade_out);

                TweenChannel op;
                op.prop = TweenProp::Opacity;
                if (fade_in > 0.0) {
                    // ⚠️ 五字段聚合构造（工程铁律：缺 v2 = interp 隐转错位）
                    op.keys.push_back({0, 0.0, 0.0, 0.0, TweenInterp::Linear});
                    op.keys.push_back({fi, head, 0.0, 0.0,
                                       TweenInterp::Linear});
                }
                if (fade_out > 0.0) {
                    op.keys.push_back({dur - fo, head, 0.0, 0.0,
                                       TweenInterp::Linear});
                    op.keys.push_back({dur, 0.0, 0.0, 0.0,
                                       TweenInterp::Linear});
                }
                c.tweens.push_back(std::move(op));

                // 音频增益同形（1.0 = 不衰减；0 = 静音——音频视频同淡；
                // 图片无音频，gain 通道无害）
                TweenChannel ag;
                ag.prop = TweenProp::AudioGain;
                if (fade_in > 0.0) {
                    ag.keys.push_back({0, 0.0, 0.0, 0.0, TweenInterp::Linear});
                    ag.keys.push_back({fi, 1.0, 0.0, 0.0,
                                       TweenInterp::Linear});
                }
                if (fade_out > 0.0) {
                    ag.keys.push_back({dur - fo, 1.0, 0.0, 0.0,
                                       TweenInterp::Linear});
                    ag.keys.push_back({dur, 0.0, 0.0, 0.0,
                                       TweenInterp::Linear});
                }
                c.tweens.push_back(std::move(ag));
            }

            timeline_end_us = (std::max)(timeline_end_us,
                                         c.start_us + c.duration_us);
            tk.clips.push_back(std::move(c));
            scene->assets.push_back(std::move(a));
            ++clip_total;
        }
        if (!tk.clips.empty()) scene->timeline.tracks.push_back(std::move(tk));
    }

    scene->timeline.duration_us = timeline_end_us;

    if (scene->timeline.tracks.empty()) {
        err = "装配失败：无有效片段\n" + skipped;
        return nullptr;
    }
    if (!skipped.empty())
        err = "部分片段已跳过:\n" + skipped; // 非致命：err 作诊断说明

    std::printf("[standalone] 场景装配: 轨道=%zu 片段=%zu 素材=%zu "
                "时长=%.2fs\n",
                scene->timeline.tracks.size(), clip_total,
                scene->assets.size(),
                static_cast<double>(timeline_end_us) / 1e6);
    return scene;
}

// ---------------------------------------------------------------------------
// 工程场景归一化（.show 工程加载产物 → standalone 可渲染）
// ---------------------------------------------------------------------------
std::shared_ptr<const Scene> SceneAssembler::adoptForStandalone(
    const Scene& src, const std::string& player_id) {
    Scene s = src; // 值拷贝（不可变快照的改写前副本）
    Screen scr;
    scr.id        = "screen-standalone";
    scr.player_id = player_id;
    // 工程画布尺寸缺失（异常数据）兜底 1080p——SceneLoader 校验下不应
    // 发生，防御性收口与装配默认值同源
    scr.w = s.canvas_w > 0.0 ? s.canvas_w : 1920.0;
    scr.h = s.canvas_h > 0.0 ? s.canvas_h : 1080.0;
    s.canvas_w = scr.w;
    s.canvas_h = scr.h;
    s.screens.clear();
    s.screens.push_back(std::move(scr));
    return std::make_shared<const Scene>(std::move(s));
}

} // namespace ms::standalone
