//
// Created by WuBin on 24-10-29.
//
#include "QtNodes/NodeData"
#include "Common/Devices/AudioPipe/AudioPipe.h"
#ifndef AUDIODATA2_H
#define AUDIODATA2_H
using QtNodes::NodeData;
using QtNodes::NodeDataType;
class AudioData2 : public NodeData
{
public:
    AudioData2():pipe(nullptr){}
    NodeDataType type() const override { return NodeDataType{"Audio2", "audio"}; }
    // struct AudioData2
    // {
    //     QQueue<uint8_t *> *fifoBuffer;
    //     QWaitCondition *condition;
    //
    // };
    AudioPipe *pipe;
    // AudioData2 NodeValues;
};
#endif //AUDIODATA2_H
