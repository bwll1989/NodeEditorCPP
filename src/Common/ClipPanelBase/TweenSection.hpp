// ============================================================================
//  TweenSection —— 变换与关键帧合并编辑区（片段属性面板的通用附加区）
//
//  【方案 A（2026-11-06 用户拍板）】画布变换 + Tween 关键帧动画合并为
//  单列属性列表——本区块是位置/尺寸/旋转/不透明度/Z/角点【唯一】编辑 UI
// （ClipPropertyPanel 的画布变换区已退役，媒体区保留）。
//
//  【v2 打包属性（2026-11-09 用户定案，参照 DaVinci Resolve Inspector）】
//  二维值打包成一条属性、每条属性单独打关键帧（一个键携带 v0/v1 两分量）：
//    * 每行 = [菱形钮] [名] [值框]（二维属性为 [值框A] [值框B] 双值框）：
//      - 菱形钮 = 动画通道开关（三态）：实心菱=播放头处有关键帧；
//        空心菱=通道存在但播放头处无键；空心圆=未启用（纯静态）。
//        点击未启用 → 建通道（当前值打首键）；点击已启用 → 整通道移除
//        （丢弃全部关键帧——tooltip 警示）；
//      - 值框 = 播放头处当前分量值（通道插值结果；无通道=静态基准值）。
//        【auto-key 打包直写】通道存在时改任一分量即整条属性在播放头处
//        写/改关键帧（v0=编辑分量新值、v1=另一分量播放头当前值——打包
//        语义下一个键携带全部分量，与画布拖拽打键同构）；无通道时改值
//        = 写静态基准 transform（分量级脏位合并）；
//      - 新关键帧插值恒 Linear（数据模型 per-key interp 字段保留——
//        文件/协议/引擎不变；行尾「N帧」计数与「插值▾」下拉按用户定案
//        删除：DaVinci 式极简行尾只留菱形，插值编辑走时间轴曲线）；
//      - 关键帧删除 = 时间轴片段曲线面板点选控制点（空心→实心）后
//        Delete/右键（2026-11-09 删帧交互改版：行尾「删帧」按钮废除）。
//    * Z 行单值（标量属性：v1 恒 0）；角点每角一条二维属性（CornerLT..
//      CornerLB 四行，各 X/Y 双值框——每角单独打键，用户三选一定案）。
//    * 挂接方式 = ClipPropertyPanel::appendSection（面板"只追加不重构"
//      扩展约定——不揉进既有通用区控件，遵守片段属性面板冻结铁律；
//      画布变换区退役属 2026-11-06 用户拍板方案 A 的明确授权范围）；
//    * 播放头来源 = 注入的 PlayheadFn（适配器提供 getPlayheadPos 帧→us），
//      值框恒显示播放头处的插值结果（TweenEngine::resolveTransform 同时
//      覆盖通道/静态两种来源——一处显示，绝无"两个值"）；
//    * 双向同步：refreshFrom（适配器广播灌值：内容变化广播 + 播放头移动
//      专用刷新），QSignalBlocker 防回环 + 焦点保护（正在输入的数值框
//      不被灌值打断）。
//  测试定位锚：objectName 前缀 tween*（tweenPosX/tweenPosY/tweenPosZ/
//  tweenSizeW/tweenSizeH/tweenRotX/tweenRotY/tweenRotZ/tweenCropT/B/L/R/
//  tweenCornerLTX…/tweenGain + tweenEnable<同后缀> + tweenReset<同后缀>）；
//  行序 = kPropOrder 表（位置/尺寸/旋转/裁切×4/角点×4/不透明度/音频增益，
//  13 行）。
//  折叠组标题钮：tweenGroupXform / tweenGroupPose / tweenGroupCrop /
//  tweenGroupCorner / tweenGroupComp（v3 五分组——DaVinci Inspector 档：
//  默认只展开「变换」，其余收起）。
//  行布局（v3 定案）：[名][值框...][◆菱形][⟲重置]——菱形移行尾、行尾追加
//  重置钮（DaVinci 同款「恢复默认值」）：有通道 → 播放头处打包写默认值
//  键（auto-key）；无通道 → 写静态基准默认值（双模落盘同值框纪律）。
//  （沿革：2026-11-05 Tween 区独立追加；2026-11-06 方案 A 收编静态
//   transform 编辑；2026-11-09 行名 Pos_X/Pos_Y/Size_W/Size_H + 锚点/
//   角点追加区 + 15 行渐进折叠；同日 v2 打包重构：15 分量行 → 9 属性行
//   （位置/尺寸/角点 X·Y 合并行），行尾只留菱形；同日 v3 五分组重设计：
//   Position 升三维 X/Y/Z（Z 行退役合并）、新增俯仰/偏航/裁切四行、
//   菱形移行尾 + ⟲ 重置钮、五折叠分组；同日 v4 姿态合并：旋转/俯仰/偏航
//   三行合并为一行三维打包（分量名 X/Y/Z=绕对应画布轴），行数 14→12。）
// ============================================================================

