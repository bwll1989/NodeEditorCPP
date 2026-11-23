#include "TimelinePlaybackController.hpp"

#include <cstdint>

#include "MsRuntimeBridge.hpp"                            // ms::MsRuntimeBridge（同目录）
#include "QtTimelineModelAdapter.hpp"                     // ms::QtTimelineModelAdapter
#include "SendScheduler.hpp"                              // ms::PlaybackSendScheduler（发送调度线程）
#include "TimelineAudioBusHost.hpp"                       // ms::TimelineAudioBusHost
#include "../PreviewCanvasWidget/PreviewRenderer.hpp"     // ms::PreviewRenderer
#include "../../Common/TimeLineBase/BaseTimeLineWidget.h" // BaseTimelineWidget + DefaultTimeLineToolBar
#include "../../Common/Devices/StatusContainer/StatusContainer.h" // 状态镜像单例
#include "../../Common/Devices/StatusContainer/GlobalEventBus.hpp" // GlobalEvent/GlobalEventBus（全局命名空间）
#include "../../Common/BaseClass/TimeLineDefines.h"       // QtTimeline::usToFrames/framesToUs（PAL 25fps）

namespace ms {

// ============================================================================
// OSC 地址树：前缀与旧 TimeLineWidget 面板级 alias "timeline/master" 一致
//（旧面板已退役，地址空间复用，外部控制脚本零迁移）。
// ============================================================================
QString TimelinePlaybackController::address(const QString& rel) {
    const QString norm = rel.startsWith('/') ? rel : ('/' + rel);
    return QStringLiteral("/timeline/master") + norm;
}

TimelinePlaybackController::TimelinePlaybackController(const Deps& deps, QObject* parent)
    : QObject(parent),
      runtime_(deps.runtime),
      preview_(deps.preview),
      audioBus_(deps.audioBus),
      adapter_(deps.adapter),
      timeline_(deps.timeline)
{
    // ---- 33ms tick 定时器：仅播放期运行（play 启动 / pause·stop 停止，
    // 非播放期零开销）；tick 推进播放头后：回显时间轴（setPlayheadFromRuntimeUs
    // 静态调基类，绕开 onSetPlayheadPos 虚派发不发 seek——防回环）。画布
    // 不出场（pull 泵自动跟进）。
    tick_ = new QTimer(this);
    tick_->setTimerType(Qt::PreciseTimer);
    connect(tick_, &QTimer::timeout, this, [this] {
        // 播放权威时钟：桥会话播放中 → 以对齐时钟为唯一权威（预览 seekTo
        // 跟随，下发画面与本地预览同拍）；否则退回本地 PreviewRenderer
        // 状态机自驱 tick（未配显示端时的主路径）。
        std::int64_t us = -1;
        if (runtime_ && runtime_->running() && runtime_->playing()) {
            us = runtime_->sessionTimeUs();
            if (us < 0) return;
            if (preview_) preview_->seekTo(us); // 预览跟随会话播放头
        } else {
            if (!preview_ || !preview_->playing()) return;
            if (!preview_->tick()) return; // 非播放态/播放头未推进
            us = preview_->playheadUs();
        }
        // ---- 到头停止 / 循环（2026-11-16，对齐节点时间轴时钟语义）----
        // 播放头越过时间轴总长（TimelineLengthRole = m_lengthFrame = 最后
        // 片段末端，μs）时：loop 关 → 全链停止（handleStop：会话停+预览归
        // 零+tick 停+播放头回零，其内部 tick_->stop() 防重入）；loop 开 →
        // 全链回零续播（会话 seek + 音频 bus 重锚 + 预览回零，本拍按 0 回
        // 显，下一 tick 从头推进）。总长 ≤0（空时间轴）不设限保持旧行为。
        if (adapter_) {
            const qint64 totalUs = adapter_->data(
                QModelIndex(), TimelineRoles::TimelineLengthRole).toLongLong();
            if (totalUs > 0 && us > totalUs) {
                if (!looping_) {
                    handleStop();
                    return;
                }
                if (runtime_ && runtime_->running())
                    runtime_->cmdSeekToUs(0); // 会话播放头回零（display 跟随）
                if (audioBus_) audioBus_->commandSeek(0); // 音频重锚
                if (preview_) preview_->seekTo(0);        // 预览回零续播
                us = 0; // 本拍按回零后位置回显
            }
        }
        // 【2026-11-18 修】补守卫——同函数前后均判空 adapter_，此处裸解
        // 引用是唯一缺口（Deps.adapter 允许为空时每 33ms 必崩）
        if (adapter_) {
            adapter_->setPlayheadFromRuntimeUs(us);
            // 播放头帧号反馈（PAL 25fps 四舍五入；经 StatusContainer 镜像后
            // statusUpdated 出站到 OSC/MQTT/WS 反馈端口）
            GlobalEventBus::instance()->publishState(
                address(QStringLiteral("/currentFrame")),
                static_cast<int>(QtTimeline::usToFrames(us)));
        }
    });
    // ---- 工具栏播放三动作 → 统一 handle 方法（与外部 OSC 命令同源）----
    if (timeline_ && timeline_->toolbar) {
        connect(timeline_->toolbar, &DefaultTimeLineToolBar::playClicked, this,
                [this] { handlePlay(); });
        connect(timeline_->toolbar, &DefaultTimeLineToolBar::pauseClicked, this,
                [this] { handlePause(); });
        connect(timeline_->toolbar, &DefaultTimeLineToolBar::stopClicked, this,
                [this] { handleStop(); });
        // 【循环 2026-11-16】工具栏 loop 开关（点击翻转）→ 状态 + 反馈
        connect(timeline_->toolbar, &DefaultTimeLineToolBar::loopToggled, this,
                [this](bool on) { handleLoop(on); });
    }
    // ---- seek 全链联动 ------------------------------------------------------
    // 视图拖播放头（适配器 80ms 节流后）/ OSC /currentFrame 命令跳转 →
    // 播放头位置变化三路同步：会话内跳转广播（Idle 时仅记录定位点）+
    // 音频 bus 重锚 + 预览 seekTo；画布内部 pull 泵检测播放头变化自动
    // 提交快照/重绘，此处无须手动刷新画布。
    if (adapter_) {
        connect(adapter_, &QtTimelineModelAdapter::seekRequestedUs, this,
                [this](std::int64_t us) {
                    if (runtime_ && runtime_->running())
                        runtime_->cmdSeekToUs(us);
                    // timeline 音频 bus：播放中重锚（flush+重定位；暂停态忽略）
                    if (audioBus_) audioBus_->commandSeek(us);
                    if (preview_) preview_->seekTo(us);
                    // 发送调度线程 seek poke（2026-11-22 治本）：暂停态对该
                    // 时刻即时求值一帧（旧 updateControlTriggers 同语义），
                    // 播放态作下一拍提示
                    if (scheduler_) scheduler_->onSeek(us);
                });
    }
    // OSC 地址树绑定：/timeline/master/{play,pause,stop,
    // currentFrame} 订阅命令 + 播放控件注册 + 初始状态推送（详见 bindOsc）。
    bindOsc();

    // ---- 时敏指令发送调度线程（2026-11-22 治本）-----------------------------
    // Mapping/Artnet 逐帧发送独立线程，直读主时钟（不再受 GUI tick 抖动
    // 影响）。时钟源双域（与 tick lambda 同构）：会话播放优先 sessionTimeUs
    //（对齐时钟纯计算），否则预览状态机 playheadUs（2026-11-22 纯函数化
    // 后线程安全）；都不在播返回 -1 = 调度线程空闲休眠（播放状态机仍在
    // 本控制器——调度线程无 play/pause/stop 命令协议，时钟有无读数即播放
    // 态）。快照由适配器构建注入（setSendScheduler + rebuildSendSnapshot）。
    if (adapter_) {
        scheduler_ = std::make_unique<PlaybackSendScheduler>(
            [this]() -> std::int64_t {
                if (runtime_ && runtime_->running() && runtime_->playing())
                    return runtime_->sessionTimeUs();
                if (preview_ && preview_->playing())
                    return preview_->playheadUs();
                return -1;
            });
        adapter_->setSendScheduler(scheduler_.get());
        adapter_->rebuildSendSnapshot(); // 初始（空/装载后）快照
    }
    // ---- 电平表数据泵转发 ---------------------------------------------------
    // 电平表已内置 DefaultTimeLineToolBar 右缘（库内自挂），33ms 采样拍亦
    // 内置 TimelineAudioBusHost（setLevelPumpSink 注入即启，采样节奏归音频
    // 宿主管）——控制器只注入转发 sink；非播放拍 host 内部已归零，无需判
    // 播放态。sink 存于 host 对象内部（host 先于本控制器消亡，无悬垂窗口）。
    if (audioBus_) {
        audioBus_->setLevelPumpSink([this](float v) {
            if (timeline_ && timeline_->toolbar)
                timeline_->toolbar->setAudioLevel(v);
        });
    }
}

TimelinePlaybackController::~TimelinePlaybackController() = default;

// ---------------------------------------------------------------------------
// 程序化播放控制（2026-11-23 headless 集成）：宿主自动起播/外部控制的
// 正式入口，与工具栏点击、OSC /timeline/master/* 命令同源（handle* 直通，
// 状态反馈统一回推）。headless 装配（preview_/audioBus_/timeline_ 全空）
// 下会话广播 + tick + 到头循环检查全链可用。
// ---------------------------------------------------------------------------
void TimelinePlaybackController::play()  { handlePlay(); }
void TimelinePlaybackController::pause() { handlePause(); }
void TimelinePlaybackController::stop()  { handleStop(); }

void TimelinePlaybackController::setLoop(bool on) {
    handleLoop(on); // 模式翻转 + /loop 反馈（与 /loop 命令同源）
}

/**
 * @brief 播放（工具栏点击与外部 /play 命令同源）
 * 函数级注释：会话广播锚点播放 + 本地预览状态机启动 + 33ms tick 启动，
 *            末尾推送播放态反馈（/play /stop）。
 */
void TimelinePlaybackController::handlePlay() {
    if (runtime_ && runtime_->running())
        runtime_->cmdPlay(); // 广播锚点播放（0 端在线 = no-op）
    if (timeline_ && timeline_->toolbar)
        timeline_->toolbar->setPlaybackState(true); // 按钮态同步（OSC 命令不经动作触发）
    if (preview_) {
        preview_->play(); // Idle→从头 / Paused→续播（状态机幂等）
        // timeline 音频 bus 起播锚定：位置取双权威当前值——会话播放中
        // 用锚定会话时间（cmdPlay 锚点值），否则预览播放头
        if (audioBus_) {
            std::int64_t t0 = -1;
            if (runtime_ && runtime_->running())
                t0 = runtime_->sessionTimeUs();
            if (t0 < 0 && preview_) t0 = preview_->playheadUs();
            audioBus_->commandPlay(t0 < 0 ? 0 : t0);
        }
    } else if (audioBus_) {
        // headless（无预览）：音频锚定退会话权威，无会话读数从 0 起
        const std::int64_t t0 = runtime_ ? runtime_->sessionTimeUs() : -1;
        audioBus_->commandPlay(t0 < 0 ? 0 : t0);
    }
    if (tick_) tick_->start(33); // tick 节奏对齐画布 pull 泵（headless：到头检查+控制触发）
    publishPlaybackState();
}

/**
 * @brief 循环开关（工具栏点击与外部 /loop 命令同源）
 * 函数级注释：记录模式 + 推送 /loop 反馈（工具栏勾选态由调用方/命令链
 *           setLoopState 回显，此处不反向驱动 UI——避免回环）。
 */
void TimelinePlaybackController::handleLoop(bool on) {
    looping_ = on;
    publishLoopState();
}

/**
 * @brief 暂停（工具栏点击与外部 /pause 命令同源）
 * 函数级注释：会话冻结于锚点 + 预览暂停 + tick 停止，末尾推送播放态反馈。
 */
void TimelinePlaybackController::handlePause() {
    if (runtime_ && runtime_->running()) runtime_->cmdPause();
    if (timeline_ && timeline_->toolbar)
        timeline_->toolbar->setPlaybackState(false);
    if (audioBus_) audioBus_->commandPause(); // 停写（下游自然静音）
    if (preview_) preview_->pause();
    if (tick_) tick_->stop();
    publishPlaybackState();
}

/**
 * @brief 停止（工具栏点击与外部 /stop 命令同源）
 * 函数级注释：会话回 Idle + 预览归零 + tick 停止 + 播放头回显起点，
 *            末尾推送播放态反馈。
 */
void TimelinePlaybackController::handleStop() {
    if (runtime_ && runtime_->running()) runtime_->cmdStop();
    if (timeline_ && timeline_->toolbar)
        timeline_->toolbar->setPlaybackState(false);
    if (audioBus_) audioBus_->commandStop();
    if (preview_) preview_->stop(); // 归零回 Idle
    if (tick_) tick_->stop();
    // 播放头回显起点（画布 pull 泵检测变化自动重绘首帧；headless 仅状态归零）
    if (adapter_) adapter_->setPlayheadFromRuntimeUs(0);
    publishPlaybackState();
}

/**
 * @brief 播放态反馈推送（/play /stop /currentFrame）
 * 函数级注释：publishState → StatusContainer 镜像（statusUpdated 出站）。
 *            播放态以本地预览状态机为准（与 handle 双源同步翻转）；
 *            非播放态补发 /currentFrame=0 归零帧（停止/暂停后反馈收敛）。
 */
void TimelinePlaybackController::publishPlaybackState() {
    // 播放态权威：有预览 = 本地预览状态机（GUI，与 handle 双源同步翻转）；
    // headless（无预览）= 下发会话播放态（runtime 未运行恒 false）
    const bool playing = preview_ ? preview_->playing()
                                  : (runtime_ && runtime_->playing());
    auto* bus = GlobalEventBus::instance();
    bus->publishState(address(QStringLiteral("/play")), playing);
    bus->publishState(address(QStringLiteral("/stop")), !playing);
    publishLoopState();
    if (!playing) {
        bus->publishState(address(QStringLiteral("/currentFrame")), 0);
    }
}

/**
 * @brief 循环态反馈推送（/loop → StatusContainer 出站镜像）
 * 函数级注释：与播放态反馈同链（构造初始推送 + handleLoop 翻转时推送）。
 */
void TimelinePlaybackController::publishLoopState() {
    GlobalEventBus::instance()->publishState(
        address(QStringLiteral("/loop")), looping_);
}

/**
 * @brief OSC 绑定入口（构造期一次）
 * 函数级注释：订阅四个命令地址（单槽分发）+ 播放/停止控件注册到
 *            StatusContainer（地址→控件反查，对齐旧 registerOSCControl
 *            登记语义）+ 推送初始状态（旧 publishCurrentState 语义）。
 */
void TimelinePlaybackController::bindOsc() {
    auto* bus = GlobalEventBus::instance();
    bus->subscribe(address(QStringLiteral("/play")), this,
                   SLOT(onTimelineCommand(GlobalEvent)));
    bus->subscribe(address(QStringLiteral("/pause")), this,
                   SLOT(onTimelineCommand(GlobalEvent)));
    bus->subscribe(address(QStringLiteral("/stop")), this,
                   SLOT(onTimelineCommand(GlobalEvent)));
    bus->subscribe(address(QStringLiteral("/currentFrame")), this,
                   SLOT(onTimelineCommand(GlobalEvent)));
    // 【循环 2026-11-16】/loop 订阅（payload bool；true=循环回零续播）
    bus->subscribe(address(QStringLiteral("/loop")), this,
                   SLOT(onTimelineCommand(GlobalEvent)));
    // 【OSC 地址拖拽】启用工具栏按钮 Ctrl+拖出地址（/play /stop）——
    // 地址入剪贴板 + x-osc-address mime 可落 OSCMessageListWidget 等既有
    // 拖放目标。base 与 address() helper 同源（/timeline/master）。
    if (timeline_ && timeline_->toolbar) {
        timeline_->toolbar->setOscBase(QStringLiteral("/timeline/master"));
    }
    // 控件注册（OSC 出站/入站不依赖控件指针；注册供地址树/反查 UI 使用）
    if (timeline_ && timeline_->toolbar) {
        if (auto* w = timeline_->toolbar->widgetForAction(
                timeline_->toolbar->playAction())) {
            StatusContainer::instance()->registerWidget(
                w, address(QStringLiteral("/play")));
        }
        if (auto* w = timeline_->toolbar->widgetForAction(
                timeline_->toolbar->stopAction())) {
            StatusContainer::instance()->registerWidget(
                w, address(QStringLiteral("/stop")));
        }
        // 【循环 2026-11-16】loop 按钮控件注册（地址树/反查 UI 同口径）
        if (auto* w = timeline_->toolbar->widgetForAction(
                timeline_->toolbar->loopAction())) {
            StatusContainer::instance()->registerWidget(
                w, address(QStringLiteral("/loop")));
        }
    }
    // 初始状态推送（/play=false /stop=true /currentFrame=0）
    publishPlaybackState();
}

/**
 * @brief GlobalEventBus 命令分发入口（/timeline/master/* 订阅槽）
 * @param ev 全局事件（Command 类）
 * 函数级注释：单槽按地址分发；/play payload 缺省或 true = 播放，
 *            false = 停止；/currentFrame payload 为帧号（PAL 25fps），
 *            换算 μs 后走 onSetPlayheadPos（80ms seek 节流内建，拖动
 *            风暴安全）。命令处理后的状态回执由 handle* 内的
 *            publishPlaybackState 统一承担。
 */
void TimelinePlaybackController::onTimelineCommand(const GlobalEvent& ev) {
    if (ev.kind != GlobalEventKind::Command) return;

    if (ev.address == address(QStringLiteral("/play"))) {
        if (!ev.payload.isValid() || ev.payload.toBool()) {
            handlePlay();
        } else {
            handleStop();
        }
        return;
    }
    if (ev.address == address(QStringLiteral("/pause"))) {
        handlePause();
        return;
    }
    if (ev.address == address(QStringLiteral("/stop"))) {
        handleStop();
        return;
    }
    // 【循环 2026-11-16】/loop 命令（payload bool）→ 工具栏勾选态回显
    //（setChecked 不触发 triggered，无回环）+ 状态与 /loop 反馈
    if (ev.address == address(QStringLiteral("/loop"))) {
        const bool on = ev.payload.isValid() ? ev.payload.toBool() : false;
        if (timeline_ && timeline_->toolbar)
            timeline_->toolbar->setLoopState(on);
        handleLoop(on);
        return;
    }
    if (ev.address == address(QStringLiteral("/currentFrame"))) {
        if (!ev.payload.isValid() || !adapter_) return;
        const qint64 frame = ev.payload.toLongLong();
        adapter_->onSetPlayheadPos(
            QtTimeline::framesToUs(static_cast<int>(frame)));
        return;
    }
}

} // namespace ms
