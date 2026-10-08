#pragma once

#include <QObject>
#include <QMutex>
#include <QVector>
#include <atomic>
#include <memory>
#include <thread>
#include <vector>

#include "Common/DataTypes/AudioTimestampRingQueue.h"
#include "TimestampGenerator/TimestampGenerator.hpp"

namespace Nodes
{
    /**
     * @brief 音频路由工作线程：按 routingMap 将指定输入整帧拷贝到对应输出（无增益混音）
     * routingMap[out] = in，未连接为 -1
     * 由输入环 pushFrame 级联唤醒（不经全局时钟抢跑）
     */
    class AudioRouterWorker : public QObject
    {
        Q_OBJECT

    public:
        explicit AudioRouterWorker(QObject *parent = nullptr);
        ~AudioRouterWorker() override;

        std::shared_ptr<AudioTimestampRingQueue> getOutputBuffer(int port);

    public slots:
        void startProcessing();
        void stopProcessing();

        void updateRouting(QVector<int> map);
        void initializeBuffers(int inputCount, int outputCount, QVector<int> map);
        void setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer);

    signals:
        void processingStatusChanged(bool isProcessing);

    private:
        void audioLoop();
        void processCurrentFrame();
        void unregisterAllInputWaitersLocked();
        int processOutputRoute(int out,
                               const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &inputs,
                               const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &outputs,
                               const QVector<int> &map,
                               qint64 &inoutLastTs,
                               qint64 &inoutDeferredEmptyTs,
                               qint64 clockTs,
                               int maxFramesPerWake);

        std::vector<std::shared_ptr<AudioTimestampRingQueue>> _inputBuffers;
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> _outputBuffers;
        QVector<int> _routingMap;
        /** 每个输出口各自的已处理时间戳，避免多源共用一个 cursor */
        std::vector<qint64> _lastProcessedByOutput;
        std::vector<qint64> _deferredEmptyTsByOutput;
        QMutex _mutex;
        bool _isProcessing = false;

        AudioTickWaiter _tickWaiter;
        std::atomic<bool> _stopRequested{false};
        std::thread _audioThread;
    };
}
