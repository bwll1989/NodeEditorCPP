//
// Created by WuBin on 24-10-29.
//
#include "QtNodes/NodeData"
#ifndef FLOATDATA_H
#define FLOATDATA_H
using QtNodes::NodeData;
using QtNodes::NodeDataType;
class FloatData:public NodeData{
public:
    FloatData():NodeValues(0.0){}
    NodeDataType type() const override { return NodeDataType{"Float", "float"}; }
    float NodeValues;
};
#endif //FLOATDATA_H
