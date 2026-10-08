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
     * @brief 多路音频时间戳延时
     * PCM 透传，输出 timestamp = 输入 timestamp + delayFrames。
     * 由输入环 pushFrame 级联唤醒。
     */
    class AudioDelayWorker : public QObject
    {
        Q_OBJECT

    public:
        explicit AudioDelayWorker(QObject *parent = nullptr);
        ~AudioDelayWorker() override;

        std::shared_ptr<AudioTimestampRingQueue> getOutputBuffer(int port);

    public slots:
        void startProcessing();
        void stopProcessing();

        void setDelayFrames(int frames);

        void initializeBuffers(int channelCount);
        void setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer);

    signals:
        void processingStatusChanged(bool isProcessing);

    private:
        void audioLoop();
        void processCurrentFrame();
        int processChannel(int ch,
                           const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &inputs,
                           const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &outputs,
                           int delayFrames,
                           qint64 &inoutLastTs,
                           int maxFramesPerWake);

        std::vector<std::shared_ptr<AudioTimestampRingQueue>> _inputBuffers;
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> _outputBuffers;
        std::vector<qint64> _lastProcessedByOutput;
        QMutex _mutex;
        bool _isProcessing = false;
        int _delayFrames = 0;

        AudioTickWaiter _tickWaiter;
        std::atomic<bool> _stopRequested{false};
        std::thread _audioThread;
    };
}
