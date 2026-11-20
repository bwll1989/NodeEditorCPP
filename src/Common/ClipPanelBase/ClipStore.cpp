// ============================================================================
//  ClipStore 实现 —— key 寻址读写（双模直写内核 + 通道生命周期）
//
//  设计依据（Doc.md §3 三件套）：Inspector 不摸 Clip/transform/tweens——
//  读走 get（WATCHOUT 终值：TweenEngine::resolveTransform/resolveGain 与
//  渲染端同源口径），写走 set（通道存在 → auto-key 打包写键；无通道 →
//  静态基准分量写入），编辑落定统一经 on_edited（宿主 notifyClipEdited
//  三腿编排：镜像回推 + sceneEdited + 面板广播——与插件专属区同构）。
//
//  双模直写语义对齐 TweenSection（2026-11-09 v2 定案）：编辑任一分量 =
//  整条属性打包写键（编辑分量取新值、其余分量取播放头处插值终值——
//  与画布拖拽打键同构）；Position 打键 v2 携带当前基准层级（2026-11-18
//  语义终定：UI 两分量 X/Y，第三数据槽=叠放层级，防老工程打零）。
// ============================================================================

#include "ClipStore.hpp"

#include <algorithm>
#include <cmath>
#include <cstdlib>

#include "ClipSchema.h"        // ms::findTweenField（分量数/分量名 schema）
#include "engine/TweenEngine.hpp"

