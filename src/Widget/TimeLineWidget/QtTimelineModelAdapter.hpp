// ============================================================================
//  QtTimelineModelAdapter.hpp —— Scene(微秒) ↔ QtTimeLine(帧) 模型适配层
//
//  设计定位（设计文档 §4.2"适配器 + 共享 Core 模型"策略的第二迭代落地）：
//    * QtTimeLine 控件以"帧"为时间单位（BaseTimeLineModel 播放头/片段
//      start/end 全是 int 帧），Core 模型全线微秒（us，§6.1）——本类负责
//      双向换算与装载/回写，控件链路层不接触 us；
//    * 数据流单向（与 EditorWindow 同一套约定）：
//        [权威场景] → scene_work_(EditorWindow 拥有) ──load──► 本模型(视图)
//            ▲                                                │
//            │        clip 拖拽/属性面板改时间（模型先行）◄────┘
//            └── EditorWindow::onPublishScene 前先 syncModelToWorkScene()
//    * 本类只做"模型态"工作（装载/换算/回写），不持有网络与广播逻辑；
//      播放控制由 EditorWindow 直连工具栏信号（2026-11-10 遗留清理：
//      playRequested 三跳转发链退役）。
//
//  时间基准：固定 PAL 25fps（timecode 亦随 PAL 显示）。帧↔us 换算取
//  四舍五入：1 帧 = 40ms = 40000us；编辑精度受帧量化限制属预期行为。
//
//  播放头联动（两个方向，防回环）：
//    * 视图拖播放头 → 虚槽 onSetPlayheadPos(int) → 发 seekRequestedUs（带
//      80ms 节流，避免拖动风暴每像素都广播命令）→ EditorWindow::cmdSeek；
//    * Player 上报 → EditorWindow::onRefresh → setPlayheadFromRuntimeUs()
//      直调基类实现（静态绑定，绕开 seek 转发）→ 视图刷新。
//
//  片段标识：QtTimeLine ClipId 是控件侧自增整数，与 Core 的字符串 clip.id
//  无关——映射关系记在 AbstractClipModel::noteName（= ms clip id），
//  回写按 noteName 定位（对增删稳定，不按下标缓存）。
// ============================================================================

#pragma once

#include <QElapsedTimer>
#include <QPointer>
#include <QObject>
#include <QTimer>
#include <functional>
#include <unordered_map>
#include <unordered_set>
#include <vector>

#include "Common/TimeLineBase/BaseTimeLineModel.h"
#include "model/SceneModel.hpp"

class AbstractClipModel;
class QWidget;
class SceneClipModel;   // 全局命名空间（plugins/SceneClipModel.hpp 定义；
                        // 仅指针形参，前置声明足够）

namespace ms {

class ClipPropertyPanel;
class TweenSection;

/// 片段属性面板宿主钩子（EditorWindow 注入；面板导入时回调）。
/// 面板本身不依赖 EditorWindow——素材导入能力经此注入，
/// 未注入（hooks_ 空）时组合面板只含库通用区（无自定义面板）。
/// 【2026-11-03 用户约定"绑定控件没意义"】素材候选下拉已移除——当前
/// 绑定文件名由适配器从工作副本素材表解析后传入面板构造（只读标签），
/// 不再需要候选列表钩子。
struct ClipPanelHooks {
    /// 指定媒体文件导入：类型校验（须匹配轨道 kind）+ 入表 + 缓存清理；
    /// 成功返回新 Asset，失败 id 为空并填 err
    std::function<Asset(const QString& file, AssetKind kind,
                        std::string& err)> import_media;
};

class QtTimelineModelAdapter : public BaseTimeLineModel {
    Q_OBJECT

public:
    /// \param parent     Qt 父对象（loader/定时器统一挂本适配器）
    /// \param plugin_dir 插件目录覆盖（config.ini [editor] plugin_dir；
    ///        空 = 默认三候选扫描：exe 相对/工作目录，见 .cpp loadPlugins）
    explicit QtTimelineModelAdapter(QObject* parent = nullptr,
                                    const QString& plugin_dir = {});

