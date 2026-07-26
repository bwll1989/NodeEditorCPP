#pragma once

#include <QFrame>

#include "ScheduledTaskModel.hpp"
#include "TaskItemWidget.hpp"

class QLabel;
class QPushButton;
class QHideEvent;

/**
 * @brief 任务浮层编辑器
 *
 * 点击日历任务条后弹出，编辑结果即时写回模型；
 * 点击浮层外部区域自动关闭。
 */
class TaskEditPopup : public QFrame {
    Q_OBJECT
public:
    explicit TaskEditPopup(QWidget* parent = nullptr);

    /** 加载并显示指定任务 */
    void editTask(ScheduledTaskModel* model, int sourceRow);
    void closePopup();
    int currentSourceRow() const { return m_sourceRow; }

signals:
    void taskChanged();
    void taskDeleted(int sourceRow);
    void closed();

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    void hideEvent(QHideEvent* event) override;

private:
    void commit();
    void refreshTitle();
    void loadFromModel();

    ScheduledTaskModel* m_model = nullptr;
    int m_sourceRow = -1;
    bool m_loading = false;

    QLabel* m_titleLabel = nullptr;
    TaskItemWidget* m_editor = nullptr;
    QPushButton* m_btnDelete = nullptr;
    QPushButton* m_btnClose = nullptr;
};
