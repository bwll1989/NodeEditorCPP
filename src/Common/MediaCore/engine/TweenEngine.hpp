// ============================================================================
//  TweenEngine.hpp —— Tween 关键帧插值纯函数（Engine Core 组件）
//
//  设计依据（2026-11-05 Tween 体系定案，对标 WATCHOUT Tween 机制）：
//    * 变换曲线定义住在 Clip（条目）上，随场景文档一次性分发；运行时只流动
//      播放头 t——参数值是每帧本地插值结果，永不在线传输（与 TimelineEngine
//      同一纯函数纪律：同一 (clip, t) 恒得同一结果，Editor 预览与 Player
//      渲染天然一致）；
//    * 通道类型无关（住在 Clip 而非 Asset）：舞台通道（X/Y/W/H/旋转/透明度）
//      对所有媒体类型通用；类型专属参数（如未来的音量）要先下沉到 Clip 级
//      才能被动画化（prop 枚举预留扩展）；
//    * 语义：有 tween 通道的属性【整条接管】该属性——通道值覆写静态基准，
//      不做叠加混合；无通道的属性恒用 Clip.transform 静态基准值；
//    * 无外部依赖（纯 C++，对序列化/Qt 零感知）。
//
//  关键帧时间约定：Keyframe.t_us 是相对 clip.start_us 的局部时间
//  ∈ [0, duration_us]——片段整体挪动时曲线随动，无需重算关键帧。
// ============================================================================

#pragma once

#include <cstdint>
#include <limits>

#include "model/SceneModel.hpp"

namespace ms {

// ---------------------------------------------------------------------------
// TweenEngine —— Tween 通道求值器（全部静态纯函数，无 this 状态）
//
// v2 打包属性（2026-11-09 用户定案，DaVinci Resolve Inspector 模式）：
// 每条属性单独打键，一个键携带 v0/v1 全部分量（二维属性打包，标量只用
// v0）；插值按分量独立进行但共用同一键级 interp 与时间轴。
// ---------------------------------------------------------------------------
class TweenEngine {
public:
    // ---- 通道求值（单条通道的插值内核，可独立测试）----

    /// 通道求值结果：v0=标量值/向量第一分量，v1=向量第二分量，v2=第三分量
    ///（标量通道后两者恒 0——调用方只应消费本属性分量数范围内的分量）
    struct ChannelValue {
        double v0 = 0.0;
        double v1 = 0.0;
        double v2 = 0.0;
    };

    /// 单条通道在局部时间 t_local（相对 clip.start_us，us）的插值值。
    /// 前置条件：ch.keys 非空且按 t_us 升序（loader 保证；editor 维护）。
    /// 边界语义【端点保持】：t 在首关键帧之前取首帧值、在末关键帧之后取
    /// 末帧值（WATCHOUT 同语义：曲线两端外推为恒值，不回绕）。
    /// \param clip_duration_us 剪裁遮蔽（2026-11-06 用户"剪裁时长"定案）：
    ///   t_us 超出出点的键不参与求值（升序不变量下越界键必为尾部连续段，
    ///   求值只看有效前缀——最后有效键端点保持）。缺省 INT64_MAX = 不裁
    /// （旧调用/单测零改动）。clip 缩短（剪裁时长）后出点外的键被遮蔽：
    ///   ① 活跃期插值不朝被剪键过渡（尾部保持最后有效键值）；
    ///   ② 数据保留——拉长片段后键自然恢复（WATCHOUT trim 无损语义）。
    static ChannelValue evalChannel(const TweenChannel& ch,
                                    std::int64_t t_local_us,
                                    std::int64_t clip_duration_us
                                        = std::numeric_limits<std::int64_t>::max()) noexcept;

    /// 两关键帧间的插值（prev.interp 决定方式；seg_len_us<=0 防御取 prev 值；
    /// v0/v1 共用同一插值形状分别过渡）
    static ChannelValue interpolate(const Keyframe& prev, const Keyframe& next,
                                    std::int64_t t_local_us) noexcept;

    // ---- 整体解算（evaluate 的每帧入口）----

    /// 解算片段在播放头 t_us 的【完整变换】：以 clip.transform 为基准，
    /// 逐条 tween 通道覆写对应属性（有通道的属性整条接管；二维打包通道
    /// v0/v1 分量分派到对应 Transform 字段对）。
    /// 纯函数：无缓存、无状态，每帧重算；片段量级（数百）下成本可忽略。
    /// opacity 结果钳制到 [0,1]（防曲线越界进入混合器产生异常）。
    static Transform resolveTransform(const Clip& clip,
                                      std::int64_t t_us) noexcept;

    /// 解算片段在播放头 t_us 的音频增益（2026-11-10）：AudioGain 通道存在
    /// → evalChannel v0（线性/阶跃/平滑同变换曲线内核），钳 [0,2]（2.0 上
    /// 限防混音削波失控）；无通道 → 静态基准 clip.audio_gain。纯函数，
    /// 与 resolveTransform 同源同纪律——音频链路（AudioEngine）消费端对
    /// "通道/静态"两来源零感知。
    static double resolveGain(const Clip& clip, std::int64_t t_us) noexcept;

    // ---- 键写入（auto-key 共用内核：画布拖拽与属性面板同语义）------------

    /// 播放头处写/改键（auto-key，v3 打包写入）：prop 通道存在且非空 →
    /// 把 (v0, v1, v2) 写入 t_local_us 处关键帧（同刻键只更新值并保留原
    /// 插值方式；新键继承左邻 interp、Linear 起步；lower_bound 保持键序
    /// 不变量），返回 true；无该属性通道 / 通道空 → 返回 false（调用方
    /// 走静态基准写入——双模直写的"静"侧）。标量属性忽略 v1/v2。
    /// 【2026-11-06 画布 auto-key 下沉 engine 层】画布拖拽（方案 A）与
    /// 属性面板值框共用一条键写入路径，防两处语义漂移。t_local_us 的
    /// 片段局部钳制（[0, duration]）归调用方（各自口径：面板 localPlayheadUs
    /// / 画布 preview 播放头反推）。
    static bool autoKeyWrite(Clip& c, TweenProp prop,
                             std::int64_t t_local_us, double v0,
                             double v1 = 0.0, double v2 = 0.0);
};

} // namespace ms
