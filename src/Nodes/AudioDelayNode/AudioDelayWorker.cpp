#include "AudioDelayWorker.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include "TimestampGenerator/AudioThreadRealtime.hpp"

#include <QtGlobal>
#include <algorithm>

namespace Nodes
{
    AudioDelayWorker::AudioDelayWorker(QObject *parent)
        : QObject(parent)
    {}

    AudioDelayWorker::~AudioDelayWorker()
    {
        stopProcessing();
    }

    void AudioDelayWorker::startProcessing()
    {
        {
            QMutexLocker locker(&_mutex);
            if (_isProcessing) {
                return;
            }
            _isProcessing = true;
            _lastProcessedByOutput.assign(_outputBuffers.size(), 0);
            for (auto &buf : _inputBuffers) {
                if (buf) {
                    buf->registerFrameWaiter(&_tickWaiter);
                }
            }
        }

        _stopRequested.store(false, std::memory_order_release);
        _tickWaiter.reset();
        _audioThread = std::thread([this]() { audioLoop(); });
        emit processingStatusChanged(true);
    }

    void AudioDelayWorker::stopProcessing()
    {
        _stopRequested.store(true, std::memory_order_release);
        {
            QMutexLocker locker(&_mutex);
            for (auto &buf : _inputBuffers) {
                if (buf) {
                    buf->unregisterFrameWaiter(&_tickWaiter);
                }
            }
        }
        _tickWaiter.requestStop();

        if (_audioThread.joinable()) {
            if (_audioThread.get_id() != std::this_thread::get_id()) {
                _audioThread.join();
            } else {
                _audioThread.detach();
            }
        }

        QMutexLocker locker(&_mutex);
        if (!_isProcessing) {
            return;
        }
        _isProcessing = false;
        emit processingStatusChanged(false);
    }

    void AudioDelayWorker::audioLoop()
    {
        AudioThreadRealtimeGuard realtimeGuard(L"Pro Audio");
        while (!_stopRequested.load(std::memory_order_acquire)
               && !_tickWaiter.isStopRequested()) {
            _tickWaiter.wait(20);
            if (_stopRequested.load(std::memory_order_acquire)
                || _tickWaiter.isStopRequested()) {
                break;
            }
            processCurrentFrame();
        }
    }

    void AudioDelayWorker::processCurrentFrame()
    {
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> inputs;
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> outputs;
        int delayFrames = 0;
        std::vector<qint64> lastByOut;
        {
            QMutexLocker locker(&_mutex);
            if (!_isProcessing || _inputBuffers.empty() || _outputBuffers.empty()) {
                return;
            }
            inputs = _inputBuffers;
            outputs = _outputBuffers;
            delayFrames = _delayFrames;
            if (_lastProcessedByOutput.size() != outputs.size()) {
                _lastProcessedByOutput.assign(outputs.size(), 0);
            }
            lastByOut = _lastProcessedByOutput;
        }

        const qint64 clockTs = TimestampGenerator::getInstance()->getCurrentFrameCount();
        bool anyProgress = false;
        const int count = static_cast<int>(qMin(inputs.size(), outputs.size()));
        for (int ch = 0; ch < count; ++ch) {
            int maxFrames = 2;
            if (clockTs > lastByOut[static_cast<size_t>(ch)] + 2) {
                maxFrames = static_cast<int>(
                    qMin<qint64>(8, clockTs - lastByOut[static_cast<size_t>(ch)]));
            }
            if (processChannel(ch, inputs, outputs, delayFrames,
                               lastByOut[static_cast<size_t>(ch)], maxFrames) > 0) {
                anyProgress = true;
            }
        }

        if (anyProgress) {
            QMutexLocker locker(&_mutex);
            _lastProcessedByOutput = std::move(lastByOut);
        }
    }

    int AudioDelayWorker::processChannel(int ch,
                                         const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &inputs,
                                         const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &outputs,
                                         int delayFrames,
                                         qint64 &inoutLastTs,
                                         int maxFramesPerWake)
    {
        if (ch < 0 || ch >= static_cast<int>(inputs.size()) || ch >= static_cast<int>(outputs.size())) {
            return 0;
        }
        if (!outputs[static_cast<size_t>(ch)] || !inputs[static_cast<size_t>(ch)]
            || !inputs[static_cast<size_t>(ch)]->isActive()) {
            return 0;
        }

        const auto &inQueue = inputs[static_cast<size_t>(ch)];
        qint64 cursor = inoutLastTs;
        int processedCount = 0;
        int holeSkips = 0;
        const int delay = qMax(0, delayFrames);

        while (processedCount < maxFramesPerWake) {
            qint64 nextTs = 0;
            if (!inQueue->peekNextTimestampAfter(cursor, nextTs) || nextTs <= 0) {
                break;
            }

            AudioFrame inFrame;
            if (!inQueue->getFrameByTimestamp(nextTs, inFrame)
                || inFrame.timestamp != nextTs
                || inFrame.data.isEmpty()) {
                cursor = nextTs;
                if (++holeSkips > 16) {
                    break;
                }
                continue;
            }

            AudioFrame outputFrame = inFrame;
            outputFrame.timestamp = nextTs + delay;
            outputs[static_cast<size_t>(ch)]->pushFrame(outputFrame);

            cursor = nextTs;
            ++processedCount;
        }

        if (processedCount > 0) {
            inoutLastTs = cursor;
        }
        return processedCount;
    }

    void AudioDelayWorker::initializeBuffers(int channelCount)
    {
        QMutexLocker locker(&_mutex);
        channelCount = qMax(1, channelCount);

        for (auto &buf : _inputBuffers) {
            if (buf) {
                buf->unregisterFrameWaiter(&_tickWaiter);
            }
        }
        _inputBuffers.resize(static_cast<size_t>(channelCount));

        std::vector<std::shared_ptr<AudioTimestampRingQueue>> oldOutputs = std::move(_outputBuffers);
        _outputBuffers.resize(static_cast<size_t>(channelCount));
        for (int i = 0; i < channelCount; ++i) {
            if (i < static_cast<int>(oldOutputs.size()) && oldOutputs[static_cast<size_t>(i)]) {
                _outputBuffers[static_cast<size_t>(i)] = oldOutputs[static_cast<size_t>(i)];
            } else {
                _outputBuffers[static_cast<size_t>(i)] = std::make_shared<AudioTimestampRingQueue>();
            }
        }
        _lastProcessedByOutput.assign(static_cast<size_t>(channelCount), 0);
    }

    void AudioDelayWorker::setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer)
    {
        QMutexLocker locker(&_mutex);
        if (port < 0 || port >= static_cast<int>(_inputBuffers.size())) {
            return;
        }
        auto &slot = _inputBuffers[static_cast<size_t>(port)];
        if (slot) {
            slot->unregisterFrameWaiter(&_tickWaiter);
        }
        slot = buffer;
        if (slot && _isProcessing) {
            slot->registerFrameWaiter(&_tickWaiter);
        }
    }

    std::shared_ptr<AudioTimestampRingQueue> AudioDelayWorker::getOutputBuffer(int port)
    {
        QMutexLocker locker(&_mutex);
        if (port >= 0 && port < static_cast<int>(_outputBuffers.size())) {
            return _outputBuffers[static_cast<size_t>(port)];
        }
        return nullptr;
    }

    void AudioDelayWorker::setDelayFrames(int frames)
    {
        QMutexLocker locker(&_mutex);
        _delayFrames = qBound(0, frames, 64);
    }
}
