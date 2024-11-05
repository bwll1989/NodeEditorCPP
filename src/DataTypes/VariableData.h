//
// Created by WuBin on 24-10-31.
//
#include "QtNodes/NodeData"
#include "QJsonObject"
#ifndef VARIBALEDATA_H
#define VARIBALEDATA_H
using QtNodes::NodeData;
using QtNodes::NodeDataType;
class VariableData : public NodeData
{
public:
    VariableData() : NodeValues(new QVariantMap()) {}
    explicit VariableData(QVariantMap *val) : NodeValues(val) {}
    explicit VariableData(const QJsonObject* val) {
        // 将 QJsonObject 转换为 QVariantMap
        for (const QString& key : val->keys()) {
            NodeValues->insert(key, val->value(key).toVariant());
        }
    }

    explicit VariableData(QVariant val) {
        NodeValues = new QVariantMap();
        NodeValues->insert("variable", val);

    }

    ~VariableData() {
        delete NodeValues;
    }
    void insert(const QString &key, const QVariant &value) {
        NodeValues->insert(key, value);
    }
    NodeDataType type() const override {
        if(isEmpty())
            return NodeDataType{"variable", "var"};
        return NodeDataType{"variable",value().typeName()};
    }



    bool isEmpty() const {
        if (NodeValues == nullptr || NodeValues->isEmpty()) {
            return true;
            }
        return false;
    }

    QVariant value(const QString key="variable") const {
        if (isEmpty()|| !NodeValues->contains(key)) {
            return QVariant();
        }
        return NodeValues->value(key);  // 确保返回 QVariant 类型
    }

    QVariantMap getMap() const {
        return *NodeValues;
    }
    std::unique_ptr<QJsonObject> json() const {
        auto jsonObject = std::make_unique<QJsonObject>();
        for (auto it = NodeValues->begin(); it != NodeValues->end(); ++it) {
            jsonObject->insert(it.key(), QJsonValue::fromVariant(it.value()));
        }

        if (isEmpty()) {
            jsonObject->insert("variable", QJsonValue::fromVariant(""));
        }

        return jsonObject;  // 返回智能指针，避免手动释放
    }
    private:
    QVariantMap *NodeValues;
};

#endif //VARIBALEDATA_H
