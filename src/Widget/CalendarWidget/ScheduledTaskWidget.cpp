#include "ScheduledTaskWidget.hpp"

#include <QDateTime>

ScheduledTaskWidget::ScheduledTaskWidget(QWidget* parent)
    : QWidget(parent)
{
    m_model = new ScheduledTaskModel(this);
    m_calendar = new OscCalendarWidget(m_model, this);
    m_taskList = new TaskListWidget(m_model, this);
    m_detailPanel = new TaskDetailPanel(m_model, m_taskList->proxyModel(), this);

    m_calendar->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    m_splitter = new QSplitter(Qt::Horizontal, this);
    m_splitter->setObjectName(QStringLiteral("taskMainSplitter"));
    m_splitter->addWidget(m_taskList);
    m_splitter->addWidget(m_detailPanel);
    m_splitter->setStretchFactor(0, 2);
    m_splitter->setStretchFactor(1, 3);
    m_splitter->setChildrenCollapsible(false);
    m_splitter->setHandleWidth(4);

    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(6);
    m_layout->addWidget(m_calendar, 1);
    m_layout->addWidget(m_splitter, 1);
    setLayout(m_layout);

    connect(m_model, &ScheduledTaskModel::modelChanged, m_calendar, [this]() {
        m_calendar->update();
    });
    connect(m_model, &QAbstractItemModel::dataChanged, m_calendar, [this]() {
        m_calendar->update();
    });
    connect(m_calendar, &QCalendarWidget::selectionChanged,
            this, &ScheduledTaskWidget::onCalendarSelectionChanged);
    connect(m_calendar, &OscCalendarWidget::oscMessageDropped,
            this, &ScheduledTaskWidget::onCalendarMessageDropped);
    connect(m_taskList, &TaskListWidget::currentTaskChanged,
            this, &ScheduledTaskWidget::onCurrentTaskChanged);
    connect(m_detailPanel, &TaskDetailPanel::taskCommitted, this, [this]() {
        if (m_taskList && m_taskList->listView()) {
            m_taskList->listView()->viewport()->update();
        }
    });

    onCalendarSelectionChanged();

    if (!m_manager) {
        m_manager = new ScheduledTaskManager(m_model, this);
        m_manager->setToleranceSeconds(1);
        m_manager->start(1000);
    }
}

ScheduledTaskWidget::~ScheduledTaskWidget()
{
    if (m_manager) {
        m_manager->stop();
    }
}

QVector<OSCMessage> ScheduledTaskWidget::tasksForDate(const QDate& date) const
{
    if (!m_model) {
        return {};
    }
    return itemsToMessages(m_model->itemsForDate(date));
}

void ScheduledTaskWidget::setTasksForDate(const QDate& date, const QVector<OSCMessage>& tasks)
{
    Q_UNUSED(tasks);
    updateTaskListForDate(date);
    emit dateTasksChanged(date, tasks);
}

void ScheduledTaskWidget::addTask(const OSCMessage& message, const QDate& date)
{
    if (!m_model) {
        return;
    }
    QDate target = date.isValid() ? date : m_calendar->selectedDate();

    ScheduledTaskItem item;
    item.osc = message;
    item.scheduled.type = QStringLiteral("once");
    item.scheduled.time = QDateTime(target, QTime(9, 0, 0));
    item.scheduled.conditions.clear();

    m_model->addItem(item);
    emit dateTasksChanged(target, tasksForDate(target));
}

QJsonObject ScheduledTaskWidget::save() const
{
    if (!m_model) {
        return {};
    }
    return m_model->toJson();
}

void ScheduledTaskWidget::load(const QJsonObject& json)
{
    if (!m_model) {
        return;
    }
    m_model->fromJson(json);
    onCalendarSelectionChanged();
}

void ScheduledTaskWidget::onCalendarSelectionChanged()
{
    updateTaskListForDate(m_calendar->selectedDate());
}

void ScheduledTaskWidget::onCalendarMessageDropped(const QDate& date, const OSCMessage& oscMessage)
{
    addTask(oscMessage, date);
}

void ScheduledTaskWidget::onCurrentTaskChanged(const QModelIndex& proxyIndex)
{
    if (!m_detailPanel) {
        return;
    }
    if (proxyIndex.isValid()) {
        m_detailPanel->loadTask(proxyIndex);
    } else {
        m_detailPanel->clearTask();
    }
}

void ScheduledTaskWidget::updateTaskListForDate(const QDate& date)
{
    if (m_taskList) {
        m_taskList->setDate(date);
    }
}

QJsonObject ScheduledTaskWidget::messageToJson(const OSCMessage& message)
{
    QJsonObject obj;
    obj[QStringLiteral("host")] = message.host;
    obj[QStringLiteral("port")] = message.port;
    obj[QStringLiteral("address")] = message.address;
    obj[QStringLiteral("type")] = message.type;
    obj[QStringLiteral("value")] = message.value.toString();
    return obj;
}

OSCMessage ScheduledTaskWidget::jsonToMessage(const QJsonObject& json)
{
    OSCMessage msg;
    msg.host = json[QStringLiteral("host")].toString(QStringLiteral("127.0.0.1"));
    msg.port = json[QStringLiteral("port")].toInt(6001);
    msg.address = json[QStringLiteral("address")].toString();
    msg.type = json[QStringLiteral("type")].toString(QStringLiteral("string"));
    msg.value = json[QStringLiteral("value")].toString();
    return msg;
}

QVector<OSCMessage> ScheduledTaskWidget::itemsToMessages(const QVector<ScheduledTaskItem>& items)
{
    QVector<OSCMessage> messages;
    messages.reserve(items.size());
    for (const auto& it : items) {
        messages.push_back(it.osc);
    }
    return messages;
}

QList<QAction*> ScheduledTaskWidget::getActions()
{
    return m_taskList->getMenu()->actions();
}
