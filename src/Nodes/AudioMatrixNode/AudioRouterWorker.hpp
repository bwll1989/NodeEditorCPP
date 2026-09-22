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
     * 由 TimestampGenerator 时钟线程直接 wake，不经 QueuedConnection
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
        void performRouting(const std::vector<AudioFrame> &inputFrames,
                            const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &outputs,
                            const QVector<int> &map,
                            qint64 timestamp);

        std::vector<std::shared_ptr<AudioTimestampRingQueue>> _inputBuffers;
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> _outputBuffers;
        QVector<int> _routingMap;
        QMutex _mutex;
        bool _isProcessing = false;
        qint64 _lastProcessedTimestamp = 0;

        AudioTickWaiter _tickWaiter;
        std::atomic<bool> _stopRequested{false};
        std::thread _audioThread;
    };
}
