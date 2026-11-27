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
//            └── 下发前先 syncModelToWorkScene()（已封装进 publishScene）
//    * 本类做"模型态"工作（装载/换算/回写）+ 场景下发/素材同步复合
//      操作编排（2026-11-16 用户定案：本类是模型对外的接口，下发分工
//      在此）；网络细节仍在 MsRuntimeBridge，本类只做编排。播放控制由
//      EditorWindow 直连工具栏信号（2026-11-10 遗留清理：playRequested
//      三跳转发链退役）。
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
#include "Common/AppConfig/OSCMessage.h" // OSC drag → Control 片段（onDropOscAddress 参数）
#include "model/SceneHistory.hpp" // 撤销历史（快照栈；收编点经 hist_ 可空借用）
#include "model/SceneModel.hpp"

class AbstractClipModel;
class QWidget;
class SceneClipModel;   // 全局命名空间（plugins/SceneClipModel.hpp 定义；
                        // 仅指针形参，前置声明足够）

namespace ms {

class ClipPropertyPanel;
class TweenSection;
class MsRuntimeBridge;      // 下发运行时桥（MsRuntimeBridge.hpp；仅指针成员）
class TimelineAudioBusHost; // 音频 bus 宿主（TimelineAudioBusHost.hpp；同上）
class PlaybackSendScheduler; // 时敏发送调度线程（SendScheduler.hpp；仅指针）

/// 片段属性面板宿主钩子（EditorWindow 注入；面板导入时回调）。
/// 面板本身不依赖 EditorWindow——素材导入能力经此注入，
/// 未注入（hooks_ 空）时组合面板只含库通用区（无自定义面板）。
/// 【2026-11-03 用户约定"绑定控件没意义"】素材候选下拉已移除——当前
/// 绑定文件名由适配器从工作副本素材表解析后传入面板构造（只读标签），
/// 不再需要候选列表钩子。
/// 【2026-11-15 轨道去类型化】import_media 的 kind 实参恒传 Unknown
///（拖入落轨路径）——轨道类型约束已退役，素材种类由文件扩展名推断。
struct ClipPanelHooks {
    /// 指定媒体文件导入：扩展名推断种类 + 入表 + 缓存清理；
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

    /// 轨道行数据写（2026-11-25）：先调基类（写 TrackData UI 镜像 +
    /// dataChanged 刷新），轨道 mute/solo 再补写 work_（ms::Track 权威
    /// 事实）+ sceneEdited（防抖自动下发——编辑器预览音频 bus 快照与
    /// display 场景同拍追平）。其余 role 直通基类，零行为变化。
    bool setData(const QModelIndex& index, const QVariant& value,
                 int role) override;

    // ---- 场景下发 / 素材同步（2026-11-16 收编为模型对外接口）----------------
    /// 场景下发复合操作（原 MainWindow::publishMsSceneNow / msUiTimer 显示端
    /// 上线追平 / 画布 publishScene 四处两步舞收敛于此）：兜底同步 →
    /// 音频 bus 场景快照（不依赖下发运行时——桥启动失败也刷新，否则打开
    /// 存档后 host 场景为空 → 不混音 → timeline 音频全静音；必须在
    /// syncModelToWorkScene 之后取值）→ runtime 在线检查 → 全量广播。
    /// \return true = 已广播；false = 未挂工作副本或 runtime 未启动
    ///（同步与音频快照仍已完成——调用方据返回值只决定提示语/节流打点）
    bool publishScene();
    /// 素材同步复合操作（原画布 syncMedia 收编）：兜底同步 → runtime 素材
    /// 清单分发 → 统计在线显示端数（口径 connected && enabled，与下发一致）。
    struct SyncMediaResult {
        bool        ok         = false; ///< true = 清单已分发
        std::size_t registered = 0;     ///< 注册素材数
        std::size_t online     = 0;     ///< 在线显示端数
    };
    SyncMediaResult syncMedia();
    /// 注入下发运行时桥 + 音频 bus 宿主（借用指针，不拥有；构造序晚于本
    /// 适配器，宿主 init() 装配点注入。桥未注入/未启动时 publishScene
    /// 返回 false，音频快照照常刷新）
    void setRuntimeBridge(MsRuntimeBridge* rt)       { runtime_ = rt; }
    void setAudioBusHost(TimelineAudioBusHost* host) { audio_bus_ = host; }
    /// 场景撤销历史（本类自带值成员，无需外部注入）。结构操作/拖拽回写/
    /// 属性漏斗各落点以 checkpoint 收编（SceneHistory 注释「核心不变量」
    /// ——新增编辑入口必须补插桩）。undo/redo 入口在画布侧快捷键
    ///（焦点作用域），经 sceneUndo/sceneRedo 落到本类执行。
    SceneHistory* undoHistory() { return &hist_; }
    /// 撤销/重做：取快照并经 loadSceneToModel 全量重灌（唯一恢复通道），
    /// 附带菱形镜像重灌 + 面板广播 + display 防抖追平。
    /// \return true = 已恢复（宿主控件可再做自身收尾：清选中/重绘/预览失效）
    bool sceneUndo();
    bool sceneRedo();

