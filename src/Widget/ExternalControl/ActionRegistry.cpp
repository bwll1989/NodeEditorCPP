#include "ActionRegistry.h"

#include <QDateTime>

namespace Flow {

namespace {

QString jsonString(const QJsonObject& obj, const QString& key)
{
    return obj.value(key).toString().trimmed();
}

} // namespace

void ActionRegistry::clear()
{
    _actions = QJsonArray();
}

void ActionRegistry::load(const QJsonArray& actions)
{
    _actions = actions;
}

QJsonArray ActionRegistry::save() const
{
    return _actions;
}

QString ActionRegistry::normalizeAccess(const QString& access)
{
    const QString v = access.trimmed().toLower();
    if (v == QStringLiteral("read") || v == QStringLiteral("readonly") || v == QStringLiteral("r")) {
        return QStringLiteral("read");
    }
    if (v == QStringLiteral("write") || v == QStringLiteral("writeonly") || v == QStringLiteral("w")) {
        return QStringLiteral("write");
    }
    return QStringLiteral("readwrite");
}

QString ActionRegistry::displayName(const QJsonObject& action)
{
    const QString name = jsonString(action, QStringLiteral("name"));
    if (!name.isEmpty()) {
        return name;
    }
    const QString suggested = jsonString(action, QStringLiteral("suggestedName"));
    if (!suggested.isEmpty()) {
        return suggested;
    }
    return jsonString(action, QStringLiteral("entity"));
}

QString ActionRegistry::addOrUpdate(const QJsonObject& binding)
{
    const QString entity = jsonString(binding, QStringLiteral("entity"));
    if (entity.isEmpty()) {
        return {};
    }

    const qint64 createdAt = binding.contains(QStringLiteral("createdAt"))
        ? binding.value(QStringLiteral("createdAt")).toInteger()
        : QDateTime::currentSecsSinceEpoch();

    QJsonObject item = binding;
    item.remove(QStringLiteral("id"));
    item[QStringLiteral("entity")] = entity;
    item[QStringLiteral("access")] = normalizeAccess(
        jsonString(binding, QStringLiteral("access")).isEmpty()
            ? QStringLiteral("readwrite")
            : jsonString(binding, QStringLiteral("access")));

    const QString name = displayName(item);
    if (!name.isEmpty()) {
        item[QStringLiteral("name")] = name;
        item[QStringLiteral("suggestedName")] = name;
    }

    const int index = indexOfEntity(entity);
    if (index >= 0) {
        QJsonObject existing = _actions.at(index).toObject();
        item[QStringLiteral("used")] = existing.value(QStringLiteral("used")).toBool(false);
        item[QStringLiteral("createdAt")] = existing.value(QStringLiteral("createdAt")).toInteger(createdAt);
        if (jsonString(binding, QStringLiteral("icon")).isEmpty()) {
            item[QStringLiteral("icon")] = existing.value(QStringLiteral("icon")).toString();
        }
        _actions.replace(index, item);
        return entity;
    }

    item[QStringLiteral("used")] = binding.value(QStringLiteral("used")).toBool(false);
    if (!item.contains(QStringLiteral("createdAt"))) {
        item[QStringLiteral("createdAt")] = createdAt;
    }
    _actions.append(item);
    return entity;
}

bool ActionRegistry::markUsed(const QString& entity, bool used)
{
    const QString key = resolveEntityKey(entity);
    const int index = indexOfEntity(key);
    if (index < 0) {
        return false;
    }
    QJsonObject obj = _actions.at(index).toObject();
    obj[QStringLiteral("used")] = used;
    _actions.replace(index, obj);
    return true;
}

bool ActionRegistry::patch(const QString& entity, const QJsonObject& patch)
{
    const QString key = resolveEntityKey(entity);
    const int index = indexOfEntity(key);
    if (index < 0) {
        return false;
    }

    QJsonObject obj = _actions.at(index).toObject();
    if (patch.contains(QStringLiteral("name"))) {
        const QString name = patch.value(QStringLiteral("name")).toString().trimmed();
        obj[QStringLiteral("name")] = name;
        obj[QStringLiteral("suggestedName")] = name;
    }
    if (patch.contains(QStringLiteral("icon"))) {
        obj[QStringLiteral("icon")] = patch.value(QStringLiteral("icon")).toString();
    }
    if (patch.contains(QStringLiteral("access"))) {
        obj[QStringLiteral("access")] = normalizeAccess(patch.value(QStringLiteral("access")).toString());
    }
    _actions.replace(index, obj);
    return true;
}

bool ActionRegistry::remove(const QString& entity)
{
    const QString key = resolveEntityKey(entity);
    const int index = indexOfEntity(key);
    if (index < 0) {
        return false;
    }
    _actions.removeAt(index);
    return true;
}

QJsonObject ActionRegistry::findByEntity(const QString& entity) const
{
    const QString key = resolveEntityKey(entity);
    const int index = indexOfEntity(key);
    if (index < 0) {
        return {};
    }
    return _actions.at(index).toObject();
}

QString ActionRegistry::resolveEntityKey(const QString& key) const
{
    if (key.isEmpty()) {
        return {};
    }
    if (indexOfEntity(key) >= 0) {
        return key;
    }
    const int legacy = indexOfLegacyId(key);
    if (legacy >= 0) {
        return _actions.at(legacy).toObject().value(QStringLiteral("entity")).toString();
    }
    return key;
}

void ActionRegistry::walkLayoutValue(const QJsonValue& value, QSet<QString>& entities)
{
    if (value.isObject()) {
        const QJsonObject obj = value.toObject();
        const QString entity = jsonString(obj, QStringLiteral("entity"));
        if (!entity.isEmpty()) {
            entities.insert(entity);
        }
        for (auto it = obj.constBegin(); it != obj.constEnd(); ++it) {
            walkLayoutValue(it.value(), entities);
        }
        return;
    }

    if (value.isArray()) {
        for (const auto& item : value.toArray()) {
            walkLayoutValue(item, entities);
        }
    }
}

QSet<QString> ActionRegistry::collectLayoutReferences(const QJsonValue& value)
{
    QSet<QString> entities;
    walkLayoutValue(value, entities);
    return entities;
}

void ActionRegistry::syncUsedFromLayout(const QJsonObject& layout)
{
    const QSet<QString> referenced = collectLayoutReferences(layout);

    for (int i = 0; i < _actions.size(); ++i) {
        if (!_actions.at(i).isObject()) {
            continue;
        }
        QJsonObject obj = _actions.at(i).toObject();
        const QString entity = jsonString(obj, QStringLiteral("entity"));
        obj[QStringLiteral("used")] = referenced.contains(entity);
        _actions.replace(i, obj);
    }
}

int ActionRegistry::indexOfEntity(const QString& entity) const
{
    if (entity.isEmpty()) {
        return -1;
    }
    for (int i = 0; i < _actions.size(); ++i) {
        if (!_actions.at(i).isObject()) {
            continue;
        }
        if (_actions.at(i).toObject().value(QStringLiteral("entity")).toString() == entity) {
            return i;
        }
    }
    return -1;
}

int ActionRegistry::indexOfLegacyId(const QString& key) const
{
    if (key.isEmpty()) {
        return -1;
    }
    for (int i = 0; i < _actions.size(); ++i) {
        if (!_actions.at(i).isObject()) {
            continue;
        }
        const QJsonObject obj = _actions.at(i).toObject();
        if (obj.value(QStringLiteral("id")).toString() == key) {
            return i;
        }
    }
    return -1;
}

} // namespace Flow
