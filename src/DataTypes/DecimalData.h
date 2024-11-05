//
// Created by WuBin on 24-10-29.
//

#ifndef DECIMALDATA_H
#define DECIMALDATA_H
class DecimalData : public NodeData
{
public:
    DecimalData()
        : _number(0.0)
    {}

    DecimalData(double const number)
        : _number(number)
    {}

    NodeDataType type() const override { return NodeDataType{"decimal", "decimal"}; }

    double number() const { return _number; }

    QString numberAsText() const { return QString::number(_number, 'f'); }

private:
    double _number;
};
#endif //DECIMALDATA_H