    // ---- 时敏发送调度线程（2026-11-22 治本）--------------------------------
    /// 注入发送调度线程（controller 构造期装配；借用指针不拥有，须先于
    /// 本适配器析构——controller 析构序满足）。
    void setSendScheduler(PlaybackSendScheduler* sched) { send_sched_ = sched; }
    /// 全量重建发送快照并注入调度线程：遍历时间轴片段，Mapping（几何取
    /// 模型实时 start/end + 表取工作副本 clip_data messages）/ Artnet（配置
    /// 取工作副本 clip_data + owner 令牌）→ 纯值快照 installSnapshot。
    /// 调用点 = 装载完成 / notifyClipEdited（编辑漏斗）/ onClipGeometry
    /// Changed（拖拽回写）/ onDeleteClip（移除序列）。GUI 线程 only。
    void rebuildSendSnapshot();

    // ---- 自动追平节流链（2026-11-16 自 MainWindow 收编）---------------------
    /// 配置节流间隔（Settings.ini Network/SceneAutoPublishMs；默认 200，
    /// 配置侧钳制 20-2000）。宿主 init 装配点调用一次；未注入时用默认值。
    void setAutoPublishInterval(int ms) { auto_publish_interval_ms_ = ms; }
    /// 编辑→下发节流决策统一入口（sceneEdited/sceneEditedLive 各路汇入）：
    /// 距上次发布满间隔立即发布，否则挂起单发定时器对齐下一拍（发布聚合：
    /// 到点自然发最新状态）。O(1)，可直连高频信号（如画布 sceneEditedLive）。
    void requestAutoPublish();

    // ---- Tween 关键帧时间轴镜像（2026-11-09 合并关键帧模式）----------------
    // 【2026-11-15 清理】原单键底层入口 applyTweenKeyMove 已删除——注释
    // 声称的"联测探针/单测直调"在本仓库从未存在（收编时随旧工程带入，
    // 零消费者）。合并菱形拖拽走 applyTweenKeyFrameMove，删键走
    // applyTweenKeyRemove（时间轴右键"按参数删除"），两者均经
    // configureClipPanel 注入闭包被模型消费。
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

    // ---- 结构编辑（时间轴右键 → 模型 + 工作副本同步）--------------
    /// 新增轨道：工作副本追加 ms::Track + 模型加行。
    /// 【2026-11-15 去类型化】kind = Unknown 合法（Add Track 单项入口 →
    /// 无类型轨）；kind 仅作显示提示。
    /// \return 新轨道在工作副本中的下标；失败（未挂工作副本）返回 -1
    int  addTrackWithKind(AssetKind kind);
    /// 【2026-11-15 清理】原 addClipOnTrack（素材树按类型添加片段）声明与
    /// 实现已随素材树改造删除——注释残留一并清除（本块原文档所述 API 在
    /// 本仓库从未存在消费者）。片段创建统一走 onAddClip（Add Item 显式
    /// type）/ onDropMediaFile（拖入按素材种类映射）。

