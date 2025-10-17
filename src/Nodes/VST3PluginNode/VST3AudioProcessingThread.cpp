#include "VST3AudioProcessingThread.hpp"
#include "VST3PluginDataModel.hpp"  // 在实现文件中包含
#include <QDebug>
#include <QElapsedTimer>
#include <chrono>
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "TimestampGenerator/TimestampGenerator.hpp"
using namespace Nodes;
using namespace std::chrono;

VST3AudioProcessingThread::VST3AudioProcessingThread(QObject* parent)
    : QThread(parent)
    , running_(0)
    , paused_(0)
    , sampleRate_(48000.0)
    , blockSize_(2048)
    , lastProcessTimestamp_(0)
    , audioEffect_(nullptr)
    , processingData_(nullptr)
{
    // 设置线程优先级为最高，确保音频处理的实时性
    // setPriority(QThread::TimeCriticalPriority);
}

VST3AudioProcessingThread::~VST3AudioProcessingThread()
{
    // 确保线程安全退出
    if (isRunning()) {
        stopProcessing();
        
        // 等待线程正常退出
        if (!wait(5000)) {
            qWarning() << "Audio processing thread did not exit gracefully, terminating...";
            terminate();
            wait(2000); // 给terminate一些时间
        }
    }
    
    qDebug() << "VST3AudioProcessingThread destroyed";
}

/**
 * @brief 设置音频处理参数
 */
void VST3AudioProcessingThread::setAudioParameters(double sampleRate, int blockSize)
{
    QMutexLocker locker(&mutex_);
    sampleRate_ = sampleRate;
    blockSize_ = blockSize;
    // qDebug() << "VST3AudioProcessingThread setting audio parameters..."<<blockSize_<<sampleRate;
}

/**
 * @brief 设置输入输出音频缓冲区
 */
void VST3AudioProcessingThread::setInputAudioBuffers(int channelIndex,std::shared_ptr<AudioTimestampRingQueue> input)
{
    QMutexLocker locker(&mutex_);
    if (!input)
        {inputBuffer_.erase(channelIndex);}
    else
    {
        inputBuffer_[channelIndex]= input;
        if (!outputBuffer_[channelIndex])
            outputBuffer_[channelIndex] = std::make_shared<AudioTimestampRingQueue>();

    }

}

std::shared_ptr<AudioTimestampRingQueue> VST3AudioProcessingThread::getOutputAudioBuffers(int channelIndex)
{
    if (!outputBuffer_.count(channelIndex))
        outputBuffer_[channelIndex] = std::make_shared<AudioTimestampRingQueue>();
    return outputBuffer_[channelIndex];
}

/**
 * @brief 设置VST3处理组件
 */
void VST3AudioProcessingThread::setVST3Components(Steinberg::IPtr<Steinberg::Vst::IAudioProcessor> audioEffect,
                                                 AudioProcessingData* processingData)
{
    QMutexLocker locker(&mutex_);
    audioEffect_ = audioEffect;
    processingData_ = processingData;
}

/**
 * @brief 启动音频处理
 */
void VST3AudioProcessingThread::startProcessing()
{
    if (running_.loadAcquire()) {
        return;
    }
    
    running_.storeRelease(1);
    paused_.storeRelease(0);
    start();
    // qDebug() << "VST3 audio processing thread started";
}

/**
 * @brief 停止音频处理
 */
void VST3AudioProcessingThread::stopProcessing()
{
    if (!running_.loadAcquire()) {
        return;
    }
    
    // qDebug() << "VST3 audio processing thread stopping...";
    
    // 设置停止标志
    running_.storeRelease(0);
    
    // 唤醒所有等待的线程
    {
        QMutexLocker locker(&mutex_);
        condition_.wakeAll();
    }
    
    // qDebug() << "Stop signal sent to audio processing thread";
}

/**
 * @brief 暂停/恢复音频处理
 */
void VST3AudioProcessingThread::pauseProcessing(bool pause)
{
    paused_.storeRelease(pause ? 1 : 0);
    if (!pause) {
        condition_.wakeAll();
    }
    
    // qDebug() << "VST3 audio processing" << (pause ? "paused" : "resumed");
}

/**
 * @brief 线程主循环
 */
