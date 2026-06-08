#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QSplitter>
#include <QDate>
#include <QJsonObject>
#include <QModelIndex>

#include "CalendarWidget.hpp"
#include "TaskListWidget.hpp"
#include "TaskDetailPanel.hpp"
#include "ScheduledTaskModel.hpp"
#include "ScheduledTaskManager.hpp"

class ScheduledTaskWidget : public QWidget {
    Q_OBJECT
public:
    explicit ScheduledTaskWidget(QWidget* parent = nullptr);
    ~ScheduledTaskWidget();

    QVector<OSCMessage> tasksForDate(const QDate& date) const;
    void setTasksForDate(const QDate& date, const QVector<OSCMessage>& tasks);
    void addTask(const OSCMessage& message, const QDate& date = QDate());
    QJsonObject save() const;
    void load(const QJsonObject& json);
    QList<QAction*> getActions();

signals:
    void dateTasksChanged(const QDate& date, const QVector<OSCMessage>& tasks);

private slots:
    void onCalendarSelectionChanged();
    void onCalendarMessageDropped(const QDate& date, const OSCMessage& oscMessage);
    void onCurrentTaskChanged(const QModelIndex& proxyIndex);

private:
    void updateTaskListForDate(const QDate& date);

    static QJsonObject messageToJson(const OSCMessage& message);
    static OSCMessage jsonToMessage(const QJsonObject& json);
    static QVector<OSCMessage> itemsToMessages(const QVector<ScheduledTaskItem>& items);

    OscCalendarWidget* m_calendar = nullptr;
    TaskListWidget* m_taskList = nullptr;
    TaskDetailPanel* m_detailPanel = nullptr;
    QSplitter* m_splitter = nullptr;
    QVBoxLayout* m_layout = nullptr;
    ScheduledTaskModel* m_model = nullptr;
    ScheduledTaskManager* m_manager = nullptr;
};
