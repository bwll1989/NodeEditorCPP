#pragma once

#include <QAbstractListModel>
#include <QVector>
#include <QJsonObject>
#include <QJsonArray>
#include "../../Common/Devices/OSCSender/OSCSender.h"

#if defined(OSCLISTWIDGET_LIBRARY)
#define OSCLISTWIDGET_EXPORT Q_DECL_EXPORT
#else
#define OSCLISTWIDGET_EXPORT Q_DECL_IMPORT
#endif

/**
 * 类：OSCMessageListModel
 * 作用：基于 QAbstractListModel 的 OSC 消息列表模型，集中管理 OSCMessage，
 *      提供插入/删除/清空、角色访问与 JSON 序列化/反序列化能力。
 */
class OSCLISTWIDGET_EXPORT OSCMessageListModel : public QAbstractListModel {
    Q_OBJECT
public:
    /**
     * 构造函数
     * 初始化空的消息列表。
     */
    explicit OSCMessageListModel(QObject* parent = nullptr);

    /**
     * 数据角色枚举（UserRole 起）
     * 可用于通过 data()/setData() 访问各个字段。
     */
    enum Roles : int {
        HostRole    = Qt::UserRole + 1,
        PortRole,
        AddressRole,
        TypeRole,
        ValueRole
    };

    /**
     * 行数：返回当前消息数量
     */
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    /**
     * 数据读取：根据角色返回指定行的字段
     * DisplayRole：返回 address 字段作为默认显示文本
     */
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;

    /**
     * 数据写入：根据角色更新指定行的字段
     * 成功写入后发出 dataChanged
     */
    bool setData(const QModelIndex& index, const QVariant& value, int role) override;

    /**
     * 项标志：可选择/可启用
     */
    Qt::ItemFlags flags(const QModelIndex& index) const override;

    /**
     * 角色名映射：供 QML 或调试使用
     */
    QHash<int, QByteArray> roleNames() const override;

    /**
     * 插入行：在 row 位置插入 count 个默认消息
     */
    bool insertRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

    /**
     * 删除行：从 row 开始删除 count 行
     */
    bool removeRows(int row, int count, const QModelIndex& parent = QModelIndex()) override;

    /**
     * 追加一条消息到末尾
     */
    void appendMessage(const OSCMessage& message);

    /**
     * 在指定位置插入一条消息
     */
    void insertMessage(int row, const OSCMessage& message);

    /**
     * 删除指定位置的消息
     */
    bool removeMessage(int row);

    /**
     * 清空所有消息
     */
    void clear();

    /**
     * 获取指定位置的消息（越界返回默认值）
     */
    OSCMessage messageAt(int row) const;

    /**
     * 获取全部消息副本
     */
    QVector<OSCMessage> messages() const;

    /**
     * 批量设置消息（先清空再插入）
     */
    void setMessages(const QVector<OSCMessage>& msgs);

    /**
     * 获取当前数量
     */
    int count() const;

    /**
     * 序列化到 JSON：结构与现有保持一致 {"messages":[...]}
     */
    QJsonObject save() const;

    /**
     * 从 JSON 反序列化：清空并重建
     */
    void load(const QJsonObject& json);

private:
    /**
     * 将单个 OSCMessage 转为 JSON 对象
     */
    static QJsonObject messageToJson(const OSCMessage& message);

    /**
     * 从 JSON 对象解析单个 OSCMessage
     */
    static OSCMessage jsonToMessage(const QJsonObject& json);

private:
    QVector<OSCMessage> m_messages;
};