void VST3AudioProcessingThread::run()
{

    while (running_.loadAcquire()) {
        // 检查是否暂停
        if (paused_.loadAcquire()) {
            QMutexLocker locker(&mutex_);
            // 在等待时也要检查running状态
            while (paused_.loadAcquire() && running_.loadAcquire()) {
                condition_.wait(&mutex_, 100); // 100ms超时，避免死锁
            }
            continue;
        }
        
        // 再次检查running状态
        if (!running_.loadAcquire()) {
            break;
        }

        // 执行音频处理
        processAudioFrame();
        msleep(20);
        // 计算下次处理时间并等待
        // calculateNextProcessTime();
    }
    
    // qDebug() << "VST3 audio processing thread finished";
}

/**
 * @brief 执行单次音频处理
 */
void VST3AudioProcessingThread::processAudioFrame()
{
    if (inputBuffer_.empty() || outputBuffer_.empty() || !audioEffect_ || !processingData_) {
        return;
    }
    
    qint64 currentSystemTime = TimestampGenerator::getInstance()->getCurrentFrameCount();
    // 检查时间戳是否与上次处理的相同
    if (currentSystemTime== lastProcessTimestamp_) {
        return; // 跳过重复帧
    }
    lastProcessTimestamp_ = currentSystemTime;
    // 执行VST3音频处理
    if (processingData_->useDoubleProcessing) {
        processAudioDouble(currentSystemTime);
    } else {
        processAudioFloat(currentSystemTime);
    }
}

/**
 * @brief 双精度音频处理 - 支持多通道输入输出
 * @param currentSystemTime 当前系统时间戳
 */
void VST3AudioProcessingThread::processAudioDouble(qint64 currentSystemTime)
{
    // 准备输入缓冲区指针数组
    std::vector<double*> inputPointers(processingData_->totalInputChannels);
    std::vector<double*> outputPointers(processingData_->totalOutputChannels);
    
    // 清空所有输入缓冲区
    int maxSampleCount = 0;
    for (int i = 0; i < processingData_->totalInputChannels; ++i) {
        std::fill(processingData_->doubleBuffers[i].begin(), 
                 processingData_->doubleBuffers[i].end(), 0.0);
    }
    
    // 从所有输入通道收集数据
    bool hasValidInput = false;
    qint64 processTimestamp = 0;
    
    for (auto& [channelIndex, inputQueue] : inputBuffer_) {
        AudioFrame inputFrame;
        
        // 从对应通道获取音频帧
        if (!inputQueue->getFrameByTimestamp(currentSystemTime, inputFrame)) {
            continue; // 如果该通道没有数据，跳过
        }

        // 将输入数据写入指定的通道
        if (channelIndex < processingData_->totalInputChannels) {
            const int16_t* inputInt16 = reinterpret_cast<const int16_t*>(inputFrame.data.constData());
            int sampleCount = inputFrame.data.size() / sizeof(int16_t);
            maxSampleCount = qMax(maxSampleCount, sampleCount);

            auto& targetBuffer = processingData_->doubleBuffers[channelIndex];
            
            // 转换16位整数到双精度浮点并写入指定通道
            for (int i = 0; i < sampleCount; ++i) {
                targetBuffer[i] = static_cast<double>(inputInt16[i]) / 32767.0;
            }
            
            hasValidInput = true;
            processTimestamp = inputFrame.timestamp;
        }
    }
    
    // 如果没有有效输入，直接返回
    if (!hasValidInput || maxSampleCount == 0) {
        return;
    }
    
    // 设置所有输入通道的指针
    for (int i = 0; i < processingData_->totalInputChannels; ++i) {
        inputPointers[i] = processingData_->doubleBuffers[i].data();
    }
    
    // 更新VST输入总线信息
    int channelIndex = 0;
    for (size_t busIndex = 0; busIndex < processingData_->inputChannelCounts.size(); ++busIndex) {
        int channelCount = processingData_->inputChannelCounts[busIndex];
        
        // 设置VST输入总线
        processingData_->vstInput[busIndex].numChannels = channelCount;
        processingData_->vstInput[busIndex].channelBuffers64 = inputPointers.data() + channelIndex;
        processingData_->vstInput[busIndex].silenceFlags = 0;
        
        // 检查当前总线是否包含活跃的输入通道
        bool hasActiveInput = false;
        for (int ch = 0; ch < channelCount; ++ch) {
            if (inputBuffer_.find(channelIndex + ch) != inputBuffer_.end()) {
                hasActiveInput = true;
                break;
            }
        }
        
        // 如果当前总线没有活跃输入，设置静音标志
        if (!hasActiveInput) {
            processingData_->vstInput[busIndex].silenceFlags = (1ULL << channelCount) - 1;
        }
        
        channelIndex += channelCount;
    }

    // 设置输出缓冲区指针
    channelIndex = 0;
    for (size_t busIndex = 0; busIndex < processingData_->outputChannelCounts.size(); ++busIndex) {
        int channelCount = processingData_->outputChannelCounts[busIndex];
        
        for (int ch = 0; ch < channelCount; ++ch) {
            outputPointers[channelIndex] = processingData_->doubleBuffers[channelIndex].data();
            channelIndex++;
        }
        
        // 设置VST输出总线
        processingData_->vstOutput[busIndex].numChannels = channelCount;
        processingData_->vstOutput[busIndex].channelBuffers64 = outputPointers.data() + channelIndex - channelCount;
        processingData_->vstOutput[busIndex].silenceFlags = 0;
    }
    
    // 执行VST处理
    processingData_->vstData.numSamples = maxSampleCount;
    Steinberg::tresult result = audioEffect_->process(processingData_->vstData);
    
    if (result == Steinberg::kResultOk) {
        // 为每个输出通道创建输出帧
        channelIndex = 0;
        for (size_t busIndex = 0; busIndex < processingData_->outputChannelCounts.size(); ++busIndex) {
            int channelCount = processingData_->outputChannelCounts[busIndex];
            
            for (int ch = 0; ch < channelCount; ++ch) {
                QByteArray outputData(maxSampleCount * sizeof(int16_t), 0);
                int16_t* outputInt16 = reinterpret_cast<int16_t*>(outputData.data());
                
                // 转换对应输出通道的数据
                const double* outputBuffer = outputPointers[channelIndex];
                for (int i = 0; i < maxSampleCount; ++i) {
                    double sample = qBound(-1.0, outputBuffer[i], 1.0);
                    outputInt16[i] = static_cast<int16_t>(sample * 32767.0);
                }
                
                // 创建输出帧并推送到对应的输出缓冲区
                AudioFrame outputFrame;
                outputFrame.data = outputData;
                outputFrame.sampleRate = 48000; // 使用默认采样率
                outputFrame.channels = 1; // 每个通道单独处理
                outputFrame.bitsPerSample = 16;
                outputFrame.timestamp = processTimestamp + 1;
                
                // 推送到对应的输出通道缓冲区
                auto outputIt = outputBuffer_.find(channelIndex);
                if (outputIt != outputBuffer_.end()) {
                    outputIt->second->pushFrame(outputFrame);
                }
                
                channelIndex++;
            }
        }
    }
}

