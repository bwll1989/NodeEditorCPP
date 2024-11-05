//
// Created by WuBin on 24-10-28.
//

#include "AudioPipe.h"
#include <qdebug.h>
#include <iostream>

AudioPipe::AudioPipe() {

}
void AudioPipe::pushAudioData(const uint8_t* outputBuffer, size_t size) {
    if (outputBuffer == nullptr || size == 0) {
        qWarning() << "Invalid audio data.";
        return;
    }

    QMutexLocker locker(&mutex);
    // 检查队列是否已满
    while (audioQueue.size() >= maxQueueSize) {
        condition.wait(&mutex); // 等待直到有空间
    }
    std::cout<<"push size:"<<size<<std::endl;
    audioQueue.emplace_back(outputBuffer, outputBuffer + size); // 从 outputBuffer 创建 std::vector

    condition.wakeOne(); // 唤醒等待的节点
}

std::vector<uint8_t> AudioPipe::popAudioData() {

    QMutexLocker locker(&mutex);
    while (audioQueue.isEmpty()) {
        condition.wait(&mutex); // 等待数据
    }
    auto data=audioQueue.dequeue();
    std::cout<<"take size:"<<data.size()<<std::endl;
    return data;
}

size_t AudioPipe::cacheSize(){
    QMutexLocker locker(&mutex);
    return audioQueue.size();
}

