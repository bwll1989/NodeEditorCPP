#pragma once

#include <QObject>
#include <QMutex>
#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#include "Common/DataTypes/AudioTimestampRingQueue.h"
#include "TimestampGenerator/TimestampGenerator.hpp"

namespace Nodes
{
    /**
     * @brief 多通道音频交叉淡化
     * N 路 A 与 N 路 B 成对淡入淡出，共用 Mix；由 TimestampGenerator 时钟 wake
     */
    class AudioCrossFaderWorker : public QObject
    {
        Q_OBJECT

    public:
        explicit AudioCrossFaderWorker(QObject *parent = nullptr);
        ~AudioCrossFaderWorker() override;

        std::shared_ptr<AudioTimestampRingQueue> getOutputBuffer(int port);

    public slots:
        void startProcessing();
        void stopProcessing();
        void processAudioData();

        void setMix(double val);
        void setFadeDuration(double ms);
        void startFadeAToB();
        void startFadeBToA();

        void initializeBuffers(int channelCount);
        void setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer);

    signals:
        void processingStatusChanged(bool isProcessing);
        void audioProcessed(const std::vector<std::shared_ptr<AudioTimestampRingQueue>>& outputBuffers);

    private:
        struct ChannelView {
            const float *data = nullptr;
            int channels = 0;
            int frames = 0;
            int sampleRate = 0;
            bool valid = false;
        };

        void audioLoop();
        void processCurrentFrame();
        void performCrossFadeOperation(const std::vector<AudioFrame> &inputFrames,
                                       const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &outputs,
                                       qint64 timestamp,
                                       int channelCount,
                                       float mix);
        static ChannelView viewOf(const AudioFrame &frame);

        std::vector<std::shared_ptr<AudioTimestampRingQueue>> _inputBuffers;
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> _outputBuffers;
        QMutex _mutex;
        bool _isProcessing = false;
        qint64 _lastProcessedTimestamp = 0;

        int _channelCount = 1;
        int _sampleRate = 48000;

        float _mix = 0.0f; // 默认输出 A
        double _fadeDurationMs = 2000.0;
        bool _fadingActive = false;
        qint64 _fadeStartFrame = 0;
        qint64 _fadeEndFrame = 0;
        float _fadeStartMix = 0.0f;
        float _fadeTargetMix = 1.0f;

        AudioTickWaiter _tickWaiter;
        std::atomic<bool> _stopRequested{false};
        std::thread _audioThread;
    };
}
