#pragma once

#include <QToolBar>
#include <QAction>
#include <QPoint>
#include <QHash>
#include "TimeLineStyle.h"
#include "TimelineExports.hpp"

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
 * - 宿主 setOscBase("/timeline/master") 后，播放/停止按钮支持
 *   Ctrl+左键拖动：地址复制进剪贴板 + 产出 "application/x-osc-address"
 *   拖拽 mime（可落到 OSCMessageListWidget/日历/时间轴 Trigger 剪辑）；
 * - mime 字段序对齐消费端（OSCMessageListWidget::handleMimeDrop 与
 *   TimeLineView::applyDroppedMedia 的读取序）：host >> port >>
 *   address >> type >> value（旧生产端 value/type 错位为历史缺陷，不沿袭）。
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

public slots:
    /// 外部播放状态回显（图标/tooltip 与实际播放态对齐）
    void setPlaybackState(bool isPlaying);

    /// 【StatusContainer 集成 2026-11-12】动作访问器：宿主（MainWindow）
    /// 把播放/停止按钮控件注册到 StatusContainer（地址→控件反查，
    /// 对齐旧 TimeLineToolBar::registerOSCControl 的登记语义）。
    QAction* playAction() const { return m_playAction; }
    QAction* stopAction() const { return m_stopAction; }

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
    bool m_isPlaying = false;

    // ---- OSC 地址拖拽状态（旧 TimeLineToolBar 同构）-------------------------
    QString m_oscBase;                      ///< 地址前缀（空 = 禁用拖拽）
    QHash<QWidget*, QString> m_oscWidgets;  ///< 控件 → 相对地址（/play /stop）
    QPoint m_dragStartPos;                  ///< 拖拽起点
    bool m_dragging = false;                ///< Ctrl 拖拽进行中
};
