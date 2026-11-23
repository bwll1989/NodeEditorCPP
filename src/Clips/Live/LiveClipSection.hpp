// ============================================================================
//  LiveClipSection —— 实时源片段专属编辑区（片段属性面板的类型扩展区）
//
//  定位（2026-11-06 实时源 Live Hub 最小闭环；2026-11-13 随片段插件化
//  迁入本目录并磁盘化——live.plugin，无内置兜底）：
//    * 【2026-11-13 片段插件化】本区块随 Live 片段插件走——宿主经
//      ClipPlugInterface::createPropertySection 工厂加载（ctx=
//      ms::ClipSectionContext*：工作副本片段/场景/编辑落定回调），广播
//      灌值经 ClipSectionInterface::refreshFromClip（qobject_cast 探测）。
//      原"适配器 applyClipLiveEdit 写回"职责内化为本类 onEdited 直写；
//    * 挂接方式 = ClipPropertyPanel::appendSection（面板"只追加不重构"
//      扩展约定——不揉进既有通用区控件，遵守片段属性面板冻结铁律；
//      TextClipSection 同款先例）；
//    * 编辑域 = 绑定素材的 Asset.live_*（接入方式/源标识/延迟），与通用
//      面板的 Clip 域（时间/变换）正交——素材按 ctx.clip->asset_id 现查
//      （换绑安全）；写回后经 ctx.on_edited 落定（宿主 sceneEdited
//      自动下发 + 面板广播灌值）；
//    * 接入方式下拉数据源 = Ingest 注册表（live/LiveIngest.hpp）——未实现
//      协议置灰展示（协议扩展的 UI 可见性）；源下拉数据源 = 当前接入方式
//      的 list_sources（「刷新」按钮触发；同步阻塞 ≤300ms——点击型操作
//      可接受）。过滤自家再发布前缀 ms-live（Hub 转发流不应作为采集源
//      ——防自环；core 注册表 listNdi 已滤，此处第二道防线）。可编辑：
//      发现列表没有的源标识可直接键入（跨网段手动指定）。
//  测试定位锚：objectName 前缀 live*（liveIngest/liveSource/
//  liveSyncLatency/liveRefresh/liveMcastUrl/liveEnable）。
//
//  【2026-11-23 启用闸（用户定案，替代同日早先纯 UI 锁方案）】
//    「启用/停止」按钮（liveEnable）= 捕获与下发的唯一运行开关：
//    * 启用 → asset.live_enabled = true 随场景 pb 分发：editor Hub 开始
//      采集（Ingest）+ 组播下发（MpegTsSender），display 端收流显示；
//      同时锁定接入方式/源/刷新/帧同步四控件 + onEdited/onRefreshSources
//      写回静默忽略（运行中切源 = 调度器指纹失效 → 连接中的源实例析构
//      重建，接收/渲染线程竞态偶发崩溃——护栏与闸合一）；
//    * 停止 → live_enabled = false：Hub 拆净收端+发送器、MediaSource 不
//      装配收流（prepare 恒成功无收端 → 占位），四控件解锁可自由切源。
//    状态存素材（Asset.live_enabled，SceneModel/proto LivePayload.enabled）：
//    跨面板重建/重启/.show 持久化/display 下发全程同源——非编辑器本地态。
//    全链路生效点：LiveHub::ensureAsset（采集+转发）、LiveMediaSource::
//    prepare（收流装配）、MediaScheduler::liveFingerprint（启停即重建）。
// ============================================================================

#pragma once

#include <QWidget>

#include "AbstractClipInterface.h"      // ClipSectionInterface（宿主广播接口）
#include "model/ClipSectionContext.hpp" // ms::ClipSectionContext（编辑上下文）
#include "model/SceneModel.hpp"

class QComboBox;
class QLabel;
class QPushButton;

