#include "AudioPriorityWorker.hpp"
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

    AudioPriorityWorker::AudioPriorityWorker(QObject *parent)
        : QObject(parent)
    {}

    AudioPriorityWorker::~AudioPriorityWorker()
    {
        stopProcessing();
    }

    void AudioPriorityWorker::unregisterAllInputWaitersLocked()
    {
        for (auto &buf : _inputBuffers) {
            if (buf) {
                buf->unregisterFrameWaiter(&_tickWaiter);
            }
        }
    }

    void AudioPriorityWorker::startProcessing()
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
            _priorityActive = false;
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

    void AudioPriorityWorker::stopProcessing()
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

    void AudioPriorityWorker::audioLoop()
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

    void AudioPriorityWorker::processCurrentFrame()
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
        }

        const int priorityIndex = static_cast<int>(inputs.size()) - 1;
        if (priorityIndex < 0) {
            return;
        }

        const qint64 clockTs = TimestampGenerator::getInstance()->getCurrentFrameCount();
        qint64 envelopeTs = -1;
        bool anyProgress = false;
        const int outCount = static_cast<int>(outputs.size());
        for (int out = 0; out < outCount; ++out) {
            int maxFrames = 2;
            if (clockTs > lastByOut[static_cast<size_t>(out)] + 2) {
                maxFrames = static_cast<int>(
                    qMin<qint64>(8, clockTs - lastByOut[static_cast<size_t>(out)]));
            }
            if (processOutput(out, priorityIndex, inputs, outputs, params,
                              lastByOut[static_cast<size_t>(out)],
                              deferredByOut[static_cast<size_t>(out)],
                              maxFrames, envelopeTs, clockTs) > 0) {
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

    void AudioPriorityWorker::updateEnvelopeFromPriority(const ChannelView &priority, const Params &params)
    {
        float rmsDb = -100.0f;
        int envFrames = 0;
        int sampleRate = 48000;
        if (priority.valid) {
            double sumSq = 0.0;
            const int total = priority.frames * priority.channels;
            for (int i = 0; i < total; ++i) {
                const float s = priority.data[i];
                sumSq += static_cast<double>(s) * static_cast<double>(s);
            }
            const float rms = (total > 0) ? std::sqrt(static_cast<float>(sumSq / total)) : 0.0f;
            rmsDb = (rms > 1.0e-7f) ? 20.0f * std::log10(rms) : -100.0f;
            envFrames = priority.frames;
            sampleRate = priority.sampleRate;
        }
        if (envFrames <= 0) {
            envFrames = qMax(1, TimestampGenerator::getInstance()->getSamplesPerFrame(sampleRate));
        }

        const float dt = static_cast<float>(envFrames) / static_cast<float>(qMax(1, sampleRate));
        QMutexLocker locker(&_mutex);
        const bool above = priority.valid && rmsDb >= params.thresholdDb;
        if (above) {
            _holdLeftSec = params.holdSec;
            _priorityActive = true;
            _gainDb = onePoleDb(_gainDb, -params.depthDb, dt, params.attackSec);
        } else if (_holdLeftSec > 0.0f) {
            _holdLeftSec = std::max(0.0f, _holdLeftSec - dt);
            _priorityActive = true;
            _gainDb = onePoleDb(_gainDb, -params.depthDb, dt, params.attackSec);
        } else {
            // Priority 低于阈值 / 缺失：进入 Release，节目恢复，且不再混入 Priority
            _priorityActive = false;
            _gainDb = onePoleDb(_gainDb, 0.0f, dt, params.releaseSec);
        }
    }

    int AudioPriorityWorker::processOutput(int outIndex,
                                           int priorityIndex,
                                           const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &inputs,
                                           const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &outputs,
                                           const Params &params,
                                           qint64 &inoutLastTs,
                                           qint64 &inoutDeferredEmptyTs,
                                           int maxFramesPerWake,
                                           qint64 &inoutEnvelopeTs,
                                           qint64 clockTs)
    {
        if (outIndex < 0 || outIndex >= static_cast<int>(outputs.size())
            || !outputs[static_cast<size_t>(outIndex)]) {
            return 0;
        }

        const bool priorityOnly = (outIndex >= priorityIndex);
        const bool programConnected = !priorityOnly
            && outIndex < static_cast<int>(inputs.size())
            && inputs[static_cast<size_t>(outIndex)]
            && inputs[static_cast<size_t>(outIndex)]->isActive();
        const bool priorityConnected = priorityIndex >= 0
            && priorityIndex < static_cast<int>(inputs.size())
            && inputs[static_cast<size_t>(priorityIndex)]
            && inputs[static_cast<size_t>(priorityIndex)]->isActive();
        const bool multiSourceMix = !priorityOnly && programConnected && priorityConnected;
        const int delayFrames = TimestampGenerator::getInstance()->getAudioOutputDelayFrames();
        constexpr qint64 kTipStaleFrames = 16;

        qint64 tipLatest = 0;
        bool haveTip = false;
        auto consider = [&](bool connected, int idx) {
            if (!connected) {
                return;
            }
            const qint64 L = inputs[static_cast<size_t>(idx)]->latestTimestamp();
            if (L <= 0) {
                return;
            }
            if (clockTs > L && (clockTs - L) > kTipStaleFrames) {
                return;
            }
            if (!haveTip || L < tipLatest) {
                tipLatest = L;
            }
            haveTip = true;
        };
        consider(programConnected, outIndex);
        consider(priorityConnected, priorityIndex);

        qint64 cursor = inoutLastTs;
        if (cursor <= 0 || cursor < clockTs - 64) {
            cursor = clockTs - 1;
        }
        const int framesThisWake = multiSourceMix ? 2 : maxFramesPerWake;

        int processedCount = 0;
        int holeSkips = 0;
        while (processedCount < framesThisWake) {
            const qint64 nextTs = cursor + 1;
            if (!haveTip || nextTs > tipLatest) {
                break; // NotYet：绝不填 0
            }

            AudioFrame programFrame;
            AudioFrame priorityFrame;
            const bool hasProgram = programConnected
                && fetchContentFrame(inputs[static_cast<size_t>(outIndex)], nextTs, programFrame);
            const bool hasPriority = priorityConnected
                && fetchContentFrame(inputs[static_cast<size_t>(priorityIndex)], nextTs, priorityFrame);

            const bool needProgram = programConnected;
            const bool needPriority = priorityConnected;
            const bool allPresent = (!needProgram || hasProgram) && (!needPriority || hasPriority);
            if (!allPresent) {
                if (inoutDeferredEmptyTs != nextTs) {
                    inoutDeferredEmptyTs = nextTs;
                    break;
                }
                inoutDeferredEmptyTs = 0;
                if (!multiSourceMix) {
                    cursor = nextTs;
                    if (++holeSkips > 16) {
                        break;
                    }
                    continue;
                }
                // 多源二次缺：缺路按 0 混，下面仍 push
            } else {
                inoutDeferredEmptyTs = 0;
            }

            const ChannelView program = hasProgram ? viewOf(programFrame) : ChannelView{};
            const ChannelView priority = hasPriority ? viewOf(priorityFrame) : ChannelView{};

            // 无论 Priority 是否到齐都推进包络：缺失/低于阈值时进入 Hold→Release，才能恢复节目电平
            if (nextTs != inoutEnvelopeTs) {
                updateEnvelopeFromPriority(priority, params);
                inoutEnvelopeTs = nextTs;
            }

            float gainStartDb = 0.0f;
            float gainEndDb = 0.0f;
            float priorityLinear = 1.0f;
            bool mixPriority = false;
            {
                QMutexLocker locker(&_mutex);
                gainStartDb = _gainDb;
                gainEndDb = _gainDb;
                priorityLinear = std::pow(10.0f, params.priorityGainDb / 20.0f);
                mixPriority = _priorityActive;
            }

            const ChannelView &layout = program.valid ? program : priority;
            int channels = layout.valid ? layout.channels : 1;
            int frames = layout.valid ? layout.frames : 0;
            if (program.valid && priority.valid) {
                frames = std::max(program.frames, priority.frames);
            }
            if (channels <= 0) {
                channels = 1;
            }
            if (frames <= 0) {
                if (!multiSourceMix) {
                    cursor = nextTs;
                    continue;
                }
                // 多源皆空：push 静音戳
                frames = qMax(1, TimestampGenerator::getInstance()->getSamplesPerFrame(48000));
            }

            std::vector<float> mixed(static_cast<size_t>(frames * channels), 0.0f);
            const int denom = qMax(1, frames - 1);
            for (int i = 0; i < frames; ++i) {
                const float t = static_cast<float>(i) / static_cast<float>(denom);
                const float gainDb = gainStartDb + (gainEndDb - gainStartDb) * t;
                const float duck = std::pow(10.0f, gainDb / 20.0f);
                for (int c = 0; c < channels; ++c) {
                    float sample = 0.0f;
                    if (!priorityOnly) {
                        sample += readSample(program, i, c, channels) * duck;
                        // 仅在 Priority 激活（超阈值或 Hold）时混入节目输出
                        if (mixPriority) {
                            sample += readSample(priority, i, c, channels) * priorityLinear;
                        }
                    } else {
                        sample = readSample(priority, i, c, channels) * priorityLinear;
                    }
                    mixed[static_cast<size_t>(i * channels + c)] = sample;
                }
            }

            AudioFrame outputFrame;
            outputFrame.sampleRate = layout.valid ? layout.sampleRate : 48000;
            outputFrame.channels = channels;
            outputFrame.bitsPerSample = 32;
            outputFrame.timestamp = nextTs + delayFrames;
            outputFrame.data = QByteArray(reinterpret_cast<const char *>(mixed.data()),
                                          static_cast<int>(mixed.size() * sizeof(float)));
            outputs[static_cast<size_t>(outIndex)]->pushFrame(outputFrame);

            cursor = nextTs;
            ++processedCount;
            inoutDeferredEmptyTs = 0;
        }

        if (processedCount > 0) {
            inoutLastTs = cursor;
        }
        return processedCount;
    }

    AudioPriorityWorker::ChannelView AudioPriorityWorker::viewOf(const AudioFrame &frame)
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

    float AudioPriorityWorker::readSample(const ChannelView &view, int frameIndex, int channel, int outChannels)
    {
        if (!view.valid || frameIndex < 0 || frameIndex >= view.frames || outChannels <= 0) {
            return 0.0f;
        }
        if (view.channels == 1) {
            return view.data[frameIndex];
        }
        if (view.channels == outChannels) {
            return view.data[frameIndex * view.channels + channel];
        }
        if (outChannels == 1) {
            float sum = 0.0f;
            for (int c = 0; c < view.channels; ++c) {
                sum += view.data[frameIndex * view.channels + c];
            }
            return sum / static_cast<float>(view.channels);
        }
        if (channel >= 0 && channel < view.channels) {
            return view.data[frameIndex * view.channels + channel];
        }
        return 0.0f;
    }

    float AudioPriorityWorker::onePoleDb(float currentDb, float targetDb, float dtSec, float tauSec)
    {
        if (tauSec <= 0.0001f) {
            return targetDb;
        }
        const float alpha = 1.0f - std::exp(-dtSec / tauSec);
        return currentDb + (targetDb - currentDb) * alpha;
    }

    void AudioPriorityWorker::initializeBuffers(int inputCount, int outputCount)
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
        _lastProcessedByOutput.assign(static_cast<size_t>(outputCount), 0);
        _deferredEmptyTsByOutput.assign(static_cast<size_t>(outputCount), 0);
    }

    void AudioPriorityWorker::setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer)
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

    std::shared_ptr<AudioTimestampRingQueue> AudioPriorityWorker::getOutputBuffer(int port)
    {
        QMutexLocker locker(&_mutex);
        if (port >= 0 && port < static_cast<int>(_outputBuffers.size())) {
            return _outputBuffers[static_cast<size_t>(port)];
        }
        return nullptr;
    }

    void AudioPriorityWorker::setThreshold(double val)
    {
        QMutexLocker locker(&_mutex);
        _params.thresholdDb = static_cast<float>(val);
    }

    void AudioPriorityWorker::setDepth(double val)
    {
        QMutexLocker locker(&_mutex);
        _params.depthDb = static_cast<float>(qMax(0.0, val));
    }

    void AudioPriorityWorker::setPriorityGain(double val)
    {
        QMutexLocker locker(&_mutex);
        _params.priorityGainDb = static_cast<float>(val);
    }

    void AudioPriorityWorker::setAttack(double val)
    {
        QMutexLocker locker(&_mutex);
        _params.attackSec = static_cast<float>(qMax(0.0005, val / 1000.0));
    }

    void AudioPriorityWorker::setHold(double val)
    {
        QMutexLocker locker(&_mutex);
        _params.holdSec = static_cast<float>(qMax(0.0, val / 1000.0));
    }

    void AudioPriorityWorker::setRelease(double val)
    {
        QMutexLocker locker(&_mutex);
        _params.releaseSec = static_cast<float>(qMax(0.001, val / 1000.0));
    }
}