namespace ms {

namespace {

/// 13 tween key → TweenProp 映射（ClipPanelBase 链 MediaCore，映射本域；
/// BaseClass schema 不引 TweenProp——跨 DLL 边界保持字符串词汇）
struct KeyEntry {
    const char* key;
    TweenProp   prop;
};
constexpr KeyEntry kTweenKeys[15] = {
    {"position",    TweenProp::Position},
    {"size",        TweenProp::Size},
    {"rotation",    TweenProp::Rotation},
    {"crop_top",    TweenProp::CropT},
    {"crop_bottom", TweenProp::CropB},
    {"crop_left",   TweenProp::CropL},
    {"crop_right",  TweenProp::CropR},
    {"corner_lt",   TweenProp::CornerLT},
    {"corner_rt",   TweenProp::CornerRT},
    {"corner_rb",   TweenProp::CornerRB},
    {"corner_lb",   TweenProp::CornerLB},
    {"opacity",     TweenProp::Opacity},
    {"audio_gain",  TweenProp::AudioGain},
    {"rgb_gain",    TweenProp::RgbGain},
    {"gamma",       TweenProp::Gamma},
};

TweenProp propForKey(const QString& key) {
    for (const auto& e : kTweenKeys)
        if (key == QLatin1String(e.key)) return e.prop;
    return TweenProp::Unknown;
}

TweenChannel* findChannel(Clip& c, TweenProp prop) {
    for (auto& ch : c.tweens)
        if (ch.prop == prop) return &ch;
    return nullptr;
}

const TweenChannel* findChannel(const Clip& c, TweenProp prop) {
    for (const auto& ch : c.tweens)
        if (ch.prop == prop) return &ch;
    return nullptr;
}

/// Transform 域字段读取（tween 13 键的解算结果统一消费；resolveTransform
/// 结果已含通道覆写+静态基准——一条口径与渲染端恒一致）。comp 越界 false。
/// 【裁切/角点下标】字面常量对齐 SceneModel（0=L 1=R 2=T 3=B；
/// corner = 角索引×2+(0=dx,1=dy)）。
bool transformGet(const Transform& tf, TweenProp prop, int comp, double& out) {
    switch (prop) {
        case TweenProp::Position: // v2 = 叠放层级（Transform::z，非 pos_z）
            if (comp == 0) { out = tf.x; return true; }
            if (comp == 1) { out = tf.y; return true; }
            if (comp == 2) { out = tf.z; return true; }
            return false;
        case TweenProp::Size:
            if (comp == 0) { out = tf.w; return true; }
            if (comp == 1) { out = tf.h; return true; }
            return false;
        case TweenProp::Rotation: // v0=俯仰 / v1=偏航 / v2=滚转（v4 打包序）
            if (comp == 0) { out = tf.rot_x; return true; }
            if (comp == 1) { out = tf.rot_y; return true; }
            if (comp == 2) { out = tf.rotation; return true; }
            return false;
        case TweenProp::Opacity:
            if (comp == 0) { out = tf.opacity; return true; }
            return false;
        case TweenProp::CropT:
            if (comp == 0) { out = tf.crop[2]; return true; } // kCropT
            return false;
        case TweenProp::CropB:
            if (comp == 0) { out = tf.crop[3]; return true; } // kCropB
            return false;
        case TweenProp::CropL:
            if (comp == 0) { out = tf.crop[0]; return true; } // kCropL
            return false;
        case TweenProp::CropR:
            if (comp == 0) { out = tf.crop[1]; return true; } // kCropR
            return false;
        case TweenProp::CornerLT:
            if (comp >= 0 && comp < 2) { out = tf.corner[0 * 2 + comp]; return true; }
            return false;
        case TweenProp::CornerRT:
            if (comp >= 0 && comp < 2) { out = tf.corner[1 * 2 + comp]; return true; }
            return false;
        case TweenProp::CornerRB:
            if (comp >= 0 && comp < 2) { out = tf.corner[2 * 2 + comp]; return true; }
            return false;
        case TweenProp::CornerLB:
            if (comp >= 0 && comp < 2) { out = tf.corner[3 * 2 + comp]; return true; }
            return false;
        // 颜色合成（2026-11-20）：RgbGain 三维打包（v0/v1/v2 = R/G/B）
        case TweenProp::RgbGain:
            if (comp >= 0 && comp < 3) { out = tf.rgb_gain[comp]; return true; }
            return false;
        case TweenProp::Gamma:
            if (comp == 0) { out = tf.gamma; return true; }
            return false;
        default:
            return false;
    }
}

/// Transform 域字段写入（静态基准分量级；整数域 z lround）
bool transformSet(Transform& tf, TweenProp prop, int comp, double v) {
    switch (prop) {
        case TweenProp::Position:
            if (comp == 0) { tf.x = v; return true; }
            if (comp == 1) { tf.y = v; return true; }
            if (comp == 2) { tf.z = static_cast<std::int32_t>(std::lround(v)); return true; }
            return false;
        case TweenProp::Size:
            if (comp == 0) { tf.w = v; return true; }
            if (comp == 1) { tf.h = v; return true; }
            return false;
        case TweenProp::Rotation:
            if (comp == 0) { tf.rot_x = v; return true; }
            if (comp == 1) { tf.rot_y = v; return true; }
            if (comp == 2) { tf.rotation = v; return true; }
            return false;
        case TweenProp::Opacity:
            if (comp == 0) { tf.opacity = v; return true; }
            return false;
        case TweenProp::CropT:
            if (comp == 0) { tf.crop[2] = v; return true; }
            return false;
        case TweenProp::CropB:
            if (comp == 0) { tf.crop[3] = v; return true; }
            return false;
        case TweenProp::CropL:
            if (comp == 0) { tf.crop[0] = v; return true; }
            return false;
        case TweenProp::CropR:
            if (comp == 0) { tf.crop[1] = v; return true; }
            return false;
        case TweenProp::CornerLT:
            if (comp >= 0 && comp < 2) { tf.corner[0 * 2 + comp] = v; return true; }
            return false;
        case TweenProp::CornerRT:
            if (comp >= 0 && comp < 2) { tf.corner[1 * 2 + comp] = v; return true; }
            return false;
        case TweenProp::CornerRB:
            if (comp >= 0 && comp < 2) { tf.corner[2 * 2 + comp] = v; return true; }
            return false;
        case TweenProp::CornerLB:
            if (comp >= 0 && comp < 2) { tf.corner[3 * 2 + comp] = v; return true; }
            return false;
        // 颜色合成（2026-11-20）：写侧钳制对齐 TweenEngine 解算口径
        case TweenProp::RgbGain:
            if (comp >= 0 && comp < 3) { tf.rgb_gain[comp] = std::clamp(v, 0.0, 2.0); return true; }
            return false;
        case TweenProp::Gamma:
            if (comp == 0) { tf.gamma = std::clamp(v, 0.1, 4.0); return true; }
            return false;
        default:
            return false;
    }
}

/// 关键帧"播放头处"命中：±20ms 内最近（帧量化 40000us/帧与场景任意 us
/// 键的编辑容差——TweenSection kKeyHitToleranceUs 同值同语义）
constexpr std::int64_t kKeyHitToleranceUs = 20'000;
int keyIndexAt(const TweenChannel& ch, std::int64_t t_local) {
    int best = -1;
    std::int64_t best_d = kKeyHitToleranceUs + 1;
    for (std::size_t i = 0; i < ch.keys.size(); ++i) {
        const std::int64_t d = std::llabs(ch.keys[i].t_us - t_local);
        if (d < best_d) { best_d = d; best = static_cast<int>(i); }
    }
    return best;
}

/// 播放头处打包打键（当前终值全分量）——建通道首键与未命中打键共用内核
void writeKeyAtPlayhead(Clip& c, TweenProp prop, const QString& key,
                        std::int64_t t_abs) {
    const std::int64_t t_local =
        std::clamp(t_abs - c.start_us, static_cast<std::int64_t>(0),
                   c.duration_us);
    double v[3] = {0, 0, 0};
    const int n = ClipStore::componentCount(key);
    if (prop == TweenProp::AudioGain) {
        v[0] = TweenEngine::resolveGain(c, t_abs); // 通道空=静态基准
    } else {
        const Transform cur = TweenEngine::resolveTransform(c, t_abs);
        for (int i = 0; i < n && i < 3; ++i) transformGet(cur, prop, i, v[i]);
    }
    TweenChannel* ch = findChannel(c, prop);
    if (!ch) {
        c.tweens.push_back({prop, {}});
        ch = &c.tweens.back();
    }
    // 保序插入（升序不变量；新键 Linear 起步——TweenSection 打键同纪律）
    const auto it = std::lower_bound(
        ch->keys.begin(), ch->keys.end(), t_local,
        [](const Keyframe& k, std::int64_t t) { return k.t_us < t; });
    ch->keys.insert(it, {t_local, v[0], v[1], v[2], TweenInterp::Linear});
}

} // namespace

// ---------------------------------------------------------------------------
// 构造 / 基础
// ---------------------------------------------------------------------------

ClipStore::ClipStore(const ClipSectionContext& ctx, PlayheadFn playhead)
    : clip_lookup_(ctx.clip_lookup),
      on_edited_(ctx.on_edited),
      playhead_(std::move(playhead)),
      ctx_clip_(ctx.clip) {}

ms::Clip* ClipStore::locateClip() const {
    // 悬垂防御纪律（2026-11-15）：vector<Clip> 搬移安全——每现查
    if (clip_lookup_) return clip_lookup_();
    return ctx_clip_; // 工厂时刻快照兜底（lookup 缺失场合）
}

std::int64_t ClipStore::playheadUs(const ms::Clip* c) const {
    return playhead_ ? playhead_() : (c ? c->start_us : 0);
}

void ClipStore::notifyEdited() {
    if (on_edited_) on_edited_();
}

int ClipStore::componentCount(const QString& key) {
    // 静态扩展（schema 表外）恒标量
    if (key == QLatin1String("anchor") || key == QLatin1String("depth_z") ||
        key == QLatin1String("stacking"))
        return 1;
    const FieldDef* fd = findTweenField(key);
    return fd ? fd->components : 0;
}

// ---------------------------------------------------------------------------
// 读：播放头处 WATCHOUT 终值（通道插值优先，无通道静态基准）
// ---------------------------------------------------------------------------

bool ClipStore::get(const QString& key, int comp, double& out,
                    std::int64_t t_us) const {
    const ms::Clip* c = locateClip();
    if (!c) return false;
    const std::int64_t t = (t_us >= 0) ? t_us : playheadUs(c);

    // ---- 静态扩展（非 tween 语义，直读 Clip/Transform 静态字段）----------
    if (key == QLatin1String("anchor")) {
        if (comp != 0) return false;
        out = c->transform.anchor;
        return true;
    }
    if (key == QLatin1String("depth_z")) {
        if (comp != 0) return false;
        out = c->transform.pos_z;
        return true;
    }
    if (key == QLatin1String("stacking")) {
        if (comp != 0) return false;
        out = c->stacking;
        return true;
    }
    if (key == QLatin1String("audio_gain")) {
        if (comp != 0) return false;
        out = TweenEngine::resolveGain(*c, t); // 通道插值/静态同源（钳 [0,2]）
        return true;
    }

    const TweenProp prop = propForKey(key);
    if (prop == TweenProp::Unknown) return false;

    // ---- tween 域：整条解算（通道覆写+静态基准）一条口径 -----------------
    const Transform tf = TweenEngine::resolveTransform(*c, t);
    return transformGet(tf, prop, comp, out);
}

bool ClipStore::get(const QString& path, double& out, std::int64_t t_us) const {
    const int dot = path.indexOf('.');
    if (dot < 0) return get(path, 0, out, t_us);
    const QString key = path.left(dot);
    const QString compName = path.mid(dot + 1);
    const FieldDef* fd = findTweenField(key);
    if (fd) {
        for (int c = 0; c < 3; ++c)
            if (compName == QLatin1String(fd->comp_names[c]))
                return get(key, c, out, t_us);
    }
    // 数字分量名（"position.2"）通用兜底
    bool ok_num = false;
    const int c = compName.toInt(&ok_num);
    if (ok_num) return get(key, c, out, t_us);
    return false;
}

// ---------------------------------------------------------------------------
// 写：双模直写（auto-key 打包 / 静态基准回退）+ 编辑落定
// ---------------------------------------------------------------------------

bool ClipStore::set(const QString& key, int comp, double value,
                    std::int64_t t_us) {
    ms::Clip* c = locateClip();
    if (!c) return false;
    const std::int64_t t = (t_us >= 0) ? t_us : playheadUs(c);

    // ---- 静态扩展直写（无通道语义）--------------------------------------
    if (key == QLatin1String("anchor")) {
        if (comp != 0) return false;
        c->transform.anchor =
            static_cast<std::int32_t>(std::lround(value));
        notifyEdited();
        return true;
    }
    if (key == QLatin1String("depth_z")) {
        if (comp != 0) return false;
        c->transform.pos_z = value;
        notifyEdited();
        return true;
    }
    if (key == QLatin1String("stacking")) {
        if (comp != 0) return false;
        c->stacking = static_cast<int>(
            std::clamp(std::lround(value), 0L, 1L)); // 0=By Layer 1=By Z
        notifyEdited();
        return true;
    }

    const TweenProp prop = propForKey(key);
    if (prop == TweenProp::Unknown) return false;
    const int n = componentCount(key);
    if (comp < 0 || comp >= n) return false;
    const std::int64_t t_local =
        std::clamp(t - c->start_us, static_cast<std::int64_t>(0),
                   c->duration_us);

    // ---- 双模直写：通道存在 → 打包写键；无通道 → 静态基准分量 -----------
    const TweenChannel* ch = findChannel(*c, prop);
    if (ch && !ch->keys.empty()) {
        // 打包语义：编辑分量取新值、其余分量取播放头处插值终值（一个键
        // 携带全部分量——与画布拖拽/值框 auto-key 同构）
        double v[3] = {0, 0, 0};
        if (prop == TweenProp::AudioGain) {
            v[0] = TweenEngine::resolveGain(*c, t);
        } else {
            const Transform cur = TweenEngine::resolveTransform(*c, t);
            for (int i = 0; i < n && i < 3; ++i)
                transformGet(cur, prop, i, v[i]);
        }
        v[comp] = value;
        if (!TweenEngine::autoKeyWrite(*c, prop, t_local, v[0], v[1], v[2]))
            return false; // 保守失败（通道存在已滤，理论不可达）
        notifyEdited();
        return true;
    }

    // 静态基准写入（分量级；audio_gain 基准在 Clip 级非 Transform）
    if (prop == TweenProp::AudioGain) {
        c->audio_gain = value;
    } else if (!transformSet(c->transform, prop, comp, value)) {
        return false;
    }
    notifyEdited();
    return true;
}

bool ClipStore::set(const QString& path, double value, std::int64_t t_us) {
    const int dot = path.indexOf('.');
    if (dot < 0) return set(path, 0, value, t_us);
    const QString key = path.left(dot);
    const QString compName = path.mid(dot + 1);
    const FieldDef* fd = findTweenField(key);
    if (fd) {
        for (int c = 0; c < 3; ++c)
            if (compName == QLatin1String(fd->comp_names[c]))
                return set(key, c, value, t_us);
    }
    bool ok_num = false;
    const int c = compName.toInt(&ok_num);
    if (ok_num) return set(key, c, value, t_us);
    return false;
}

// ---------------------------------------------------------------------------
// 通道生命周期（菱形钮三态后端；TweenSection onDiamondClicked 同款纪律）
// ---------------------------------------------------------------------------

bool ClipStore::hasChannel(const QString& key) const {
    const ms::Clip* c = locateClip();
    if (!c) return false;
    const TweenProp prop = propForKey(key);
    if (prop == TweenProp::Unknown) return false;
    const TweenChannel* ch = findChannel(*c, prop);
    return ch && !ch->keys.empty();
}

bool ClipStore::toggleKey(const QString& key, std::int64_t t_us) {
    ms::Clip* c = locateClip();
    if (!c) return false;
    const TweenProp prop = propForKey(key);
    if (prop == TweenProp::Unknown) return false; // 静态扩展无动画语义
    const std::int64_t t = (t_us >= 0) ? t_us : playheadUs(c);

    TweenChannel* ch = findChannel(*c, prop);
    if (!ch || ch->keys.empty()) {
        // 无通道 → 建通道 + 播放头处打包打首键（当前终值）
        writeKeyAtPlayhead(*c, prop, key, t);
        notifyEdited();
        return true;
    }

    // 有通道 → 播放头处切换：命中（±20ms）删该处键（删空连通道删——
    // R8 通道生命终点纪律）；未命中按当前值打键
    const std::int64_t t_local =
        std::clamp(t - c->start_us, static_cast<std::int64_t>(0),
                   c->duration_us);
    const int hit = keyIndexAt(*ch, t_local);
    if (hit >= 0) {
        ch->keys.erase(ch->keys.begin() + hit);
        if (ch->keys.empty()) {
            c->tweens.erase(
                std::remove_if(c->tweens.begin(), c->tweens.end(),
                               [prop](const TweenChannel& x) {
                                   return x.prop == prop;
                               }),
                c->tweens.end());
        }
        notifyEdited();
        return true;
    }
    writeKeyAtPlayhead(*c, prop, key, t);
    notifyEdited();
    return true;
}

bool ClipStore::removeChannel(const QString& key) {
    ms::Clip* c = locateClip();
    if (!c) return false;
    const TweenProp prop = propForKey(key);
    if (prop == TweenProp::Unknown) return false;
    const auto it = std::remove_if(
        c->tweens.begin(), c->tweens.end(),
        [prop](const TweenChannel& x) { return x.prop == prop; });
    if (it == c->tweens.end()) return false; // 本就无通道
    c->tweens.erase(it, c->tweens.end());
    notifyEdited();
    return true;
}

} // namespace ms
