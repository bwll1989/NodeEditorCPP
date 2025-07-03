//
// Created by WuBin on 24-10-31.
//
#include "QtNodes/NodeData"
#include "QJsonObject"
#ifndef VARIBALEDATA_H
#define VARIBALEDATA_H
using QtNodes::NodeData;
using QtNodes::NodeDataType;
namespace NodeDataTypes
{
    class VariableData : public NodeData {
    public:
        VariableData() : NodeValues() {}

        explicit VariableData(QVariantMap* val) : NodeValues(*val) {}

        explicit VariableData(const QVariantMap &val) : NodeValues(val) {}

        explicit VariableData(const QJsonObject* val) {
            for (const QString& key : val->keys()) {
                NodeValues.insert(key, val->value(key).toVariant());
            }
        }

        explicit VariableData(const QVariant &val) {
            NodeValues.insert("default", val);
        }

        void insert(const QString &key, const QVariant &value) {
            NodeValues.insert(key, value);
        }

        NodeDataType type() const override {
            return isEmpty()
                ? NodeDataType{"default", "Info"}
            : NodeDataType{"default", value().typeName()};
        }

        bool hasKey(const QString &key) const {
            return NodeValues.contains(key);
        }

        bool isEmpty() const {
            return NodeValues.isEmpty();
        }

        QVariant value(const QString &key = "default") const {
            return hasKey(key) ? NodeValues.value(key) : QVariant();
        }

        QVariantMap getMap() const {
            return NodeValues;
        }

        std::unique_ptr<QJsonObject> json() const {
            auto jsonObject = std::make_unique<QJsonObject>();
            for (auto it = NodeValues.begin(); it != NodeValues.end(); ++it) {
                jsonObject->insert(it.key(), QJsonValue::fromVariant(it.value()));
            }

            if (isEmpty()) {
                jsonObject->insert("default", QJsonValue::fromVariant(""));
            }

            return jsonObject;
        }

    private:
        QVariantMap NodeValues;
    };
}
#endif //VARIBALEDATA_H
