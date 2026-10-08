#include "AudioCrossFaderWorker.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include "TimestampGenerator/AudioThreadRealtime.hpp"

#include <QtGlobal>
#include <algorithm>
#include <cmath>

#ifndef M_PI_2
#define M_PI_2 1.57079632679489661923
#endif

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

    AudioCrossFaderWorker::AudioCrossFaderWorker(QObject *parent)
        : QObject(parent)
    {
    }

    AudioCrossFaderWorker::~AudioCrossFaderWorker()
    {
        stopProcessing();
    }

    void AudioCrossFaderWorker::unregisterAllInputWaitersLocked()
    {
        for (auto &buf : _inputBuffers) {
            if (buf) {
                buf->unregisterFrameWaiter(&_tickWaiter);
            }
        }
    }

    void AudioCrossFaderWorker::startProcessing()
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

    void AudioCrossFaderWorker::stopProcessing()
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

    void AudioCrossFaderWorker::audioLoop()
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

    void AudioCrossFaderWorker::processAudioData()
    {
    }

    float AudioCrossFaderWorker::mixAtTimestamp(qint64 timestamp)
    {
        QMutexLocker locker(&_mutex);
        if (_fadingActive) {
            if (timestamp >= _fadeEndFrame) {
                _mix = _fadeTargetMix;
                _fadingActive = false;
            } else if (timestamp >= _fadeStartFrame) {
                const qint64 span = std::max<qint64>(1, _fadeEndFrame - _fadeStartFrame);
                const double progress = double(timestamp - _fadeStartFrame) / double(span);
                const float newMix = _fadeStartMix
                    + static_cast<float>(progress) * (_fadeTargetMix - _fadeStartMix);
                _mix = std::clamp(newMix, 0.0f, 1.0f);
            }
        }
        return _mix;
    }

    void AudioCrossFaderWorker::processCurrentFrame()
    {
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> inputs;
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> outputs;
        int channelCount = 1;
        std::vector<qint64> lastByOut;
        std::vector<qint64> deferredByOut;
        {
            QMutexLocker locker(&_mutex);
            if (!_isProcessing || _inputBuffers.empty() || _outputBuffers.empty()) {
                return;
            }
            inputs = _inputBuffers;
            outputs = _outputBuffers;
            channelCount = _channelCount;
            if (_lastProcessedByOutput.size() != outputs.size()) {
                _lastProcessedByOutput.assign(outputs.size(), 0);
            }
            if (_deferredEmptyTsByOutput.size() != outputs.size()) {
                _deferredEmptyTsByOutput.assign(outputs.size(), 0);
            }
            lastByOut = _lastProcessedByOutput;
            deferredByOut = _deferredEmptyTsByOutput;
        }

        if (channelCount <= 0 || static_cast<int>(inputs.size()) < channelCount * 2) {
            return;
        }

        const qint64 clockTs = TimestampGenerator::getInstance()->getCurrentFrameCount();
        bool anyProgress = false;
        const int outCount = qMin(channelCount, static_cast<int>(outputs.size()));
        for (int ch = 0; ch < outCount; ++ch) {
            int maxFrames = 2;
            if (clockTs > lastByOut[static_cast<size_t>(ch)] + 2) {
                maxFrames = static_cast<int>(
                    qMin<qint64>(8, clockTs - lastByOut[static_cast<size_t>(ch)]));
            }
            if (processOutputPair(ch, channelCount, inputs, outputs,
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
        if (anyProgress) {
            emit audioProcessed(outputs);
        }
    }

    int AudioCrossFaderWorker::processOutputPair(int ch,
                                                 int channelCount,
                                                 const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &inputs,
                                                 const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &outputs,
                                                 qint64 &inoutLastTs,
                                                 qint64 &inoutDeferredEmptyTs,
                                                 qint64 clockTs,
                                                 int maxFramesPerWake)
    {
        if (ch < 0 || ch >= static_cast<int>(outputs.size()) || !outputs[static_cast<size_t>(ch)]) {
            return 0;
        }
        const int aIndex = ch;
        const int bIndex = channelCount + ch;
        if (aIndex >= static_cast<int>(inputs.size()) || bIndex >= static_cast<int>(inputs.size())) {
            return 0;
        }

        const bool aConnected = inputs[static_cast<size_t>(aIndex)]
            && inputs[static_cast<size_t>(aIndex)]->isActive();
        const bool bConnected = inputs[static_cast<size_t>(bIndex)]
            && inputs[static_cast<size_t>(bIndex)]->isActive();
        if (!aConnected && !bConnected) {
            return 0;
        }
        const bool multiSourceMix = aConnected && bConnected;
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
        consider(aConnected, aIndex);
        consider(bConnected, bIndex);

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

            AudioFrame frameA;
            AudioFrame frameB;
            bool hasA = aConnected
                && fetchContentFrame(inputs[static_cast<size_t>(aIndex)], nextTs, frameA);
            bool hasB = bConnected
                && fetchContentFrame(inputs[static_cast<size_t>(bIndex)], nextTs, frameB);

            const bool allPresent = (!aConnected || hasA) && (!bConnected || hasB);
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

            const ChannelView viewA = hasA ? viewOf(frameA) : ChannelView{};
            const ChannelView viewB = hasB ? viewOf(frameB) : ChannelView{};
            if (!viewA.valid && !viewB.valid) {
                if (!multiSourceMix) {
                    cursor = nextTs;
                    continue;
                }
                // 多源两路皆空：下面 push 静音戳，保时间线
            }

            const float mix = mixAtTimestamp(nextTs);
            const float theta = mix * static_cast<float>(M_PI_2);
            float wA = std::cos(theta);
            float wB = std::sin(theta);
            if (!viewA.valid && viewB.valid) {
                wA = 0.0f;
                wB = 1.0f;
            } else if (viewA.valid && !viewB.valid) {
                wA = 1.0f;
                wB = 0.0f;
            }

            int channels = 1;
            int frames = 0;
            int sampleRate = _sampleRate;
            if (viewA.valid) {
                channels = viewA.channels;
                frames = viewA.frames;
                sampleRate = viewA.sampleRate;
            }
            if (viewB.valid) {
                channels = std::max(channels, viewB.channels);
                frames = std::max(frames, viewB.frames);
                if (!viewA.valid) {
                    sampleRate = viewB.sampleRate;
                }
            }
            if (channels <= 0) {
                channels = 1;
            }
            if (frames <= 0) {
                frames = qMax(1, TimestampGenerator::getInstance()->getSamplesPerFrame(sampleRate));
            }

            std::vector<float> mixed(static_cast<size_t>(frames * channels), 0.0f);
            for (int i = 0; i < frames; ++i) {
                for (int c = 0; c < channels; ++c) {
                    float a = 0.0f;
                    float b = 0.0f;
                    if (viewA.valid && c < viewA.channels && i < viewA.frames) {
                        a = viewA.data[i * viewA.channels + c];
                    }
                    if (viewB.valid && c < viewB.channels && i < viewB.frames) {
                        b = viewB.data[i * viewB.channels + c];
                    }
                    mixed[static_cast<size_t>(i * channels + c)] = a * wA + b * wB;
                }
            }

            AudioFrame outputFrame;
            outputFrame.sampleRate = sampleRate;
            outputFrame.channels = channels;
            outputFrame.bitsPerSample = 32;
            outputFrame.timestamp = nextTs + delayFrames;
            outputFrame.data = QByteArray(reinterpret_cast<const char *>(mixed.data()),
                                          static_cast<int>(mixed.size() * sizeof(float)));
            outputs[static_cast<size_t>(ch)]->pushFrame(outputFrame);

            cursor = nextTs;
            ++processedCount;
            inoutDeferredEmptyTs = 0;
        }

        if (processedCount > 0) {
            inoutLastTs = cursor;
        }
        return processedCount;
    }

    AudioCrossFaderWorker::ChannelView AudioCrossFaderWorker::viewOf(const AudioFrame &frame)
    {
        ChannelView view;
        if (frame.data.isEmpty() || frame.channels <= 0) {
            return view;
        }
        if (frame.bitsPerSample != 32) {
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

    void AudioCrossFaderWorker::initializeBuffers(int channelCount)
    {
        QMutexLocker locker(&_mutex);
        channelCount = qMax(1, channelCount);
        _channelCount = channelCount;

        const int inputCount = channelCount * 2;
        unregisterAllInputWaitersLocked();
        _inputBuffers.resize(static_cast<size_t>(inputCount));

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
        _deferredEmptyTsByOutput.assign(static_cast<size_t>(channelCount), 0);
    }

    void AudioCrossFaderWorker::setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer)
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

    std::shared_ptr<AudioTimestampRingQueue> AudioCrossFaderWorker::getOutputBuffer(int port)
    {
        QMutexLocker locker(&_mutex);
        if (port >= 0 && port < static_cast<int>(_outputBuffers.size())) {
            return _outputBuffers[static_cast<size_t>(port)];
        }
        return nullptr;
    }

    void AudioCrossFaderWorker::setMix(double val)
    {
        QMutexLocker locker(&_mutex);
        _mix = std::clamp(static_cast<float>(val), 0.0f, 1.0f);
        _fadingActive = false;
    }

    void AudioCrossFaderWorker::setFadeDuration(double ms)
    {
        QMutexLocker locker(&_mutex);
        _fadeDurationMs = ms;
    }

    void AudioCrossFaderWorker::startFadeAToB()
    {
        QMutexLocker locker(&_mutex);
        _fadeStartMix = _mix;
        _fadeTargetMix = 1.0f;
        const qint64 startFrame = TimestampGenerator::getInstance()->getCurrentFrameCount();
        const double frames = _fadeDurationMs / TimestampGenerator::getInstance()->getFrameInterval();
        _fadeStartFrame = startFrame;
        _fadeEndFrame = startFrame + static_cast<qint64>(std::round(frames));
        _fadingActive = true;
    }

    void AudioCrossFaderWorker::startFadeBToA()
    {
        QMutexLocker locker(&_mutex);
        _fadeStartMix = _mix;
        _fadeTargetMix = 0.0f;
        const qint64 startFrame = TimestampGenerator::getInstance()->getCurrentFrameCount();
        const double frames = _fadeDurationMs / TimestampGenerator::getInstance()->getFrameInterval();
        _fadeStartFrame = startFrame;
        _fadeEndFrame = startFrame + static_cast<qint64>(std::round(frames));
        _fadingActive = true;
    }
}
