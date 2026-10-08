#include "VST3AudioProcessingThread.hpp"
#include "VST3PluginDataModel.hpp"  // 在实现文件中包含
#include <QDebug>
#include <algorithm>
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include "TimestampGenerator/AudioThreadRealtime.hpp"
using namespace Nodes;

namespace
{
    bool fetchContentFrame(const std::shared_ptr<AudioTimestampRingQueue> &queue,
                           qint64 contentTs,
                           AudioFrame &outFrame)
    {
        if (!queue) {
            return false;
        }
        return queue->getFrameByTimestamp(contentTs, outFrame)
            && outFrame.timestamp == contentTs
            && !outFrame.data.isEmpty();
    }
}

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
    auto it = inputBuffer_.find(channelIndex);
    if (it != inputBuffer_.end() && it->second) {
        it->second->unregisterFrameWaiter(&tickWaiter_);
    }
    if (!input) {
        inputBuffer_.erase(channelIndex);
    } else {
        inputBuffer_[channelIndex] = input;
        if (running_.loadAcquire()) {
            input->registerFrameWaiter(&tickWaiter_);
        }
        if (!outputBuffer_[channelIndex]) {
            outputBuffer_[channelIndex] = std::make_shared<AudioTimestampRingQueue>();
        }
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
    lastProcessTimestamp_ = 0;
    deferredEmptyTs_ = 0;
    tickWaiter_.reset();
    {
        QMutexLocker locker(&mutex_);
        for (auto &[channelIndex, inputQueue] : inputBuffer_) {
            Q_UNUSED(channelIndex)
            if (inputQueue) {
                inputQueue->registerFrameWaiter(&tickWaiter_);
            }
        }
    }
    start();
}

/**
 * @brief 停止音频处理
 */
void VST3AudioProcessingThread::stopProcessing()
{
    if (!running_.loadAcquire()) {
        return;
    }

    running_.storeRelease(0);
    {
        QMutexLocker locker(&mutex_);
        for (auto &[channelIndex, inputQueue] : inputBuffer_) {
            Q_UNUSED(channelIndex)
            if (inputQueue) {
                inputQueue->unregisterFrameWaiter(&tickWaiter_);
            }
        }
    }
    tickWaiter_.requestStop();
    {
        QMutexLocker locker(&mutex_);
        pauseCondition_.wakeAll();
    }
}

/**
 * @brief 暂停/恢复音频处理
 */
void VST3AudioProcessingThread::pauseProcessing(bool pause)
{
    paused_.storeRelease(pause ? 1 : 0);
    if (!pause) {
        QMutexLocker locker(&mutex_);
        pauseCondition_.wakeAll();
    }
}

/**
 * @brief 线程主循环：由输入环 push 唤醒；超时也处理（NotYet clock 截止）
 */
void VST3AudioProcessingThread::run()
{
    AudioThreadRealtimeGuard realtimeGuard(L"Pro Audio");
    while (running_.loadAcquire()) {
        if (paused_.loadAcquire()) {
            QMutexLocker locker(&mutex_);
            while (paused_.loadAcquire() && running_.loadAcquire()) {
                pauseCondition_.wait(&mutex_, 100);
            }
            continue;
        }

        tickWaiter_.wait(20);
        if (!running_.loadAcquire() || tickWaiter_.isStopRequested()) {
            break;
        }
        if (paused_.loadAcquire()) {
            continue;
        }

        processAudioFrame();
    }
}

/**
 * @brief 与 Matrix 对齐：环级联唤醒后追帧；有输入写出 nextTs+D
 */
