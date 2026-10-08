#include "AudioRouterWorker.hpp"
#include "TimestampGenerator/AudioThreadRealtime.hpp"

#include <QtGlobal>
#include <algorithm>
#include <cstring>

namespace Nodes
{
    namespace
    {
        bool fetchContentFrame(const std::shared_ptr<AudioTimestampRingQueue> &queue,
                               qint64 contentTs,
                               AudioFrame &outFrame)
        {
            if (!queue) {
                return false;
            }
            return queue->getFrameByTimestamp(contentTs, outFrame)
                && outFrame.timestamp == contentTs
                && !outFrame.data.isEmpty();
        }
    }

    AudioRouterWorker::AudioRouterWorker(QObject *parent)
        : QObject(parent)
    {}

    AudioRouterWorker::~AudioRouterWorker()
    {
        stopProcessing();
    }

    void AudioRouterWorker::unregisterAllInputWaitersLocked()
    {
        for (auto &buf : _inputBuffers) {
            if (buf) {
                buf->unregisterFrameWaiter(&_tickWaiter);
            }
        }
    }

    void AudioRouterWorker::startProcessing()
    {
        {
            QMutexLocker locker(&_mutex);
            if (_isProcessing) {
                return;
            }
            _isProcessing = true;
            _lastProcessedByOutput.assign(_outputBuffers.size(), 0);
            _deferredEmptyTsByOutput.assign(_outputBuffers.size(), 0);
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

    void AudioRouterWorker::stopProcessing()
    {
        _stopRequested.store(true, std::memory_order_release);
        {
            QMutexLocker locker(&_mutex);
            unregisterAllInputWaitersLocked();
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

    void AudioRouterWorker::audioLoop()
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

    void AudioRouterWorker::updateRouting(QVector<int> map)
    {
        QMutexLocker locker(&_mutex);
        _routingMap = std::move(map);
        if (_lastProcessedByOutput.size() != _outputBuffers.size()) {
            _lastProcessedByOutput.assign(_outputBuffers.size(), 0);
        }
    }

    void AudioRouterWorker::processCurrentFrame()
    {
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> inputs;
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> outputs;
        QVector<int> map;
        std::vector<qint64> lastByOut;
        std::vector<qint64> deferredByOut;
        {
            QMutexLocker locker(&_mutex);
            if (!_isProcessing || _inputBuffers.empty() || _outputBuffers.empty()) {
                return;
            }
            inputs = _inputBuffers;
            outputs = _outputBuffers;
            map = _routingMap;
            if (_lastProcessedByOutput.size() != outputs.size()) {
                _lastProcessedByOutput.assign(outputs.size(), 0);
            }
            if (_deferredEmptyTsByOutput.size() != outputs.size()) {
                _deferredEmptyTsByOutput.assign(outputs.size(), 0);
            }
            lastByOut = _lastProcessedByOutput;
            deferredByOut = _deferredEmptyTsByOutput;
        }

        const qint64 clockTs = TimestampGenerator::getInstance()->getCurrentFrameCount();
        bool anyProgress = false;
        const int outCount = static_cast<int>(outputs.size());
        for (int out = 0; out < outCount; ++out) {
            // 每 wake：nextTs / nextTs+1；仅落后 clock 时追赶（不跟 tip 猛追）
            int maxFrames = 2;
            if (clockTs > lastByOut[static_cast<size_t>(out)] + 2) {
                maxFrames = static_cast<int>(
                    qMin<qint64>(8, clockTs - lastByOut[static_cast<size_t>(out)]));
            }
            if (processOutputRoute(out, inputs, outputs, map, lastByOut[static_cast<size_t>(out)],
                                   deferredByOut[static_cast<size_t>(out)],
                                   clockTs, maxFrames) > 0) {
                anyProgress = true;
            }
        }

        {
            QMutexLocker locker(&_mutex);
            _lastProcessedByOutput = std::move(lastByOut);
            _deferredEmptyTsByOutput = std::move(deferredByOut);
        }
    }

    int AudioRouterWorker::processOutputRoute(int out,
                                              const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &inputs,
                                              const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &outputs,
                                              const QVector<int> &map,
                                              qint64 &inoutLastTs,
                                              qint64 &inoutDeferredEmptyTs,
                                              qint64 clockTs,
                                              int maxFramesPerWake)
    {
        if (out < 0 || out >= static_cast<int>(outputs.size()) || !outputs[static_cast<size_t>(out)]) {
            return 0;
        }
        const int src = (out < map.size()) ? map[out] : -1;
        if (src < 0 || src >= static_cast<int>(inputs.size())) {
            return 0;
        }
        const auto &inQueue = inputs[static_cast<size_t>(src)];
        if (!inQueue || !inQueue->isActive()) {
            return 0;
        }

        constexpr int kSampleRate = 48000;
        const size_t frameSize = static_cast<size_t>(
            TimestampGenerator::getInstance()->getSamplesPerFrame(kSampleRate));
        if (frameSize == 0) {
            return 0;
        }

        // 按 nextTs 顺序前进：可超过 clock（把上游 lead 逐次拷到下一级）；
        // 源尚无该戳则等下次 wake；缺帧第二次跳过。写出 nextTs+D。
        const int delayFrames = TimestampGenerator::getInstance()->getAudioOutputDelayFrames();
        const qint64 latest = inQueue->latestTimestamp();
        qint64 cursor = inoutLastTs;
        if (cursor <= 0 || cursor < clockTs - 64) {
            cursor = clockTs - 1; // 首次 / 严重落后：从系统 T 起读 T、T+1
        }

        int processedCount = 0;
        int holeSkips = 0;
        while (processedCount < maxFramesPerWake) {
            const qint64 nextTs = cursor + 1;
            if (latest <= 0 || nextTs > latest) {
                break; // 源还没到这戳，等下次 push
            }

            AudioFrame inFrame;
            if (!fetchContentFrame(inQueue, nextTs, inFrame)) {
                if (inoutDeferredEmptyTs == nextTs) {
                    inoutDeferredEmptyTs = 0;
                    cursor = nextTs;
                    if (++holeSkips > 16) {
                        break;
                    }
                    continue;
                }
                inoutDeferredEmptyTs = nextTs;
                break;
            }
            inoutDeferredEmptyTs = 0;

            AudioFrame outputFrame;
            outputFrame.timestamp = nextTs + delayFrames;
            outputFrame.sampleRate = inFrame.sampleRate > 0 ? inFrame.sampleRate : kSampleRate;
            outputFrame.channels = 1;
            outputFrame.bitsPerSample = 32;
            outputFrame.data.resize(static_cast<int>(frameSize * sizeof(float)));
            float *dst = reinterpret_cast<float *>(outputFrame.data.data());
            const float *srcPtr = reinterpret_cast<const float *>(inFrame.data.constData());
            const size_t available = inFrame.data.size() / sizeof(float);
            const size_t copyCount = qMin(frameSize, available);
            std::memcpy(dst, srcPtr, copyCount * sizeof(float));
            for (size_t i = copyCount; i < frameSize; ++i) {
                dst[i] = 0.0f;
            }

            outputs[static_cast<size_t>(out)]->pushFrame(outputFrame);
            cursor = nextTs;
            ++processedCount;
        }

        if (processedCount > 0) {
            inoutLastTs = cursor;
        }
        return processedCount;
    }

    void AudioRouterWorker::initializeBuffers(int inputCount, int outputCount, QVector<int> map)
    {
        QMutexLocker locker(&_mutex);
        inputCount = qMax(1, inputCount);
        outputCount = qMax(1, outputCount);

        unregisterAllInputWaitersLocked();
        _inputBuffers.resize(static_cast<size_t>(inputCount));

        std::vector<std::shared_ptr<AudioTimestampRingQueue>> oldOutputs = std::move(_outputBuffers);
        _outputBuffers.resize(static_cast<size_t>(outputCount));
        for (int i = 0; i < outputCount; ++i) {
            if (i < static_cast<int>(oldOutputs.size()) && oldOutputs[static_cast<size_t>(i)]) {
                _outputBuffers[static_cast<size_t>(i)] = oldOutputs[static_cast<size_t>(i)];
            } else {
                _outputBuffers[static_cast<size_t>(i)] = std::make_shared<AudioTimestampRingQueue>();
            }
        }

        map.resize(outputCount);
        for (int i = 0; i < outputCount; ++i) {
            if (map[i] < 0 || map[i] >= inputCount) {
                map[i] = -1;
            }
        }
        _routingMap = std::move(map);
        _lastProcessedByOutput.assign(static_cast<size_t>(outputCount), 0);
        _deferredEmptyTsByOutput.assign(static_cast<size_t>(outputCount), 0);
    }

    void AudioRouterWorker::setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer)
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

    std::shared_ptr<AudioTimestampRingQueue> AudioRouterWorker::getOutputBuffer(int port)
    {
        QMutexLocker locker(&_mutex);
        if (port >= 0 && port < static_cast<int>(_outputBuffers.size())) {
            return _outputBuffers[static_cast<size_t>(port)];
        }
        return nullptr;
    }
}
