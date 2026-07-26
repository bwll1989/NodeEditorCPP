#pragma once

#include <QWidget>
#include <QDate>
#include <QJsonObject>
#include <QMenu>
#include <QAction>

#include "CalendarWidget.hpp"
#include "TaskEditPopup.hpp"
#include "ScheduledTaskModel.hpp"
#include "ScheduledTaskManager.hpp"

/**
 * @brief 计划任务面板：单日历 + 浮层编辑 + 定时调度
 */
class ScheduledTaskWidget : public QWidget {
    Q_OBJECT
public:
    explicit ScheduledTaskWidget(QWidget* parent = nullptr);
    ~ScheduledTaskWidget() override;

    QVector<OSCMessage> tasksForDate(const QDate& date) const;
    void addTask(const OSCMessage& message, const QDate& date = QDate());
    QJsonObject save() const;
    void load(const QJsonObject& json);

    /** 供 Dock 标题栏菜单使用 */
    QList<QAction*> getActions();

signals:
    void dateTasksChanged(const QDate& date, const QVector<OSCMessage>& tasks);

private:
    void setupUi();
    void setupConnections();
    void setupActions();

    void openTaskEditor(int sourceRow, const QPoint& globalPos);
    void deleteSourceRow(int sourceRow);
    void clearTasksOnDate(const QDate& date);
    void refreshCalendar();
    void notifyDateChanged(const QDate& date);

    OscCalendarWidget* m_calendar = nullptr;
    TaskEditPopup* m_popup = nullptr;
    ScheduledTaskModel* m_model = nullptr;
    ScheduledTaskManager* m_manager = nullptr;
    QMenu* m_actionsMenu = nullptr;
};