    // ---- 工作副本挂接与装载 ---------------------------------------------------
    /// 挂接编辑工作副本（外部拥有；生命周期须覆盖本适配器）。回写目标。
    void attachWorkScene(Scene* work);
    /// 全量装载：清空模型 → 按 scene 重建轨道/片段 → 通知视图整体刷新。
    /// 属性面板/sync 等改变几何后也走这里？否——单 clip 变化请走
    /// setClipGeometryFromScene（增量、不重置视图缩放）。
    void loadSceneToModel(const Scene& scene);
    /// 属性面板时间字段 → 模型内单个 clip 的几何（按 clip_id 找 noteName）。
    /// 找不到（模型里没有该片段，如刚删）静默忽略。
    void setClipGeometryFromScene(const std::string& clip_id,
                                  std::int64_t start_us, std::int64_t dur_us);
    /// 属性面板素材绑定 → 模型内单个 clip 的 asset 镜像（增量）。
    /// SceneClipModel.assetId 供画布渲染/依赖联动；ms 工作副本仍是权威。
    void setClipAssetFromScene(const std::string& clip_id,
                               const std::string& asset_id);
    /// 注入片段属性面板宿主钩子（素材导入；构造后、首载前调用一次）
    void setClipPanelHooks(ClipPanelHooks hooks) { hooks_ = std::move(hooks); }
    /// 兜底同步：仅回写"时间轴改过"的 clip（按 clip 记脏，见 dirty_clips_）。
    /// 面板精确值（如 1.5s）不能被帧量化值覆盖，故不能全量同步。
    void syncModelToWorkScene();
    bool isDirty() const { return !dirty_clips_.empty(); } // 时间轴有未下发修改
    void clearDirty() { dirty_clips_.clear(); }

    // ---- Tween 关键帧时间轴镜像（2026-11-09 合并关键帧模式）----------------
    /// 单键【移动/改值】底层写回：local_us 直通（μs 同基，无量化）+
    /// 片段区间/邻键升序钳制（双保险）→ 工作副本 keys[key] 的 t_us/value 落值 →
    /// sceneEdited + 面板广播。公开以便单测直调与联测探针（--auto-tween-
    /// edit-ms）使用。【2026-11-09】合并关键帧模式后模型拖拽不再走本函数
    ///（改走 applyTweenKeyFrameMove 全通道联动），保留为底层编辑入口。
    /// \return 是否被接受（片段/下标非法 = false）
    bool applyTweenKeyMove(const std::string& clip_id,
                           int channel, int key, double local_us,
                           double value);
    /// 时间轴合并菱形【拖拽移动】写回（SceneClipModel::moveKeyframe → 注入
    /// 闭包 → 此处）：把时刻 old_us 处【全部通道】的键移到 new_us
    ///（Resolve 合并关键帧语义——拖一个菱形 = 全参数时间联动）。μs 直通
    /// + 片段区间钳制（R8 不变量）；通道内升序由模型桶级
    /// 邻钳保证（桶序保持 = 各通道键序保持），此处不再逐通道钳制。
    /// \return 是否有键被移动（片段不存在/该时刻无键 = false）
    bool applyTweenKeyFrameMove(const std::string& clip_id,
                                double old_us, double new_us);
    /// 时间轴右键菜单【删键】确认（单通道单键："按参数删除"子菜单路径）：
    /// 删键；空通道连通道删（R8 非空纪律，镜像同步删通道）。
    bool applyTweenKeyRemove(const std::string& clip_id, int channel, int key);

    // ---- Tween 关键帧时间轴镜像（镜像重灌）----------------------------------
    /// 按 clip id 用工作副本 clip.tweens 重灌该片段模型的关键帧镜像
    /// （us→帧换算 + 菱形重绘走 appearanceChanged→dataChanged→
    /// updateClipGeometry 局部刷新）。调用点：装载（loadSceneToModel，
    /// 直接走 buildTweenDots 不经本函数）与 Tween 区写回（通道增删后）。
    /// 【2026-11-09】片段带拖拽直接写镜像（跟手），release 后属性面板
    /// 写回链（applyClipTweenEdit → 本函数）把镜像与工作副本重新对齐。
    void syncClipTweenDots(const std::string& clip_id);

    /// 【画布 auto-key 镜像（2026-11-09 #89）】工作副本全部片段 tweens →
    /// 各 SceneClipModel 关键帧镜像重灌。画布拖拽/方向键的 auto-key 写在
    /// 工作副本（tweenAutoKeyWrite → TweenEngine::autoKeyWrite），不经
    /// applyClipTweenEdit 链——片段带菱形此前无人重灌（用户报告"拖画布
    /// 打键后时间轴看不到菱形"的根因）。全量幂等，只在离散时机调用
    ///（画布松手 sceneEdited / 方向键步进），无高频开销。
    void syncAllClipTweenDots();