    // 时间基已原生 μs（2026-11-10 路线 C）：库模型与 Scene 权威域同基，
    // 原帧↔μs 换算静态函数（framesToUs/usToFrames）已整体退役——显示层
    // 帧数换算收敛在 TimeLineDefines.h 的 framesToUs/usToFrames 内联函数。

signals:
    void seekRequestedUs(std::int64_t us); // 播放头拖动/程序化 seek → cmdSeek
    void sceneEdited();               // 时间轴有本地修改（编辑→节流下发汇入）
    /// 场景撤销/恢复落定（2026-11-26）：sceneUndo/sceneRedo 成功后 emit——
    /// 画布 connect 做收尾（选中高亮清空/预览失效/重绘），执行点与收尾解耦
    ///（入口可来自画布快捷键或宿主）。
    void sceneRestored();
    /// 【2026-11-16 删除 mediaImportFailed 信号】唯一消费者是宿主一条
    /// qWarning（与适配器 emit 点的 stderr 日志重复）——按死信号判据
    /// 删除，失败留痕内化为适配器 qWarning（file+err 一条）。

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
    /// 原生"添加片段"（时间轴右键 "Add Item" 子菜单）→ 基类按显式 type 造
    /// 片段 + 工作副本同步 push（asset_id 置空）。type 来自子菜单选择的片段
    /// 插件类型（2026-11-15 轨道去类型化：片段类型不再由轨道决定）；空 =
    /// 旧语义回退轨道类型串（兼容装载旧场景路径）。用户双击片段在属性面板
    /// 完成绑定；下发场景前由 EditorWindow 做 R1 前置提示（空绑定拦截）。
    void onAddClip(int trackIndex, qint64 startUs,
                   const QString& type = QString()) override;
    /// 拖入媒体文件（资源管理器拖拽 → 视图 dropEvent）：导入素材（复用
    /// 面板导入钩子 hooks_.import_media——扩展名推断/入路径策略/元数据
    /// 探测/轨道类型校验/入表）→ 占位片段创建（onAddClip 同链）→ 素材
    /// 绑定 + 时长联动（素材时长 >0 时片段长度跟随，音频/视频即刻有
    /// 真实长度——2026-11-11 音频片段时长联动）。导入失败回退纯占位
    ///（与未注入钩子时同语义，不阻断拖入）。
    void onDropMediaFile(int trackIndex, qint64 startUs,
                         const QString& file) override;
    /// OSC 地址拖拽落轨（OSCMessageListWidget 卡片 / DefaultTimeLineToolBar
    /// 控件拖出 → 视图 dropEvent，2026-11-15 恢复）：onAddClip("Control")
    /// 创建片段 → 定位链尾新片段把 OSC 五元组写入工作副本
    /// control_messages（{"messages":[…]} 与 OSCMessageListWidget::save()
    /// 同构；host/port/type/value 空缺按旧 applyDroppedMedia 语义兜底）。
    void onDropOscAddress(int trackIndex, qint64 startUs,
                          const OSCMessage& msg) override;
    /// 时间轴删除片段 → 工作副本先删（按 noteName 定位）+ 基类删模型行
    void onDeleteClip(QModelIndex clipIndex) override;
    /// 轨道重排 → 工作副本轨道序同步（维持「模型轨 row = work_ 轨 row」
    /// 不变量——addTrack/deleteTrack/clip 跨轨搬移都按此索引平行定位）
    void onMoveTrack(int sourceRow, int targetRow) override;
    /// 片段跨轨搬移落定（2026-11-15 跨轨拖拽）→ 工作副本把 Clip 从原轨
    /// clips 搬到目标轨（按 start_us 升序插入保持表序；ms id/业务字段不动）。
    /// 几何回写链 locateWorkClip 按全局 id 检索，与轨道归属无关。
    void onClipTrackChanged(AbstractClipModel* clip, int fromTrack,
                            int toTrack) override;

