#include "ScheduledTaskModel.hpp"

/**
 * @brief 构造函数：加载初始示例数据
 */
ScheduledTaskModel::ScheduledTaskModel(QObject* parent)
    : QAbstractListModel(parent) {
    // 从 JSON 字符串导入初始任务数据（与原始示例一致）
//     fromJson(QJsonDocument::fromJson(R"(
// {
//    "tasks":
//    [
//     {
//         "osc message":
//         {
//             "address": "/ewqeq/ewq",
//             "host": "127.0.0.1",
//             "port": 8082,
//             "type": "Float",
//             "value": "1.0"
//         },
//         "remarks": "这是一个示例任务",
//         "scheduled":
//         {
//             "type":"loop",
//             "time":"2025/10/13 17:00:32",
//             "Conditions":[
//                 "Monday",
//                 "Wednesday"
//             ]
//         }
//     },
//     {
//         "osc message":
//         {
//             "address": "/ewqeq/ewq2",
//             "host": "127.0.0.1",
//             "port": 8080,
//             "type": "int",
//             "value": 2
//         },
//         "remarks": "这是一个示例任务2",
//         "scheduled":
//         {
//             "type":"once",
//             "time":"2025/10/13 17:00:32"
//         }
//     },
//     {
//         "osc message":
//         {
//             "address": "/ewqeq/ewq2",
//             "host": "127.0.0.1",
//             "port": 8080,
//             "type": "int",
//             "value": 2
//         },
//         "scheduled":
//         {
//             "type":"loop",
//             "time":"2025/10/13 17:00:32",
//             "Conditions":[
//
//             ]
//         }
//     }    ]
// }
// )").object());
}

/**
 * @brief 行数：返回任务项数量
 */
int ScheduledTaskModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return m_items.size();
}

/**
 * @brief 索引创建（单列列表）
 */
QModelIndex ScheduledTaskModel::index(int row, int column, const QModelIndex& parent) const {
    if (parent.isValid() || column != 0 || row < 0 || row >= m_items.size()) {
        return QModelIndex();
    }
    return createIndex(row, column);
}

/**
 * @brief 父索引：列表模型无层级，返回无效索引
 */
QModelIndex ScheduledTaskModel::parent(const QModelIndex& child) const {
    Q_UNUSED(child);
    return QModelIndex();
}

/**
 * @brief 数据访问（各角色）
 */
QVariant ScheduledTaskModel::data(const QModelIndex& idx, int role) const {
    if (!idx.isValid() || idx.row() < 0 || idx.row() >= m_items.size()) return {};
    const auto& it = m_items.at(idx.row());
    switch (role) {
    case Qt::DisplayRole:
        return it.osc.address;
    case RoleHost:
        return it.osc.host;
    case RolePort:
        return it.osc.port;
    case RoleAddress:
        return it.osc.address;
    case RoleType:
        return it.osc.type;
    case RoleValue:
        return it.osc.value.toString();
    case RoleDate:
        return it.scheduled.time.date();
    case RoleScheduleType:
        return it.scheduled.type;
    case RoleTime:
        return it.scheduled.time.time();
    case RoleLoopDays:
        return it.scheduled.conditions;
    case RoleRemarks:
        return it.remarks;
    default:
        return {};
    }
}

/**
 * @brief 数据写回（编辑）
 */
bool ScheduledTaskModel::setData(const QModelIndex& idx, const QVariant& value, int role) {
    if (!idx.isValid() || idx.row() < 0 || idx.row() >= m_items.size()) return false;
    auto& it = m_items[idx.row()];

    switch (role) {
    case RoleHost:
        it.osc.host = value.toString(); break;
    case RolePort:
        it.osc.port = value.toInt();  break;
    case RoleAddress:
        it.osc.address = value.toString();  break;
    case RoleType:
        it.osc.type = value.toString(); break;
    case RoleValue:
        it.osc.value = value; break;
    case RoleScheduleType:
        it.scheduled.type = value.toString(); break;
    case RoleDate: {
        const QDate newDate = value.toDate();
        const QTime oldTime = it.scheduled.time.time();
        it.scheduled.time = QDateTime(newDate, oldTime.isValid() ? oldTime : QTime(0, 0, 0));
        break;
    }
    case RoleTime: {
        // 仅替换时间部分，保留原日期部分
        const QTime t = value.toTime();
        const QDate d = it.scheduled.time.date().isValid() ? it.scheduled.time.date() : QDate::currentDate();
        it.scheduled.time = QDateTime(d, t.isValid() ? t : QTime(0,0,0));
        break;
    }
    case RoleLoopDays:
        it.scheduled.conditions = value.toStringList(); break;
    case RoleRemarks:
        it.remarks = value.toString(); break;
    default:
        break;
    }
    return false;
}

/**
 * @brief 标志位：允许选中与编辑
 */
