#include "OSCMessageListModel.hpp"
#include <QtAlgorithms>

/**
 * 函数：OSCMessageListModel::OSCMessageListModel
 * 作用：构造空模型
 */
OSCMessageListModel::OSCMessageListModel(QObject* parent)
    : QAbstractListModel(parent)
{
}

/**
 * 函数：OSCMessageListModel::rowCount
 * 作用：返回当前消息数量
 */
int OSCMessageListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return m_messages.size();
}

/**
 * 函数：OSCMessageListModel::data
 * 作用：根据角色返回指定行的数据；DisplayRole 返回 address
 */
QVariant OSCMessageListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid()) return QVariant();
    int row = index.row();
    if (row < 0 || row >= m_messages.size()) return QVariant();

    const OSCMessage& msg = m_messages[row];
    switch (role) {
    case Qt::DisplayRole:  return msg.address;
    case HostRole:         return msg.host;
    case PortRole:         return msg.port;
    case AddressRole:      return msg.address;
    case TypeRole:         return msg.type;
    case ValueRole:        return msg.value;
    default:               return QVariant();
    }
}

/**
 * 函数：OSCMessageListModel::setData
 * 作用：根据角色更新指定行的数据，成功后发出 dataChanged
 */
bool OSCMessageListModel::setData(const QModelIndex& index, const QVariant& value, int role)
{
    if (!index.isValid()) return false;
    int row = index.row();
    if (row < 0 || row >= m_messages.size()) return false;

    OSCMessage& msg = m_messages[row];
    switch (role) {
    case HostRole:    msg.host    = value.toString();     break;
    case PortRole:    msg.port    = value.toInt();        break;
    case AddressRole: msg.address = value.toString();     break;
    case TypeRole:    msg.type    = value.toString();     break;
    case ValueRole:   msg.value   = value;                break;
    default:          return false;
    }

    emit dataChanged(index, index, { role });
    return true;
}

/**
 * 函数：OSCMessageListModel::flags
 * 作用：返回项的交互标志（可选择、可启用）
 */
Qt::ItemFlags OSCMessageListModel::flags(const QModelIndex& index) const
{
    Qt::ItemFlags base = Qt::ItemIsEnabled | Qt::ItemIsSelectable;
    if (index.isValid()) {
        return base;
    }
    return base;
}

/**
 * 函数：OSCMessageListModel::roleNames
 * 作用：返回角色名映射，便于 QML 和调试
 */
QHash<int, QByteArray> OSCMessageListModel::roleNames() const
{
    QHash<int, QByteArray> names;
    names[HostRole]    = "host";
    names[PortRole]    = "port";
    names[AddressRole] = "address";
    names[TypeRole]    = "type";
    names[ValueRole]   = "value";
    return names;
}

/**
 * 函数：OSCMessageListModel::insertRows
 * 作用：在指定位置插入 count 个默认消息
 */
bool OSCMessageListModel::insertRows(int row, int count, const QModelIndex& parent)
{
    if (parent.isValid() || count <= 0) return false;
    row = qBound(0, row, m_messages.size());

    beginInsertRows(QModelIndex(), row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        m_messages.insert(row, OSCMessage{});
    }
    endInsertRows();
    return true;
}

/**
 * 函数：OSCMessageListModel::removeRows
 * 作用：从指定位置删除 count 行
 */
bool OSCMessageListModel::removeRows(int row, int count, const QModelIndex& parent)
{
    if (parent.isValid() || count <= 0) return false;
    if (row < 0 || row + count > m_messages.size()) return false;

    beginRemoveRows(QModelIndex(), row, row + count - 1);
    for (int i = 0; i < count; ++i) {
        m_messages.removeAt(row);
    }
    endRemoveRows();
    return true;
}

/**
 * 函数：OSCMessageListModel::appendMessage
 * 作用：在末尾追加一条消息
 */
void OSCMessageListModel::appendMessage(const OSCMessage& message)
{
    insertMessage(m_messages.size(), message);
}

/**
 * 函数：OSCMessageListModel::insertMessage
 * 作用：在指定位置插入一条消息
 */
void OSCMessageListModel::insertMessage(int row, const OSCMessage& message)
{
    row = qBound(0, row, m_messages.size());
    beginInsertRows(QModelIndex(), row, row);
    m_messages.insert(row, message);
    endInsertRows();
}

/**
 * 函数：OSCMessageListModel::removeMessage
 * 作用：删除指定位置的消息
 */
bool OSCMessageListModel::removeMessage(int row)
{
    if (row < 0 || row >= m_messages.size()) return false;
    return removeRows(row, 1);
}

/**
 * 函数：OSCMessageListModel::clear
 * 作用：清空所有消息
 */
void OSCMessageListModel::clear()
{
    if (m_messages.isEmpty()) return;
    beginResetModel();
    m_messages.clear();
    endResetModel();
}

/**
 * 函数：OSCMessageListModel::messageAt
 * 作用：获取指定位置的消息（越界返回默认）
 */
OSCMessage OSCMessageListModel::messageAt(int row) const
{
    if (row < 0 || row >= m_messages.size()) return OSCMessage{};
    return m_messages[row];
}

/**
 * 函数：OSCMessageListModel::messages
 * 作用：获取全部消息副本
 */
QVector<OSCMessage> OSCMessageListModel::messages() const
{
    return m_messages;
}

/**
 * 函数：OSCMessageListModel::setMessages
 * 作用：批量设置消息（先清空再插入）
 */
void OSCMessageListModel::setMessages(const QVector<OSCMessage>& msgs)
{
    beginResetModel();
    m_messages = msgs;
    endResetModel();
}

/**
 * 函数：OSCMessageListModel::count
 * 作用：返回当前数量
 */
int OSCMessageListModel::count() const
{
    return m_messages.size();
}

/**
 * 函数：OSCMessageListModel::save
 * 作用：序列化到 JSON：{"messages":[{host,port,address,type,value}, ...]}
 */
QJsonObject OSCMessageListModel::save() const
{
    QJsonObject json;
    QJsonArray messagesArray;
    for (const auto& msg : m_messages) {
        messagesArray.append(messageToJson(msg));
    }
    json["messages"] = messagesArray;
    return json;
}

/**
 * 函数：OSCMessageListModel::load
 * 作用：从 JSON 反序列化：清空并重建
 */
void OSCMessageListModel::load(const QJsonObject& json)
{
    QVector<OSCMessage> newMsgs;
    const QJsonArray arr = json["messages"].toArray();
    newMsgs.reserve(arr.size());
    for (const QJsonValue& v : arr) {
        if (!v.isObject()) continue;
        newMsgs.push_back(jsonToMessage(v.toObject()));
    }
    setMessages(newMsgs);
}

/**
 * 函数：OSCMessageListModel::messageToJson
 * 作用：将 OSCMessage 转为 JSON 对象
 */
QJsonObject OSCMessageListModel::messageToJson(const OSCMessage& message)
{
    QJsonObject obj;
    obj["host"]    = message.host;
    obj["port"]    = message.port;
    obj["address"] = message.address;
    obj["type"]    = message.type;
    obj["value"]   = message.value.toString();
    return obj;
}

/**
 * 函数：OSCMessageListModel::jsonToMessage
 * 作用：从 JSON 对象解析 OSCMessage
 */
OSCMessage OSCMessageListModel::jsonToMessage(const QJsonObject& json)
{
    OSCMessage msg;
    msg.host    = json["host"].toString();
    msg.port    = json["port"].toInt();
    msg.address = json["address"].toString();
    msg.type    = json["type"].toString();
    msg.value   = json["value"].toString();
    return msg;
}