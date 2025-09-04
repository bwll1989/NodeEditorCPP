#pragma once

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QAtomicInt>
#include <memory>
#include "DataTypes/NodeDataList.hpp"
// #include "VST3PluginDataModel.hpp"  // 删除这行，避免循环依赖
#include "pluginterfaces/base/funknown.h"
#include "pluginterfaces/vst/ivstaudioprocessor.h"
// 前向声明
namespace Steinberg {
    namespace Vst {
        class IAudioProcessor;
    }
}

// 前向声明 AudioProcessingData 结构体
struct AudioProcessingData;

namespace Nodes {

/**
 * @brief 专用的VST3音频处理线程
 * 独立于主线程运行，确保音频处理的实时性
 */
class VST3AudioProcessingThread : public QThread
{
    Q_OBJECT

public:
    explicit VST3AudioProcessingThread(QObject* parent = nullptr);
    ~VST3AudioProcessingThread();

    /**
     * @brief 设置音频处理参数
     */
    void setAudioParameters(double sampleRate, int blockSize);
    
    /**
     * @brief 设置输入输出音频缓冲区
     */
    void setAudioBuffers(std::shared_ptr<AudioTimestampRingQueue> input, 
                        std::shared_ptr<AudioTimestampRingQueue> output,
                        int consumerId);
    
    /**
     * @brief 设置VST3处理组件
     */
    void setVST3Components(Steinberg::IPtr<Steinberg::Vst::IAudioProcessor> audioEffect,
                          AudioProcessingData* processingData);
    
    /**
     * @brief 启动音频处理
     */
    void startProcessing();
    
    /**
     * @brief 停止音频处理
     */
    void stopProcessing();
    
    /**
     * @brief 暂停/恢复音频处理
     */
    void pauseProcessing(bool pause);

protected:
    /**
     * @brief 线程主循环
     */
    void run() override;

private:
    /**
     * @brief 处理单个音频帧
     */
    void processAudioFrame();
    
    /**
     * @brief 双精度音频处理
     */
    void processAudioDouble(const AudioFrame& inputFrame);
    
    /**
     * @brief 单精度音频处理
     */
    void processAudioFloat(const AudioFrame& inputFrame);
    
    /**
     * @brief 计算下次处理时间
     */
    void calculateNextProcessTime();

private:
    // 线程控制
    QAtomicInt running_;
    QAtomicInt paused_;
    QMutex mutex_;
    QWaitCondition condition_;
    
    // 音频参数
    double sampleRate_;
    int blockSize_;
    qint64 frameDurationNs_;  // 帧持续时间（纳秒）
    qint64 nextProcessTime_;  // 下次处理时间
    
    // 音频缓冲区
    std::shared_ptr<AudioTimestampRingQueue> inputBuffer_;
    std::shared_ptr<AudioTimestampRingQueue> outputBuffer_;
    int consumerId_;
    
    // VST3 组件
    Steinberg::IPtr<Steinberg::Vst::IAudioProcessor> audioEffect_;
    AudioProcessingData* processingData_;
    
    // 性能监控
    qint64 lastProcessTime_;
    int processedFrames_;
    double averageProcessTime_;
};

} // namespace Nodes