#pragma once

#include <QJsonArray>
#include <QJsonObject>
#include <QJsonValue>
#include <QSet>
#include <QString>

namespace Flow {

/**
 * @brief 网页动作库：以 OSC entity（地址）为主键；运行时读写仍走 StatusContainer。
 */
class ActionRegistry {
public:
    void clear();

    void load(const QJsonArray& actions);
    QJsonArray save() const;

    QJsonArray all() const { return _actions; }

    /**
     * @brief 注册或更新动作（主键 entity）
     * @return entity；无效时返回空字符串
     */
    QString addOrUpdate(const QJsonObject& binding);

    bool markUsed(const QString& entity, bool used = true);

    /** 允许修改 name / icon / access（网页可改 icon） */
    bool patch(const QString& entity, const QJsonObject& patch);

    bool remove(const QString& entity);

    QJsonObject findByEntity(const QString& entity) const;

    /** 将 key 解析为 entity（兼容旧数据中的 id 字段） */
    QString resolveEntityKey(const QString& key) const;

    void syncUsedFromLayout(const QJsonObject& layout);

private:
    QJsonArray _actions;

    static QString displayName(const QJsonObject& action);
    static QString normalizeAccess(const QString& access);
    static QSet<QString> collectLayoutReferences(const QJsonValue& value);
    static void walkLayoutValue(const QJsonValue& value, QSet<QString>& entities);

    int indexOfEntity(const QString& entity) const;
    int indexOfLegacyId(const QString& key) const;
};

} // namespace Flow