void VST3AudioProcessingThread::processAudioFrame()
{
    if (outputBuffer_.empty() || !audioEffect_ || !processingData_) {
        return;
    }

    const int connected = connectedInputCount();
    const qint64 clockTs = TimestampGenerator::getInstance()->getCurrentFrameCount();
    qint64 cursor = lastProcessTimestamp_;

    auto runProcess = [&](qint64 nextTs, qint64 outputTs) {
        if (processingData_->useDoubleProcessing) {
            processAudioDouble(nextTs, outputTs);
        } else {
            processAudioFloat(nextTs, outputTs);
        }
    };

    int processedCount = 0;

    // 无输入：仍按 clock 推进静音
    if (connected == 0) {
        if (cursor <= 0 || cursor < clockTs - 64) {
            cursor = clockTs - 1;
        }
        int maxFramesPerWake = 2;
        if (clockTs > cursor + 2) {
            maxFramesPerWake = static_cast<int>(qMin<qint64>(8, clockTs - cursor));
        }
        while (processedCount < maxFramesPerWake) {
            const qint64 nextTs = cursor + 1;
            if (nextTs > clockTs) {
                break;
            }
            runProcess(nextTs, nextTs);
            cursor = nextTs;
            ++processedCount;
        }
        if (processedCount > 0) {
            lastProcessTimestamp_ = cursor;
        }
        return;
    }

    // 有输入：找帧与透传一致；多源二次缺→缺路静音仍 process；写出 nextTs+D
    const int delayFrames = TimestampGenerator::getInstance()->getAudioOutputDelayFrames();
    const bool multiSource = connected > 1;
    constexpr qint64 kTipStaleFrames = 16;
    qint64 tipLatest = 0;
    bool haveTip = false;
    for (const auto &[channelIndex, inputQueue] : inputBuffer_) {
        Q_UNUSED(channelIndex)
        if (!inputQueue || !inputQueue->isActive()) {
            continue;
        }
        const qint64 L = inputQueue->latestTimestamp();
        if (L <= 0) {
            continue;
        }
        if (clockTs > L && (clockTs - L) > kTipStaleFrames) {
            continue;
        }
        if (!haveTip || L < tipLatest) {
            tipLatest = L;
        }
        haveTip = true;
    }
    if (cursor <= 0 || cursor < clockTs - 64) {
        cursor = clockTs - 1;
    }
    int maxFramesPerWake = 2;
    if (clockTs > cursor + 2) {
        maxFramesPerWake = static_cast<int>(qMin<qint64>(8, clockTs - cursor));
    }
    const int framesThisWake = multiSource ? 2 : maxFramesPerWake;

    int holeSkips = 0;
    while (processedCount < framesThisWake) {
        const qint64 nextTs = cursor + 1;
        if (!haveTip || nextTs > tipLatest) {
            break; // NotYet：绝不填 0
        }

        const bool allPresent = allConnectedInputsPresentAt(nextTs);
        if (!allPresent) {
            if (deferredEmptyTs_ != nextTs) {
                deferredEmptyTs_ = nextTs;
                break;
            }
            deferredEmptyTs_ = 0;
            if (!multiSource) {
                cursor = nextTs;
                if (++holeSkips > 16) {
                    break;
                }
                continue;
            }
            // 多源二次缺：process 内缺路缓冲已清零，仍出帧
        } else {
            deferredEmptyTs_ = 0;
        }

        runProcess(nextTs, nextTs + delayFrames);

        cursor = nextTs;
        ++processedCount;
        deferredEmptyTs_ = 0;
    }

    if (processedCount > 0) {
        lastProcessTimestamp_ = cursor;
    }
}

int VST3AudioProcessingThread::connectedInputCount() const
{
    int n = 0;
    for (const auto &[channelIndex, inputQueue] : inputBuffer_) {
        Q_UNUSED(channelIndex)
        if (inputQueue && inputQueue->isActive()) {
            ++n;
        }
    }
    return n;
}

bool VST3AudioProcessingThread::allConnectedInputsPresentAt(qint64 contentTs) const
{
    bool any = false;
    for (const auto &[channelIndex, inputQueue] : inputBuffer_) {
        Q_UNUSED(channelIndex)
        if (!inputQueue || !inputQueue->isActive()) {
            continue;
        }
        any = true;
        AudioFrame frame;
        if (!fetchContentFrame(inputQueue, contentTs, frame)) {
            return false;
        }
    }
    return any;
}

/**
 * @brief 双精度音频处理 - 支持多通道输入输出
 */
