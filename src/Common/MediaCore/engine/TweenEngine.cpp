// ============================================================================
//  TweenEngine.cpp —— Tween 关键帧插值纯函数实现
//
//  实现要点：
//    * keys 升序不变量（loader 排序 + 校验，editor 维护）允许二分定位；
//      典型通道关键帧个位/十数量级，二分足够（无须额外缓存）；
//    * interp 语义（存放在段起始关键帧上，即"本帧→下一帧怎么走"；v2 打包
//      属性整键统一——同一形状作用到 v0/v1 全部分量）：
//        Linear: 值域线性过渡
//        Hold:   保持 prev 值直到 next（阶跃）
//        Smooth: 3t²-2t³ 平滑步（两端斜率为零，等价 WATCHOUT 的
//                ease-in-out 简化形，无过冲）
//    * 端点保持：t < 首帧 → 首帧值；t >= 末帧 → 末帧值（外推恒值）。
// ============================================================================

#include "engine/TweenEngine.hpp"

#include <algorithm>
#include <cmath> // std::lround（Z 层级求值取整）

namespace ms {
namespace {

// 段内插值形状：归一化进度 u ∈ [0,1) → 过渡系数 s（Linear: s=u；
// Smooth: 3t²-2t³；Hold 由调用方短路处理）
inline double segmentShape(TweenInterp interp, double u) noexcept {
    switch (interp) {
        case TweenInterp::Smooth:
            return u * u * (3.0 - 2.0 * u); // 平滑步（零端斜率，无过冲）
        case TweenInterp::Linear:
        default:
            return u; // 线性
    }
}

} // namespace

// ---------------------------------------------------------------------------
// interpolate：段 [prev.t, next.t) 内的插值（v0/v1/v2 同形状分别过渡）
// ---------------------------------------------------------------------------
TweenEngine::ChannelValue TweenEngine::interpolate(
    const Keyframe& prev, const Keyframe& next,
    std::int64_t t_local_us) noexcept {
    const std::int64_t seg = next.t_us - prev.t_us;
    if (seg <= 0) return {prev.v0, prev.v1, prev.v2}; // 非法段防御（升序不变量下不可达）

    // 阶跃：不过渡（整键统一——全部分量同时保持）
    if (prev.interp == TweenInterp::Hold) return {prev.v0, prev.v1, prev.v2};

    // 归一化段内进度 u ∈ [0,1) → 过渡系数 s
    const double u = static_cast<double>(t_local_us - prev.t_us) /
                     static_cast<double>(seg);
    const double s = segmentShape(prev.interp, u);
    return {prev.v0 + s * (next.v0 - prev.v0),
            prev.v1 + s * (next.v1 - prev.v1),
            prev.v2 + s * (next.v2 - prev.v2)};
}

// ---------------------------------------------------------------------------
// evalChannel：单通道插值（二分定位 + 端点保持 + 出点外键剪裁遮蔽）
// ---------------------------------------------------------------------------
TweenEngine::ChannelValue TweenEngine::evalChannel(const TweenChannel& ch,
                                                   std::int64_t t_local_us,
                                                   std::int64_t clip_duration_us) noexcept {
    const auto& keys = ch.keys;
    if (keys.empty()) return {}; // 空通道防御（loader 校验下不可达）

    // ---- 剪裁遮蔽（2026-11-06 用户"剪裁时长"定案）--------------------------
    // 升序不变量下 t_us > duration 的键必为尾部连续段——二分取有效前缀
    // [begin, validEnd)。快路径：末键未越界（常态，无剪裁发生）直接全量，
    // 零 upper_bound 开销。最后有效键之后 = 端点保持（被剪键不参与过渡）。
    const auto validEnd =
        clip_duration_us >= keys.back().t_us
            ? keys.end() // 快路径：全部键有效（常态）
            : std::upper_bound(
                  keys.begin(), keys.end(), clip_duration_us,
                  [](std::int64_t d, const Keyframe& k) { return d < k.t_us; });
    if (validEnd == keys.begin())
        return {keys.front().v0, keys.front().v1,
                keys.front().v2}; // 全部键越界（防御）：取首键恒值

    const Keyframe& lastValid = *(validEnd - 1); // 最后有效键

    // 端点保持：t 在首帧之前（含相等）取首帧值
    if (t_local_us <= keys.front().t_us)
        return {keys.front().v0, keys.front().v1, keys.front().v2};
    // t 在最后【有效】帧之后（含相等）取其值——被剪键遮蔽（不外推到它们）
    if (t_local_us >= lastValid.t_us)
        return {lastValid.v0, lastValid.v1, lastValid.v2};

    // 二分定位：upper = 有效范围内第一个 t_us > t 的关键帧；
    // prev = upper-1 满足 t_us <= t（走到这里必有 prev 存在且段内插值）
    const auto upper = std::upper_bound(
        keys.begin(), validEnd, t_local_us,
        [](std::int64_t t, const Keyframe& k) { return t < k.t_us; });
    if (upper == keys.begin() || upper == validEnd) {
        // 不可达（上方端点分支已兜底）；防御返回边界值
        return upper == validEnd
                   ? ChannelValue{lastValid.v0, lastValid.v1, lastValid.v2}
                   : ChannelValue{keys.front().v0, keys.front().v1,
                                  keys.front().v2};
    }
    const Keyframe& prev = *(upper - 1);
    const Keyframe& next = *upper;
    if (prev.t_us == t_local_us)
        return {prev.v0, prev.v1, prev.v2}; // 精确命中（防御）
    return interpolate(prev, next, t_local_us);
}

// ---------------------------------------------------------------------------
// resolveTransform：静态基准 + 逐通道覆写 = 该帧的完整变换
// （v3/v4 打包属性：Position 三维通道 v0/v1/v2 分派 x/y/z——Z=5 通道 v3 起
//  不再产出，读端迁移合并；Rotation 三维通道 v0/v1/v2 分派 rot_x/rot_y/
// rotation（v4）——RotationX/Y 通道 v4 起不再产出，读端迁移合并；
// 二维/标量通道同 v2 语义）
// ---------------------------------------------------------------------------
Transform TweenEngine::resolveTransform(const Clip& clip,
                                        std::int64_t t_us) noexcept {
    // 基准：无通道的属性恒用静态 transform
    Transform out = clip.transform;

    // 片段局部时间（t_us 越界时同样走端点保持语义——clip 活跃区间外
    // 不会被 evaluate 收集，此处仅防御）
    const std::int64_t t_local = t_us - clip.start_us;

    for (const auto& ch : clip.tweens) {
        if (ch.keys.empty()) continue; // 空通道不覆写（防御）
        // 传入片段出点做剪裁遮蔽（出点外键不参与活跃期求值——用户
        // "剪裁时长"语义；与 TimelineEngine 可见性 [start, start+duration)
        // 同一口径）
        const ChannelValue v = evalChannel(ch, t_local, clip.duration_us);
        switch (ch.prop) {
            case TweenProp::Position:
                out.x = v.v0;
                out.y = v.v1;
                out.z = std::lround(v.v2); // v3：z 为第三分量（层级取整）
                break;
            case TweenProp::Size:
                out.w = v.v0;
                out.h = v.v1;
                break;
            case TweenProp::Rotation: // v4 三维打包：X 俯仰/Y 偏航/Z 滚转
                out.rot_x    = v.v0;
                out.rot_y    = v.v1;
                out.rotation = v.v2;
                break;
            case TweenProp::RotationX:  out.rot_x    = v.v0;             break; // v4 退役（迁移前遗留内存态防御）
            case TweenProp::RotationY:  out.rot_y    = v.v0;             break; // v4 退役（迁移前遗留内存态防御）
            case TweenProp::Opacity:  out.opacity = std::clamp(v.v0, 0.0, 1.0); break;
            case TweenProp::Z:        out.z = std::lround(v.v0);       break; // 层级取整
            // 裁切通道（v3 四边各一条标量）：求值结果进渲染端 crop 管线
            //（末端修正语义——不回写 Pos/Size）
            case TweenProp::CropL: out.crop[kCropL] = v.v0; break;
            case TweenProp::CropR: out.crop[kCropR] = v.v0; break;
            case TweenProp::CropT: out.crop[kCropT] = v.v0; break;
            case TweenProp::CropB: out.crop[kCropB] = v.v0; break;
            // 角点位移通道（每角一条二维打包）：v0/v1 分派到 corner 的
            // (dx, dy) 对（CornerIdx 序）；求值结果进渲染端既有 corner
            // 管线——位移叠加在旋转之后
            case TweenProp::CornerLT: out.corner[0] = v.v0; out.corner[1] = v.v1; break;
            case TweenProp::CornerRT: out.corner[2] = v.v0; out.corner[3] = v.v1; break;
            case TweenProp::CornerRB: out.corner[4] = v.v0; out.corner[5] = v.v1; break;
            case TweenProp::CornerLB: out.corner[6] = v.v0; out.corner[7] = v.v1; break;
            case TweenProp::Unknown:
            default:
                break; // 未知属性（新版本协议向下兼容）不覆写任何字段
        }
    }
    return out;
}

// ---------------------------------------------------------------------------
// resolveGain（2026-11-10）：音频增益解算——AudioGain 通道存在 = 整条接管
//（与 transform 属性同语义），无通道 = 静态基准 clip.audio_gain。结果钳
// [0,2]（上限 2.0 防增益曲线异常把混音推入持续削波；0=静音合法）。
// ---------------------------------------------------------------------------
double TweenEngine::resolveGain(const Clip& clip,
                                std::int64_t t_us) noexcept {
    const std::int64_t t_local = t_us - clip.start_us;
    for (const auto& ch : clip.tweens) {
        if (ch.prop != TweenProp::AudioGain || ch.keys.empty()) continue;
        const ChannelValue v =
            evalChannel(ch, t_local, clip.duration_us); // 出点剪裁遮蔽同源
        return std::clamp(v.v0, 0.0, 2.0);
    }
    return std::clamp(clip.audio_gain, 0.0, 2.0);
}

bool TweenEngine::autoKeyWrite(Clip& c, TweenProp prop,
                               std::int64_t t_local_us, double v0, double v1,
                               double v2) {
    TweenChannel* ch = nullptr;
    for (auto& t : c.tweens)
        if (t.prop == prop) { ch = &t; break; }
    if (!ch || ch->keys.empty()) return false; // 无通道：调用方走静态写入

    Keyframe k{};
    k.t_us   = t_local_us;
    k.v0     = v0;
    k.v1     = v1; // 标量属性调用方不传（默认 0），消费端按分量数解释
    k.v2     = v2; // 三维属性（Position）第三分量；其余属性为 0 无害
    auto it  = std::lower_bound(ch->keys.begin(), ch->keys.end(), k.t_us,
                                [](const Keyframe& a, std::int64_t t) {
                                    return a.t_us < t;
                                });
    // 插值继承（2026-11-09 平滑连续性）：新键沿用左邻键的过渡方式——
    // "平滑"通道连续打键不再回退直线（corner），WATCHOUT 同语义
    k.interp = (it != ch->keys.begin()) ? std::prev(it)->interp
                                        : TweenInterp::Linear;
    if (it != ch->keys.end() && it->t_us == t_local_us) {
        it->v0 = v0; // 同刻关键帧：只更新值（保留原插值方式）
        it->v1 = v1;
        it->v2 = v2;
    } else {
        ch->keys.insert(it, k); // 新键（升序插入，键序不变量）
    }
    return true;
}

} // namespace ms