namespace ms {

struct IngestDesc; // 注册表条目（live/LiveIngest.hpp）——仅返回指针用前置声明

/// 实时源片段专属编辑区：素材 live_* 域即时编辑（写回内化 + on_edited
/// 落定）。生命周期：作为 ClipPropertyPanel::appendSection 的子控件由
/// Qt 父子关系管理（随面板销毁）；ctx 内指针由宿主保证覆盖本控件存活期。
class LiveClipSection : public QWidget, public ClipSectionInterface {
    Q_OBJECT
    Q_INTERFACES(ClipSectionInterface)
public:
    /// \param ctx    编辑上下文（工作副本片段/场景/编辑落定回调；
    ///               初值素材按 ctx.clip->asset_id 现查——工厂已校验
    ///               kind==Live，查不到用默认空素材兜底）
    /// \param parent Qt 父控件（= 承载面板 ClipPropertyPanel）
    explicit LiveClipSection(const ClipSectionContext& ctx,
                             QWidget* parent = nullptr);

    /// 【ClipSectionInterface】宿主广播灌值（模型→面板向）：\p clip 为
    /// const ms::Clip*（经 void* 穿越基类边界）——按其当前绑定现查素材
    /// （换绑安全）→ refreshFrom。QSignalBlocker 防回环；源下拉有焦点
    ///（正在挑源/键入）时跳过。
    void refreshFromClip(const ms::Clip* clip) override; // P3-B 类型化签名

private slots:
    /// 「刷新」：当前接入方式注册表 list_sources 重灌源下拉（保留当前选择）
    void onRefreshSources();

    /// 「启用/停止」切换（运行闸）：启用 → asset.live_enabled=true 落定
    /// （Hub 起采集+下发）并锁定编辑控件；停止 → false（拆收端占位）解锁。
    /// 无绑定素材时状态弹回。经 applyTo 直写 + on_edited 落定（不走
    /// onEdited——其含"启用中禁写"守卫）。
    void onEnableToggled(bool on);

private:
    /// 当前绑定素材是否启用中（读 asset.live_enabled——状态唯一事实源）
    bool currentEnabled() const;

    /// 使能状态 → 按钮两态文案/样式 + 四个编辑控件的可用性（幂等；
    /// 构造/广播灌值/切换三处落点——QSignalBlocker 防回环）
    void syncEnableUi();

    /// 控件当前值 → 素材 live_* 全字段（无条件覆盖：Live 域只有本区块
    /// 一个写者）
    void applyTo(Asset& a) const;

    /// 【双向同步·模型→面板向】素材当前值 → 控件（refreshFromClip 落点）
    void refreshFrom(const Asset& a);

    /// 工作副本绑定素材定位（const/非 const 双形态；按 asset_id 现查，
    /// 未绑定/类型不符返回 nullptr——写回与灌值统一防御）
    const Asset* boundAsset() const;
    Asset* writableAsset();

    /// 【2026-11-15 悬垂修复】工作副本片段现查（与 TextClipSection 同；
    /// clip 存于 vector<Clip>，工厂时刻指针会因片段增删失效）
    Clip* boundClip();
    const Clip* boundClip() const;

    /// 任一控件变更 → 直写素材 live_* 全字段 + ctx.on_edited 落定回调
    ///（原适配器 applyClipLiveEdit 职责内化，2026-11-13 片段插件化）
    void onEdited();

    /// 接入方式下拉当前选中的注册表条目（未实现/未知返回 nullptr）
    const IngestDesc* currentDesc() const;

    // ---- 控件（objectName = 测试定位锚）---------------------------------------
    QPushButton* enable_  = nullptr; ///< liveEnable：启用/停止（运行闸 +
                                     ///< 锁定切换；两态按钮）
    QComboBox*   ingest_  = nullptr; ///< liveIngest：接入方式（注册表驱动）
    QComboBox*   source_ = nullptr;  ///< liveSource：源标识（可编辑）
    QPushButton* refresh_ = nullptr; ///< liveRefresh：刷新发现列表
    QComboBox*   sync_latency_ = nullptr; ///< liveSyncLatency：帧同步目标延迟
                                         ///< （档位 currentData 存 μs；恒 >0
                                         ///< ——监视档 2026-11-23 退役，
                                         ///< 「必须同步」用户定案）
    /// liveMcastUrl：组播地址展示（组播恒启用 2026-11-23——勾选框退役，
    /// live_url 由 applyTo 恒生成，此处仅信息性展示）
    QLabel*      mcast_url_ = nullptr;

    ClipSectionContext ctx_; ///< 编辑上下文（clip/scene/on_edited——值拷贝）
};

} // namespace ms
