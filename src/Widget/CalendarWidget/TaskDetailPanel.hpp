#pragma once

#include <QWidget>
#include <QModelIndex>

#include "ScheduledTaskModel.hpp"
#include "ScheduledTaskFilterProxyModel.hpp"
#include "TaskItemWidget.hpp"

class QLabel;
class QStackedWidget;

class TaskDetailPanel : public QWidget {
    Q_OBJECT
public:
    explicit TaskDetailPanel(ScheduledTaskModel* model,
                             ScheduledTaskFilterProxyModel* proxy,
                             QWidget* parent = nullptr);

    void loadTask(const QModelIndex& proxyIndex);
    void clearTask();

signals:
    void taskCommitted();

private slots:
    void onEditorChanged();
    void onTestClicked();

private:
    void commitCurrentTask();
    void refreshTitle();
    void applyModelDataToEditor(const QModelIndex& index);
    static void writeEditorToModel(TaskItemWidget* editor,
                                   ScheduledTaskModel* model,
                                   const QModelIndex& sourceIndex);

    ScheduledTaskModel* m_model = nullptr;
    ScheduledTaskFilterProxyModel* m_proxy = nullptr;
    TaskItemWidget* m_editor = nullptr;
    QLabel* m_titleLabel = nullptr;
    QWidget* m_emptyPage = nullptr;
    QWidget* m_editorPage = nullptr;
    QStackedWidget* m_stack = nullptr;
    QModelIndex m_currentProxyIndex;
    bool m_loading = false;
};
