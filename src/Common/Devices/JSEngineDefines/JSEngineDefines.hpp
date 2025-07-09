#pragma once

#include <QtCore/QObject>
#include <QtQml/QJSEngine>
#include <QtQml/QJSValue>
#include <QtQml/QJSValueIterator>
#include <QVariantMap>
#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QVariant>
#include <QString>
namespace JSEngineDefines {

/**
 * @brief 将QVariantMap转换为JavaScript对象
 * @param map 变体映射
 * @return JavaScript对象
 */
static QJSValue variantMapToJSValue(QJSEngine *m_jsEngine,const QVariantMap& map) {
    QJSValue obj = m_jsEngine->newObject();

    for (auto it = map.begin(); it != map.end(); ++it) {
        obj.setProperty(it.key(), m_jsEngine->toScriptValue(it.value()));
    }

    return obj;
}

/**
 * @brief 将JavaScript对象转换为QVariantMap
 * @param value JavaScript对象
 * @return 变体映射
 */
static QVariantMap jsValueToVariantMap(const QJSValue& value) {
    QVariantMap result;

    if (value.isObject()) {
        QJSValueIterator it(value);
        while (it.hasNext()) {
            it.next();
            if (!it.name().isEmpty() && !it.value().isCallable()) {
                result[it.name()] = it.value().toVariant();
            }
        }
    } else {
        // 如果不是对象，则使用默认键存储值
        result["default"] = value.toVariant();
    }

    return result;
}
}