    // ---- 片段分组（2026-11-27 用户定案：REAPER 式轻量关系语义）----------
    // 事实源 = work_->timeline.groups（ms::ClipGroup{id, member_ids}）。
    // toggleGroupOfClips 切换语义：选中片段均无组 → 合建新组；
    // 任一已组 → 全部从所属组摘出（<2 员的组删除）。收编 undoCheckpoint
    // + sceneEdited（保存脏标记）；display 快照不重建（组不影响求值输出）。
    QStringList groupMemberIdsOf(const QString& clipId) const override;
    void toggleGroupOfClips(const QStringList& clipIds) override;
    QModelIndex clipIndexById(const QString& clipId) override;
    QSet<QString> groupedClipIds() const override;
    // ---- 时间轴拖拽真会话（2026-11-26）--------------------------------------
    // 基类会话钩子（view beginClipDrag/finishClipDrag 调用）虚分派到此处
    // 接管撤销历史：begin 冻结 SceneHistory 会话基线（此刻几何未变，一次
    // 深拷贝）+ 置位抑制（拖拽中每拍 onClipGeometryChanged → undoCheckpoint
    // 静默）；end 先撤抑制再一条历史收口（SceneHistory 内部 before==after
    // 去重——点选未拖 = 0 条历史）。label 区分移动/裁边（view 传参）。
    void beginInteractiveGeometryChange(
        AbstractClipModel* clip,
        const QString& undoLabel = QStringLiteral("移动片段")) override;
    void endInteractiveGeometryChange() override;

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
    /// 全部打开中面板广播（undo/redo 恢复后的灌值收口——快照重灌后对象
    /// 值整体变化，逐 clip 调 refreshClipPanels 按 panel_refs_ 现存键展开；
    /// 键快照先行，防广播惰性清理与迭代互踩）。
    void refreshAllOpenPanels();
    // 【2026-11-15 清理】refreshAllClipPanels（全量广播变体）已删除——收编
    // 后零消费者（refreshClipPanels 按 clip 精确广播已覆盖全部触发源）。
    /// 【Tween 专用·播放头驱动刷新】播放头移动（用户拖动/Player 回显）时
    /// 刷新已打开 Tween 区的"播放头处数值显示"（数值框恒显示插值结果——
    /// WATCHOUT 取景语义）。只灌 tween 区显示，不触发通用面板/写回链路
    /// （内容未变，仅观测点移动）。注册表空时零开销（播放期高频调用安全）。
    void refreshTweenPanelsPlayhead();


    // ---- 时间轴选中 → 画布高亮（2026-11-15 画布选中分工配套）---------------
    // 【分工定案】画布只选 screen（"只显当前"舞台快照非素材清单），
    // clip 选中唯一入口 = 时间轴；画布对时间轴已选中的片段只承担空间
    // 编辑（"时间轴选谁，画布摆哪"）。本映射是直连链的反向半边：
    /// 时间轴片段模型索引 → 工作副本 (track, clip) 下标（currentChanged
    /// → 画布 highlight 用）。轨道行走「模型轨 row = work 轨 row」不变量；
    /// 片段按 clipAnchor（= ms clip id，2026-11-15 与备注名分离）在工作
    /// 副本同轨检索——对增删/跨轨搬移稳定，不信任两边下标一致。
    /// 非片段行（轨道行/无效索引/装载清空）= false。
    bool workClipIndices(const QModelIndex& clip_index,
                         int* track_idx, int* clip_idx) const;

private:
    /// clip 信号（timelinePositionChanged/lengthChanged）→ 工作副本回写
    void onClipGeometryChanged(AbstractClipModel* clip);
    /// 撤销收编（离散一步式）：loading_ 装载中 / 拖入包裹抑制中 / 未注入
    /// 历史时静默跳过；否则以 label 把当前工作副本收编进历史（写后调用，
    /// before 取历史基线，见 SceneHistory::checkpoint）。
    void undoCheckpoint(const char* label);
    /// 按片段显示名（noteName = ms clip id）在工作副本中定位；未找到返回 nullptr
    Clip* locateWorkClip(const QString& note);
    /// ms::Track.kind → QtTimeLine 轨道类型串（2026-11-15 去类型化后仅作
    /// 显示提示/旧场景兼容，不再承担类型约束）
    static QString typeForKind(AssetKind kind);
    /// QtTimeLine 轨道类型串 → ms::AssetKind（typeForKind 逆映射）
    static AssetKind kindForType(const QString& type);
    /// AssetKind 的短名（轨道/片段 id 前缀）：video/image/text/audio/clip
    static const char* kindShortName(AssetKind kind);

