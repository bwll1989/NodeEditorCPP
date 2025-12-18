//
// Created by WuBin on 24-10-31.
//
#include "VariableData.h"
using namespace NodeDataTypes;
VariableData::VariableData() : NodeValues() {}

VariableData::VariableData(QVariantMap* val) : NodeValues(*val) {}

VariableData::VariableData(const QVariantMap &val) : NodeValues(val) {}

VariableData::VariableData(const QJsonObject* val) {
    for (const QString& key : val->keys()) {
        NodeValues.insert(key, val->value(key).toVariant());
    }
}

VariableData::VariableData(const QVariant &val) {
    NodeValues.insert("default", val);
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

QVariantMap VariableData::getMap() const {
    return NodeValues;
}

std::unique_ptr<QJsonObject> VariableData::json() const {
    auto jsonObject = std::make_unique<QJsonObject>();
    for (auto it = NodeValues.begin(); it != NodeValues.end(); ++it) {
        jsonObject->insert(it.key(), QJsonValue::fromVariant(it.value()));
    }

    if (isEmpty()) {
        jsonObject->insert("default", QJsonValue::fromVariant(""));
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
