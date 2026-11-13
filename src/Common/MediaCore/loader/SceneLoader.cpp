// ============================================================================
//  SceneLoader.cpp —— 场景序列化/反序列化与校验实现
//
//  校验规则（fromProto / validateScene——一切场景数据入口统一校验，
//  全部为加载期 fail-fast 拦截）：
//    R1 clip.asset_id 必须存在于 Scene.assets（防"引用悬空"）
//    R2 clip 素材的 AssetKind 必须等于所在轨道的 kind（§4.2 轨道类型约束）；
//      Unknown（未知 proto 枚举值）显式拦截——不得进入内存模型
//    R3 clip.start_us >= 0、duration_us > 0、in_us >= 0（时间合法性）；
//      transform 各 double 字段必须有限（proto3 可携带 NaN/Inf，进入
//      渲染管线的 scissor/clamp 是未定义行为源）
//    R4 screen 矩形 w/h > 0（画布内映射需要正矩形）
//    R5 canvas_w/h > 0（虚拟画布必须有效）
//    R6 Timeline.duration_us == 0 时按片段自动推算并回填内存模型；
//      负值非法（拒绝）
//    R7 素材/屏幕/轨道/片段 id 非空，且各自集合内唯一（防补丁按 id
//      定位时命中歧义条目）
//    R8 tween 通道合法性（2026-11-05；2026-11-06 放宽上限）：属性非
//      Unknown / 非空通道 / 关键帧时间 ≥ 0（超出 duration_us 的键合法
//      ——剪裁遮蔽语义，求值/绘制端不消费）/ 数值有限 / 无重复时间点
// ============================================================================

#include "loader/SceneLoader.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <sstream>
#include <unordered_set>

#include "engine/TimelineEngine.hpp" // R6 总时长推算用 computeDurationUs
#include "engine/TweenEngine.hpp"    // v2→v3 迁移采样（evalChannel）

