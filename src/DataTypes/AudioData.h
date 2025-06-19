//
// Created by WuBin on 24-10-31.
//
#include "QtNodes/NodeData"

#ifndef AUDIONODEDATA_H
#define AUDIONODEDATA_H
using QtNodes::NodeData;
using QtNodes::NodeDataType;
class AudioData : public NodeData
{
public:
    AudioData():NodeValues({"a1","b1"}){}
    NodeDataType type() const override { return NodeDataType{"Audio", "audio"}; }
    struct Data
    {
        QString path;
        QString name;
        bool play;
        float gain;
    };
    Data NodeValues;
};
#endif //AUDIONODEDATA_H