#pragma once

#include <cstdint>
#include <functional>
#include <vector>

#include <QString>
#include <QWidget>

#include "model/SceneModel.hpp"

#include "ClipPanelBaseExport.h" // CLIPPANELBASE_EXPORT（2026-11-13 迁入本 DLL）

class QComboBox;
class QToolButton;
class FloatDragValueWidget; // 全局命名空间类（GUIElements）——前向声明必须在 namespace 外

namespace ms {

/// 变换与关键帧合并编辑区：Clip.transform（静态基准）+ Clip.tweens（关键
/// 帧通道）的统一编辑域。生命周期：作为 ClipPropertyPanel::appendSection
/// 的子控件由 Qt 父子关系管理（随面板销毁）；写回只依赖注入的回调。
class CLIPPANELBASE_EXPORT TweenSection : public QWidget {
    Q_OBJECT
public:
    /// 写回钩子：控件变更 → 适配器 applyClipTweenEdit（定位工作副本片段）
    using ApplyFn = std::function<void()>;
    /// 播放头钩子：返回当前播放头（时间轴绝对时间 us）。「打键/删除」
    /// 定位 = 播放头 - clip.start_us（钳制到 [0, duration_us]）。
    using PlayheadFn = std::function<std::int64_t()>;

    /// \param clip   当前片段快照（通道/静态值初值来源；写回走 applier）
    /// \param parent Qt 父控件（= 承载面板 ClipPropertyPanel）
    explicit TweenSection(const Clip& clip, QWidget* parent = nullptr);

    /// 注入写回回调（构造后立即设置；未设置时控件只改显示不写回）。
    void setApplier(std::function<void()> applier) {
        applier_ = std::move(applier);
    }
    /// 注入播放头拉取钩子（适配器提供；未注入时打点落在片段起点）。
    void setPlayheadFn(PlayheadFn fn) { playhead_fn_ = std::move(fn); }
    /// 尺寸行 ⟲ 默认值 hint（2026-11-09 补设）：绑定素材的原始像素宽高
    ///（画布 px）。适配器 makeClipPropertyPanel 从工作副本素材表回填；
    /// 未注入（=0，Live/未绑定素材）时尺寸行 ⟲ 点击不动作。
    void setSizeDefaultHint(double w, double h) {
        size_hint_w_ = w;
        size_hint_h_ = h;
    }

    /// 控件态 → clip（tween 域无条件覆盖 + transform 静态脏字段合并）：
    ///   * clip.tweens = channels_（通道增删/关键帧编辑全部在本区块内存态
    ///     channels_ 上发生，applyTo 只做拷贝）；
    ///   * clip.transform 静态字段 = 仅覆盖 static_dirty_mask_ 置位的分量
    ///    （无通道属性上用户的直接编辑；防过期快照交叉覆盖，与
    ///     ClipPropertyPanel 的 dirty 机制同纪律）。
    void applyTo(Clip& c) const;

    /// 【双向同步·模型→面板向】外部变化/播放头移动灌显示：
    ///   * 菱形钮三态按 c.tweens 重建；
    ///   * 数值框显示播放头处（resolveTransform 统一来源：通道插值结果，
    ///     无通道即静态基准值）；焦点所在数值框跳过。
    /// QSignalBlocker 包裹全部灌值——不产生 toggled/valueChanged，不写回。
    void refreshFrom(const Clip& c);

