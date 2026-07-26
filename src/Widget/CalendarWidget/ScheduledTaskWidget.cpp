#include "ScheduledTaskWidget.hpp"

#include <QVBoxLayout>
#include <QIcon>
#include <QScreen>
#include <QGuiApplication>
#include <QCursor>
#include <algorithm>

ScheduledTaskWidget::ScheduledTaskWidget(QWidget* parent)
    : QWidget(parent)
{
    m_model = new ScheduledTaskModel(this);
    m_calendar = new OscCalendarWidget(m_model, this);
    m_popup = new TaskEditPopup(this);
    m_manager = new ScheduledTaskManager(m_model, this);

    setupUi();
    setupActions();
    setupConnections();

    m_manager->setToleranceSeconds(1);
    m_manager->start(1000);
}

ScheduledTaskWidget::~ScheduledTaskWidget()
{
    if (m_manager) {
        m_manager->stop();
    }
}

void ScheduledTaskWidget::setupUi()
{
    m_calendar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_calendar->setMinimumHeight(280);

    auto* layout = new QVBoxLayout(this);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);
    layout->addWidget(m_calendar, 1);
}

void ScheduledTaskWidget::setupConnections()
{
    // 模型变化时刷新格内任务条
    const auto refresh = [this]() { refreshCalendar(); };
    connect(m_model, &ScheduledTaskModel::modelChanged, this, refresh);
    connect(m_model, &QAbstractItemModel::dataChanged, this, refresh);
    connect(m_model, &QAbstractItemModel::rowsInserted, this, refresh);
    connect(m_model, &QAbstractItemModel::rowsRemoved, this, refresh);
    connect(m_model, &QAbstractItemModel::modelReset, this, refresh);

    connect(m_calendar, &OscCalendarWidget::oscMessageDropped, this,
            [this](const QDate& date, const OSCMessage& msg) { addTask(msg, date); });
    connect(m_calendar, &OscCalendarWidget::taskChipClicked, this,
            &ScheduledTaskWidget::openTaskEditor);
    connect(m_calendar, &OscCalendarWidget::emptyDateDoubleClicked, this,
            [this](const QDate& date) { addTask(OSCMessage(), date); });
    connect(m_calendar, &OscCalendarWidget::requestAddTask, this,
            [this](const QDate& date) { addTask(OSCMessage(), date); });
    connect(m_calendar, &OscCalendarWidget::requestClearDate, this,
            &ScheduledTaskWidget::clearTasksOnDate);
    connect(m_calendar, &OscCalendarWidget::requestDeleteSelectedTask, this, [this]() {
        const int row = m_calendar->selectedSourceRow();
        if (row >= 0) {
            deleteSourceRow(row);
        }
    });

    connect(m_popup, &TaskEditPopup::taskDeleted, this, &ScheduledTaskWidget::deleteSourceRow);
    connect(m_popup, &TaskEditPopup::taskChanged, this, [this]() {
        refreshCalendar();
        notifyDateChanged(m_calendar->selectedDate());
    });
    connect(m_popup, &TaskEditPopup::closed, m_calendar, &OscCalendarWidget::clearSelectedSourceRow);
}

void ScheduledTaskWidget::setupActions()
{
    m_actionsMenu = new QMenu(this);

    QAction* addAction = m_actionsMenu->addAction(tr("添加任务"));
    addAction->setIcon(QIcon(QStringLiteral(":/icons/icons/add.png")));
    connect(addAction, &QAction::triggered, this, [this]() {
        addTask(OSCMessage(), m_calendar->selectedDate());
    });

    QAction* clearAction = m_actionsMenu->addAction(tr("清空全部"));
    clearAction->setIcon(QIcon(QStringLiteral(":/icons/icons/clear.png")));
    connect(clearAction, &QAction::triggered, this, [this]() {
        m_popup->closePopup();
        for (int r = m_model->rowCount() - 1; r >= 0; --r) {
            m_model->removeItem(r);
        }
        m_calendar->clearSelectedSourceRow();
        notifyDateChanged(m_calendar->selectedDate());
    });
}

