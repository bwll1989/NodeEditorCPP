#pragma once

#include "QtNodes/NodeData"
#include <QtGui/QPixmap>
#include <QWaitCondition>
#include "QtCore/QVariant"
#include "QQueue"
using QtNodes::NodeData;
using QtNodes::NodeDataType;

/// The class can potentially incapsulate any user data which
/// need to be transferred within the Node Editor graph
class DecimalData : public NodeData
{
public:
    DecimalData()
        : _number(0.0)
    {}

    DecimalData(double const number)
        : _number(number)
    {}

    NodeDataType type() const override { return NodeDataType{"decimal", ""}; }

    double number() const { return _number; }

    QString numberAsText() const { return QString::number(_number, 'f'); }

private:
    double _number;
};

class ProjectorNodeData : public NodeData
{
public:
    struct ProjectorData
    {
        QString path;
        QString name;
    };
    ProjectorData NodeValues;
    ProjectorNodeData():NodeValues({"a1","b1"}){}
    NodeDataType type() const override { return NodeDataType{"Projector", ""}; }
};

class ClientNodeData : public NodeData
{
public:
    ClientNodeData():NodeValues({"a1","b1"}){}
    NodeDataType type() const override { return NodeDataType{"Client", ""}; }
    struct ClientData
    {
        QString path;
        QString name;
    };
    ClientData NodeValues;
};

class ProjectNodeData : public NodeData
{
public:

    ProjectNodeData(): NodeValues({"Project_path","Project_name"}){}
    QString type_name="项目";
    NodeDataType type() const override { return NodeDataType{"Project", ""}; }
    struct ProjectData
    {
        QString path;
        QString name;
    };
    ProjectData NodeValues;

private:

};

class ScreenNodeData : public NodeData
{
public:
    struct ScreenData
    {
        QString path;
        QString name;
    };
    ScreenData NodeValues;
    ScreenNodeData():NodeValues({"a1","b1"}){}

    NodeDataType type() const override { return NodeDataType{"Screen", ""}; }
};

class TextData : public NodeData
{
public:
    TextData() {}

    TextData(QString const &text)
            : _text(text)
    {}

    NodeDataType type() const override { return NodeDataType{"text", ""}; }

    QString text() const { return _text; }

    QString _text;
};

class PixmapData : public NodeData
{
public:
    PixmapData() {}

    PixmapData(QPixmap const &pixmap)
            : _pixmap(pixmap)
    {}

    NodeDataType type() const override
    {
        //       id      name
        return {"pixmap", "P"};
    }

    QPixmap pixmap() const { return _pixmap; }

private:
    QPixmap _pixmap;
};

class PythonNodeData : public NodeData
{
public:
    PythonNodeData():NodeValues({"",""}){}
    NodeDataType type() const override { return NodeDataType{"Python", ""}; }
    struct PythonData
    {
        QString path;
        QString name;
    };
    PythonData NodeValues;
};

class AudioNodeData : public NodeData
{
public:
    AudioNodeData():NodeValues({"a1","b1"}){}
    NodeDataType type() const override { return NodeDataType{"Audio", ""}; }
    struct AudioData
    {
        QString path;
        QString name;
        bool play;
        float gain;
    };
    AudioData NodeValues;
};

class FloatData:public NodeData{
public:
    FloatData():NodeValues(0.0){}
    NodeDataType type() const override { return NodeDataType{"Float", ""}; }
    float NodeValues;
};

class BoolData:public NodeData{
public:
    BoolData(bool const &val= false):NodeValues(val){}
    NodeDataType type() const override { return NodeDataType{"Bool", ""}; }
    bool NodeValues;
};

class IntData:public NodeData{
public:
    IntData(int const &val=0):NodeValues(val){}
    NodeDataType type() const override { return NodeDataType{"Int", ""}; }
    int NodeValues;
};
class StringData:public NodeData{
public:

    StringData(QString const &text=""):NodeValues(text){};
    NodeDataType type() const override { return NodeDataType{"String", ""}; }
    QString NodeValues;
};

class VariantData:public NodeData{
public:

    VariantData(QVariant const &text=""):NodeValues(text){};
    NodeDataType type() const override { return NodeDataType{"Variant", ""}; }
    QVariant NodeValues;
};
class AudioNodeData2 : public NodeData
{
public:
    AudioNodeData2():NodeValues({nullptr, nullptr}){}
    NodeDataType type() const override { return NodeDataType{"Audio2", ""}; }
    struct AudioData2
    {
        QQueue<uint8_t *> *fifoBuffer;
        QWaitCondition *condition;

    };
    AudioData2 NodeValues;
};