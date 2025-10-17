#pragma once

#include <QAbstractListModel>
#include <QVector>
#include <QStringList>
#include <QDate>
#include <QDateTime>
#include <QJsonObject>
#include <QJsonArray>
#include <QJsonDocument>
#include "../../Common/Devices/OSCSender/OSCSender.h"

/**
 * @brief 调度信息结构体
 * type: "once" 或 "loop"
 * time: 计划时间（once 使用完整日期时间；loop 使用时间部分）
 * conditions: 循环的星期条件（如 Monday, Wednesday）
 */
struct ScheduledInfo {
    QString type = "once";
    QDateTime time;
    QStringList conditions;
};

/**
 * @brief 单个计划任务项，包含 OSC 消息与调度信息
 */
struct ScheduledTaskItem {
    OSCMessage osc;
    ScheduledInfo scheduled;
    QString remarks; // 备注
};

/**
 * @brief 计划任务数据模型（基于 QAbstractListModel）
 * - 提供列表模型接口（rowCount/data/setData/flags/roleNames）
 * - 保留原有业务 API（itemsForDate、toJson/fromJson、addItem/removeItem/setItems/items）
 */
class ScheduledTaskModel : public QAbstractListModel {
    Q_OBJECT
public:
    /**
     * @brief 模型角色枚举，供视图/委托访问
     */
    enum Roles {
        RoleHost = Qt::UserRole + 1,
        RolePort,
        RoleAddress,
        RoleType,
        RoleValue,
        RoleScheduleType,   // once / loop
        RoleDate,
        RoleTime,           // QTime（仅时间部分）
        RoleLoopDays,        // QStringList
        RoleRemarks         // 备注
    };

    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit ScheduledTaskModel(QObject* parent = nullptr);

    /**
     * @brief 行数
     */
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    /**
     * @brief 索引创建（单列列表模型）
     */
    QModelIndex index(int row, int column = 0, const QModelIndex& parent = QModelIndex()) const override;

    /**
     * @brief 父索引（列表模型返回无效索引）
     */
    QModelIndex parent(const QModelIndex& child) const override;

    /**
     * @brief 数据访问（DisplayRole 返回地址；其它角色见枚举）
     */
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    /**
     * @brief 数据写回（支持编辑）
     */
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;

    /**
     * @brief 项标志（支持选中与编辑）
     */
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    /**
     * @brief 角色名（调试/绑定使用）
     */
    QHash<int, QByteArray> roleNames() const override;

    /**
     * @brief 获取所有任务项（只读引用）
     */
    const QVector<ScheduledTaskItem>& items() const;

    /**
     * @brief 直接设置完整任务列表（触发视图重置）
     */
    void setItems(const QVector<ScheduledTaskItem>& items);

    /**
     * @brief 添加一个任务项（触发插入行）
     */
    void addItem(const ScheduledTaskItem& item);

    /**
     * @brief 删除指定索引的任务项（触发移除行）
     */
    void removeItem(int index);

    /**
     * @brief 根据日期筛选当天应显示的任务项
     * once: 任务日期 == 指定日期
     * loop: 指定日期的星期在 Conditions 中
     */
    QVector<ScheduledTaskItem> itemsForDate(const QDate& date) const;

    /**
     * @brief 序列化为 JSON（与示例一致）
     */
    QJsonObject toJson() const;

    /**
     * @brief 从 JSON 反序列化（重置模型数据）
     */
    void fromJson(const QJsonObject& json);

    /**
     * @brief 将 Qt 的 dayOfWeek(1..7) 转为英文星期名（Monday..Sunday）
     */
    static QString dayOfWeekToName(int qtDayOfWeek);

signals:
    /**
     * @brief 模型数据变更信号（新增/删除/设置/编辑）
     * 保留用于兼容现有连接
     */
    void modelChanged();
private:
    /**
     * @brief 序列化：OSCMessage -> JSON
     */
    static QJsonObject oscMessageToJson(const OSCMessage& message);

    /**
     * @brief 反序列化：JSON -> OSCMessage
     */
    static OSCMessage oscMessageFromJson(const QJsonObject& json);

    /**
     * @brief 序列化：ScheduledInfo -> JSON
     */
    static QJsonObject scheduledToJson(const ScheduledInfo& info);

    /**
     * @brief 反序列化：JSON -> ScheduledInfo
     */
    static ScheduledInfo scheduledFromJson(const QJsonObject& json);

private:
    QVector<ScheduledTaskItem> m_items;
};