namespace ms {

// ---------------------------------------------------------------------------
// 内部工具：AssetKind（proto 枚举）↔ AssetKind（内存模型枚举）
// 未知 proto 枚举值（新版本协议向下兼容场景）统一落回 Unknown，
// 由 R2 校验自然拒绝其进入轨道（Unknown 轨道不接受 clip）。
// ---------------------------------------------------------------------------
static AssetKind FromProtoKind(ms_proto::AssetKind k) noexcept {
    switch (k) {
        case ms_proto::ASSET_VIDEO: return AssetKind::Video;
        case ms_proto::ASSET_IMAGE: return AssetKind::Image;
        case ms_proto::ASSET_TEXT:  return AssetKind::Text;
        case ms_proto::ASSET_AUDIO: return AssetKind::Audio;
        case ms_proto::ASSET_LIVE:  return AssetKind::Live;
        case ms_proto::ASSET_CONTROL: return AssetKind::Control;
        default:                    return AssetKind::Unknown;
    }
}

static ms_proto::AssetKind ToProtoKind(AssetKind k) noexcept {
    switch (k) {
        case AssetKind::Video: return ms_proto::ASSET_VIDEO;
        case AssetKind::Image: return ms_proto::ASSET_IMAGE;
        case AssetKind::Text:  return ms_proto::ASSET_TEXT;
        case AssetKind::Audio: return ms_proto::ASSET_AUDIO;
        case AssetKind::Live:  return ms_proto::ASSET_LIVE;
        case AssetKind::Control: return ms_proto::ASSET_CONTROL;
        default:               return ms_proto::ASSET_UNKNOWN;
    }
}

// ---------------------------------------------------------------------------
// 内部工具：TweenProp / TweenInterp（proto 枚举 ↔ 内存模型枚举）
// 未知插值方式（新版本协议向下兼容）宽容映射 Linear（线性行为最接近直觉）；
// 未知属性落回 Unknown，由 R8 校验拒绝进入内存模型（与 R2 同纪律）。
// v2 打包属性（2026-11-09）：旧 v1 标量通道（X/Y/W/H/Corner*_X/_Y）不再
// 映射——落到 Unknown，加载端配合 tween_ver<2 整段清空（用户定案"直接
// 重置"，不做逐通道迁移）。
// ---------------------------------------------------------------------------
/// Tween 打包属性格式版本（v4，2026-11-09）：Clip.tween_ver ≥ 4 才按
/// v4 语义读 tweens（Rotation 三维 v0/v1/v2 = 俯仰绕X/偏航绕Y/滚转绕Z）；
/// == 3 = v3 打包（读入后姿态三通道迁移合并进 Rotation 三分量）；
/// == 2 = v2 打包（读入后 Z 通道迁移进 Position 第三分量 + 姿态迁移）；
/// < 2 = 旧 v1 标量通道格式（ver 缺席=0 或 1）整段清空——变换回落静态
/// 基准值
constexpr std::uint32_t kTweenVersion = 4;

static TweenProp FromProtoTweenProp(ms_proto::TweenProp p) noexcept {
    switch (p) {
        case ms_proto::TWEEN_PROP_POSITION:   return TweenProp::Position;
        case ms_proto::TWEEN_PROP_SIZE:       return TweenProp::Size;
        case ms_proto::TWEEN_PROP_ROTATION:   return TweenProp::Rotation;
        case ms_proto::TWEEN_PROP_OPACITY:    return TweenProp::Opacity;
        case ms_proto::TWEEN_PROP_Z:          return TweenProp::Z;
        case ms_proto::TWEEN_PROP_CORNER_LT:  return TweenProp::CornerLT;
        case ms_proto::TWEEN_PROP_CORNER_RT:  return TweenProp::CornerRT;
        case ms_proto::TWEEN_PROP_CORNER_RB:  return TweenProp::CornerRB;
        case ms_proto::TWEEN_PROP_CORNER_LB:  return TweenProp::CornerLB;
        case ms_proto::TWEEN_PROP_ROTATION_X: return TweenProp::RotationX; // v4 退役：仅迁移期读入（migrateV3PoseChannel 合并后删除）
        case ms_proto::TWEEN_PROP_ROTATION_Y: return TweenProp::RotationY; // v4 退役：仅迁移期读入
        case ms_proto::TWEEN_PROP_CROP_LEFT:  return TweenProp::CropL;
        case ms_proto::TWEEN_PROP_CROP_RIGHT: return TweenProp::CropR;
        case ms_proto::TWEEN_PROP_CROP_TOP:   return TweenProp::CropT;
        case ms_proto::TWEEN_PROP_CROP_BOTTOM: return TweenProp::CropB;
        case ms_proto::TWEEN_PROP_AUDIO_GAIN: return TweenProp::AudioGain;
        default:                              return TweenProp::Unknown;
    }
}

static ms_proto::TweenProp ToProtoTweenProp(TweenProp p) noexcept {
    switch (p) {
        case TweenProp::Position:  return ms_proto::TWEEN_PROP_POSITION;
        case TweenProp::Size:      return ms_proto::TWEEN_PROP_SIZE;
        case TweenProp::Rotation:  return ms_proto::TWEEN_PROP_ROTATION;
        case TweenProp::Opacity:   return ms_proto::TWEEN_PROP_OPACITY;
        case TweenProp::Z:         return ms_proto::TWEEN_PROP_Z;
        case TweenProp::CornerLT:  return ms_proto::TWEEN_PROP_CORNER_LT;
        case TweenProp::CornerRT:  return ms_proto::TWEEN_PROP_CORNER_RT;
        case TweenProp::CornerRB:  return ms_proto::TWEEN_PROP_CORNER_RB;
        case TweenProp::CornerLB:  return ms_proto::TWEEN_PROP_CORNER_LB;
        case TweenProp::RotationX: return ms_proto::TWEEN_PROP_ROTATION_X;
        case TweenProp::RotationY: return ms_proto::TWEEN_PROP_ROTATION_Y;
        case TweenProp::CropL:     return ms_proto::TWEEN_PROP_CROP_LEFT;
        case TweenProp::CropR:     return ms_proto::TWEEN_PROP_CROP_RIGHT;
        case TweenProp::CropT:     return ms_proto::TWEEN_PROP_CROP_TOP;
        case TweenProp::CropB:     return ms_proto::TWEEN_PROP_CROP_BOTTOM;
        case TweenProp::AudioGain: return ms_proto::TWEEN_PROP_AUDIO_GAIN;
        default:                   return ms_proto::TWEEN_PROP_UNKNOWN;
    }
}

static TweenInterp FromProtoTweenInterp(ms_proto::TweenInterp i) noexcept {
    switch (i) {
        case ms_proto::TWEEN_INTERP_HOLD:   return TweenInterp::Hold;
        case ms_proto::TWEEN_INTERP_SMOOTH: return TweenInterp::Smooth;
        case ms_proto::TWEEN_INTERP_LINEAR:
        default:                            return TweenInterp::Linear;
    }
}

static ms_proto::TweenInterp ToProtoTweenInterp(TweenInterp i) noexcept {
    switch (i) {
        case TweenInterp::Hold:   return ms_proto::TWEEN_INTERP_HOLD;
        case TweenInterp::Smooth: return ms_proto::TWEEN_INTERP_SMOOTH;
        case TweenInterp::Linear:
        default:                  return ms_proto::TWEEN_INTERP_LINEAR;
    }
}

// ---------------------------------------------------------------------------
// v2→v3 迁移（2026-11-09 定案）：独立 Z 通道（TweenProp::Z=5，v3 起退役）
// 合并进 Position 通道第三分量。语义：
//   * 合并时刻集 = Position 键时刻 ∪ Z 键时刻（升序去重）；
//   * 各时刻：v0/v1 = Position 通道插值（无通道 = 静态基准 transform.x/y，
//     与 v2 "通道整条接管"语义逐位一致）；v2 = Z 通道插值（端点保持）；
//   * 每键 interp：该时刻 Position 键的 interp 优先（x/y 动画主导），其次
//     Z 键的 interp，均无 = Linear；
//   * 迁移后删除 Z 通道；采样不做出点遮蔽（INT64_MAX——出点外键数据
//     保留语义在合并后经键值自身延续）。
// ---------------------------------------------------------------------------
static void migrateV2ZChannel(Clip& c) {
    const TweenChannel* zch   = nullptr;
    const TweenChannel* posch = nullptr;
    for (const auto& tw : c.tweens) {
        if (tw.prop == TweenProp::Z)          zch   = &tw;
        else if (tw.prop == TweenProp::Position) posch = &tw;
    }
    if (!zch || zch->keys.empty()) return; // 无 Z 通道：无需迁移

    // 合并时刻集（去重升序）
    std::vector<std::int64_t> times;
    times.reserve((posch ? posch->keys.size() : 0) + zch->keys.size());
    if (posch)
        for (const auto& k : posch->keys) times.push_back(k.t_us);
    for (const auto& k : zch->keys) times.push_back(k.t_us);
    std::sort(times.begin(), times.end());
    times.erase(std::unique(times.begin(), times.end()), times.end());

    constexpr std::int64_t kNoTrim = std::numeric_limits<std::int64_t>::max();
    std::vector<Keyframe> merged;
    merged.reserve(times.size());
    for (const std::int64_t t : times) {
        Keyframe k;
        k.t_us = t;
        if (posch) {
            const auto v = TweenEngine::evalChannel(*posch, t, kNoTrim);
            k.v0 = v.v0;
            k.v1 = v.v1;
        } else {
            k.v0 = c.transform.x; // v2 无 Position 通道：x/y 恒静态
            k.v1 = c.transform.y;
        }
        k.v2 = TweenEngine::evalChannel(*zch, t, kNoTrim).v0;
        // interp：Position 键优先 → Z 键 → Linear
        const Keyframe* pk = nullptr;
        const Keyframe* zk = nullptr;
        if (posch)
            for (const auto& q : posch->keys)
                if (q.t_us == t) { pk = &q; break; }
        for (const auto& q : zch->keys)
            if (q.t_us == t) { zk = &q; break; }
        k.interp = pk   ? pk->interp
                   : zk ? zk->interp
                        : TweenInterp::Linear;
        merged.push_back(k);
    }

    // 落位：删除 Z 通道；Position 通道存在则整体替换键集，否则新建
    TweenChannel* dst = nullptr;
    for (auto& tw : c.tweens)
        if (tw.prop == TweenProp::Position) { dst = &tw; break; }
    if (dst) {
        dst->keys = std::move(merged);
    } else {
        TweenChannel nc;
        nc.prop = TweenProp::Position;
        nc.keys = std::move(merged);
        c.tweens.push_back(std::move(nc));
    }
    c.tweens.erase(std::remove_if(c.tweens.begin(), c.tweens.end(),
                                  [](const TweenChannel& tw) {
                                      return tw.prop == TweenProp::Z;
                                  }),
                   c.tweens.end());
}

// ---------------------------------------------------------------------------
// v3→v4 姿态迁移（2026-11-09 用户定案"姿态合并一行按轴命名"）：独立俯仰
// 通道（RotationX=10）/偏航通道（RotationY=11）与旧标量滚转通道
//（Rotation=3，v3 单分量 v0=滚转）合并为 Rotation 三维打包通道（v0=俯仰
// rot_x / v1=偏航 rot_y / v2=滚转 rotation——UI 分量名 X/Y/Z）。语义与
// migrateV2ZChannel 同构：
//   * 合并时刻集 = 三条通道键时刻并集（升序去重）；
//   * 各时刻：v0 = 俯仰通道插值（无通道 = 静态基准 transform.rot_x）、
//     v1 = 偏航通道插值（无 = 静态 rot_y）、v2 = 滚转通道插值（无 = 静态
//     rotation）——与"通道整条接管"语义逐位一致；
//   * 每键 interp：该时刻滚转键的 interp 优先（历史主属性），其次俯仰、
//     偏航，均无 = Linear；
//   * 迁移后删除俯仰/偏航通道；滚转通道存在则整体替换键集，否则新建。
// ---------------------------------------------------------------------------
static void migrateV3PoseChannel(Clip& c) {
    const TweenChannel* roll_ch = nullptr; // Rotation=3（v3 单分量滚转）
    const TweenChannel* pitch_ch = nullptr; // RotationX=10
    const TweenChannel* yaw_ch   = nullptr; // RotationY=11
    for (const auto& tw : c.tweens) {
        if (tw.prop == TweenProp::Rotation)       roll_ch  = &tw;
        else if (tw.prop == TweenProp::RotationX) pitch_ch = &tw;
        else if (tw.prop == TweenProp::RotationY) yaw_ch   = &tw;
    }
    // 三条通道全无（或全空键集）：无需迁移
    const bool any_keys = (roll_ch && !roll_ch->keys.empty()) ||
                          (pitch_ch && !pitch_ch->keys.empty()) ||
                          (yaw_ch && !yaw_ch->keys.empty());
    if (!any_keys) return;

    // 合并时刻集（去重升序）
    std::vector<std::int64_t> times;
    times.reserve((roll_ch ? roll_ch->keys.size() : 0) +
                  (pitch_ch ? pitch_ch->keys.size() : 0) +
                  (yaw_ch ? yaw_ch->keys.size() : 0));
    if (roll_ch)
        for (const auto& k : roll_ch->keys) times.push_back(k.t_us);
    if (pitch_ch)
        for (const auto& k : pitch_ch->keys) times.push_back(k.t_us);
    if (yaw_ch)
        for (const auto& k : yaw_ch->keys) times.push_back(k.t_us);
    std::sort(times.begin(), times.end());
    times.erase(std::unique(times.begin(), times.end()), times.end());

    constexpr std::int64_t kNoTrim = std::numeric_limits<std::int64_t>::max();
    std::vector<Keyframe> merged;
    merged.reserve(times.size());
    for (const std::int64_t t : times) {
        Keyframe k;
        k.t_us = t;
        // 各分量：通道插值优先，无通道 = 静态基准（v3"通道整条接管"语义）
        k.v0 = pitch_ch ? TweenEngine::evalChannel(*pitch_ch, t, kNoTrim).v0
                        : c.transform.rot_x;
        k.v1 = yaw_ch ? TweenEngine::evalChannel(*yaw_ch, t, kNoTrim).v0
                      : c.transform.rot_y;
        k.v2 = roll_ch ? TweenEngine::evalChannel(*roll_ch, t, kNoTrim).v0
                       : c.transform.rotation;
        // interp：滚转键优先（历史主属性）→ 俯仰 → 偏航 → Linear
        const Keyframe* rk = nullptr;
        const Keyframe* pk = nullptr;
        const Keyframe* yk = nullptr;
        if (roll_ch)
            for (const auto& q : roll_ch->keys)
                if (q.t_us == t) { rk = &q; break; }
        if (pitch_ch)
            for (const auto& q : pitch_ch->keys)
                if (q.t_us == t) { pk = &q; break; }
        if (yaw_ch)
            for (const auto& q : yaw_ch->keys)
                if (q.t_us == t) { yk = &q; break; }
        k.interp = rk   ? rk->interp
                   : pk ? pk->interp
                   : yk ? yk->interp
                        : TweenInterp::Linear;
        merged.push_back(k);
    }

    // 落位：删俯仰/偏航通道；滚转通道存在则整体替换键集，否则新建
    TweenChannel* dst = nullptr;
    for (auto& tw : c.tweens)
        if (tw.prop == TweenProp::Rotation) { dst = &tw; break; }
    if (dst) {
        dst->keys = std::move(merged);
    } else {
        TweenChannel nc;
        nc.prop = TweenProp::Rotation;
        nc.keys = std::move(merged);
        c.tweens.push_back(std::move(nc));
    }
    c.tweens.erase(std::remove_if(c.tweens.begin(), c.tweens.end(),
                                  [](const TweenChannel& tw) {
                                      return tw.prop == TweenProp::RotationX ||
                                             tw.prop == TweenProp::RotationY;
                                  }),
                   c.tweens.end());
}

// ---------------------------------------------------------------------------
// 子结构转换（公开静态；toProto/fromProto 委托同款映射）
// ---------------------------------------------------------------------------
ms_proto::Asset SceneLoader::toProtoAsset(const Asset& a) {
    ms_proto::Asset pa;
    pa.set_id(a.id);
    pa.set_kind(ToProtoKind(a.kind));
    pa.set_path(a.path);
    pa.set_size_bytes(a.size_bytes);
    pa.set_checksum(a.checksum);
    pa.set_duration_us(a.duration_us);
    pa.set_width(a.width);
    pa.set_height(a.height);
    pa.set_fps(a.fps);
    if (a.kind == AssetKind::Text) {
        auto* tx = pa.mutable_text();
        tx->set_content(a.text_content);
        tx->set_font(a.text_font);
        tx->set_font_size(a.text_font_size);
        tx->set_color(a.text_color);
        tx->set_bold(a.text_bold);
        tx->set_italic(a.text_italic);
        tx->set_h_align(a.text_h_align);
        tx->set_bg_color(a.text_bg_color);
    }
    if (a.kind == AssetKind::Live) {
        // 实时载荷（2026-11-08 Live Hub）：无文件随 pb 分发，同文本模式。
        // ingest/latency 为普通标量，缺席 = 0 = 业务默认（ndi/最低延迟）
        auto* lv = pa.mutable_live();
        lv->set_source_name(a.live_source_name);
        lv->set_url(a.live_url);
        lv->set_ingest(a.live_ingest);
        lv->set_latency(a.live_latency);
    }
    return pa;
}

Asset SceneLoader::fromProtoAsset(const ms_proto::Asset& pa) {
    Asset a;
    a.id          = pa.id();
    a.kind        = FromProtoKind(pa.kind());
    a.path        = pa.path();
    a.size_bytes  = pa.size_bytes();
    a.checksum    = pa.checksum();
    a.duration_us = pa.duration_us();
    a.width       = pa.width();
    a.height      = pa.height();
    a.fps         = pa.fps();
    if (pa.has_text()) {
        a.text_content   = pa.text().content();
        a.text_font      = pa.text().font();
        a.text_font_size = pa.text().font_size();
        a.text_color     = pa.text().color();
        // 新增样式字段（2026-11-05）：proto3 标量缺席 = 0 = 业务默认
        // （非粗斜/左对齐/无背景），无需 optional has_ 判定
        a.text_bold     = pa.text().bold();
        a.text_italic   = pa.text().italic();
        a.text_h_align  = std::clamp(pa.text().h_align(), 0, 2);
        a.text_bg_color = pa.text().bg_color();
    }
    if (pa.has_live()) {
        // 实时载荷（2026-11-08）：ingest 越界值钳回 ndi（0）——宽容解析
        // 与 h_align 同纪律；latency 越界钳回最低延迟（0）
        // 【2026-11-10 钳制上界修正】上界必须对齐 SceneModel::LiveIngest
        // 全枚举（0=ndi 1=window 2=spout 3=network 4=monitor 5=camera）——
        // 旧版 <=3 漏 Monitor/Camera，pb 往返（.show 保存重开 + MsgScene
        // 下发）把摄像头素材钳回 NDI：重开即"丢"配置、display 按 NDI 连
        // 摄像头设备名恒无显示。
        a.live_source_name = pa.live().source_name();
        a.live_url         = pa.live().url();
        a.live_ingest  = (pa.live().ingest() >= 0 &&
                          pa.live().ingest() <= static_cast<int>(LiveIngest::Camera))
                             ? pa.live().ingest() : 0;
        a.live_latency = (pa.live().latency() == 0 || pa.live().latency() == 1)
                             ? pa.live().latency() : 0;
    }
    return a;
}

ms_proto::Screen SceneLoader::toProtoScreen(const Screen& s) {
    ms_proto::Screen ps;
    ps.set_id(s.id);
    ps.set_player_id(s.player_id);
    ps.set_x(s.x);
    ps.set_y(s.y);
    ps.set_w(s.w);
    ps.set_h(s.h);
    ps.set_rotation(s.rotation); // 屏旋转（取景窗语义）与叠放层级随场景分发
    ps.set_z(s.z);
    // 输出变形（2026-11-09）：仅 mode 非 0 时写入——保持"无变形 screen"
    // 的 wire 格式与旧版本完全一致（message 缺席 = has_warp()==false）
    if (s.warp.active()) {
        auto* pw = ps.mutable_warp();
        pw->set_mode(s.warp.mode);
        pw->set_cols(s.warp.cols);
        pw->set_rows(s.warp.rows);
        for (double v : s.warp.pts) pw->add_pts(v);
    }
    return ps;
}

Screen SceneLoader::fromProtoScreen(const ms_proto::Screen& ps) {
    Screen s;
    s.id        = ps.id();
    s.player_id = ps.player_id();
    s.x = ps.x();
    s.y = ps.y();
    s.w = ps.w();
    s.h = ps.h();
    s.rotation = ps.rotation();
    s.z        = ps.z();
    // 输出变形（2026-11-09）：缺席 = 无变形（默认构造 mode=0）；存在则
    // 拷贝后经 normalize() 收口信任边界（尺寸/坐标钳制）——长度根本
    // 不匹配（节点数对不上 cols*rows）时整份丢弃，回退无变形。
    if (ps.has_warp()) {
        ScreenWarp w;
        w.mode = ps.warp().mode();
        w.cols = ps.warp().cols();
        w.rows = ps.warp().rows();
        w.pts.assign(ps.warp().pts().begin(), ps.warp().pts().end());
        w.normalize();
        const std::size_t need =
            static_cast<std::size_t>(w.cols) * w.rows * 2;
        if (w.mode >= 0 && w.mode <= 2 && w.pts.size() == need &&
            static_cast<std::size_t>(ps.warp().pts().size()) ==
                static_cast<std::size_t>(need)) {
            s.warp = w; // mode 越界/长度不符 → 丢弃（保持默认无变形）
        }
    }
    return s;
}

ms_proto::Track SceneLoader::toProtoTrack(const Track& t) {
    ms_proto::Track pt;
    pt.set_id(t.id);
    pt.set_name(t.name);
    pt.set_kind(ToProtoKind(t.kind));
    for (const auto& c : t.clips) {
        ms_proto::Clip* pc = pt.add_clips();
        pc->set_id(c.id);
        pc->set_asset_id(c.asset_id);
        pc->set_start_us(c.start_us);
        pc->set_duration_us(c.duration_us);
        pc->set_in_us(c.in_us);
        // 音频增益静态基准（2026-11-10）：默认 1.0 不写（wire 兼容——缺席
        // = has_audio_gain()==false = 读端回填 1.0；显式静音 0 必须写出，
        // 否则会被缺席回填静默改回 1.0）
        if (c.audio_gain != 1.0)
            pc->set_audio_gain(c.audio_gain);
        // Control 片段触发指令表（2026-11-13）：空串不写（wire 兼容——
        // 缺席 = 读端空串；非 Control 类型恒空）
        if (!c.control_messages.empty())
            pc->set_control_messages(c.control_messages);
        auto* tr = pc->mutable_transform();
        tr->set_x(c.transform.x);
        tr->set_y(c.transform.y);
        tr->set_w(c.transform.w);
        tr->set_h(c.transform.h);
        tr->set_rotation(c.transform.rotation);
        tr->set_opacity(c.transform.opacity);
        tr->set_z(c.transform.z);
        // 锚点（2026-11-09）：0=中心为业务默认，非 0 才写（wire 兼容——
        // 旧读端遇未知字段自动忽略，新读端缺席 = 0 = 中心）
        if (c.transform.anchor != 0)
            tr->set_anchor(c.transform.anchor);
        // 四角点位移（2026-11-09 corner pin）：全 0 = 无变形省略不写
        if (c.transform.cornerActive())
            for (int i = 0; i < 8; ++i)
                tr->add_corner(c.transform.corner[i]);
        // 姿态/裁切扩展（2026-11-09 v3）：非 0 才写（wire 兼容——缺席 =
        // 0 = 无俯仰偏航/无裁切，业务默认与旧读端语义一致）
        if (c.transform.rot_x != 0.0) tr->set_rot_x(c.transform.rot_x);
        if (c.transform.rot_y != 0.0) tr->set_rot_y(c.transform.rot_y);
        if (c.transform.cropActive())
            for (int i = 0; i < 4; ++i)
                if (c.transform.crop[i] != 0.0) {
                    switch (i) {
                        case kCropL: tr->set_crop_l(c.transform.crop[i]); break;
                        case kCropR: tr->set_crop_r(c.transform.crop[i]); break;
                        case kCropT: tr->set_crop_t(c.transform.crop[i]); break;
                        case kCropB: tr->set_crop_b(c.transform.crop[i]); break;
                    }
                }
        // ---- Tween 通道（2026-11-05 建立；2026-11-09 v4 打包属性）----
        // v4 格式版本号恒写 4（读端 <2 清空；==2 迁移 Z；≤3 迁移姿态三通道）
        pc->set_tween_ver(kTweenVersion);
        for (const auto& tw : c.tweens) {
            auto* ptw = pc->add_tweens();
            ptw->set_prop(ToProtoTweenProp(tw.prop));
            for (const auto& k : tw.keys) {
                auto* pk = ptw->add_keys();
                pk->set_t_us(k.t_us);
                pk->set_v0(k.v0); // 标量值 / 向量第一分量
                pk->set_v1(k.v1); // 向量第二分量（标量属性为 0，无害）
                pk->set_v2(k.v2); // 向量第三分量（Position 的 z / Rotation 的滚转）
                pk->set_interp(ToProtoTweenInterp(k.interp));
            }
        }
    }
    return pt;
}

Track SceneLoader::fromProtoTrack(const ms_proto::Track& pt) {
    Track t;
    t.id   = pt.id();
    t.name = pt.name();
    t.kind = FromProtoKind(pt.kind());
    for (const auto& pc : pt.clips()) {
        Clip c;
        c.id          = pc.id();
        c.asset_id    = pc.asset_id();
        c.start_us    = pc.start_us();
        c.duration_us = pc.duration_us();
        c.in_us       = pc.in_us();
        c.transform.x        = pc.transform().x();
        c.transform.y        = pc.transform().y();
        c.transform.w        = pc.transform().w();
        c.transform.h        = pc.transform().h();
        c.transform.rotation = pc.transform().rotation();
        // 【opacity 缺席回填（2026-11-05）】proto3 optional：字段缺席 =
        // 旧场景/手工场景未写透明度 → 回填业务默认 1.0（显式 0 全透明照常
        // 保留）。此前普通标量缺席反序列化 0.0 直接覆盖默认值，clip 隐身。
        c.transform.opacity  = pc.transform().has_opacity()
                                   ? static_cast<double>(pc.transform().opacity())
                                   : 1.0;
        // 音频增益静态基准（2026-11-10）：optional 缺席回填 1.0（opacity
        // 同款缺席陷阱——普通标量缺席=0=静音会打哑旧工程）；显式 0（静音）
        // 照常保留；NaN/Inf 钳回默认（R3 同口径宁缺勿错）
        c.audio_gain = pc.has_audio_gain() && std::isfinite(pc.audio_gain())
                           ? pc.audio_gain() : 1.0;
        // Control 片段触发指令表（2026-11-13）：JSON 原文直读（空串 =
        // 无指令；解析校验在消费端——编辑器触发引擎/编辑区控件容错）
        c.control_messages = pc.control_messages();
        c.transform.z        = pc.transform().z();
        // 锚点（2026-11-09）：缺席 = 0 = 中心（业务默认）；越界值钳到
        // 中心（渲染端 anchorUV 越界兜底同口径，数据损坏安全降级）
        c.transform.anchor   = (pc.transform().anchor() >= 0 &&
                                pc.transform().anchor() <= 4)
                                   ? pc.transform().anchor() : 0;
        // 四角点位移（2026-11-09 corner pin）：定长 8 校验——长度非 0 非 8
        // 视作数据损坏，整体丢弃置 0（宁缺勿错，不 fail 场景——同 ScreenWarp
        // "非法丢弃"哲学，变形绝不黑屏）；NaN/Inf 钳 0（R3 同口径）
        if (const int n = pc.transform().corner_size(); n > 0) {
            if (n == 8) {
                for (int i = 0; i < 8; ++i) {
                    const double v = pc.transform().corner(i);
                    c.transform.corner[i] =
                        std::isfinite(v) ? v : 0.0;
                }
            }
        }
        // 姿态/裁切扩展（2026-11-09 v3）：普通标量缺席 = 0 = 业务默认；
        // NaN/Inf 钳 0（同 corner 纪律——数据损坏宁缺勿错）
        c.transform.rot_x = std::isfinite(pc.transform().rot_x())
                                ? pc.transform().rot_x() : 0.0;
        c.transform.rot_y = std::isfinite(pc.transform().rot_y())
                                ? pc.transform().rot_y() : 0.0;
        c.transform.crop[kCropL] = std::isfinite(pc.transform().crop_l())
                                       ? pc.transform().crop_l() : 0.0;
        c.transform.crop[kCropR] = std::isfinite(pc.transform().crop_r())
                                       ? pc.transform().crop_r() : 0.0;
        c.transform.crop[kCropT] = std::isfinite(pc.transform().crop_t())
                                       ? pc.transform().crop_t() : 0.0;
        c.transform.crop[kCropB] = std::isfinite(pc.transform().crop_b())
                                       ? pc.transform().crop_b() : 0.0;
        // ---- Tween 通道（2026-11-05 建立；2026-11-09 v3 打包属性重构）----
        // 升序不变量（TweenEngine 二分定位的前置条件）在加载期统一建立：
        // 稳定排序宽容手工场景/旧数据的乱序关键帧；排序后 validateScene
        // 的 R8 拒绝重复时间点（同刻双帧语义二义）。
        // 【版本判据】< 2 = 旧 v1 标量通道格式——不兼容整段清空；== 2 =
        // v2 打包（Position 二维）——读入后执行 Z→Position.v2 迁移；
        // ≥ 4 = v4 原生。
        if (pc.tween_ver() >= 2) {
            for (const auto& ptw : pc.tweens()) {
                TweenChannel tw;
                tw.prop = FromProtoTweenProp(ptw.prop());
                tw.keys.reserve(ptw.keys_size());
                for (const auto& pk : ptw.keys()) {
                    Keyframe k;
                    k.t_us   = pk.t_us();
                    k.v0     = pk.v0(); // 标量值 / 向量第一分量
                    k.v1     = pk.v1(); // 向量第二分量
                    k.v2     = pk.v2(); // 向量第三分量（v3 Position 的 z / v4 Rotation 的滚转）
                    k.interp = FromProtoTweenInterp(pk.interp());
                    tw.keys.push_back(k);
                }
                std::stable_sort(tw.keys.begin(), tw.keys.end(),
                                 [](const Keyframe& a, const Keyframe& b) {
                                     return a.t_us < b.t_us;
                                 });
                c.tweens.push_back(std::move(tw));
            }
            // 【v2→v3 迁移（2026-11-09 定案）】ver==2 的独立 Z 通道合并进
            // Position 第三分量：
            //   * Position 键各点采样 Z 通道（端点保持）写 v2；
            //   * Z 键独有时刻补 Position 键（v0/v1 取静态基准 transform）；
            //   * 无 Position 通道但 Z 有键 → 以静态 x/y 建三维 Position 通道；
            //   * 迁移后 Z 通道删除（v3 不再产独立 Z）。
            // 【v3→v4 姿态迁移（2026-11-09 用户定案）】ver≤3 的独立俯仰/
            // 偏航通道（RotationX/Y）与 v3 单分量滚转通道（Rotation）合并
            // 为 Rotation 三维打包通道（v0=俯仰/v1=偏航/v2=滚转），迁移后
            // RotationX/Y 通道删除（v4 不再产独立姿态通道）。
            // ver ≥ 4 原生无独立 Z / 姿态通道（写端不再产出），跳过。
            if (pc.tween_ver() == 2) migrateV2ZChannel(c);
            if (pc.tween_ver() <= 3) migrateV3PoseChannel(c);
        } // tween_ver < 2：旧格式 tweens 整段丢弃（变换回落静态基准值）
        t.clips.push_back(std::move(c));
    }
    return t;
}

// ---------------------------------------------------------------------------
// 内存模型 → Protobuf
// 直映转换：字段一一对应；文本载荷仅在 kind==Text 时填充。
// ---------------------------------------------------------------------------
ms_proto::Scene SceneLoader::toProto(const Scene& scene) {
    ms_proto::Scene pb;
    pb.set_id(scene.id);
    pb.set_version(scene.version);
    pb.set_canvas_w(scene.canvas_w);
    pb.set_canvas_h(scene.canvas_h);

    // ---- 素材表 ----
    for (const auto& a : scene.assets) *pb.add_assets() = toProtoAsset(a);

    // ---- 屏幕布局 ----
    for (const auto& s : scene.screens) *pb.add_screens() = toProtoScreen(s);

    // ---- 时间轴（轨道 → 片段 → 变换）----
    *pb.mutable_timeline() = toProtoTimeline(scene.timeline);
    return pb;
}

// ---------------------------------------------------------------------------
// 子结构转换（公开静态；与 Scene 级映射一致）
// ---------------------------------------------------------------------------
ms_proto::Timeline SceneLoader::toProtoTimeline(const Timeline& tl) {
    ms_proto::Timeline ptl;
    ptl.set_duration_us(tl.duration_us);
    for (const auto& t : tl.tracks) *ptl.add_tracks() = toProtoTrack(t);
    return ptl;
}

// ---------------------------------------------------------------------------
// 场景完整性校验（R1/R2/R3/R7；一切场景数据入口统一走此校验）
// ---------------------------------------------------------------------------
bool SceneLoader::validateScene(const Scene& scene, std::string& err) {
    // R7：素材 id 非空 + 唯一
    {
        std::unordered_set<std::string> seen;
        seen.reserve(scene.assets.size() * 2 + 1);
        for (const auto& a : scene.assets) {
            if (a.id.empty()) {
                err = "R7: 素材 id 为空 (path='" + a.path + "')";
                return false;
            }
            if (!seen.insert(a.id).second) {
                err = "R7: 素材 id 重复 '" + a.id + "'";
                return false;
            }
        }
    }
    // R7：屏幕 id 非空 + 唯一；R4：正矩形 + 旋转数值合法
    {
        std::unordered_set<std::string> seen;
        for (const auto& s : scene.screens) {
            if (s.id.empty()) {
                err = "R7: screen id 为空";
                return false;
            }
            if (!seen.insert(s.id).second) {
                err = "R7: screen id 重复 '" + s.id + "'";
                return false;
            }
            if (s.w <= 0 || s.h <= 0) {
                err = "R4: screen '" + s.id + "' 矩形非法 (w=" +
                      std::to_string(s.w) + ", h=" + std::to_string(s.h) + ")";
                return false;
            }
            // 屏旋转与 clip transform 同为 double 载荷——NaN/Inf 进入
            // ScreenMapper 的三角函数/仿射是 UB 源，入口统一拦截
            if (!std::isfinite(s.rotation)) {
                err = "R4: screen '" + s.id + "' rotation 含 NaN/Inf";
                return false;
            }
        }
    }
    // 轨道/片段逐条校验（R7 id + R1/R2/R3）
    std::unordered_set<std::string> track_ids;
    // 片段 id 全场景唯一（而非仅轨道内）：Editor 适配层按 id 反查片段
    // （noteName 解析），id 语义是全局的；补丁按 id 索引也不容许二义
    std::unordered_set<std::string> clip_ids;
    clip_ids.reserve(64);
    for (const auto& t : scene.timeline.tracks) {
        if (t.id.empty()) {
            err = "R7: 轨道 id 为空";
            return false;
        }
        if (!track_ids.insert(t.id).second) {
            err = "R7: 轨道 id 重复 '" + t.id + "'";
            return false;
        }
        for (const auto& c : t.clips) {
            if (c.id.empty()) {
                err = "R7: 轨道 '" + t.id + "' 内片段 id 为空";
                return false;
            }
            if (!clip_ids.insert(c.id).second) {
                err = "R7: 片段 id 重复 '" + c.id + "' (track '" + t.id + "')";
                return false;
            }

            // R3：时间合法性
            if (c.start_us < 0 || c.duration_us <= 0 || c.in_us < 0) {
                err = "R3: clip '" + c.id + "' 时间参数非法 (start=" +
                      std::to_string(c.start_us) + ", duration=" +
                      std::to_string(c.duration_us) + ", in=" +
                      std::to_string(c.in_us) + ")";
                return false;
            }

            // R3：transform 数值合法性（NaN/Inf 拦截）
            const auto& tr = c.transform;
            if (!std::isfinite(tr.x) || !std::isfinite(tr.y) ||
                !std::isfinite(tr.w) || !std::isfinite(tr.h) ||
                !std::isfinite(tr.rotation) || !std::isfinite(tr.opacity)) {
                err = "R3: clip '" + c.id + "' transform 含 NaN/Inf";
                return false;
            }

            // R8：tween 通道合法性（2026-11-05）——属性已知 / 非空 / 时间
            // 在片段局部区间内 / 数值有限 / 无重复时间点（加载期已按 t_us
            // 升序稳定排序，此处相邻相等即重复）
            for (const auto& tw : c.tweens) {
                if (tw.prop == TweenProp::Unknown) {
                    err = "R8: clip '" + c.id +
                          "' tween 通道属性为 Unknown（未知枚举值，拒绝进入模型）";
                    return false;
                }
                if (tw.keys.empty()) {
                    err = "R8: clip '" + c.id + "' 存在无关键帧的 tween 通道";
                    return false;
                }
                for (std::size_t i = 0; i < tw.keys.size(); ++i) {
                    const auto& k = tw.keys[i];
                    // 关键帧时间 ≥ 0（相对 clip.start_us 的局部时间）。
                    // 【2026-11-06 放宽"t_us ≤ duration"上限】clip 在时间轴
                    // 上缩短（剪裁时长）后，出点外的既有键保留在通道里
                    // （WATCHOUT trim 无损语义：拉长片段键自然恢复）——越界
                    // 键是【遮蔽】而非损坏：求值端（TweenEngine::evalChannel
                    // 的 clip_duration_us 遮蔽）与绘制端（panel/曲线带按
                    // length 裁剪）都不消费它们。此前严格拒绝会导致带越界
                    // 键的场景在 display 端整体校验失败——编辑器缩短带键
                    // 片段后显示端永久停留旧场景（"预览正常、display 不同
                    // 步"故障根因）。负值仍是非法（真损坏）。
                    if (k.t_us < 0) {
                        err = "R8: clip '" + c.id +
                              "' tween 关键帧时间为负 (t=" +
                              std::to_string(k.t_us) + ")";
                        return false;
                    }
                    if (!std::isfinite(k.v0) || !std::isfinite(k.v1)) {
                        err = "R8: clip '" + c.id + "' tween 关键帧值含 NaN/Inf";
                        return false;
                    }
                    if (i > 0 && tw.keys[i - 1].t_us == k.t_us) {
                        err = "R8: clip '" + c.id + "' tween 关键帧时间重复 (t=" +
                              std::to_string(k.t_us) + ")";
                        return false;
                    }
                }
            }

            // R1：素材引用存在性
            // 【Control 豁免 2026-11-13】指令片段无素材绑定语义（asset_id
            // 恒空）——R1/R2 素材存在性与类型约束不适用；指令表在
            // clip.control_messages（片段自身业务数据，随 pb 分发）。
            // R3/R7/R8 通用约束（时间/transform/tween）照走。
            if (t.kind == AssetKind::Control) {
                if (!c.asset_id.empty()) {
                    err = "R2: Control 轨道 '" + t.id + "' 的 clip '" +
                          c.id + "' 不应绑定素材 (asset_id='" +
                          c.asset_id + "')";
                    return false;
                }
            } else {
                const Asset* asset = scene.findAsset(c.asset_id);
                if (asset == nullptr) {
                    err = "R1: clip '" + c.id + "' 引用的素材不存在 (asset_id='" +
                          c.asset_id + "')";
                    return false;
                }

                // R2：轨道类型约束（clip 素材种类必须等于轨道 kind，且两者
                // 均不得为 Unknown——未知 proto 枚举值不得进入内存模型）
                if (asset->kind == AssetKind::Unknown ||
                    t.kind == AssetKind::Unknown) {
                    err = "R2: clip '" + c.id + "' 素材类型或轨道 '" + t.id +
                          "' 约束类型为 Unknown（未知枚举值，拒绝进入模型）";
                    return false;
                }
                if (asset->kind != t.kind) {
                    err = "R2: clip '" + c.id + "' 素材类型(" +
                          std::to_string(static_cast<int>(asset->kind)) +
                          ")与轨道 '" + t.id + "' 约束类型(" +
                          std::to_string(static_cast<int>(t.kind)) + ")不符";
                    return false;
                }
            }
        }
    }
    return true;
}

// ---------------------------------------------------------------------------
// Protobuf → 内存模型（含 R1~R7 校验）
// 任一校验失败：返回 nullopt，err 给出「规则编号 + 上下文」便于定位。
// ---------------------------------------------------------------------------
std::optional<Scene> SceneLoader::fromProto(const ms_proto::Scene& pb,
                                            std::string& err) {
    // R5：画布有效性
    if (pb.canvas_w() <= 0 || pb.canvas_h() <= 0) {
        err = "R5: canvas 尺寸非法 (" + std::to_string(pb.canvas_w()) + "x" +
              std::to_string(pb.canvas_h()) + ")";
        return std::nullopt;
    }

    Scene scene;
    scene.id       = pb.id();
    scene.version  = pb.version();
    scene.canvas_w = pb.canvas_w();
    scene.canvas_h = pb.canvas_h();

    // ---- 素材表 ----
    for (const auto& pa : pb.assets())
        scene.assets.push_back(fromProtoAsset(pa));

    // ---- 屏幕布局（R4 正矩形/旋转合法 + R7 id 唯一：统一走 validateScene）----
    for (const auto& ps : pb.screens())
        scene.screens.push_back(fromProtoScreen(ps));

    // ---- 时间轴（轨道 → 片段，纯转换；校验统一走 validateScene）----
    for (const auto& pt : pb.timeline().tracks())
        scene.timeline.tracks.push_back(fromProtoTrack(pt));

    // R1/R2/R3/R7：统一完整性校验（唯一校验入口，任何来源场景不过则拒）
    if (!validateScene(scene, err)) return std::nullopt;

    // R6：总时长自动推算（显式 0 = 未指定；负值非法）
    scene.timeline.duration_us = pb.timeline().duration_us();
    if (scene.timeline.duration_us < 0) {
        err = "R3: timeline.duration_us 负值非法 (" +
              std::to_string(scene.timeline.duration_us) + ")";
        return std::nullopt;
    }
    if (scene.timeline.duration_us == 0) {
        scene.timeline.duration_us = TimelineEngine::computeDurationUs(scene.timeline);
    }

    return scene;
}

// ---------------------------------------------------------------------------
// 二进制序列化 / 反序列化
// ---------------------------------------------------------------------------
std::string SceneLoader::serialize(const Scene& scene) {
    const ms_proto::Scene pb = toProto(scene);
    return pb.SerializeAsString();
}

std::optional<Scene> SceneLoader::parse(const std::string& bytes,
                                        std::string& err) {
    ms_proto::Scene pb;
    // ParseFromString 对截断/损坏数据返回 false（proto3 严格校验）
    if (!pb.ParseFromString(bytes)) {
        err = "Protobuf 反序列化失败：数据损坏或格式不匹配";
        return std::nullopt;
    }
    return fromProto(pb, err);
}

// ---------------------------------------------------------------------------
// 文件 IO（二进制模式，避免 Windows 文本模式改动字节流）
// ---------------------------------------------------------------------------
std::optional<Scene> SceneLoader::loadFromFile(const std::string& path,
                                               std::string& err) {
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        err = "无法打开场景文件: " + path;
        return std::nullopt;
    }
    std::ostringstream ss;
    ss << file.rdbuf();
    return parse(ss.str(), err);
}

bool SceneLoader::saveToFile(const Scene& scene, const std::string& path,
                             std::string& err) {
    std::ofstream file(path, std::ios::binary | std::ios::trunc);
    if (!file) {
        err = "无法写入场景文件: " + path;
        return false;
    }
    file << serialize(scene);
    // 【显式冲刷后再检查】good() 在缓冲未落盘时恒真——磁盘满/IO 错误要
    // 到 flush（或析构 close）才暴露；显式 flush 后检查才能可靠报错
    file.flush();
    if (!file.good()) {
        err = "场景写入失败（磁盘满或 IO 错误）: " + path;
        return false;
    }
    return true;
}

} // namespace ms
