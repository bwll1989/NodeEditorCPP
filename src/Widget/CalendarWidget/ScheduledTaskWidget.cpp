#include "ScheduledTaskWidget.hpp"
#include <QLabel>

ScheduledTaskWidget::ScheduledTaskWidget(QWidget* parent)
    : QWidget(parent)
{

    // 初始化模型，并监听模型变化以刷新视图
    m_model = new ScheduledTaskModel(this);
    // 初始化子控件
    m_calendar = new OscCalendarWidget(m_model,this);
    m_taskList = new TaskListWidget(m_model,this);

    // 初始化布局（上部日历，下部任务列表）
    m_layout = new QVBoxLayout(this);
    m_layout->setContentsMargins(0, 0, 0, 0);
    m_layout->setSpacing(6);
    m_layout->addWidget(m_calendar, /*stretch*/ 1);
    m_layout->addWidget(m_taskList, /*stretch*/ 2);
    setLayout(m_layout);


    connect(m_model, &ScheduledTaskModel::modelChanged,
            this, &ScheduledTaskWidget::onCalendarSelectionChanged);

    // 连接信号：日历选择变化 -> 刷新列表
    connect(m_calendar, &QCalendarWidget::selectionChanged,
            this, &ScheduledTaskWidget::onCalendarSelectionChanged);

    // 连接信号：在日历上拖入消息 -> 加入对应日期任务列表（写入模型）
    connect(m_calendar, &OscCalendarWidget::oscMessageDropped,
            this, &ScheduledTaskWidget::onCalendarMessageDropped);

    // 初始刷新
    onCalendarSelectionChanged();
    // 当 m_model 初始化后，启动任务执行器
    if (!m_manager) {
        m_manager = new ScheduledTaskManager(m_model, this);
        m_manager->setToleranceSeconds(1);     // 秒级容差
        // m_manager->setUseQueue(true);         // 默认使用队列
        m_manager->start(1000);                // 每秒轮询一次
    }
}

ScheduledTaskWidget::~ScheduledTaskWidget()
{
    if (m_manager) {
        m_manager->stop();
    }
}

/**
 * @brief 获取指定日期的任务列表（从模型筛选转换）
 */
QVector<OSCMessage> ScheduledTaskWidget::tasksForDate(const QDate& date) const
{
    if (!m_model) return {};
    const auto items = m_model->itemsForDate(date);
    return itemsToMessages(items);
}

/**
 * @brief 设置指定日期的任务列表，并更新下方显示（不直接写入模型）
 */
void ScheduledTaskWidget::setTasksForDate(const QDate& date, const QVector<OSCMessage>& tasks)
{
    // 仅更新显示与信号，保持模型一致性由上层负责
    updateTaskListForDate(date);
    emit dateTasksChanged(date, tasks);
}

/**
 * @brief 添加任务到指定日期（默认当前选中日期），写入模型
 */
void ScheduledTaskWidget::addTask(const OSCMessage& message, const QDate& date)
{
    if (!m_model) return;
    QDate target = date.isValid() ? date : m_calendar->selectedDate();

    // 组装模型项：默认一次性任务，时间为当天 00:00:00（可根据需求调整）
    ScheduledTaskItem item;
    item.osc = message;
    item.scheduled.type = "once";
    item.scheduled.time = QDateTime(target, QTime(0, 0, 0));
    item.scheduled.conditions.clear();

    m_model->addItem(item);

    // 写入后刷新视图与信号
    updateTaskListForDate(target);
    emit dateTasksChanged(target, tasksForDate(target));
}

/**
 * @brief 保存整个日程为 JSON（委托模型）
 */
QJsonObject ScheduledTaskWidget::save() const
{
    if (!m_model) return {};
    return m_model->toJson();
}

/**
 * @brief 从 JSON 对象加载整个日程（委托模型）
 */
void ScheduledTaskWidget::load(const QJsonObject& json)
{
    if (!m_model) return;
    m_model->fromJson(json);
    // 加载后刷新当前选中日期
    onCalendarSelectionChanged();
}

/**
 * @brief 响应日历选择变化，更新列表显示（从模型筛选）
 */
void ScheduledTaskWidget::onCalendarSelectionChanged()
{
    QDate date = m_calendar->selectedDate();
    updateTaskListForDate(date);
}

/**
 * @brief 响应在日历上拖入消息：加入到对应日期的任务列表（写入模型）
 */
void ScheduledTaskWidget::onCalendarMessageDropped(const QDate& date, const OSCMessage& oscMessage)
{
    addTask(oscMessage, date);
}

/**
 * @brief 根据日期刷新 TaskListWidget 的显示内容（转换模型项为 OSCMessage）
 */
void ScheduledTaskWidget::updateTaskListForDate(const QDate& date)
{
    if (!m_model) {
        // m_taskList->setScheduleds({});
        return;
    }
    // 通过列表控件提供的接口设置过滤日期
    if (m_taskList) {
        m_taskList->setDate(date);
    }
    const auto items = m_model->itemsForDate(date);
    // m_taskList->setScheduleds(items);
}

/**
 * @brief 将 OSCMessage 转为 JSON 对象（保留：若有外部调用）
 */
QJsonObject ScheduledTaskWidget::messageToJson(const OSCMessage& message)
{
    QJsonObject obj;
    obj["host"] = message.host;
    obj["port"] = message.port;
    obj["address"] = message.address;
    obj["type"] = message.type;
    obj["value"] = message.value.toString();
    return obj;
}

/**
 * @brief 从 JSON 对象构建 OSCMessage（保留：若有外部调用）
 */
OSCMessage ScheduledTaskWidget::jsonToMessage(const QJsonObject& json)
{
    OSCMessage msg;
    msg.host = json["host"].toString("127.0.0.1");
    msg.port = json["port"].toInt(6001);
    msg.address = json["address"].toString();
    msg.type = json["type"].toString("string");
    msg.value = json["value"].toString();
    return msg;
}

/**
 * @brief 将模型任务项转换为消息列表（仅提取 OSCMessage）
 */
QVector<OSCMessage> ScheduledTaskWidget::itemsToMessages(const QVector<ScheduledTaskItem>& items)
{
    QVector<OSCMessage> messages;
    messages.reserve(items.size());
    for (const auto& it : items) {
        messages.push_back(it.osc);
    }
    return messages;
}

/**
 * @brief 获取任务列表控件的右键菜单操作列表
 * @return 操作列表（QAction*）
 */
QList<QAction*> ScheduledTaskWidget::getActions()
{
    return m_taskList->getMenu()->actions();
}
