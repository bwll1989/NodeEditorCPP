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
     * 每路 Out 只对齐对应 A/B 两输入；多组互不拖累。
     * 由输入环 pushFrame 级联唤醒；缺帧同戳 defer 一次再 Missing
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
        void unregisterAllInputWaitersLocked();
        int processOutputPair(int ch,
                              int channelCount,
                              const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &inputs,
                              const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &outputs,
                              qint64 &inoutLastTs,
                              qint64 &inoutDeferredEmptyTs,
                              qint64 clockTs,
                              int maxFramesPerWake);
        float mixAtTimestamp(qint64 timestamp);
        static ChannelView viewOf(const AudioFrame &frame);

        std::vector<std::shared_ptr<AudioTimestampRingQueue>> _inputBuffers;
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> _outputBuffers;
        std::vector<qint64> _lastProcessedByOutput;
        std::vector<qint64> _deferredEmptyTsByOutput;
        QMutex _mutex;
        bool _isProcessing = false;

        int _channelCount = 1;
        int _sampleRate = 48000;

        float _mix = 0.0f;
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
