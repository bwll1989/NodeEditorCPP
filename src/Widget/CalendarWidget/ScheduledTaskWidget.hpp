#pragma once

#include <QWidget>
#include <QVBoxLayout>
#include <QMap>
#include <QDate>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include "CalendarWidget.hpp"
#include "TaskListWidget.hpp"
#include "ScheduledTaskModel.hpp" // 新增：引入模型类型
#include "ScheduledTaskManager.hpp" // 新增：引入任务执行器类型

/**
 * @brief ScheduledTask 组合控件
 * 上部为 OscCalendarWidget，下部为 TaskListWidget。
 * 负责维护“日期 -> 任务列表（OSCMessage）”映射，并完成联动显示与保存/加载。
 */
class ScheduledTaskWidget : public QWidget {
    Q_OBJECT
public:
    /**
     * @brief 构造函数，初始化布局与信号连接
     * @param parent 父对象
     */
    explicit ScheduledTaskWidget(QWidget* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~ScheduledTaskWidget();

    /**
     * @brief 获取指定日期的任务列表
     * @param date 日期
     * @return 该日期的 OSCMessage 向量（从模型筛选转换）
     */
    QVector<OSCMessage> tasksForDate(const QDate& date) const;

    /**
     * @brief 设置指定日期的任务列表
     * 说明：本方法仅更新下方显示与信号，不直接写入模型数据。
     * 建议使用模型 API 管理数据。
     * @param date 日期
     * @param tasks 任务列表（OSCMessage）
     */
    void setTasksForDate(const QDate& date, const QVector<OSCMessage>& tasks);

    /**
     * @brief 添加任务到指定日期（若未指定，则添加到当前选中日期）
     * @param message OSCMessage 任务
     * @param date 可选日期（默认当前选中日期）
     */
    void addTask(const OSCMessage& message, const QDate& date = QDate());

    /**
     * @brief 保存整个日程为 JSON 对象（委托模型）
     * @return JSON 对象
     */
    QJsonObject save() const;

    /**
     * @brief 从 JSON 对象加载整个日程（委托模型）
     * @param json JSON 对象
     */
    void load(const QJsonObject& json);
    /**
     * @brief 获取任务列表控件的右键菜单操作列表
     * @return 操作列表（QAction*）
     */
    QList<QAction*> getActions();
signals:
    /**
     * @brief 日期任务变更信号
     * @param date 日期
     * @param tasks 新的任务列表
     */
    void dateTasksChanged(const QDate& date, const QVector<OSCMessage>& tasks);

private slots:
    /**
     * @brief 响应日历选择变化，更新下方 TaskListWidget 显示
     */
    void onCalendarSelectionChanged();

    /**
     * @brief 响应在日历上拖入 OSCMessage 的事件，将消息加入对应日期任务列表
     * @param date 拖入的目标日期
     * @param oscMessage OSCMessage
     */
    void onCalendarMessageDropped(const QDate& date, const OSCMessage& oscMessage);

private:
    /**
     * @brief 根据指定日期刷新 TaskListWidget 的显示内容
     * @param date 日期
     */
    void updateTaskListForDate(const QDate& date);

    /**
     * @brief 将 OSCMessage 转换为 JSON 对象（用于保存）
     * @param message OSCMessage
     * @return JSON 对象
     */
    static QJsonObject messageToJson(const OSCMessage& message);

    /**
     * @brief 从 JSON 对象构建 OSCMessage（用于加载）
     * @param json JSON 对象
     * @return OSCMessage
     */
    static OSCMessage jsonToMessage(const QJsonObject& json);

    /**
     * @brief 将模型任务项转换为消息列表（仅提取 OSCMessage）
     * @param items 模型的任务项
     * @return 消息列表
     */
    static QVector<OSCMessage> itemsToMessages(const QVector<ScheduledTaskItem>& items);

private:
    OscCalendarWidget* m_calendar = nullptr;
    TaskListWidget* m_taskList = nullptr;
    QVBoxLayout* m_layout = nullptr;
    // 使用模型替代内部的日期->消息映射
    ScheduledTaskModel* m_model = nullptr;
    // 新增：任务执行器（可选）
    ScheduledTaskManager* m_manager = nullptr;
};