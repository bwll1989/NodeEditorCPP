// ============================================================================
//  ClipStore —— 片段属性 key 寻址访问层（面板三件套之存储端，P4-C）
//
//  定位（Doc.md §3 终态架构）：
//    Inspector（通用渲染器，P4-E）——输入 = FieldDef 列表 + ClipStore 指针
//      └─ 只跟 FieldDef（schema 行）+ ClipStore 打交道，不直接摸
//         Clip.transform / clip.tweens / TweenProp——渲染与写回全走本层；
//    ClipStore（本头文件）—— key 寻址的读写门面：
//      * get(key, comp, t_us)  = 播放头处【WATCHOUT 终值】（通道插值优先，
//        无通道静态基准——TweenEngine::resolveTransform/resolveGain 同源
//        口径，与渲染端恒一致，绝无"两个值"）；
//      * set(key, comp, value) = 双模直写（通道存在 → auto-key 打包写键，
//        其余分量取播放头当前值；无通道 → 静态基准分量写入）+ 编辑落定
//        回调（宿主镜像+sceneEdited+广播，与插件专属区 ctx.on_edited 同编排）；
//      * hasChannel / toggleKey / removeChannel = 菱形钮三态的后端语义
//       （TweenSection onDiamondClicked 同款交互纪律：命中 ±20ms 删键/
//        未命中打键/无通道建通道打首键；⟲ 重置 = removeChannel+静态默认）。
//
//  key 词汇契约（与 P4-A ClipTweenExposure::exposed_keys 同词汇）：
//    13 个 tween key（对齐 BaseClass ms::kTweenSchema 行，见 ClipSchema.h）
//    + 3 个静态扩展：anchor（锚点 0..4）/ depth_z（几何深度 px）/
//    stacking（遮挡次序 0/1）。叠放层级 = position 分量 2（Transform::z，
//    非独立 key——UI 独立「层级 Z」行在 Inspector 侧映射）。
//
//  与 TweenSection 的关系（绞杀者模式）：TweenSection 自持 channels_ 内存
//  态 + applyTo 批量写回（既有链路不动）；ClipStore 是【逐字段即时写】
//  路径——Inspector 与未来专属区经它读写，两路径共享 TweenEngine 同一
//  求值/写键内核，语义不会漂移。TweenSection 后续迁 Inspector 时整体
//  换底（P4 后续切片）。
//
//  依赖：MediaCore（Clip/Transform/TweenEngine）+ BaseClass（kTweenSchema
//  分量数）——本库 ClipPanelBase 两者均链，符号自足。
// ============================================================================

#pragma once

#include <cstdint>
#include <functional>

#include <QString>

#include "model/ClipSectionContext.hpp" // ms::ClipSectionContext（闭包值拷贝）
#include "model/SceneModel.hpp"

#include "ClipPanelBaseExport.h"

namespace ms {

class CLIPPANELBASE_EXPORT ClipStore {
public:
    /// 播放头拉取（时间轴绝对 us；get/set 缺省 t 时消费）
    using PlayheadFn = std::function<std::int64_t()>;

    ClipStore() = default;

    /// \param ctx 宿主载荷——闭包【值拷贝】（makeClipPropertyPanel 的 ctx
    ///            是栈对象，工厂返回即销毁；std::function 值语义安全持有。
    ///            与插件 Section 对 ctx 的使用同纪律）；clip_lookup 缺失时
    ///            回退 ctx.clip 快照指针。
    /// \param playhead 播放头钩子（适配器注入；缺省 t_us<0 时回退片段起点）
    explicit ClipStore(const ms::ClipSectionContext& ctx,
                       PlayheadFn playhead = {});

    // ---- 读：播放头处终值（WATCHOUT：通道插值优先，无通道静态基准）-------
    /// \param key  属性 key（13 tween 词汇或 anchor/depth_z/stacking）
    /// \param comp 分量下标（0 基；超属性分量数返回 false）
    /// \param t_us 时间轴绝对 us；-1 = 注入播放头（无播放头 = 片段起点）
    /// \return false = 未知 key / 片段丢失 / 分量越界（out 不动）
    bool get(const QString& key, int comp, double& out,
             std::int64_t t_us = -1) const;
    /// path 便利重载："key"（=comp 0）或 "key.分量名"（如 "position.X"——
    /// 分量名取 schema comp_names；未知分量名 = false）
    bool get(const QString& path, double& out, std::int64_t t_us = -1) const;

    // ---- 写：双模直写（auto-key 判定 + 静态基准回退）+ 编辑落定回调 -------
    /// 通道存在 → 打包写键（编辑分量取新值、其余分量取播放头当前值——
    /// 与画布拖拽/值框 auto-key 同构；一个键携带全部分量）；无通道 →
    /// 静态基准分量写入。成功（含静态路径）后调 on_edited（宿主镜像 +
    /// sceneEdited + 面板广播）。未知 key / 分量越界 / 片段丢失 = false。
    bool set(const QString& key, int comp, double value,
             std::int64_t t_us = -1);
    bool set(const QString& path, double value, std::int64_t t_us = -1);

    // ---- 通道生命周期（属性面板菱形钮后端）--------------------------------
    /// 该属性是否已启用动画（通道存在且非空——菱形 ≠○）
    bool hasChannel(const QString& key) const;
    /// 菱形点击（TweenSection onDiamondClicked 同款切换语义）：无通道 →
    /// 建通道 + 播放头处打包打首键；有通道 → 播放头命中（±20ms 容差）
    /// 删该处键（删空连通道删）／未命中按当前值打键。
    bool toggleKey(const QString& key, std::int64_t t_us = -1);
    /// 整通道移除（⟲ 重置的动画侧：删该属性全部关键帧，动画停用）。
    /// 值的静态恢复（default_value 写回）由调用方 set 配套完成。
    bool removeChannel(const QString& key);

    /// 属性分量数（kTweenSchema schema 行；静态扩展恒 1；未知 key = 0）
    static int componentCount(const QString& key);

private:
    ms::Clip* locateClip() const;          ///< clip_lookup 现查（悬垂安全）
    std::int64_t playheadUs(const ms::Clip* c) const; ///< 缺省 t 解析
    void        notifyEdited();            ///< 编辑落定（宿主编排）

    std::function<ms::Clip*()> clip_lookup_; ///< 片段实时定位（vector 搬移安全）
    std::function<void()> on_edited_;        ///< 编辑落定回调（宿主镜像+广播）
    PlayheadFn playhead_;                    ///< 播放头拉取（适配器注入）
    ms::Clip* ctx_clip_ = nullptr;           ///< 工厂时刻快照（lookup 缺失兜底）
};

} // namespace ms
