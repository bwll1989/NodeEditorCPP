#pragma once

#include <QCalendarWidget>
#include <QDate>
#include <QVector>
#include <QRect>
#include <QEvent>

#include "Common/Devices/OSCSender/OSCSender.h"
#include "ScheduledTaskModel.hpp"

#include <QModelIndex>

class QTableView;
class QMimeData;
class QShowEvent;
class QMouseEvent;
class QContextMenuEvent;
class QDragEnterEvent;
class QDragMoveEvent;
class QDropEvent;

/**
 * @brief 单日历计划任务视图
 *
 * 职责：
 * - 在日期格内绘制任务摘要条
 * - 点击任务条发出编辑请求
 * - 右键菜单新建/清空/删除
 * - 接受 OSC 地址拖放以创建任务
 *
 * 说明：QCalendarWidget 内部用 QTableView 绘制格子，鼠标与拖放事件发生在
 * viewport 上，因此需对该 viewport 安装事件过滤并开启 DropOnly。
 */
class OscCalendarWidget : public QCalendarWidget
{
    Q_OBJECT

public:
    explicit OscCalendarWidget(ScheduledTaskModel* model = nullptr, QWidget* parent = nullptr);
    ~OscCalendarWidget() override;

    /** 指定日期是否有任务 */
    bool hasTasksOnDate(const QDate& date) const;

    /** 当前选中的任务在模型中的行号，无选中为 -1 */
    int selectedSourceRow() const { return m_selectedSourceRow; }

    /** 清除任务条选中高亮 */
    void clearSelectedSourceRow();

    /** 刷新格子绘制（模型变更后调用） */
    void refresh();

signals:
    /** OSC 被拖放到某日 */
    void oscMessageDropped(const QDate& date, const OSCMessage& oscMessage);
    /** 点击了格内任务条 */
    void taskChipClicked(int sourceRow, const QPoint& globalPos);
    /** 双击空白日期（用于新建） */
    void emptyDateDoubleClicked(const QDate& date);
    void requestAddTask(const QDate& date);
    void requestClearDate(const QDate& date);
    void requestDeleteSelectedTask();

protected:
    void paintCell(QPainter* painter, const QRect& rect, QDate date) const override;
    void showEvent(QShowEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
    void contextMenuEvent(QContextMenuEvent* event) override;
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dragMoveEvent(QDragMoveEvent* event) override;
    void dropEvent(QDropEvent* event) override;

private:
    /** 命中结果：日期 + 任务行（未点中任务条时 sourceRow = -1） */
    struct ChipHit {
        QDate date;
        int sourceRow = -1;
    };

    /** 某日在给定格子矩形内的任务条布局（坐标系与 cellRect 一致） */
    struct ChipLayout {
        QVector<int> rows;   ///< 可见任务行号
        int overflow = 0;    ///< 未显示数量
        QVector<QRect> rects; ///< 任务条矩形；末尾可能多一个 "+N" 区域
    };

    void ensureViewHooks();
    void applyWeekdayFormats();
    void showDateContextMenu(const QDate& date, const QPoint& globalPos);

    bool onViewportMousePress(QMouseEvent* event);
    bool onViewportMouseDoubleClick(QMouseEvent* event);
    bool onViewportDrag(QEvent* event);
    void acceptOscDrop(const QPoint& globalPos, const QMimeData* mime);

    QTableView* calendarView() const;
    QDate dateAtGlobalPos(const QPoint& globalPos) const;
    QDate dateFromModelIndex(const QModelIndex& index) const;
    QRect cellRectInViewport(const QDate& date) const;
    ChipLayout buildChipLayout(const QDate& date, const QRect& cellRect) const;
    ChipHit hitTest(const QPoint& globalPos) const;

    static int maxChipsForHeight(int cellHeight);
    static QString shortAddress(const QString& address);
    static bool isOscMime(const QMimeData* mime);
    static OSCMessage oscFromMime(const QMimeData* mime);

    ScheduledTaskModel* m_model = nullptr;
    int m_selectedSourceRow = -1;
    bool m_viewHooksInstalled = false;
};