    // ---- 编程化打点（公开供单测/未来快捷键复用；auto-key 内部即此）--------
    /// 在播放头处写入关键帧（存在则更新 / 不存在则插入；通道不存在自动
    /// 建通道）。【打包语义】二维属性一个键携带 v0/v1 两分量（取自该行
    /// 两个值框当前显示值——未编辑分量=播放头当前值）；新键插值恒
    /// Linear。值框 valueChanged 的 auto-key 即本函数转发。写回经 applier。
    void recordKeyframe(int prop_index);
    //（2026-11-09 删帧交互改版：行尾「删帧」按钮废除，关键帧删除迁至
    //  时间轴曲线面板——点选控制点后 Delete/右键；本区不再提供删除入口。
    //  同日 v2 定案：行尾「N帧」计数与「插值▾」下拉一并删除——DaVinci
    //  式行尾只留菱形，新键插值恒 Linear，数据 interp 字段保留。）

    /// 属性行数（= kPropOrder 表长度；测试遍历用）
    static int propCount();
    /// 行访问器（测试定位锚/程序化操作）：index ∈ [0, propCount)、
    /// component ∈ {0,1}（标量属性仅 0；越界返回 nullptr）
    FloatDragValueWidget* valueSpin(int prop_index, int component = 0) const;
    QToolButton*    enableWidget(int prop_index) const; // 菱形开关钮（见下）
    /// 锚点下拉访问器（tweenAnchor；2026-11-09 追加区测试锚）
    QComboBox* anchorCombo() const { return anchor_combo_; }
    /// 角点位移 spin 访问器：corner_index ∈ [0,4)（LT/RT/RB/LB）、
    /// axis 0=dx 1=dy；越界返回 nullptr。（v4：内部即
    /// rows_[7 + corner_index] 的分量值框——与主行同构）
    FloatDragValueWidget* cornerSpin(int corner_index, int axis) const {
        const int row = 7 + corner_index;
        return (corner_index >= 0 && corner_index < 4 && axis >= 0 &&
                axis < 2)
                   ? valueSpin(row, axis)
                   : nullptr;
    }

private:
    /// 菱形钮（2026-11-09 打键/删键切换语义）的点击转发：无通道 → 建通道
    ///（当前值打首键）；有通道 → 播放头处切换——命中(±20ms)删该处键
    ///（删空连通道删，R8 纪律），未命中按当前值打键（旧"整通道移除"退役）
    void onDiamondClicked(int prop_index);
    /// 值框编辑转发（auto-key 打包 / 静态分量双模写回，见文件头）
    void onValueEdited(int prop_index, int component, double v);
    /// ⟲ 重置钮（2026-11-10 用户定案）：删除该属性上全部 key 帧（通道
    /// 连删）+ 值恢复默认静态直写（旧"通道存在打默认值键"退役——默认值
    /// 键插进动画中间会拉直曲线，且用户体感 reset = 清掉该属性动画）。
    /// 尺寸行恢复目标 = 素材原始尺寸（hint 缺失点击不动作）
    void onResetClicked(int prop_index);
    /// 行内控件状态 → 菱形三态刷新（增删关键帧后调用）
    void updateRowState(int prop_index);
    /// 播放头 → 片段局部时间 us（钳制 [0, duration_us]；无钩子=0）
    std::int64_t localPlayheadUs() const;
    /// 在 channels_ 中定位/创建指定属性行的通道；\param create 为 false
    /// 且通道不存在时返回 null
    TweenChannel*       findChannel(TweenProp prop);
    const TweenChannel* findChannel(TweenProp prop) const;
    /// 定位"播放头处"关键帧下标：精确命中，或最近且 ±20ms 内（帧量化
    /// 编辑容差）；无命中返回 -1
    int keyIndexAtPlayhead(const TweenChannel& ch) const;

