#include "AudioGateWorker.hpp"
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

    AudioGateWorker::AudioGateWorker(QObject *parent)
        : QObject(parent)
    {}

    AudioGateWorker::~AudioGateWorker()
    {
        stopProcessing();
    }

    void AudioGateWorker::startProcessing()
    {
        {
            QMutexLocker locker(&_mutex);
            if (_isProcessing) {
                return;
            }
            _isProcessing = true;
            _lastProcessedByOutput.assign(_outputBuffers.size(), 0);
            _deferredEmptyTsByOutput.assign(_outputBuffers.size(), 0);
            for (auto &s : _states) {
                s.gainDb = -_params.depthDb;
                s.holdLeftSec = 0.0f;
            }
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

    void AudioGateWorker::stopProcessing()
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

    void AudioGateWorker::audioLoop()
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

    void AudioGateWorker::processCurrentFrame()
    {
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> inputs;
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> outputs;
        Params params;
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
            if (_lastProcessedByOutput.size() != outputs.size()) {
                _lastProcessedByOutput.assign(outputs.size(), 0);
            }
            if (_deferredEmptyTsByOutput.size() != outputs.size()) {
                _deferredEmptyTsByOutput.assign(outputs.size(), 0);
            }
            lastByOut = _lastProcessedByOutput;
            deferredByOut = _deferredEmptyTsByOutput;
            if (static_cast<int>(_states.size()) < static_cast<int>(outputs.size())) {
                const size_t old = _states.size();
                _states.resize(outputs.size());
                for (size_t i = old; i < _states.size(); ++i) {
                    _states[i].gainDb = -params.depthDb;
                    _states[i].holdLeftSec = 0.0f;
                }
            }
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
            if (processChannel(ch, inputs, outputs, params, lastByOut[static_cast<size_t>(ch)],
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
    }

    int AudioGateWorker::processChannel(int ch,
                                        const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &inputs,
                                        const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &outputs,
                                        const Params &params,
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

            const ChannelView view = viewOf(inFrame);
            if (!view.valid) {
                cursor = nextTs;
                continue;
            }

            ChannelState state;
            {
                QMutexLocker locker(&_mutex);
                if (ch >= static_cast<int>(_states.size())) {
                    _states.resize(static_cast<size_t>(ch) + 1);
                    _states[static_cast<size_t>(ch)].gainDb = -params.depthDb;
                }
                state = _states[static_cast<size_t>(ch)];
            }

            const int sampleRate = view.sampleRate;
            const int frames = view.frames;
            const int channels = view.channels > 0 ? view.channels : 1;
            const float dt = static_cast<float>(frames) / static_cast<float>(qMax(1, sampleRate));
            const float rmsDb = rmsDbOf(view);
            const bool above = rmsDb >= params.thresholdDb;
            const float gainStartDb = state.gainDb;

            if (above) {
                state.holdLeftSec = params.holdSec;
                state.gainDb = onePoleDb(state.gainDb, 0.0f, dt, params.attackSec);
            } else if (state.holdLeftSec > 0.0f) {
                state.holdLeftSec = std::max(0.0f, state.holdLeftSec - dt);
                state.gainDb = onePoleDb(state.gainDb, 0.0f, dt, params.attackSec);
            } else {
                state.gainDb = onePoleDb(state.gainDb, -params.depthDb, dt, params.releaseSec);
            }
            const float gainEndDb = state.gainDb;

            std::vector<float> out(static_cast<size_t>(frames * channels), 0.0f);
            const int denom = qMax(1, frames - 1);
            const int srcTotal = view.frames * view.channels;
            for (int i = 0; i < frames; ++i) {
                const float t = static_cast<float>(i) / static_cast<float>(denom);
                const float gainDb = gainStartDb + (gainEndDb - gainStartDb) * t;
                const float gain = std::pow(10.0f, gainDb / 20.0f);
                for (int c = 0; c < channels; ++c) {
                    const int idx = i * channels + c;
                    if (idx < srcTotal) {
                        out[static_cast<size_t>(idx)] = view.data[idx] * gain;
                    }
                }
            }

            AudioFrame outputFrame;
            outputFrame.sampleRate = sampleRate;
            outputFrame.channels = channels;
            outputFrame.bitsPerSample = 32;
            outputFrame.timestamp = nextTs + delayFrames;
            outputFrame.data = QByteArray(reinterpret_cast<const char *>(out.data()),
                                          static_cast<int>(out.size() * sizeof(float)));
            outputs[static_cast<size_t>(ch)]->pushFrame(outputFrame);

            {
                QMutexLocker locker(&_mutex);
                if (ch < static_cast<int>(_states.size())) {
                    _states[static_cast<size_t>(ch)] = state;
                }
            }

            cursor = nextTs;
            ++processedCount;
        }

        if (processedCount > 0) {
            inoutLastTs = cursor;
        }
        return processedCount;
    }

    AudioGateWorker::ChannelView AudioGateWorker::viewOf(const AudioFrame &frame)
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

    float AudioGateWorker::rmsDbOf(const ChannelView &view)
    {
        if (!view.valid) {
            return -100.0f;
        }
        double sumSq = 0.0;
        const int total = view.frames * view.channels;
        for (int i = 0; i < total; ++i) {
            const float s = view.data[i];
            sumSq += static_cast<double>(s) * static_cast<double>(s);
        }
        const float rms = (total > 0) ? std::sqrt(static_cast<float>(sumSq / total)) : 0.0f;
        return (rms > 1.0e-7f) ? 20.0f * std::log10(rms) : -100.0f;
    }

    float AudioGateWorker::onePoleDb(float currentDb, float targetDb, float dtSec, float tauSec)
    {
        if (tauSec <= 0.0001f) {
            return targetDb;
        }
        const float alpha = 1.0f - std::exp(-dtSec / tauSec);
        return currentDb + (targetDb - currentDb) * alpha;
    }

    void AudioGateWorker::initializeBuffers(int channelCount)
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

        const size_t oldStateCount = _states.size();
        _states.resize(static_cast<size_t>(channelCount));
        for (size_t i = oldStateCount; i < _states.size(); ++i) {
            _states[i].gainDb = -_params.depthDb;
            _states[i].holdLeftSec = 0.0f;
        }
        _lastProcessedByOutput.assign(static_cast<size_t>(channelCount), 0);
        _deferredEmptyTsByOutput.assign(static_cast<size_t>(channelCount), 0);
    }

    void AudioGateWorker::setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer)
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

    std::shared_ptr<AudioTimestampRingQueue> AudioGateWorker::getOutputBuffer(int port)
    {
        QMutexLocker locker(&_mutex);
        if (port >= 0 && port < static_cast<int>(_outputBuffers.size())) {
            return _outputBuffers[static_cast<size_t>(port)];
        }
        return nullptr;
    }

    void AudioGateWorker::setThreshold(double val)
    {
        QMutexLocker locker(&_mutex);
        _params.thresholdDb = static_cast<float>(val);
    }

    void AudioGateWorker::setDepth(double val)
    {
        QMutexLocker locker(&_mutex);
        _params.depthDb = static_cast<float>(qBound(0.0, val, 60.0));
    }

    void AudioGateWorker::setAttack(double val)
    {
        QMutexLocker locker(&_mutex);
        _params.attackSec = static_cast<float>(qMax(0.0001, val / 1000.0));
    }

    void AudioGateWorker::setHold(double val)
    {
        QMutexLocker locker(&_mutex);
        _params.holdSec = static_cast<float>(qMax(0.0, val / 1000.0));
    }

    void AudioGateWorker::setRelease(double val)
    {
        QMutexLocker locker(&_mutex);
        _params.releaseSec = static_cast<float>(qMax(0.001, val / 1000.0));
    }
}
