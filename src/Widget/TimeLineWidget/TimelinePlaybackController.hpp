// ============================================================================
//  TimelinePlaybackController — 时间轴播放总控
// ============================================================================
// 职责：工具栏播放三动作与外部 OSC 命令（/timeline/master/*）同源驱动的
// 播放状态机——下发会话广播（runtime）+ 本地预览状态机（preview）+
// timeline 音频 bus（audioBus）+ 33ms tick 播放头回显 + seek 全链联动 +
// 电平表数据泵转发（audioBus 33ms 采样拍 → toolbar 电平显示）+ 播放态/
// 帧号出站反馈（GlobalEventBus → StatusContainer 镜像 → OSC/MQTT/WS 出站）。
//
// 依赖全部为非拥有指针（Deps 包一次注入，逐指针允许 null，各 handle 逐
// 空判）；33ms tick 定时器自持（父=本类，随控制器级联销毁）；OSC 订阅
// receiver=本控制器（QObject 连接随析构自动断开）。构造时序约束：依赖
// 子系统须全部就绪（MainWindow 中 msAudioBusHost_->start() 之后注入）。
// ============================================================================
#pragma once

#include <QObject>
#include <QString>
#include <QTimer>

class BaseTimelineWidget;   // TimeLineBase（时间轴视图；全局命名空间）
struct GlobalEvent;         // GlobalEventBus 事件（全局命名空间；const& 传参，前置即可）

namespace ms {

class MsRuntimeBridge;          // 下发运行时桥（PIMPL 隔离 C++20）
class PreviewRenderer;          // 本地预览渲染器（状态机）
class QtTimelineModelAdapter;   // Scene↔帧模型适配器
class TimelineAudioBusHost;     // timeline 混音写入宿主（非 QObject）

class TimelinePlaybackController : public QObject {
    Q_OBJECT
public:
    /// 非拥有依赖包（构造期一次注入；聚合初始化顺序即成员声明顺序）
    struct Deps {
        MsRuntimeBridge* runtime = nullptr;         // 下发运行时桥
        PreviewRenderer* preview = nullptr;         // 本地预览状态机
        TimelineAudioBusHost* audioBus = nullptr;   // timeline 音频 bus 宿主
        QtTimelineModelAdapter* adapter = nullptr;  // Scene↔帧模型适配器
        BaseTimelineWidget* timeline = nullptr;     // 时间轴视图（取 toolbar）
    };

    explicit TimelinePlaybackController(const Deps& deps, QObject* parent = nullptr);
    ~TimelinePlaybackController() override;

public Q_SLOTS:
    /// GlobalEventBus 命令分发入口（/timeline/master/* 订阅槽；旧
    /// TimeLineToolBar::onGlobalEvent 同构：单槽按地址分发 + 回执反馈）
    void onTimelineCommand(const GlobalEvent& ev);

private:
    /// 播放（工具栏点击与外部 /play 命令同源）：会话广播 + 本地预览 + tick 启动
    void handlePlay();
    /// 暂停（同上双源）：会话冻结 + 预览暂停 + tick 停止
    void handlePause();
    /// 停止（同上双源）：会话回 Idle + 预览归零 + 播放头回显起点
    void handleStop();
    /// 【循环 2026-11-16】loop 开关（工具栏点击与外部 /loop 命令同源）：
    /// 状态翻转 + /loop 反馈；生效点在 33ms tick 到头检查（回零续播/停止）。
    void handleLoop(bool on);
    /// 播放态反馈推送（/play /stop /currentFrame → StatusContainer 出站镜像）
    void publishPlaybackState();
    /// 循环态反馈推送（/loop → StatusContainer 出站镜像；含构造初始推送）
    void publishLoopState();
    /// OSC 绑定入口：订阅命令地址 + 注册控件 + 推送初始状态（构造期一次）
    void bindOsc();
    /// 地址拼装："/timeline/master" + rel（rel 带不带首 '/' 皆可）
    static QString address(const QString& rel);

    // ---- 非拥有依赖（Deps 拆平存持）----
    MsRuntimeBridge* runtime_ = nullptr;
    PreviewRenderer* preview_ = nullptr;
    TimelineAudioBusHost* audioBus_ = nullptr;
    QtTimelineModelAdapter* adapter_ = nullptr;
    BaseTimelineWidget* timeline_ = nullptr;

    QTimer* tick_ = nullptr; ///< 33ms 播放期 tick（播放头回显 + /currentFrame 反馈）

    bool looping_ = false; ///< 循环模式（2026-11-16）：到头回零续播而非停止
};

} // namespace ms