    // ---- 每属性一行（行序 = kPropOrder；objectName = 测试定位锚）------------
    struct Row {
        /// tweenEnable<suffix>：菱形开关钮（QToolButton + QPainter 自绘
        /// 12x12 pixmap 三态图标 ○/◇/◆——状态存 property("st") 0/1/2、
        /// 通道色预存 property("edge")；2026-11-09 终版：QCheckBox 指示器
        /// 与 Unicode 字符方案两轮皆因视觉占位不可控退役，pixmap 像素级
        /// 等大且与 ⟲ 同源）
        QToolButton*    enable = nullptr;
        FloatDragValueWidget* value[3] = {nullptr, nullptr,
                                    nullptr}; ///< tween<suffix><分量>：播放
                                          ///< 处分量值（Position 行三分量
                                          ///< X/Y/Z、二维行双框、标量仅 [0]）
                                          ///< ——通道存在时改任一分量即整
                                          ///< 属性打包打键；无通道=静态分量
        QToolButton*    reset  = nullptr; ///< ⟲ 重置钮（tweenReset<suffix>；
                                          ///<  全 12 行齐备——尺寸行语义=
                                          ///<  恢复素材原始尺寸 hint）
        TweenProp       prop   = TweenProp::Unknown; ///< v3 打包属性
        QString         name;             ///< 显示名（位置/尺寸/旋转/…）
        int             components = 1;   ///< 1=标量 2=二维 3=三维（Position）
    };
    std::vector<Row> rows_;   ///< 固定 13 行（kPropOrder 表序：位置/尺寸/
                              ///<  旋转/裁切×4/角点×4/不透明度/音频增益——
                              ///<  v4 五分组定案；Z 行与俯仰/偏航行退役合并）

    // ---- 折叠组（v3 五分组，DaVinci Inspector 同构；行连续切分）------------
    // 0=变换（锚点+位置/尺寸，默认展开）/ 1=姿态（旋转三轴一行）/
    // 2=裁切（上下左右）/ 3=形变（角点×4）/ 4=合成（不透明度）——除组 0
    // 外默认收。折叠只动可见性（控件/通道/写回全链路不感知）。
    QToolButton*          grp_btn_[5]  = {}; ///< 组标题钮（箭头随展开态翻转；
                                              ///<  objectName tweenGroupXform/
                                              ///<  Pose/Crop/Corner/Comp = 测试锚）
    std::vector<QWidget*> grp_rows_[5];  ///< 组容器（每组一个，hide/show
                                         ///<  对象——组内所有行同网格）

    // ---- 状态 -----------------------------------------------------------------
    std::vector<TweenChannel> channels_; ///< 内存态通道（编辑直接落这里，
                                         ///<  applyTo 拷贝进 clip.tweens）
    std::uint32_t static_dirty_ = 0;     ///< 静态基准脏位图（无通道属性的
                                         ///<  分量级直接编辑；applyTo 覆盖
                                         ///<  transform 对应字段；位定义见
                                         ///<  .cpp kRowSlot/kSAnchor）
    double        static_vals_[22] = {}; ///< 静态编辑值暂存（脏字段写回源；
                                         ///<  下标 = kRowSlot[行]+分量
                                         ///<  （13 行按分量数累计 22 槽）；
                                         ///<  存【画布绝对 px】——显示/数据
                                         ///<  一个口径直通）
    std::int64_t clip_start_us_ = 0;     ///< 片段起点（局部时间换算锚）
    std::int64_t clip_dur_us_   = 0;     ///< 片段时长（局部时间钳制上界）
    std::function<void()> applier_;      ///< 即时写回（适配器按 clip 定位）
    PlayheadFn playhead_fn_;             ///< 播放头拉取（适配器注入）
    double size_hint_w_ = 0.0;           ///< 尺寸行 ⟲ 默认值（素材原始宽；
                                         ///<  适配器注入；0=缺失点击不动作）
    double size_hint_h_ = 0.0;           ///< 尺寸行 ⟲ 默认值（素材原始高）

    // ---- 锚点（2026-11-09 追加区；静态属性非 tween 通道）--------------------
    QComboBox* anchor_combo_ = nullptr;  ///< tweenAnchor：锚点选择（中心/四角）
    int        anchor_val_ = 0;          ///< 锚点编辑值（0中心 1左上 2右上 3右下 4左下）
};

} // namespace ms
