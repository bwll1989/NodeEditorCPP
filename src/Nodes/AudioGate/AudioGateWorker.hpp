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
     * @brief QSC Noise Gate
     * 各通道独立追帧与门限；多源互不拖累。
     * 由输入环 pushFrame 级联唤醒（不经全局时钟抢跑）
     */
    class AudioGateWorker : public QObject
    {
        Q_OBJECT

    public:
        explicit AudioGateWorker(QObject *parent = nullptr);
        ~AudioGateWorker() override;

        std::shared_ptr<AudioTimestampRingQueue> getOutputBuffer(int port);

    public slots:
        void startProcessing();
        void stopProcessing();

        void setThreshold(double val);
        void setDepth(double val);
        void setAttack(double val);
        void setHold(double val);
        void setRelease(double val);

        void initializeBuffers(int channelCount);
        void setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer);

    signals:
        void processingStatusChanged(bool isProcessing);

    private:
        struct Params {
            float thresholdDb = -40.0f;
            float depthDb = 60.0f;
            float attackSec = 0.001f;
            float holdSec = 0.100f;
            float releaseSec = 0.200f;
        };

        struct ChannelState {
            float gainDb = -60.0f;
            float holdLeftSec = 0.0f;
        };

        struct ChannelView {
            const float *data = nullptr;
            int channels = 0;
            int frames = 0;
            int sampleRate = 0;
            bool valid = false;
        };

        void audioLoop();
        void processCurrentFrame();
        int processChannel(int ch,
                           const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &inputs,
                           const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &outputs,
                           const Params &params,
                           qint64 &inoutLastTs,
                           qint64 &inoutDeferredEmptyTs,
                           qint64 clockTs,
                           int maxFramesPerWake);
        static ChannelView viewOf(const AudioFrame &frame);
        static float rmsDbOf(const ChannelView &view);
        static float onePoleDb(float currentDb, float targetDb, float dtSec, float tauSec);

        std::vector<std::shared_ptr<AudioTimestampRingQueue>> _inputBuffers;
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> _outputBuffers;
        std::vector<ChannelState> _states;
        std::vector<qint64> _lastProcessedByOutput;
        std::vector<qint64> _deferredEmptyTsByOutput;
        QMutex _mutex;
        bool _isProcessing = false;
        Params _params;

        AudioTickWaiter _tickWaiter;
        std::atomic<bool> _stopRequested{false};
        std::thread _audioThread;
    };
}
