#include "VST3AudioProcessingThread.hpp"
#include "VST3PluginDataModel.hpp"  // 在实现文件中包含
#include <QDebug>
#include <QElapsedTimer>
#include <chrono>
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
using namespace Nodes;
using namespace std::chrono;

VST3AudioProcessingThread::VST3AudioProcessingThread(QObject* parent)
    : QThread(parent)
    , running_(0)
    , paused_(0)
    , sampleRate_(48000.0)
    , blockSize_(1024)
    , frameDurationNs_(0)
    , nextProcessTime_(0)
    , consumerId_(-1)
    , audioEffect_(nullptr)
    , processingData_(nullptr)
    , lastProcessTime_(0)
    , processedFrames_(0)
    , averageProcessTime_(0.0)
{
    // 设置线程优先级为最高，确保音频处理的实时性
    setPriority(QThread::TimeCriticalPriority);
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
    
    // 计算帧持续时间（纳秒）
    frameDurationNs_ = static_cast<qint64>((double)blockSize_ * 1000000000.0 / sampleRate_);
    
    qDebug() << "Audio thread parameters: sampleRate=" << sampleRate_ 
             << "blockSize=" << blockSize_ 
             << "frameDuration=" << frameDurationNs_ / 1000000.0 << "ms";
}

/**
 * @brief 设置输入输出音频缓冲区
 */
void VST3AudioProcessingThread::setAudioBuffers(std::shared_ptr<AudioTimestampRingQueue> input,
                                               std::shared_ptr<AudioTimestampRingQueue> output,
                                               int consumerId)
{
    QMutexLocker locker(&mutex_);
    inputBuffer_ = input;
    outputBuffer_ = output;
    consumerId_ = consumerId;
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
    
    // 初始化时间
    nextProcessTime_ = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
    
    start();
    qDebug() << "VST3 audio processing thread started";
}

/**
 * @brief 停止音频处理
 */
void VST3AudioProcessingThread::stopProcessing()
{
    if (!running_.loadAcquire()) {
        return;
    }
    
    qDebug() << "VST3 audio processing thread stopping...";
    
    // 设置停止标志
    running_.storeRelease(0);
    
    // 唤醒所有等待的线程
    {
        QMutexLocker locker(&mutex_);
        condition_.wakeAll();
    }
    
    qDebug() << "Stop signal sent to audio processing thread";
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
    
    qDebug() << "VST3 audio processing" << (pause ? "paused" : "resumed");
}

/**
 * @brief 线程主循环
 */
void VST3AudioProcessingThread::run()
{
    qDebug() << "VST3 audio processing thread running";
    
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
        
        // 计算下次处理时间并等待
        calculateNextProcessTime();
    }
    
    qDebug() << "VST3 audio processing thread finished";
}

/**
 * @brief 执行单次音频处理
 */
void VST3AudioProcessingThread::processAudioFrame()
{
    if (!inputBuffer_ || !outputBuffer_ || !audioEffect_ || !processingData_ || consumerId_ == -1) {
        return;
    }
    
    QElapsedTimer timer;
    timer.start();
    
    // 获取输入音频帧
    qint64 currentSystemTime = QDateTime::currentMSecsSinceEpoch();
    double frameDurationMs = (double)blockSize_ * 1000.0 / sampleRate_;
    qint64 tolerance = static_cast<qint64>(frameDurationMs * 0.5);
    
    AudioFrame inputFrame;
    if (!inputBuffer_->getFrameByTimestamp(consumerId_, currentSystemTime, tolerance, inputFrame)) {
        return;
    }
    
    // 验证输入数据
    if (inputFrame.data.isEmpty() || inputFrame.sampleRate <= 0 || inputFrame.channels <= 0) {
        return;
    }
    
    // 执行VST3音频处理
    if (processingData_->useDoubleProcessing) {
        processAudioDouble(inputFrame);
    } else {
        processAudioFloat(inputFrame);
    }
    
    // 处理完成，更新统计信息
    qint64 processTime = timer.nsecsElapsed();
    processedFrames_++;
    averageProcessTime_ = (averageProcessTime_ * (processedFrames_ - 1) + processTime / 1000000.0) / processedFrames_;
    
    // 每1000帧输出一次性能统计
    if (processedFrames_ % 1000 == 0) {
        qDebug() << "Audio processing stats: avg time=" << averageProcessTime_ << "ms, frames=" << processedFrames_;
    }
}

/**
 * @brief 双精度音频处理
 */