    // ---- 片段属性面板（QtTimeLine 原生附加体系）------------------------------
    /// 唯一片段 id 生成（2026-11-06 收敛原两份同构探测循环）：clip-N 线性
    /// 探测，与工作副本全表查重。onAddClip / onDropMediaFile 创建路径共用
    ///（单点维护）。
    std::string nextClipId() const;
    /// 唯一素材 id 生成（2026-11-18 收敛原三份同构探测循环——loadSceneToModel
    /// Live 自愈 / onAddClip Text / onAddClip Live）：prefix-N 线性探测，
    /// 与工作副本素材全表查重。
    std::string uniqueAssetId(const char* prefix) const;
    /// 为 SceneClipModel 注入片段自定义面板工厂 + 诊断留痕。
    /// 【片段创建路径必须统一调用】loadSceneToModel（装载场景）/
    /// onAddClip（时间轴右键 Add Item / onDropMediaFile 拖入同链）——
    /// 漏注入的路径其片段双击时 clipPropertyWidget() 返回 null，
    /// 库面板附加点会报 "Cannot add a null widget to QVBoxLayout"。
    /// type = 片段插件类型（2026-11-15 去类型化：跟片段走，不再取轨道）。
    void configureClipPanel(SceneClipModel* sc, const std::string& clip_id,
                            const QString& type);
    /// 组装片段自定义面板（SceneClipModel::clipPropertyWidget 的工厂体）：
    /// 打开时实时取工作副本快照 + 按片段类型定制编辑区 + 接写回/导入钩子。
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
    /// 编辑落定统一编排（P1-A2 收编 2026-11-17）：模型镜像回推（按需）+
    /// sceneEdited + 面板广播——三条写回路径（通用面板合并
    /// applyClipPanelMerge / Tween 区 applyClipTweenEdit / 插件专属区
    /// ctx.on_edited / 时间轴键编辑 applyTweenKey*）统一收口。
    /// 【不对称修复】原 applyClipPanelMerge 尾只 emit sceneEdited 不广播
    ///（靠 EditorWindow refreshSceneViews 间接链兜底），其余路径双调——
    /// 收口后广播成为机制而非约定。
    /// \param mirror_payload true = 工作副本 clip_data → 模型镜像回推
    ///（setClipPayload，插件专属区编辑路径；幂等 diff 在模型侧）
    void notifyClipEdited(const std::string& clip_id,
                          bool mirror_payload = false);

