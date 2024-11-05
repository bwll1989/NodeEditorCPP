//
// Created by WuBin on 24-10-28.
//

#ifndef AUDIOPIPE_H
#define AUDIOPIPE_H

#include <QQueue>
#include <QMutex>
#include <QWaitCondition>
#include <cstdint> // 为了使用 uint8_t

class AudioPipe {
public:
    AudioPipe();
    QQueue<std::vector<uint8_t>> audioQueue;
    QMutex mutex;
    QWaitCondition condition;
    size_t maxQueueSize=100;
    void pushAudioData(const uint8_t* outputBuffer, size_t size)  ;
    size_t cacheSize();
    std::vector<uint8_t> popAudioData();
};


#endif //AUDIOPIPE_H