    // ---- 播放头回显（Player 上报 → 视图；不发 seek）---------------------------
    /// Player 观测回显播放头。绕开 onSetPlayheadPos 虚派发（静态调基类），
    /// 否则会反向触发 seekRequestedUs 形成回环。
    void setPlayheadFromRuntimeUs(std::int64_t us);

    // ---- 结构编辑（时间轴右键/场景树按钮 → 模型 + 工作副本同步）--------------
    /// 新增轨道（按素材类型）：工作副本追加 ms::Track + 模型加行。
    /// \param kind 轨道素材类型约束（Video/Image/Text/Audio）
    /// \return 新轨道在工作副本中的下标；失败（未挂工作副本）返回 -1
    int  addTrackWithKind(AssetKind kind);
    /// 给指定轨道新增片段（引用既有素材）：工作副本追加 ms::Clip + 模型加行。
    /// \param track_idx 工作副本轨道下标（= 模型行号，两边顺序始终一致）
    /// \param start_us  片段起点（微秒）
    /// \param asset_id  绑定素材 id（必须已存在于工作副本 assets，且 kind 匹配）
    /// \param dur_us    片段时长（微秒；<=0 时取素材时长或默认 5s）
    /// \return 成功返回新片段 id（字符串）；失败返回空

    // 时间基已原生 μs（2026-11-10 路线 C）：库模型与 Scene 权威域同基，
    // 原帧↔μs 换算静态函数（framesToUs/usToFrames）已整体退役——显示层
    // 帧数换算收敛在 TimeLineDefines.h 的 framesToUs/usToFrames 内联函数。

signals:
    void seekRequestedUs(std::int64_t us); // 播放头拖动/程序化 seek → cmdSeek
    void sceneEdited();               // 时间轴有本地修改（状态栏"未下发"提示）
    /// [E1] 拖入媒体导入失败（钩子注入且失败）：宿主弹可见提示，且本次
    /// 拖入不留 1 帧空占位片段（原实现静默占位——用户以为导入成功）
    void mediaImportFailed(const QString& file, const QString& reason);

public slots:
    /// 播放头变化入口（视图拖动 / 外部调槽）：更新模型+视图，并排队 seek 转发
    /// （μs 同基直传，2026-11-10 微秒原生）
    void onSetPlayheadPos(qint64 newPlayheadPos) override;

    // ---- 结构编辑：时间轴原生入口（轨道列表右键菜单/删除键）覆写 ------------
    // 基类实现只改模型；此处先调基类再同步工作副本（顺序：模型成功才写业务态）。
    // 轨道列表右键 "Add Track" 子菜单来自注入的 WorkScenePluginLoader 类型表。
    void onAddTrack(const QString& type) override;
    /// 时间轴删除轨道 → 基类删模型行 + 工作副本删 ms::Track
    void onDeleteTrack(int trackIndex) override;
    /// 原生"添加片段"（时间轴右键 "Add Item"）→ 基类造占位片段 + 工作副本
    /// 同步 push（asset_id 置空）。用户双击片段在属性对话框「指定媒体文件…」
    /// 完成绑定；下发场景前由 EditorWindow 做 R1 前置提示（空绑定拦截）。
    void onAddClip(int trackIndex, qint64 startUs) override;
    /// 拖入媒体文件（资源管理器拖拽 → 视图 dropEvent）：导入素材（复用
    /// 面板导入钩子 hooks_.import_media——扩展名推断/入路径策略/元数据
    /// 探测/轨道类型校验/入表）→ 占位片段创建（onAddClip 同链）→ 素材
    /// 绑定 + 时长联动（素材时长 >0 时片段长度跟随，音频/视频即刻有
    /// 真实长度——2026-11-11 音频片段时长联动）。导入失败回退纯占位
    ///（与未注入钩子时同语义，不阻断拖入）。
    void onDropMediaFile(int trackIndex, qint64 startUs,
                         const QString& file) override;
    /// 时间轴删除片段 → 工作副本先删（按 noteName 定位）+ 基类删模型行
    void onDeleteClip(QModelIndex clipIndex) override;

    /// seek 节流定时器到点回调：发送最后一个待发位置（拖动结束后的最终值必达）。
    /// 公开以便测试手动触发（单测不依赖真实定时器事件循环）。
    void flushPendingSeek();

