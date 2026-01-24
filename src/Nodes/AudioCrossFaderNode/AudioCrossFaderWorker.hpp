#pragma once

#include <QObject>
#include <QThread>
#include <QTimer>
#include <QMutex>
#include <QWaitCondition>
#include "DataTypes/AudioTimestampRingQueue.h"
#include "TimeCodeDefines.h"
#include <memory>

namespace Nodes
{
    /**
     * @brief 音频交叉淡化处理工作线程类
     */
    class AudioCrossFaderWorker : public QObject
    {
        Q_OBJECT

    public:
        explicit AudioCrossFaderWorker(QObject *parent = nullptr);
        ~AudioCrossFaderWorker();

    public slots:
        /**
         * @brief 启动处理
         */
        void startProcessing();
        /**
         * @brief 停止处理
         */
        void stopProcessing();
        void processAudioData();
        void onFrameTick(qint64 frameCount);

        /**
         * @brief 设置交叉淡化混合比例 [0.0, 1.0]
         * @param val 0 表示完全 A，1 表示完全 B
         */
        void setMix(double val);
        /**
         * @brief 设置是否使用等功率曲线
         * @param enabled true 使用等功率（Cos/Sin）曲线
         */
        /**
         * @brief 设置淡入淡出时长（毫秒）
         * @param ms 时长，单位毫秒
         */
        void setFadeDuration(double ms);
        /**
         * @brief 启动 A->B 的淡入淡出
         */
        void startFadeAToB();
        /**
         * @brief 启动 B->A 的淡入淡出
         */
        void startFadeBToA();

        void initializeBuffers(int inputCount, int outputCount);
        void setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer);
        std::shared_ptr<AudioTimestampRingQueue> getOutputBuffer(int port);

    signals:
        void processingStatusChanged(bool isProcessing);
        void audioProcessed(const std::vector<std::shared_ptr<AudioTimestampRingQueue>>& outputBuffers);

    private:
        /**
         * @brief 执行交叉淡化混合
         */
        void performCrossFadeOperation(const std::vector<AudioFrame>& inputFrames, qint64 timestamp);

        std::vector<std::shared_ptr<AudioTimestampRingQueue>> _inputBuffers;
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> _outputBuffers;
        QMutex _mutex;
        bool _isProcessing;
        qint64 _lastProcessedTimestamp;

        int _sampleRate = 48000;

        // Crossfade parameters
        float _mix = 0.5f;           // 0.0 -> A, 1.0 -> B（始终使用等功率曲线）
        double _fadeDurationMs = 2000.0; // 淡入淡出时长（毫秒）
        bool _fadingActive = false;
        qint64 _fadeStartFrame = 0;
        qint64 _fadeEndFrame = 0;
        float _fadeStartMix = 0.0f;
        float _fadeTargetMix = 1.0f;

        // 便于连续帧的平滑（可选）
        float _lastMixApplied = 0.0f;
    };
}
