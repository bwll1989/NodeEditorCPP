//
// Created by WuBin on 24-10-29.
//
#include "QtNodes/NodeData"
#ifndef TEXTDATA_H
#define TEXTDATA_H
using QtNodes::NodeData;
using QtNodes::NodeDataType;
class TextData : public NodeData
{
public:
    TextData() {}

    TextData(QString const &text)
            : _text(text)
    {}

    NodeDataType type() const override { return NodeDataType{"text", "text"}; }

    QString text() const { return _text; }

    QString _text;
};
#endif //TEXTDATA_H
