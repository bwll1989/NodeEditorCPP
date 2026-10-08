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
     * @brief QSC Priority Ducker
     * 混音输出口对齐 program[out]+priority（仅 Priority 激活时混入）；priority-only 口只追 priority。
     * Priority 低于阈值或缺失时推进 Hold→Release，恢复节目电平。
     * 由输入环 pushFrame 级联唤醒；缺帧同戳 defer 一次再 Missing
     */
    class AudioPriorityWorker : public QObject
    {
        Q_OBJECT

    public:
        explicit AudioPriorityWorker(QObject *parent = nullptr);
        ~AudioPriorityWorker() override;

        std::shared_ptr<AudioTimestampRingQueue> getOutputBuffer(int port);

    public slots:
        void startProcessing();
        void stopProcessing();

        void setThreshold(double val);
        void setDepth(double val);
        void setPriorityGain(double val);
        void setAttack(double val);
        void setHold(double val);
        void setRelease(double val);

        void initializeBuffers(int inputCount, int outputCount);
        void setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer);

    signals:
        void processingStatusChanged(bool isProcessing);

    private:
        struct Params {
            float thresholdDb = -40.0f;
            float depthDb = 20.0f;
            float priorityGainDb = 0.0f;
            float attackSec = 0.010f;
            float holdSec = 0.200f;
            float releaseSec = 1.0f;
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
        void unregisterAllInputWaitersLocked();
        int processOutput(int outIndex,
                          int priorityIndex,
                          const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &inputs,
                          const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &outputs,
                          const Params &params,
                          qint64 &inoutLastTs,
                          qint64 &inoutDeferredEmptyTs,
                          int maxFramesPerWake,
                          qint64 &inoutEnvelopeTs,
                          qint64 clockTs);
        void updateEnvelopeFromPriority(const ChannelView &priority, const Params &params);
        static ChannelView viewOf(const AudioFrame &frame);
        static float readSample(const ChannelView &view, int frameIndex, int channel, int outChannels);
        static float onePoleDb(float currentDb, float targetDb, float dtSec, float tauSec);

        std::vector<std::shared_ptr<AudioTimestampRingQueue>> _inputBuffers;
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> _outputBuffers;
        std::vector<qint64> _lastProcessedByOutput;
        std::vector<qint64> _deferredEmptyTsByOutput;
        QMutex _mutex;
        bool _isProcessing = false;

        Params _params;
        float _gainDb = 0.0f;
        float _holdLeftSec = 0.0f;
        /** Priority 超过阈值或仍在 Hold 时为真；Release 阶段为假，节目输出不再混入 Priority */
        bool _priorityActive = false;

        AudioTickWaiter _tickWaiter;
        std::atomic<bool> _stopRequested{false};
        std::thread _audioThread;
    };
}
