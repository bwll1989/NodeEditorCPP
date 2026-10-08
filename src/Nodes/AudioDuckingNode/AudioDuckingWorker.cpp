#include "AudioDuckingWorker.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include "TimestampGenerator/AudioThreadRealtime.hpp"

#include <QtGlobal>
#include <algorithm>
#include <cmath>

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

    AudioDuckingWorker::AudioDuckingWorker(QObject *parent)
        : QObject(parent)
    {}

    AudioDuckingWorker::~AudioDuckingWorker()
    {
        stopProcessing();
    }

    void AudioDuckingWorker::startProcessing()
    {
        {
            QMutexLocker locker(&_mutex);
            if (_isProcessing) {
                return;
            }
            _isProcessing = true;
            _lastProcessedByOutput.assign(_outputBuffers.size(), 0);
            _deferredEmptyTsByOutput.assign(_outputBuffers.size(), 0);
            _gainDb = 0.0f;
            _holdLeftSec = 0.0f;
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

    void AudioDuckingWorker::stopProcessing()
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

    void AudioDuckingWorker::audioLoop()
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

    void AudioDuckingWorker::processCurrentFrame()
    {
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> inputs;
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> outputs;
        Params params;
        bool duckActive = false;
        float gainStartDb = 0.0f;
        float gainEndDb = 0.0f;
        std::vector<qint64> lastByOut;
        std::vector<qint64> deferredByOut;
        {
            QMutexLocker locker(&_mutex);
            if (!_isProcessing || _inputBuffers.empty() || _outputBuffers.empty()) {
                return;
            }
            inputs = _inputBuffers;
            outputs = _outputBuffers;
            params = _params;
            duckActive = _duckActive;
            if (_lastProcessedByOutput.size() != outputs.size()) {
                _lastProcessedByOutput.assign(outputs.size(), 0);
            }
            if (_deferredEmptyTsByOutput.size() != outputs.size()) {
                _deferredEmptyTsByOutput.assign(outputs.size(), 0);
            }
            lastByOut = _lastProcessedByOutput;
            deferredByOut = _deferredEmptyTsByOutput;

            // Duck 包络每拍共享推进一次，避免多通道各自推进倍速
            constexpr int kSampleRate = 48000;
            const int envFrames = qMax(1, TimestampGenerator::getInstance()->getSamplesPerFrame(kSampleRate));
            const float dt = static_cast<float>(envFrames) / static_cast<float>(kSampleRate);
            gainStartDb = _gainDb;
            if (duckActive) {
                _holdLeftSec = params.holdSec;
                _gainDb = onePoleDb(_gainDb, -params.depthDb, dt, params.attackSec);
            } else if (_holdLeftSec > 0.0f) {
                _holdLeftSec = std::max(0.0f, _holdLeftSec - dt);
                _gainDb = onePoleDb(_gainDb, -params.depthDb, dt, params.attackSec);
            } else {
                _gainDb = onePoleDb(_gainDb, 0.0f, dt, params.releaseSec);
            }
            gainEndDb = _gainDb;
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
            if (processChannel(ch, inputs, outputs, gainStartDb, gainEndDb,
                               lastByOut[static_cast<size_t>(ch)],
                               deferredByOut[static_cast<size_t>(ch)],
                               clockTs, maxFrames) > 0) {
                anyProgress = true;
            }
        }

        {
            QMutexLocker locker(&_mutex);
            _lastProcessedByOutput = std::move(lastByOut);
            _deferredEmptyTsByOutput = std::move(deferredByOut);
        }
        Q_UNUSED(anyProgress);
    }

    int AudioDuckingWorker::processChannel(int ch,
                                           const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &inputs,
                                           const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &outputs,
                                           float gainStartDb,
                                           float gainEndDb,
                                           qint64 &inoutLastTs,
                                           qint64 &inoutDeferredEmptyTs,
                                           qint64 clockTs,
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
        const int delayFrames = TimestampGenerator::getInstance()->getAudioOutputDelayFrames();
        const qint64 latest = inQueue->latestTimestamp();
        qint64 cursor = inoutLastTs;
        if (cursor <= 0 || cursor < clockTs - 64) {
            cursor = clockTs - 1;
        }

        int processedCount = 0;
        int holeSkips = 0;
        while (processedCount < maxFramesPerWake) {
            const qint64 nextTs = cursor + 1;
            if (latest <= 0 || nextTs > latest) {
                break;
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

            const ChannelView program = viewOf(inFrame);
            if (!program.valid) {
                cursor = nextTs;
                continue;
            }

            const int channels = program.channels;
            const int frames = program.frames;
            const int denom = qMax(1, frames - 1);
            std::vector<float> mixed(static_cast<size_t>(frames * channels), 0.0f);
            for (int i = 0; i < frames; ++i) {
                const float t = static_cast<float>(i) / static_cast<float>(denom);
                const float gainDb = gainStartDb + (gainEndDb - gainStartDb) * t;
                const float duck = std::pow(10.0f, gainDb / 20.0f);
                for (int c = 0; c < channels; ++c) {
                    const int idx = i * channels + c;
                    mixed[static_cast<size_t>(idx)] = program.data[idx] * duck;
                }
            }

            AudioFrame outputFrame;
            outputFrame.sampleRate = program.sampleRate;
            outputFrame.channels = channels;
            outputFrame.bitsPerSample = 32;
            outputFrame.timestamp = nextTs + delayFrames;
            outputFrame.data = QByteArray(reinterpret_cast<const char *>(mixed.data()),
                                          static_cast<int>(mixed.size() * sizeof(float)));
            outputs[static_cast<size_t>(ch)]->pushFrame(outputFrame);

            cursor = nextTs;
            ++processedCount;
        }

        if (processedCount > 0) {
            inoutLastTs = cursor;
        }
        return processedCount;
    }

    AudioDuckingWorker::ChannelView AudioDuckingWorker::viewOf(const AudioFrame &frame)
    {
        ChannelView view;
        if (frame.data.isEmpty() || frame.channels <= 0) {
            return view;
        }
        const int total = frame.data.size() / static_cast<int>(sizeof(float));
        const int frames = total / frame.channels;
        if (frames <= 0) {
            return view;
        }
        view.data = reinterpret_cast<const float *>(frame.data.constData());
        view.channels = frame.channels;
        view.frames = frames;
        view.sampleRate = frame.sampleRate > 0 ? frame.sampleRate : 48000;
        view.valid = true;
        return view;
    }

    float AudioDuckingWorker::onePoleDb(float currentDb, float targetDb, float dtSec, float tauSec)
    {
        if (tauSec <= 0.0001f) {
            return targetDb;
        }
        const float alpha = 1.0f - std::exp(-dtSec / tauSec);
        return currentDb + (targetDb - currentDb) * alpha;
    }

    void AudioDuckingWorker::initializeBuffers(int audioChannelCount)
    {
        QMutexLocker locker(&_mutex);
        audioChannelCount = qMax(1, audioChannelCount);

        for (auto &buf : _inputBuffers) {
            if (buf) {
                buf->unregisterFrameWaiter(&_tickWaiter);
            }
        }
        _inputBuffers.resize(static_cast<size_t>(audioChannelCount));

        std::vector<std::shared_ptr<AudioTimestampRingQueue>> oldOutputs = std::move(_outputBuffers);
        _outputBuffers.resize(static_cast<size_t>(audioChannelCount));
        for (int i = 0; i < audioChannelCount; ++i) {
            if (i < static_cast<int>(oldOutputs.size()) && oldOutputs[static_cast<size_t>(i)]) {
                _outputBuffers[static_cast<size_t>(i)] = oldOutputs[static_cast<size_t>(i)];
            } else {
                _outputBuffers[static_cast<size_t>(i)] = std::make_shared<AudioTimestampRingQueue>();
            }
        }
        _lastProcessedByOutput.assign(static_cast<size_t>(audioChannelCount), 0);
        _deferredEmptyTsByOutput.assign(static_cast<size_t>(audioChannelCount), 0);
    }

    void AudioDuckingWorker::setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer)
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

    std::shared_ptr<AudioTimestampRingQueue> AudioDuckingWorker::getOutputBuffer(int port)
    {
        QMutexLocker locker(&_mutex);
        if (port >= 0 && port < static_cast<int>(_outputBuffers.size())) {
            return _outputBuffers[static_cast<size_t>(port)];
        }
        return nullptr;
    }

    void AudioDuckingWorker::setDepth(double val)
    {
        QMutexLocker locker(&_mutex);
        _params.depthDb = static_cast<float>(qMax(0.0, val));
    }

    void AudioDuckingWorker::setAttack(double val)
    {
        QMutexLocker locker(&_mutex);
        _params.attackSec = static_cast<float>(qMax(0.0005, val / 1000.0));
    }

    void AudioDuckingWorker::setHold(double val)
    {
        QMutexLocker locker(&_mutex);
        _params.holdSec = static_cast<float>(qMax(0.0, val / 1000.0));
    }

    void AudioDuckingWorker::setRelease(double val)
    {
        QMutexLocker locker(&_mutex);
        _params.releaseSec = static_cast<float>(qMax(0.001, val / 1000.0));
    }

    void AudioDuckingWorker::setDuckActive(bool active)
    {
        QMutexLocker locker(&_mutex);
        _duckActive = active;
    }
}
