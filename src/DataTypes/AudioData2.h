//
// Created by WuBin on 24-10-29.
//
#include "QtNodes/NodeData"
// #include "Common/Devices/AudioPipe/AudioPipe.h"
#ifndef AUDIODATA2_H
#define AUDIODATA2_H
#include <QMetaType>

using QtNodes::NodeData;
using QtNodes::NodeDataType;

// 定义音频帧数据结构
struct AudioFrame {
    QByteArray data;        // 音频数据
    int sampleRate;         // 采样率
    int channels;           // 通道数
    int bitsPerSample;     // 每个采样的位数
    qint64 timestamp;       // 时间戳
    
    AudioFrame() : sampleRate(0), channels(0), bitsPerSample(0), timestamp(0) {}
};
Q_DECLARE_METATYPE(AudioFrame)  // 注册到Qt元对象系统


class AudioData2 : public NodeData
{
public:
    AudioData2() {
        // 确保在使用前注册类型
        qRegisterMetaType<AudioFrame>("AudioFrame");
    }
    NodeDataType type() const override { return NodeDataType{"Audio2", "audio"}; }
    void updateAudioFrame(const AudioFrame& frame) {
        audioFrame = frame;
    }
    const AudioFrame& getAudioFrame() const {
        return audioFrame;
    }
    int getAudioChannels() const {
        return audioFrame.channels;
    }
    QByteArray getAudioData() const {
        return audioFrame.data;
    }
    int getAudioSampleRate() const {
        return audioFrame.sampleRate;
    }
    int getAudioBitsPerSample() const {
        return audioFrame.bitsPerSample;
    }
private:
    AudioFrame audioFrame;
};
#endif //AUDIODATA2_H