void VST3AudioProcessingThread::processAudioDouble(const AudioFrame& inputFrame)
{
    const int16_t* inputInt16 = reinterpret_cast<const int16_t*>(inputFrame.data.constData());
    int sampleCount = inputFrame.data.size() / sizeof(int16_t);
    
    // 准备输入缓冲区指针数组
    std::vector<double*> inputPointers(processingData_->totalInputChannels);
    std::vector<double*> outputPointers(processingData_->totalOutputChannels);
    
    // 转换输入数据到双精度并设置指针
    int channelIndex = 0;
    for (size_t busIndex = 0; busIndex < processingData_->inputChannelCounts.size(); ++busIndex) {
        int channelCount = processingData_->inputChannelCounts[busIndex];
        
        for (int ch = 0; ch < channelCount; ++ch) {
            auto& buffer = processingData_->doubleBuffers[channelIndex];
            
            // 转换16位整数到双精度浮点
            for (int i = 0; i < sampleCount; ++i) {
                buffer[i] = static_cast<double>(inputInt16[i]) / 32767.0;
            }
            
            inputPointers[channelIndex] = buffer.data();
            channelIndex++;
        }
        
        // 设置VST输入总线
        processingData_->vstInput[busIndex].numChannels = channelCount;
        processingData_->vstInput[busIndex].channelBuffers64 = inputPointers.data() + channelIndex - channelCount;
        processingData_->vstInput[busIndex].silenceFlags = 0;
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
    processingData_->vstData.numSamples = sampleCount;
    Steinberg::tresult result = audioEffect_->process(processingData_->vstData);
    
    if (result == Steinberg::kResultOk) {
        // 转换输出数据回16位整数
        QByteArray outputData(sampleCount * sizeof(int16_t), 0);
        int16_t* outputInt16 = reinterpret_cast<int16_t*>(outputData.data());
        
        // 使用第一个输出通道的数据
        if (processingData_->totalOutputChannels > 0) {
            const double* outputBuffer = outputPointers[0];
            for (int i = 0; i < sampleCount; ++i) {
                double sample = qBound(-1.0, outputBuffer[i], 1.0);
                outputInt16[i] = static_cast<int16_t>(sample * 32767.0);
            }
        }
        
        // 创建输出帧
        AudioFrame outputFrame;
        outputFrame.data = outputData;
        outputFrame.sampleRate = inputFrame.sampleRate;
        outputFrame.channels = inputFrame.channels;
        outputFrame.bitsPerSample = 16;
        outputFrame.timestamp = inputFrame.timestamp + static_cast<qint64>(85);
        
        outputBuffer_->pushFrame(outputFrame);
    } else {
        qWarning() << "VST3 double processing failed with result:" << result;
        // 传递原始数据
        AudioFrame outputFrame = inputFrame;
        outputFrame.timestamp += static_cast<qint64>(85);
        outputBuffer_->pushFrame(outputFrame);
    }
}

/**
 * @brief 单精度音频处理
 */
void VST3AudioProcessingThread::processAudioFloat(const AudioFrame& inputFrame)
{
    const int16_t* inputInt16 = reinterpret_cast<const int16_t*>(inputFrame.data.constData());
    int sampleCount = inputFrame.data.size() / sizeof(int16_t);
    
    // 准备输入缓冲区指针数组
    std::vector<float*> inputPointers(processingData_->totalInputChannels);
    std::vector<float*> outputPointers(processingData_->totalOutputChannels);
    
    // 转换输入数据到单精度并设置指针
    int channelIndex = 0;
    for (size_t busIndex = 0; busIndex < processingData_->inputChannelCounts.size(); ++busIndex) {
        int channelCount = processingData_->inputChannelCounts[busIndex];
        
        for (int ch = 0; ch < channelCount; ++ch) {
            auto& buffer = processingData_->floatBuffers[channelIndex];
            
            // 转换16位整数到单精度浮点
            for (int i = 0; i < sampleCount; ++i) {
                buffer[i] = static_cast<float>(inputInt16[i]) / 32767.0f;
            }
            
            inputPointers[channelIndex] = buffer.data();
            channelIndex++;
        }
        
        // 设置VST输入总线
        processingData_->vstInput[busIndex].numChannels = channelCount;
        processingData_->vstInput[busIndex].channelBuffers32 = inputPointers.data() + channelIndex - channelCount;
        processingData_->vstInput[busIndex].silenceFlags = 0;
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
    processingData_->vstData.numSamples = sampleCount;
    Steinberg::tresult result = audioEffect_->process(processingData_->vstData);
    
    if (result == Steinberg::kResultOk) {
        // 转换输出数据回16位整数
        QByteArray outputData(sampleCount * sizeof(int16_t), 0);
        int16_t* outputInt16 = reinterpret_cast<int16_t*>(outputData.data());
        
        // 使用第一个输出通道的数据
        if (processingData_->totalOutputChannels > 0) {
            const float* outputBuffer = outputPointers[0];
            for (int i = 0; i < sampleCount; ++i) {
                float sample = qBound(-1.0f, outputBuffer[i], 1.0f);
                outputInt16[i] = static_cast<int16_t>(sample * 32767.0f);
            }
        }
        
        // 创建输出帧
        AudioFrame outputFrame;
        outputFrame.data = outputData;
        outputFrame.sampleRate = inputFrame.sampleRate;
        outputFrame.channels = inputFrame.channels;
        outputFrame.bitsPerSample = 16;
        outputFrame.timestamp = inputFrame.timestamp + static_cast<qint64>(85);
        
        outputBuffer_->pushFrame(outputFrame);
    } else {
        qWarning() << "VST3 float processing failed with result:" << result;
        // 传递原始数据
        AudioFrame outputFrame = inputFrame;
        outputFrame.timestamp += static_cast<qint64>(85);
        outputBuffer_->pushFrame(outputFrame);
    }
}

/**
 * @brief 计算下一次处理的等待时间
 */
void VST3AudioProcessingThread::calculateNextProcessTime()
{
    if (frameDurationNs_ <= 0) {
        // 如果没有设置帧持续时间，使用默认等待
        QThread::msleep(1);
        return;
    }
    
    qint64 currentTime = duration_cast<nanoseconds>(high_resolution_clock::now().time_since_epoch()).count();
    nextProcessTime_ += frameDurationNs_*0.8;
    
    qint64 waitTime = nextProcessTime_ - currentTime;
    
    if (waitTime > 0) {
        // 将等待时间转换为毫秒，并分段等待以便响应停止信号
        qint64 waitMs = waitTime / 1000000;
        
        // 分段等待，每次最多等待10ms，以便及时响应停止信号
        while (waitMs > 0 && running_.loadAcquire()) {
            qint64 sleepTime = qMin(waitMs, 10LL);
            QThread::msleep(sleepTime);
            waitMs -= sleepTime;
        }
    } else {
        // 如果已经超时，重新同步时间
        nextProcessTime_ = currentTime;
    }
}