Qt::ItemFlags ScheduledTaskModel::flags(const QModelIndex& index) const {
    if (!index.isValid()) return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

/**
 * @brief 角色名映射
 */
QHash<int, QByteArray> ScheduledTaskModel::roleNames() const {
    QHash<int, QByteArray> names;
    names[RoleHost] = "host";
    names[RolePort] = "port";
    names[RoleAddress] = "address";
    names[RoleType] = "type";
    names[RoleValue] = "value";
    names[RoleScheduleType] = "scheduleType";
    names[RoleTime] = "time";
    names[RoleLoopDays] = "loopDays";
    names[RoleDate] = "date";
    names[RoleRemarks] = "remarks";
    return names;
}

/**
 * @brief 获取所有任务项（只读引用）
 */
const QVector<ScheduledTaskItem>& ScheduledTaskModel::items() const {
    return m_items;
}

/**
 * @brief 设置完整任务列表（触发模型重置）
 */
void ScheduledTaskModel::setItems(const QVector<ScheduledTaskItem>& items) {
    beginResetModel();
    m_items = items;
    endResetModel();
    emit modelChanged();
}

/**
 * @brief 添加一个任务项（触发插入行）
 */
void ScheduledTaskModel::addItem(const ScheduledTaskItem& item) {
    const int pos = m_items.size();
    beginInsertRows(QModelIndex(), pos, pos);
    m_items.push_back(item);
    endInsertRows();
    emit modelChanged();
}

/**
 * @brief 删除指定索引的任务项（触发移除行）
 */
void ScheduledTaskModel::removeItem(int index) {
    if (index < 0 || index >= m_items.size()) return;
    beginRemoveRows(QModelIndex(), index, index);
    m_items.remove(index);
    endRemoveRows();
    emit modelChanged();
}

/**
 * @brief 根据日期筛选当天的任务项
 */
QVector<ScheduledTaskItem> ScheduledTaskModel::itemsForDate(const QDate& date) const {
    QVector<ScheduledTaskItem> result;
    const int qtDow = date.dayOfWeek(); // 1=Monday ... 7=Sunday
    const QString dowName = dayOfWeekToName(qtDow);

    for (const auto& item : m_items) {
        if (item.scheduled.type.compare("once", Qt::CaseInsensitive) == 0) {
            if (item.scheduled.time.date() == date) {
                result.push_back(item);
            }
        } else if (item.scheduled.type.compare("loop", Qt::CaseInsensitive) == 0) {
            if (item.scheduled.conditions.contains(dowName, Qt::CaseInsensitive)) {
                result.push_back(item);
            }
        }
    }
    return result;
}

/**
 * @brief 序列化为 JSON
 */
QJsonObject ScheduledTaskModel::toJson() const {
    QJsonObject root;
    QJsonArray tasksArr;
    for (const auto& item : m_items) {
        QJsonObject obj;
        obj.insert("osc message", oscMessageToJson(item.osc));
        obj.insert("scheduled", scheduledToJson(item.scheduled));
        obj.insert("remarks", item.remarks);
        tasksArr.append(obj);
    }
    root.insert("tasks", tasksArr);
    return root;
}

/**
 * @brief 从 JSON 反序列化（重置模型数据）
 */
void ScheduledTaskModel::fromJson(const QJsonObject& json) {
    QVector<ScheduledTaskItem> newItems;
    QJsonArray arr = json["tasks"].toArray();
    newItems.reserve(arr.size());
    for (const auto& v : arr) {
        if (!v.isObject()) continue;
        QJsonObject obj = v.toObject();
        ScheduledTaskItem item;
        item.osc = oscMessageFromJson(obj["osc message"].toObject());
        item.scheduled = scheduledFromJson(obj["scheduled"].toObject());
        item.remarks = obj["remarks"].toString();
        newItems.push_back(item);
    }
    setItems(newItems);
}

/**
 * @brief OSCMessage -> JSON
 */
QJsonObject ScheduledTaskModel::oscMessageToJson(const OSCMessage& message) {
    QJsonObject obj;
    obj["address"] = message.address;
    obj["host"] = message.host;
    obj["port"] = message.port;
    obj["type"] = message.type;
    obj["value"] = message.value.toString();
    return obj;
}

/**
 * @brief JSON -> OSCMessage
 */
OSCMessage ScheduledTaskModel::oscMessageFromJson(const QJsonObject& json) {
    OSCMessage m;
    m.address = json["address"].toString();
    m.host = json["host"].toString("127.0.0.1");
    m.port = json["port"].toInt(6001);
    m.type = json["type"].toString("String");
    m.value = json["value"].toVariant();
    return m;
}

/**
 * @brief ScheduledInfo -> JSON
 */
QJsonObject ScheduledTaskModel::scheduledToJson(const ScheduledInfo& info) {
    QJsonObject obj;
    obj["type"] = info.type;
    obj["time"] = info.time.toString("yyyy/MM/dd HH:mm:ss");
    QJsonArray conds;
    for (const auto& c : info.conditions) conds.append(c);
    obj["Conditions"] = conds;
    return obj;
}

/**
 * @brief JSON -> ScheduledInfo
 */
ScheduledInfo ScheduledTaskModel::scheduledFromJson(const QJsonObject& json) {
    ScheduledInfo info;
    info.type = json["type"].toString("once");
    info.time = QDateTime::fromString(json["time"].toString(), "yyyy/MM/dd HH:mm:ss");
    info.conditions.clear();
    QJsonArray conds = json["Conditions"].toArray();
    for (const auto& v : conds) {
        if (v.isString()) info.conditions.append(v.toString());
    }
    return info;
}

/**
 * @brief 将 Qt 周数字转换为英文星期名
 */
QString ScheduledTaskModel::dayOfWeekToName(int qtDayOfWeek) {
    switch (qtDayOfWeek) {
        case 1: return "Monday";
        case 2: return "Tuesday";
        case 3: return "Wednesday";
        case 4: return "Thursday";
        case 5: return "Friday";
        case 6: return "Saturday";
        case 7: return "Sunday";
        default: return "";
    }
}