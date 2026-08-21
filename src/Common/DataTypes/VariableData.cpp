//
// Created by WuBin on 24-10-31.
//
#include "VariableData.h"

#include "DataConvert.hpp"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonParseError>

using namespace NodeDataTypes;

namespace {

bool variantIsList(const QVariant &v)
{
    const int id = v.typeId();
    return id == QMetaType::QVariantList || id == QMetaType::QStringList;
}

} // namespace

VariableData::VariableData() : NodeValues() {}

VariableData::VariableData(QVariantMap* val) : NodeValues(*val) {}

VariableData::VariableData(const QVariantMap &val) : NodeValues(val) {}

VariableData::VariableData(const QJsonObject* val) {
    for (const QString& key : val->keys()) {
        NodeValues.insert(key, val->value(key).toVariant());
    }
}

VariableData::VariableData(const QVariant &val) {
    NodeValues.insert(QStringLiteral("default"), val);
}

void VariableData::insert(const QString &key, const QVariant &value) {
    NodeValues.insert(key, value);
}

NodeDataType VariableData::type() const {
    return isEmpty()
        ? NodeDataType{"default", "Info"}
    : NodeDataType{"default", value().typeName()};
}

bool VariableData::hasKey(const QString &key) const {
    return NodeValues.contains(key);
}

bool VariableData::isEmpty() const {
    return NodeValues.isEmpty();
}

QVariant VariableData::value(const QString &key ) const {
    return hasKey(key) ? NodeValues.value(key) : QVariant();
}

QVariantMap VariableData::asMap() const {
    return NodeValues;
}

std::unique_ptr<QJsonObject> VariableData::json() const {
    auto jsonObject = std::make_unique<QJsonObject>();
    for (auto it = NodeValues.begin(); it != NodeValues.end(); ++it) {
        jsonObject->insert(it.key(), QJsonValue::fromVariant(it.value()));
    }

    if (isEmpty()) {
        jsonObject->insert(QStringLiteral("default"), QJsonValue::fromVariant(QString()));
    }

    return jsonObject;
}

QString VariableData::toJsonString() const {
    QJsonObject jsonObject;
    for (auto it = NodeValues.begin(); it != NodeValues.end(); ++it) {
        jsonObject.insert(it.key(), QJsonValue::fromVariant(it.value()));
    }
    QJsonDocument doc(jsonObject);
    return doc.toJson(QJsonDocument::Indented);
}

QStringList VariableData::keys() const {
    return NodeValues.keys();
}

bool VariableData::isList(const QString &key) const
{
    return variantIsList(value(key));
}

double VariableData::asNumber(const QString &key, double fallback) const
{
    const QVector<float> v = asFloats(0, key);
    if (v.isEmpty()) {
        return fallback;
    }
    return double(v.at(0));
}

int VariableData::asInt(const QString &key, int fallback) const
{
    const QVariant v = value(key);
    if (!v.isValid()) {
        return fallback;
    }
    // 纯整型/可直接 toInt 的优先走整数路径，避免 double 往返
    if (!variantIsList(v) && v.canConvert<int>()
        && (v.typeId() == QMetaType::Int || v.typeId() == QMetaType::UInt
            || v.typeId() == QMetaType::LongLong || v.typeId() == QMetaType::ULongLong
            || v.typeId() == QMetaType::Bool)) {
        return v.toInt();
    }
    return int(asNumber(key, double(fallback)));
}

bool VariableData::asBool(const QString &key, bool fallback) const
{
    const QVariant v = value(key);
    if (!v.isValid()) {
        return fallback;
    }
    if (variantIsList(v)) {
        const QVariantList list = v.toList();
        if (list.isEmpty()) {
            return fallback;
        }
        return list.first().toBool();
    }
    return v.toBool();
}

QVector<float> VariableData::asFloats(int size, const QString &key) const
{
    return floatVectorFromVariant(value(key), size);
}

QVariantList VariableData::asList(const QString &key) const
{
    const QVariant v = value(key);
    if (variantIsList(v)) {
        return v.toList();
    }
    if (!v.isValid()) {
        return {};
    }
    return QVariantList{v};
}

QString VariableData::asString(const QString &key) const
{
    return asDisplay(key);
}

QString VariableData::asDisplay(const QString &key) const
{
    const QVariant v = value(key);
    if (variantIsList(v)) {
        return QString::fromUtf8(
            QJsonDocument(QJsonArray::fromVariantList(v.toList()))
                .toJson(QJsonDocument::Compact));
    }
    return v.toString();
}

QVariant VariableData::fromDisplay(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return QVariant();
    }

    if (trimmed.startsWith(QLatin1Char('['))) {
        QJsonParseError err{};
        const QJsonDocument doc = QJsonDocument::fromJson(trimmed.toUtf8(), &err);
        if (err.error == QJsonParseError::NoError && doc.isArray()) {
            return doc.array().toVariantList();
        }
    }

    if (trimmed.compare(QStringLiteral("true"), Qt::CaseInsensitive) == 0) {
        return true;
    }
    if (trimmed.compare(QStringLiteral("false"), Qt::CaseInsensitive) == 0) {
        return false;
    }

    bool ok = false;
    const int i = trimmed.toInt(&ok);
    if (ok) {
        // 避免 "1.0" 被当成 int；带小数点走 double
        if (!trimmed.contains(QLatin1Char('.')) && !trimmed.contains(QLatin1Char('e'))
            && !trimmed.contains(QLatin1Char('E'))) {
            return i;
        }
    }
    const double d = trimmed.toDouble(&ok);
    if (ok) {
        return d;
    }

    return trimmed;
}
