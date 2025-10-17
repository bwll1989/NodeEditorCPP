#pragma once

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <QSet>
#include <QMap>
#include <QString>
#include "ScheduledTaskModel.hpp"
#include "../../Common/Devices/OSCSender/OSCSender.h"

/**
 * @brief 计划任务执行器
 * 
 * 职责：
 * - 轮询 ScheduledTaskModel 的任务，根据当前时间判断是否触发
 * - 支持 once（一次性）与 loop（循环）两种任务类型
 * - 触发后通过 OSCSender 发送 OSC 消息
 * - 内置防重复与容差控制，避免同一时间点重复触发
 */
class ScheduledTaskManager : public QObject {
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param model 可选的任务模型（可后续 setModel 替换）
     * @param parent 父对象
     */
    explicit ScheduledTaskManager(ScheduledTaskModel* model = nullptr, QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~ScheduledTaskManager();

    /**
     * @brief 设置任务模型
     * @param model 任务模型指针
     */
    void setModel(ScheduledTaskModel* model);

    /**
     * @brief 启动定时轮询
     * @param intervalMs 轮询间隔（毫秒），默认 1000ms
     */
    void start(int intervalMs = 1000);

    /**
     * @brief 停止定时轮询
     */
    void stop();

    /**
     * @brief 设置触发容差秒数（用于时间对比，默认 1 秒）
     * @param seconds 容差秒
     */
    void setToleranceSeconds(int seconds);

    /**
     * @brief 设置是否使用队列发送（true：入队列；false：直接发送）
     * @param useQueue 布尔
     */
    void setUseQueue(bool useQueue);

signals:
    /**
     * @brief 某任务被执行时发出
     * @param item 被执行的任务项
     */
    void taskExecuted(const ScheduledTaskItem& item);

private slots:
    /**
     * @brief 定时器回调：轮询并执行应触发的任务
     */
    void onTick();

private:
    /**
     * @brief 判断一次性任务是否应触发，并记录防重复状态
     * @param item 任务项
     * @param now 当前时间
     * @return 是否应触发
     */
    bool shouldTriggerOnce(const ScheduledTaskItem& item, const QDateTime& now);

    /**
     * @brief 判断循环任务是否应触发，并进行防抖控制
     * @param item 任务项
     * @param now 当前时间
     * @return 是否应触发
     */
    bool shouldTriggerLoop(const ScheduledTaskItem& item, const QDateTime& now);

    /**
     * @brief 发送 OSC 消息（可选择直接或入队列）
     * @param msg OSCMessage
     */
    void sendOsc(const OSCMessage& msg);

    /**
     * @brief 为任务生成稳定键，用于防重复/防抖标识
     * @param item 任务项
     * @return 字符串键
     */
    static QString keyForItem(const ScheduledTaskItem& item);

private:
    QTimer m_timer;
    ScheduledTaskModel* m_model = nullptr;
    int m_toleranceSec = 1;
    bool m_useQueue = true;
    // 循环任务（loop）最近一次触发时间
    QMap<QString, QDateTime> m_lastTriggerTime;
};