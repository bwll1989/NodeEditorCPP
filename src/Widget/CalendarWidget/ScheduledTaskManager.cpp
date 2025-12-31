#include "ScheduledTaskManager.hpp"

#include "StatusContainer/StatusContainer.h"

/**
 * @brief 构造：可选传入模型，默认不启动定时器
 */
ScheduledTaskManager::ScheduledTaskManager(ScheduledTaskModel* model, QObject* parent)
    : QObject(parent), m_model(model) {
    connect(&m_timer, &QTimer::timeout, this, &ScheduledTaskManager::onTick);
}

/**
 * @brief 析构：停止定时器
 */
ScheduledTaskManager::~ScheduledTaskManager() {
    stop();
}

/**
 * @brief 设置模型（可在运行时切换）
 */
void ScheduledTaskManager::setModel(ScheduledTaskModel* model) {
    m_model = model;
    m_lastTriggerTime.clear();
}

/**
 * @brief 启动轮询
 */
void ScheduledTaskManager::start(int intervalMs) {
    m_timer.start(intervalMs);
}

/**
 * @brief 停止轮询
 */
void ScheduledTaskManager::stop() {
    m_timer.stop();
}

/**
 * @brief 设置秒级容差
 */
void ScheduledTaskManager::setToleranceSeconds(int seconds) {
    m_toleranceSec = qMax(0, seconds);
}

/**
 * @brief 设置发送方式（队列/直接）
 */
// void ScheduledTaskManager::setUseQueue(bool useQueue) {
//     m_useQueue = useQueue;
// }

/**
 * @brief 定时器回调：轮询并执行任务
 * 
 * 逻辑：
 * - 遍历模型中的所有任务项
 * - 按类型分别判断是否应触发
 * - 触发则发送 OSC，并发出 taskExecuted 信号
 */
void ScheduledTaskManager::onTick() {
    if (!m_model) return;

    const auto& items = m_model->items();
    const QDateTime now = QDateTime::currentDateTime();

    for (const auto& item : items) {
        bool fire = false;
        if (item.scheduled.type.compare("once", Qt::CaseInsensitive) == 0) {
            fire = shouldTriggerOnce(item, now);
        } else {
            fire = shouldTriggerLoop(item, now);
        }

        if (fire) {
            qDebug()<<"The scheduled task has been carried out"<<item.remarks<<item.scheduled.time.toString("yyyy-MM-dd hh:mm:ss");
            executeTask(item.osc);
            emit taskExecuted(item);
        }
    }
}

/**
 * @brief 判断一次性任务是否应触发
 * 
 * 规则：
 * - 若当前时间 >= 计划时间，且未触发过则触发
 * - 容差仅用于靠近计划时间的提前/延后触发允许窗口
 */
bool ScheduledTaskManager::shouldTriggerOnce(const ScheduledTaskItem& item, const QDateTime& now) {
    /**
     * @brief 一次性任务触发判定（仅在到达设定时间点触发，过时不处理）
     *
     * 规则：
     * - 当前日期必须与计划日期相同
     * - 当前“时分秒”未超过计划时间（不允许过时触发）
     * - 当前时间与计划时间的差值在容差秒数以内（容差默认 1 秒）
     *   若需要严格“只有相同时才触发”，请将容差设置为 0（setToleranceSeconds(0)）
     */
    const QDateTime sched = item.scheduled.time;
    if (!sched.isValid()) return false;

    const QString key = keyForItem(item);
    // 日期必须相同
    if (now.date() != sched.date()) return false;

    // 计算“计划时间 - 当前时间”的毫秒差值
    const int diffMs = sched.time().msecsSinceStartOfDay() - now.time().msecsSinceStartOfDay();

    // 仅允许在计划时间点到来之前/刚到时触发；已过时（diffMs < 0）不触发
    if (diffMs >= 0 && diffMs <= m_toleranceSec * 1000) {

        return true;
    }
    return false;
}

/**
 * @brief 判断循环任务是否应触发
 * 
 * 规则：
 * - 星期条件匹配（conditions 为空视为每天）
 * - 当前时间与计划的“时:分:秒”匹配（秒级容差）
 * - 防抖：同一任务同一天同一分钟只触发一次
 */
bool ScheduledTaskManager::shouldTriggerLoop(const ScheduledTaskItem& item, const QDateTime& now) {
    const QTime schedTime = item.scheduled.time.time();
    if (!schedTime.isValid()) return false;

    // 星期匹配：conditions 为空视为无效配置；否则必须包含当前星期名
    const int dow = now.date().dayOfWeek(); // 1..7（周一=1）
    const QString dowName = ScheduledTaskModel::dayOfWeekToName(dow);
    const auto& conds = item.scheduled.conditions;
    const bool dayMatch = conds.contains(dowName, Qt::CaseInsensitive);
    if (!dayMatch) return false;

    // 时间匹配（容差秒）
    const int diffMs = now.time().msecsSinceStartOfDay() - schedTime.msecsSinceStartOfDay();
    if (diffMs < 0 || diffMs > m_toleranceSec * 1000) return false;

    // // 防抖：避免在同一分钟内重复触发
    // const QString key = keyForItem(item);
    // const auto last = m_lastTriggerTime.value(key);
    // if (last.isValid()) {
    //     const bool sameDay = (last.date() == now.date());
    //     const bool sameMinute = (last.time().hour() == now.time().hour() &&
    //                              last.time().minute() == now.time().minute());
    //     if (sameDay && sameMinute) return false;
    // }
    // m_lastTriggerTime.insert(key, now);
    return true;
}

/**
 * @brief 发送 OSC 消息（队列或直接）
 */
void ScheduledTaskManager::executeTask(const OSCMessage& msg) {
    // if (m_useQueue) {
         StatusContainer::instance()->parseOSC(msg);
    // } else {
    //     OSCSender::instance()->sendOSCMessageDirectly(msg);
    // }
}

/**
 * @brief 生成稳定键，用于标记/防抖
 */
QString ScheduledTaskManager::keyForItem(const ScheduledTaskItem& item) {
    const auto& s = item.scheduled;
    const auto& o = item.osc;
    return QString("%1|%2|%3|%4|%5|%6")
        .arg(s.type)
        .arg(s.time.toString(Qt::ISODate))
        .arg(o.host)
        .arg(o.port)
        .arg(o.address)
        .arg(o.value.toString());
}