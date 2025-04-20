//
// Created by WuBin on 24-10-28.
//

#include "AudioPipe.h"
#include <qdebug.h>
#include <iostream>

AudioPipe::AudioPipe() {
    // 初始化时确保队列为空
    audioQueue.clear();
}

void AudioPipe::pushAudioData(const uint8_t* outputBuffer, size_t size) {
    if (outputBuffer == nullptr || size == 0) {
        qWarning() << "Invalid audio data.";
        return;
    }

    QMutexLocker locker(&mutex);
    // 检查队列是否已满
    while (audioQueue.size() >= maxQueueSize) {
        // 队列已满，移除最旧的数据
        if (!audioQueue.isEmpty()) {
            audioQueue.dequeue();
        }
        // condition.wait(&mutex); // 等待直到有空间
    }
    
    // 创建新的数据块并复制音频数据
    audioQueue.enqueue(std::vector<uint8_t>(outputBuffer, outputBuffer + size));
    
    // 唤醒等待的消费者
    condition.wakeOne();
}

std::vector<uint8_t> AudioPipe::popAudioData() {
    QMutexLocker locker(&mutex);
    
    // 如果队列为空，返回空数据
    if (audioQueue.isEmpty()) {
        condition.wait(&mutex, 100); // 最多等待100ms
        if (audioQueue.isEmpty()) {
            return std::vector<uint8_t>();
        }
    }
    
    return audioQueue.dequeue();
}

size_t AudioPipe::cacheSize() {
    QMutexLocker locker(&mutex);
    return audioQueue.size();
}

