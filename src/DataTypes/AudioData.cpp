//
// Created by WuBin on 24-10-29.
//
#include "AudioData.h"
using namespace  NodeDataTypes;

AudioData::AudioData() {
    qRegisterMetaType<AudioData>("AudioData");
}

NodeDataType AudioData::type() const {
    return NodeDataType{"Audio", "audio"};
}

/**
 * @brief 设置共享环形缓冲区（用于连接建立时）
 * @param buffer 共享的环形缓冲区指针
 */
void AudioData::setSharedAudioBuffer(std::shared_ptr<AudioTimestampRingQueue> buffer) {
    sharedAudioBuffer_ = buffer;
}

/**
 * @brief 获取共享环形缓冲区指针
 * @return 环形缓冲区指针，nullptr表示未连接
 */
std::shared_ptr<AudioTimestampRingQueue> AudioData::getSharedAudioBuffer() const {
    return sharedAudioBuffer_;
}

/**
 * @brief 检查是否已连接到共享缓冲区
 * @return true表示已连接，false表示未连接
 */
bool AudioData::isConnectedToSharedBuffer() const {
    return sharedAudioBuffer_ != nullptr;
}

/**
 * @brief 断开连接
 */
void AudioData::disconnect() {
    sharedAudioBuffer_.reset();
}



    