    /// 【双向同步·模型→面板向 2026-11-05】广播刷新：工作副本当前值 → 该
    /// 片段所有打开中的属性面板（timeline 组合面板自定义区 + 画布双击
    /// 对话框，同片段可并存多实例）的非焦点控件。触发源 = 画布拖拽 /
    /// 另一面板写回 / 时间轴拖动 / 场景装载（EditorWindow 在 refreshSceneViews
    /// 汇聚点与画布 sceneEdited 两处调用）。面板销毁（组合面板关闭 /
    /// WA_DeleteOnClose）由 QPointer 自动感知，广播时清理失效项。
    void refreshClipPanels(const std::string& clip_id);
    /// 全量广播：注册表内所有片段逐一 refreshClipPanels（注册表空时零开销）。
    void refreshAllClipPanels();
    /// 【Tween 专用·播放头驱动刷新】播放头移动（用户拖动/Player 回显）时
    /// 刷新已打开 Tween 区的"播放头处数值显示"（数值框恒显示插值结果——
    /// WATCHOUT 取景语义）。只灌 tween 区显示，不触发通用面板/写回链路
    /// （内容未变，仅观测点移动）。注册表空时零开销（播放期高频调用安全）。
    void refreshTweenPanelsPlayhead();

    /// 片段自有属性面板独立打开（画布/场景树双击入口，2026-11-04 用户约定）：
    /// 按工作副本 (track, clip) 下标定位模型片段 → clipPropertyWidget()
    /// （工厂闭包 = makeClipPropertyPanel：媒体绑定 + 画布变换区，与时间轴
    /// 双击组合面板的"片段自有区"完全同源）→ 包非模态对话框显示。
    /// 面板控件即时写回工作副本（sceneEdited 信号闭环已有，无需调用方善后）。
    /// \return 面板已弹出返回 true；下标越界/模型片段缺失/hooks 未注入返回 false
    bool showClipOwnPanel(int track_idx, int clip_idx, QWidget* parent);

private:
    /// clip 信号（timelinePositionChanged/lengthChanged）→ 工作副本回写
    void onClipGeometryChanged(AbstractClipModel* clip);
    /// 按片段显示名（noteName = ms clip id）在工作副本中定位；未找到返回 nullptr
    Clip* locateWorkClip(const QString& note);
    /// ms::Track.kind → QtTimeLine 轨道类型串（同名即约束语义）
    static QString typeForKind(AssetKind kind);
    /// QtTimeLine 轨道类型串 → ms::AssetKind（typeForKind 逆映射）
    static AssetKind kindForType(const QString& type);
    /// AssetKind 的短名（轨道/片段 id 前缀）：video/image/text/audio/clip
    static const char* kindShortName(AssetKind kind);

    // ---- 片段属性面板（QtTimeLine 原生附加体系）------------------------------
    /// 唯一片段 id 生成（2026-11-06 收敛原两份同构探测循环）：clip-N 线性
    /// 探测，与工作副本全表查重。onDuplicateClip / addClipOnTrack 等
    /// 创建路径共用（单点维护）。
    std::string nextClipId() const;
    /// 为 SceneClipModel 注入片段自定义面板工厂 + 诊断留痕。
    /// 【三条片段创建路径必须统一调用】loadSceneToModel（装载场景）/
    /// addClipOnTrack（素材树添加）/ onAddClip（时间轴右键 Add Item）——
    /// 漏注入的路径其片段双击时 clipPropertyWidget() 返回 null，
    /// 库面板附加点会报 "Cannot add a null widget to QVBoxLayout"。
    void configureClipPanel(SceneClipModel* sc, const std::string& clip_id,
                            const QString& type);
    /// 组装片段自定义面板（SceneClipModel::clipPropertyWidget 的工厂体）：
    /// 打开时实时取工作副本快照 + 按轨道类型过滤候选 + 接写回/导入钩子。
    /// 宿主未注入 hooks_ 时返回 nullptr（组合面板仅通用区）。
    QWidget* makeClipPropertyPanel(const std::string& clip_id,
                                   const QString& type);
    /// 面板写回（合并语义）：工作副本当前值 + 面板 dirty 字段 → 写回 +
    /// 模型镜像（时长变化推时间轴、素材变化推 assetId）+ sceneEdited。
    /// 合并保证面板过期快照不覆盖通用面板/画布拖拽的修改（见面板注释）。
    void applyClipPanelMerge(const std::string& clip_id,
                             ClipPropertyPanel* panel);
    /// 【2026-11-13 片段插件化】原 applyClipTextEdit/applyClipLiveEdit 已
    /// 随专属区块内化（Text/Live 区块随片段插件走，直写绑定素材 + 编辑
    /// 落定经 ctx.on_edited 回调回到本类的 sceneEdited/广播编排）。
    /// Tween 区写回（2026-11-05 Tween 体系）：按 clip id 定位工作副本片段
    /// → TweenSection 控件态写回 clip.tweens（tween 域唯一写者，无条件
    /// 覆盖）→ sceneEdited（触发"未下发"提示与自动下发）+ 面板广播。
    /// 编辑域是 Clip 的舞台通道（类型无关），与通用面板静态 transform 域
    /// 正交：通道启用的属性每帧被 TweenEngine 插值覆写。
    void applyClipTweenEdit(const std::string& clip_id,
                            TweenSection* section);

