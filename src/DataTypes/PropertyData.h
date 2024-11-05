//
// Created by WuBin on 24-10-31.
//
#include "QtNodes/NodeData"
#include "QJsonObject"
#ifndef PROPPERTYDATA_H
#define PROPPERTYDATA_H
using QtNodes::NodeData;
using QtNodes::NodeDataType;
class PropertyData : public NodeData
{
public:
    PropertyData() : NodeValues(nullptr) {}
    explicit PropertyData(QJsonObject *val) : NodeValues(val) {}
    explicit PropertyData(QVariant val) {
        NodeValues = new QJsonObject();  // 初始化 NodeValues 指向一个新对象
        if (&val) {
            NodeValues->insert("value", QJsonValue::fromVariant(val));  // 确保 val 不为空
        }

    }
    ~PropertyData() {
        delete NodeValues;  // 确保指针被正确释放，避免内存泄漏
    }
    NodeDataType type() const override { return NodeDataType{"Property", "info"}; }

    QJsonObject *NodeValues;

    bool isEmpty() const {
        if (NodeValues == nullptr||NodeValues->empty())
            return true;
        return false;
    }
};

#endif //PROPPERTYDATA_H
