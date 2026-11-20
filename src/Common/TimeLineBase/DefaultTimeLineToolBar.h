#pragma once

#include <QToolBar>
#include <QAction>
#include <QPoint>
#include <QHash>
#include "TimeLineStyle.h"
#include "TimelineExports.hpp"
#include "AudioLevelMeter.hpp" // 同目录（总输出电平表，工具栏右缘内置）

/**
 * @brief 时间轴工具栏（2026-11-10 结构简化）：
 * - 原 BaseTimelineToolbar 抽象层（仅 2 个纯虚 + 唯一实现）已扁平化删除，
 *   本类直接继承 QToolBar；
 * - 仅保留实际接线的播放控制（play/pause 切换、stop）与播放态回显；
 * - 上游遗留的无功能动作（循环/输出/设置/逐帧/移动剪辑/删除/缩放/定位/
 *   媒体切换/全屏——信号全工程零连接、connect 全被注释的死按钮）随
 *   抽象层一起退役，宿主如需同功能经自有 UI 承担。
 *
 * 【OSC 地址拖拽 2026-11-12 保留旧 TimeLineToolBar 能力】
 * - 宿主 setOscBase("/timeline/master") 后，播放/停止按钮与当前帧入口支持
 *   Ctrl+左键拖动：地址复制进剪贴板 + 产出 "application/x-osc-address"
 *   拖拽 mime（可落到 OSCMessageListWidget/日历/时间轴 Trigger 剪辑）；
 * - mime 字段序对齐消费端（OSCMessageListWidget::handleMimeDrop 与
 *   TimeLineView::applyDroppedMedia 的读取序）：host >> port >>
 *   address >> type >> value（旧生产端 value/type 错位为历史缺陷，不沿袭）。
 *
 * 【当前帧拖拽入口 2026-11-16 对齐节点体系 TimeLineNodeToolBar】
 * - cursor 图标动作（点击无操作——纯地址载体，节点工具栏同口径）：
 *   Ctrl+拖出 "/currentFrame"（完整地址 = base + rel，如
 *   /timeline/master/currentFrame）——落 OSC 消息卡即绑定该地址；
 *   该地址双向贯通：MainWindow tick 发布播放头帧号（usToFrames），
 *   命令写入跳转播放头（cue 链）。
 */
class NODE_TIMELINE_PUBLIC DefaultTimeLineToolBar : public QToolBar {
    Q_OBJECT
public:
    explicit DefaultTimeLineToolBar(QWidget* parent = nullptr);
    ~DefaultTimeLineToolBar() override;

    /// 【OSC 拖拽】设置地址前缀（如 "/timeline/master"；空 = 禁用拖拽）。
    /// 宿主在 OSC 绑定阶段调用一次（与 GlobalEventBus 订阅同源）。
    void setOscBase(const QString& base) {
        m_oscBase = base.startsWith('/') ? base : ('/' + base);
    }
    /// 【OSC 拖拽】控件 → 完整 OSC 地址（未登记/未设 base 返回空）
    QString oscAddressFor(QWidget* w) const {
        const QString rel = m_oscWidgets.value(w);
        return (rel.isEmpty() || m_oscBase.isEmpty()) ? QString()
                                                      : (m_oscBase + rel);
    }

signals:
    /// 播放/暂停按钮：进入播放
    void playClicked();
    /// 播放/暂停按钮：切回暂停
    void pauseClicked();
    /// 停止按钮
    void stopClicked();
    /// 【循环 2026-11-16】loop 开关翻转（用户点击；宿主接 TimelinePlaybackController）
    void loopToggled(bool on);
    /// 【Add Track 2026-11-20】新增无类型轨道（宿主/视图接
    /// BaseTimeLineModel::onAddTrack(QString())——轨道列表右键菜单同接口）
    void addTrackClicked();

public slots:
    /// 外部播放状态回显（图标/tooltip 与实际播放态对齐）
    void setPlaybackState(bool isPlaying);

    /// 【循环 2026-11-16】外部循环状态回显（OSC /loop 命令同源；只置勾选
    /// 态不发 triggered——无回环）
    void setLoopState(bool on);

    /// 【音频电平 2026-11-14 内置】灌入 timeline 混音 RMS——宿主 33ms 拍
    /// 轮询调用（数据源=TimelineAudioBusHost 混音引擎电平快照；非播放拍
    /// 宿主灌 0，表归零回落）。控件内置本工具栏右缘，宿主零 UI 组装。
    void setAudioLevel(float rms);

    /// 【StatusContainer 集成 2026-11-12】动作访问器：宿主（MainWindow）
    /// 把播放/停止按钮控件注册到 StatusContainer（地址→控件反查，
    /// 对齐旧 TimeLineToolBar::registerOSCControl 的登记语义）。
    QAction* playAction() const { return m_playAction; }
    QAction* stopAction() const { return m_stopAction; }
    QAction* loopAction() const { return m_loopAction; }

protected:
    /// 【OSC 拖拽】对登记控件启动地址拖拽（剪贴板 + x-osc-address mime）
    void startDrag(QWidget* widget);
    /// 【OSC 拖拽】Ctrl+左键按下记起点，移动超阈值触发 startDrag
    ///（旧 BaseTimelineToolbar::eventFilter 同逻辑）
    bool eventFilter(QObject* watched, QEvent* event) override;

private:
    void createActions();
    void setupUI();
    /// 登记控件→相对地址 并安装事件过滤器（拖拽检测）
    void installOscDrag(QWidget* w, const QString& rel);

    QAction* m_playAction {nullptr};
    QAction* m_stopAction {nullptr};
    QAction* m_loopAction {nullptr};        ///< 循环开关（2026-11-16 接线回归）
    QAction* m_addTrackAction {nullptr};    ///< 新增轨道（2026-11-20 接线在视图 initToolBar）
    QAction* m_currentFrameAction {nullptr}; ///< 当前帧拖拽入口（纯地址载体）
    bool m_isPlaying = false;
    bool m_isLooping = false;               ///< 循环模式（回显态与 m_loopAction 同步）
    AudioLevelMeter* m_levelMeter {nullptr}; ///< 右缘电平表（setupUI 内置）

    // ---- OSC 地址拖拽状态（旧 TimeLineToolBar 同构）-------------------------
    QString m_oscBase;                      ///< 地址前缀（空 = 禁用拖拽）
    QHash<QWidget*, QString> m_oscWidgets;  ///< 控件 → 相对地址（/play /stop）
    QPoint m_dragStartPos;                  ///< 拖拽起点
    bool m_dragging = false;                ///< Ctrl 拖拽进行中
};