    // ---- 状态 ---------------------------------------------------------------
    Scene* work_   = nullptr;  // 编辑工作副本（不拥有）
    bool   loading_ = false;   // 装载/程序化写模型中：屏蔽 clip 信号的回写
    /// 时间轴真正改过的片段（noteName=ms clip id）。拖拽/面板改帧 → 记入；
    /// 面板→模型推送 → 移出（面板值对本 clip 保持权威）；sync 后清空。
    std::unordered_set<std::string> dirty_clips_;
    /// 已打开片段属性面板注册表（clip id → 面板列表）：双向同步的广播
    /// 目标。登记点 = makeClipPropertyPanel（timeline 组合面板自定义区与
    /// 画布双击对话框两条入口同源经过）；面板销毁经 QPointer 失效感知，
    /// 广播时惰性清理（不依赖关闭回调——库组合面板的销毁时机不可控）。
    std::unordered_map<std::string,
                       std::vector<QPointer<ClipPropertyPanel>>> clip_panels_;
    /// 片段专属编辑区注册表（clip id → 区块列表，2026-11-13 片段插件化）：
    /// 广播目标，经 ClipSectionInterface 灌值（宿主无具体区块类型——区块
    /// 随片段插件走）。登记点 = makeClipPropertyPanel（工厂加载处）；销毁
    /// 经 QPointer 失效感知 + 广播时惰性清理。区块是 ClipPropertyPanel 的
    /// 子控件（appendSection 追加），面板关闭随其销毁。
    std::unordered_map<std::string,
                       std::vector<QPointer<QWidget>>> clip_sections_;
    /// Tween 关键帧动画区注册表（clip id → 区块列表，2026-11-05）：广播
    /// 目标（Clip 域 tweens 灌值，全类型片段），与 clip_panels_ 同生命
    /// 周期语义——登记点 = makeClipPropertyPanel（所有片段挂接处）；
    /// 销毁经 QPointer 失效感知 + 广播时惰性清理。区块是 ClipPropertyPanel
    /// 的子控件（appendSection 追加），面板关闭随其销毁。
    std::unordered_map<std::string,
                       std::vector<QPointer<TweenSection>>> tween_panels_;
    /// 片段属性面板宿主钩子（EditorWindow 注入；空 = 无自定义面板）
    ClipPanelHooks hooks_;

    // seek 节流：拖动中 80ms 一发，松手后 flush 补发最终值
    QTimer*    seek_flush_      = nullptr;
    std::int64_t pending_seek_us_ = -1;  // -1 = 无待发

    // ---- Control 触发引擎（2026-11-13）--------------------------------------
    /// 播放头上升沿检测（Control 片段语义，2026-11-13 用户定案「时间到
    /// 那个地方了发送指令」）：prev < start && now >= start 的跨入瞬间，
    /// 对该 Control 片段 control_messages 逐条 StatusContainer::parseOSC
    ///（经 ms::fireControlMessages，见 BaseClass/ControlMessages.hpp）。
    /// 挂在播放头两落点（onSetPlayheadPos 手动 seek / setPlayheadFromRuntimeUs
    /// 播放回显）——同一检测保证 scrub 与播放语义一致。同区间内持续推进
    /// 不重触发（prev 已 >= start）；后退跨出后再次进入重新武装。
    void updateControlTriggers(std::int64_t now_us);
    /// 上一拍播放头（μs；-1 = 未初始化——首拍只建立基线不触发）
    std::int64_t prev_playhead_us_ = -1;
};

} // namespace ms