QList<QAction*> ScheduledTaskWidget::getActions()
{
    return m_actionsMenu ? m_actionsMenu->actions() : QList<QAction*>{};
}

QVector<OSCMessage> ScheduledTaskWidget::tasksForDate(const QDate& date) const
{
    QVector<OSCMessage> messages;
    if (!m_model) {
        return messages;
    }
    const auto items = m_model->itemsForDate(date);
    messages.reserve(items.size());
    for (const auto& item : items) {
        messages.push_back(item.osc);
    }
    return messages;
}

void ScheduledTaskWidget::addTask(const OSCMessage& message, const QDate& date)
{
    if (!m_model) {
        return;
    }

    const QDate target = date.isValid() ? date : m_calendar->selectedDate();
    ScheduledTaskItem item;
    item.osc = message;
    item.scheduled.type = QStringLiteral("once");
    item.scheduled.time = QDateTime(target, QTime(9, 0, 0));

    m_model->addItem(item);
    notifyDateChanged(target);

    const int newRow = m_model->rowCount() - 1;
    if (newRow >= 0) {
        openTaskEditor(newRow, QCursor::pos());
    }
}

QJsonObject ScheduledTaskWidget::save() const
{
    return m_model ? m_model->toJson() : QJsonObject{};
}

void ScheduledTaskWidget::load(const QJsonObject& json)
{
    if (!m_model) {
        return;
    }
    m_popup->closePopup();
    m_model->fromJson(json);
    m_calendar->clearSelectedSourceRow();
    refreshCalendar();
}

void ScheduledTaskWidget::openTaskEditor(int sourceRow, const QPoint& globalPos)
{
    if (!m_popup || !m_model || sourceRow < 0 || sourceRow >= m_model->rowCount()) {
        return;
    }

    m_popup->editTask(m_model, sourceRow);
    m_popup->adjustSize();

    // 尽量把浮层放在点击点附近，并限制在屏幕可见区域内
    QPoint pos = globalPos + QPoint(8, 8);
    if (QScreen* screen = QGuiApplication::screenAt(globalPos)) {
        const QRect ag = screen->availableGeometry();
        const QSize sz = m_popup->size();
        pos.setX(std::clamp(pos.x(), ag.left() + 8, ag.right() - sz.width() - 8));
        pos.setY(std::clamp(pos.y(), ag.top() + 8, ag.bottom() - sz.height() - 8));
    }
    m_popup->move(pos);
    m_popup->show();
}

void ScheduledTaskWidget::deleteSourceRow(int sourceRow)
{
    if (!m_model || sourceRow < 0 || sourceRow >= m_model->rowCount()) {
        return;
    }

    const QDate date = m_model->index(sourceRow, 0).data(ScheduledTaskModel::RoleDate).toDate();
    if (m_popup->currentSourceRow() == sourceRow) {
        m_popup->closePopup();
    }
    m_model->removeItem(sourceRow);
    m_calendar->clearSelectedSourceRow();
    notifyDateChanged(date.isValid() ? date : m_calendar->selectedDate());
}

void ScheduledTaskWidget::clearTasksOnDate(const QDate& date)
{
    if (!m_model || !date.isValid()) {
        return;
    }

    m_popup->closePopup();
    QVector<int> rows = m_model->rowIndexesForDate(date);
    std::sort(rows.begin(), rows.end(), std::greater<int>());
    for (int row : rows) {
        m_model->removeItem(row);
    }
    m_calendar->clearSelectedSourceRow();
    notifyDateChanged(date);
}

void ScheduledTaskWidget::refreshCalendar()
{
    if (m_calendar) {
        m_calendar->refresh();
    }
}

void ScheduledTaskWidget::notifyDateChanged(const QDate& date)
{
    emit dateTasksChanged(date, tasksForDate(date));
}