void VST3AudioProcessingThread::processAudioDouble(qint64 targetTimestamp, qint64 outputTimestamp)
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
    
    const int frameSamples = TimestampGenerator::getInstance()->getSamplesPerFrame(
        static_cast<int>(sampleRate_ > 0 ? sampleRate_ : 48000.0));
    const int bufferCap = frameSamples > 0 ? frameSamples : blockSize_;

    for (auto& [channelIndex, inputQueue] : inputBuffer_) {
        if (!inputQueue || channelIndex >= processingData_->totalInputChannels) {
            continue;
        }
        AudioFrame inputFrame;
        if (!fetchContentFrame(inputQueue, targetTimestamp, inputFrame)) {
            continue;
        }

        const float* inputFloat = reinterpret_cast<const float*>(inputFrame.data.constData());
        const int sampleCount = inputFrame.data.size() / static_cast<int>(sizeof(float));
        const int n = qMin(sampleCount, bufferCap);
        auto& targetBuffer = processingData_->doubleBuffers[channelIndex];
        for (int i = 0; i < n; ++i) {
            targetBuffer[static_cast<size_t>(i)] = static_cast<double>(inputFloat[i]);
        }
    }

    // 统一按系统帧长 process，避免缺帧时用错误 blockSize_ 写出
    maxSampleCount = bufferCap;
    
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
                QByteArray outputData(maxSampleCount * sizeof(float), 0);
                float* outputFloat = reinterpret_cast<float*>(outputData.data());
                
                // 转换对应输出通道的数据
                const double* outputBuffer = outputPointers[channelIndex];
                for (int i = 0; i < maxSampleCount; ++i) {
                    double sample = qBound(-1.0, outputBuffer[i], 1.0);
                    outputFloat[i] = static_cast<float>(sample);
                }
                
                // 创建输出帧并推送到对应的输出缓冲区
                AudioFrame outputFrame;
                outputFrame.data = outputData;
                outputFrame.sampleRate = 48000; // 使用默认采样率
                outputFrame.channels = 1; // 每个通道单独处理
                outputFrame.bitsPerSample = 32;
                outputFrame.timestamp = outputTimestamp;
                
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
 */
void VST3AudioProcessingThread::processAudioFloat(qint64 targetTimestamp, qint64 outputTimestamp)
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

    const int frameSamples = TimestampGenerator::getInstance()->getSamplesPerFrame(
        static_cast<int>(sampleRate_ > 0 ? sampleRate_ : 48000.0));
    const int bufferCap = frameSamples > 0 ? frameSamples : blockSize_;

    for (auto& [channelIndex, inputQueue] : inputBuffer_) {
        if (!inputQueue || channelIndex >= processingData_->totalInputChannels) {
            continue;
        }
        AudioFrame inputFrame;
        if (!fetchContentFrame(inputQueue, targetTimestamp, inputFrame)) {
            continue;
        }

        const float* inputFloat = reinterpret_cast<const float*>(inputFrame.data.constData());
        const int sampleCount = inputFrame.data.size() / static_cast<int>(sizeof(float));
        const int n = qMin(sampleCount, bufferCap);
        auto& targetBuffer = processingData_->floatBuffers[channelIndex];
        for (int i = 0; i < n; ++i) {
            targetBuffer[static_cast<size_t>(i)] = inputFloat[i];
        }
    }

    maxSampleCount = bufferCap;

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
                QByteArray outputData(maxSampleCount * sizeof(float), 0);
                float* outputFloat = reinterpret_cast<float*>(outputData.data());
                
                // 转换对应输出通道的数据
                const float* outputBuffer = outputPointers[channelIndex];
                for (int i = 0; i < maxSampleCount; ++i) {
                    outputFloat[i] = qBound(-1.0f, outputBuffer[i], 1.0f);
                }
                
                // 创建输出帧并推送到对应的输出缓冲区
                AudioFrame outputFrame;
                outputFrame.data = outputData;
                outputFrame.sampleRate = 48000; // 使用默认采样率
                outputFrame.channels = 1; // 每个通道单独处理
                outputFrame.bitsPerSample = 32;
                outputFrame.timestamp = outputTimestamp;
                
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