/**
 * @brief 单精度音频处理 - 支持多通道输入输出
 * @param currentSystemTime 当前系统时间戳
 */
void VST3AudioProcessingThread::processAudioFloat(qint64 currentSystemTime)
{
    // 准备输入缓冲区指针数组
    std::vector<float*> inputPointers(processingData_->totalInputChannels);
    std::vector<float*> outputPointers(processingData_->totalOutputChannels);
    
    // 清空所有输入缓冲区
    int maxSampleCount = 0;
    for (int i = 0; i < processingData_->totalInputChannels; ++i) {
        std::fill(processingData_->floatBuffers[i].begin(), 
                 processingData_->floatBuffers[i].end(), 0.0f);
    }
    
    // 从所有输入通道收集数据
    bool hasValidInput = false;
    qint64 processTimestamp = 0;
    
    for (auto& [channelIndex, inputQueue] : inputBuffer_) {
        AudioFrame inputFrame;
        
        // 从对应通道获取音频帧
        if (!inputQueue->getFrameByTimestamp(currentSystemTime, inputFrame)) {
            continue; // 如果该通道没有数据，跳过
        }
        
        // 验证输入数据
        if (inputFrame.data.isEmpty() || inputFrame.sampleRate <= 0 || inputFrame.channels <= 0) {
            continue;
        }
        
        // 将输入数据写入指定的通道
        if (channelIndex < processingData_->totalInputChannels) {
            const int16_t* inputInt16 = reinterpret_cast<const int16_t*>(inputFrame.data.constData());
            int sampleCount = inputFrame.data.size() / sizeof(int16_t);
            maxSampleCount = qMax(maxSampleCount, sampleCount);
            
            auto& targetBuffer = processingData_->floatBuffers[channelIndex];
            
            // 转换16位整数到单精度浮点并写入指定通道
            for (int i = 0; i < sampleCount; ++i) {
                targetBuffer[i] = static_cast<float>(inputInt16[i]) / 32767.0f;
            }
            
            hasValidInput = true;
            processTimestamp = inputFrame.timestamp;
        }
    }
    
    // 如果没有有效输入，直接返回
    if (!hasValidInput || maxSampleCount == 0) {
        return;
    }

    
    // 设置所有输入通道的指针
    for (int i = 0; i < processingData_->totalInputChannels; ++i) {
        inputPointers[i] = processingData_->floatBuffers[i].data();
    }
    
    // 更新VST输入总线信息
    int channelIndex = 0;
    for (size_t busIndex = 0; busIndex < processingData_->inputChannelCounts.size(); ++busIndex) {
        int channelCount = processingData_->inputChannelCounts[busIndex];
        
        // 设置VST输入总线
        processingData_->vstInput[busIndex].numChannels = channelCount;
        processingData_->vstInput[busIndex].channelBuffers32 = inputPointers.data() + channelIndex;
        processingData_->vstInput[busIndex].silenceFlags = 0;
        
        // 检查当前总线是否包含活跃的输入通道
        bool hasActiveInput = false;
        for (int ch = 0; ch < channelCount; ++ch) {
            if (inputBuffer_.find(channelIndex + ch) != inputBuffer_.end()) {
                hasActiveInput = true;
                break;
            }
        }
        
        // 如果当前总线没有活跃输入，设置静音标志
        if (!hasActiveInput) {
            processingData_->vstInput[busIndex].silenceFlags = (1ULL << channelCount) - 1;
        }
        
        channelIndex += channelCount;
    }
    
    // 设置输出缓冲区指针
    channelIndex = 0;
    for (size_t busIndex = 0; busIndex < processingData_->outputChannelCounts.size(); ++busIndex) {
        int channelCount = processingData_->outputChannelCounts[busIndex];
        
        for (int ch = 0; ch < channelCount; ++ch) {
            outputPointers[channelIndex] = processingData_->floatBuffers[channelIndex].data();
            channelIndex++;
        }
        
        // 设置VST输出总线
        processingData_->vstOutput[busIndex].numChannels = channelCount;
        processingData_->vstOutput[busIndex].channelBuffers32 = outputPointers.data() + channelIndex - channelCount;
        processingData_->vstOutput[busIndex].silenceFlags = 0;
    }
    
    // 执行VST处理
    processingData_->vstData.numSamples = maxSampleCount;
    Steinberg::tresult result = audioEffect_->process(processingData_->vstData);
    
    if (result == Steinberg::kResultOk) {
        // 为每个输出通道创建输出帧
        channelIndex = 0;
        for (size_t busIndex = 0; busIndex < processingData_->outputChannelCounts.size(); ++busIndex) {
            int channelCount = processingData_->outputChannelCounts[busIndex];
            
            for (int ch = 0; ch < channelCount; ++ch) {
                QByteArray outputData(maxSampleCount * sizeof(int16_t), 0);
                int16_t* outputInt16 = reinterpret_cast<int16_t*>(outputData.data());
                
                // 转换对应输出通道的数据
                const float* outputBuffer = outputPointers[channelIndex];
                for (int i = 0; i < maxSampleCount; ++i) {
                    float sample = qBound(-1.0f, outputBuffer[i], 1.0f);
                    outputInt16[i] = static_cast<int16_t>(sample * 32767.0f);
                }
                
                // 创建输出帧并推送到对应的输出缓冲区
                AudioFrame outputFrame;
                outputFrame.data = outputData;
                outputFrame.sampleRate = 48000; // 使用默认采样率
                outputFrame.channels = 1; // 每个通道单独处理
                outputFrame.bitsPerSample = 16;
                outputFrame.timestamp = processTimestamp + 1;
                
                // 推送到对应的输出通道缓冲区
                auto outputIt = outputBuffer_.find(channelIndex);
                if (outputIt != outputBuffer_.end()) {
                    outputIt->second->pushFrame(outputFrame);
                }
                
                channelIndex++;
            }
        }
    }
}