    // ---- 状态 ---------------------------------------------------------------
    Scene* work_   = nullptr;  // 编辑工作副本（不拥有）
    /// 下发运行时桥 + 音频 bus 宿主（均不拥有；publishScene/syncMedia 消费）
    MsRuntimeBridge*      runtime_   = nullptr;
    TimelineAudioBusHost* audio_bus_ = nullptr;
    /// 时敏发送调度线程（不拥有；controller 装配注入——2026-11-22 治本）
    PlaybackSendScheduler* send_sched_ = nullptr;
    bool   loading_ = false;   // 装载/程序化写模型中：屏蔽 clip 信号的回写
    /// 撤销历史（值成员；收编点直用，undo/redo 由 sceneUndo/sceneRedo 执行）
    SceneHistory hist_;
    /// 撤销恢复进行中（sceneUndo/sceneRedo 重入 loadSceneToModel 标记——
    /// 区分"恢复重灌"与"工程装载"：后者才重置 undo 基线 setBase）
    bool undoRestoring_ = false;
    /// 撤销收编抑制计数（复合操作包裹内层 checkpoint——onDropMediaFile/
    /// onDropOscAddress 内部调 onAddClip，终态只在最外层收编一条）
    int undo_suppress_ = 0;
    /// 拖拽撤销会话已布防（2026-11-26 拖拽真会话）：beginInteractiveGeometry
    /// Change 成功接管时置位，endInteractiveGeometryChange 对称撤除——
    /// begin 条件不满足（装载中/历史未激活）时 end 不误撤抑制计数。
    bool geo_session_armed_ = false;
    /// 时间轴真正改过的片段（noteName=ms clip id）。拖拽/面板改帧 → 记入；
    /// 面板→模型推送 → 移出（面板值对本 clip 保持权威）；sync 后清空。
    std::unordered_set<std::string> dirty_clips_;
    /// 【2026-11-15 性能】locateWorkClip 的上次命中下标（O(1) 快路径锚）：
    /// 只存下标不存指针（vector 搬移安全）——下标处 id 失配即回退全扫并
    /// 更新，语义与全扫一致。拖拽会话高频回写的线性扫描消除。
    int m_lastLocateTrack = -1;
    int m_lastLocateClip  = -1;
    /// 属性面板区注册表（P1 收编 2026-11-17：原 clip_panels_ / tween_panels_
    /// / clip_sections_ 三张同构表合一——三者生命周期语义完全相同（QPointer
    /// 失效感知 + 广播时惰性清理），一张表 + Kind 标记分派即可。登记点 =
    /// makeClipPropertyPanel（timeline 组合面板自定义区与画布双击对话框
    /// 两条入口同源经过）。宿主仍不持有专属区块具体类型——广播分派经
    /// qobject_cast（Generic/Tween 直接还原，Section 经 ClipSectionInterface）。
    struct PanelRef {
        enum class Kind { Generic, Tween, Section };
        QPointer<QWidget> w;
        Kind              kind;
    };
    std::unordered_map<std::string, std::vector<PanelRef>> panel_refs_;
    /// 片段属性面板宿主钩子（EditorWindow 注入；空 = 无自定义面板）
    ClipPanelHooks hooks_;

    // seek 节流：拖动中 80ms 一发，松手后 flush 补发最终值
    QTimer*    seek_flush_      = nullptr;
    std::int64_t pending_seek_us_ = -1;  // -1 = 无待发

    // ---- 自动追平节流三件（2026-11-16 自 MainWindow msAutoPublish* 收编）----
    /// 立即发布（publishScene 复合操作 + 节流打点 + 日志留痕；立即路径与
    /// 定时器到点路径共用）
    void publishNow();
    QTimer* auto_publish_timer_ = nullptr; // 单发对齐定时器（挂起时只重算时长）
    int  auto_publish_interval_ms_ = 200;  // 节流间隔（宿主经 setAutoPublishInterval 注入）
    QElapsedTimer last_publish_since_;     // 上次发布时刻（节流基准；isValid=发过）

    // ---- Control 触发引擎（2026-11-13）--------------------------------------
    /// 播放头上升沿检测（Control 片段语义，2026-11-13 用户定案「时间到
    /// 那个地方了发送指令」）：prev < start && now >= start 的跨入瞬间，
    /// 对该 Control 片段 control_messages 逐条 StatusContainer::parseOSC
    ///（经 ms::fireControlMessages，见 BaseClass/ControlMessages.hpp）。
    /// 挂在播放头两落点（onSetPlayheadPos 手动 seek / setPlayheadFromRuntimeUs
    /// 播放回显）——同一检测保证 scrub 与播放语义一致。同区间内持续推进
    /// 不重触发（prev 已 >= start）；后退跨出后再次进入重新武装。
    /// 【2026-11-15 扩展→2026-11-27 退役】本链曾同调 onPlayheadAdvanced 虚钩子
    ///（Artnet/Mapping 类型专属播放挂点）；播放驱动已移交 PlaybackSendScheduler
    /// 独立线程（快照直读工作副本），虚钩子已从 BaseClass 删除，本链只保
    /// Control 触发检测语义。
    void updateControlTriggers(std::int64_t now_us);
    /// 上一拍播放头（μs；-1 = 未初始化——首拍只建立基线不触发）
    std::int64_t prev_playhead_us_ = -1;

    // ---- 片段业务数据镜像（2026-11-15 片段移植）------------------------------
    /// 按锚点（ms clip id）定位时间轴片段模型（全轨线性扫；面板编辑落点
    /// 低频）。用于把工作副本 clip_data 推给模型（setClipPayload）。
    AbstractClipModel* findClipModelByAnchor(const std::string& clip_id);
};

} // namespace ms
