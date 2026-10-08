#pragma once

#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QAtomicInt>
#include <memory>
#include <map>
#include "NodeDataList.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
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
    void setInputAudioBuffers(int channelIndex,std::shared_ptr<AudioTimestampRingQueue> input);

    std::shared_ptr<AudioTimestampRingQueue> getOutputAudioBuffers(int channelIndex);
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
     * @brief 环级联唤醒后追帧；有输入时写出 nextTs+D
     */
    void processAudioFrame();

    /** 已接线且 active 的输入路数 */
    int connectedInputCount() const;

    /** 所有已接线输入是否都有 contentTs 对应帧 */
    bool allConnectedInputsPresentAt(qint64 contentTs) const;

    /**
     * @brief 双精度音频处理
     * @param targetTimestamp 取帧对齐戳
     * @param outputTimestamp 写出戳（多源为 target+固定延迟）
     */
    void processAudioDouble(qint64 targetTimestamp, qint64 outputTimestamp);

    /**
     * @brief 单精度音频处理
     * @param targetTimestamp 取帧对齐戳
     * @param outputTimestamp 写出戳（多源为 target+固定延迟）
     */
    void processAudioFloat(qint64 targetTimestamp, qint64 outputTimestamp);

    // 线程控制
    QAtomicInt running_;
    QAtomicInt paused_;
    QMutex mutex_;
    QWaitCondition pauseCondition_;
    AudioTickWaiter tickWaiter_;

    // 音频参数
    double sampleRate_;
    int blockSize_;
    int64 lastProcessTimestamp_;
    /** 缺帧 defer：同 nextTs 连续两次失败 → Missing */
    qint64 deferredEmptyTs_ = 0;
    // 音频缓冲区
    std::map<int, std::shared_ptr<AudioTimestampRingQueue>>  inputBuffer_;
    std::map<int, std::shared_ptr<AudioTimestampRingQueue>> outputBuffer_;
    // VST3 组件
    Steinberg::IPtr<Steinberg::Vst::IAudioProcessor> audioEffect_;
    AudioProcessingData* processingData_;

};

} // namespace Nodes
