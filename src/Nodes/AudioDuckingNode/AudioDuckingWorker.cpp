#include "AudioDuckingWorker.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"

#include <QtGlobal>
#include <algorithm>
#include <cmath>
#include <cstring>

namespace Nodes
{
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
            _lastProcessedTimestamp = 0;
            _gainDb = 0.0f;
            _holdLeftSec = 0.0f;
        }

        _stopRequested.store(false, std::memory_order_release);
        _tickWaiter.reset();
        TimestampGenerator::getInstance()->registerAudioTickWaiter(&_tickWaiter);
        _audioThread = std::thread([this]() { audioLoop(); });
        emit processingStatusChanged(true);
    }

    void AudioDuckingWorker::stopProcessing()
    {
        _stopRequested.store(true, std::memory_order_release);
        _tickWaiter.requestStop();
        TimestampGenerator::getInstance()->unregisterAudioTickWaiter(&_tickWaiter);

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
        while (!_stopRequested.load(std::memory_order_acquire)
               && !_tickWaiter.isStopRequested()) {
            if (!_tickWaiter.wait(50)) {
                continue;
            }
            if (_stopRequested.load(std::memory_order_acquire)
                || _tickWaiter.isStopRequested()) {
                break;
            }
            processCurrentFrame();
        }
    }

    void AudioDuckingWorker::processCurrentFrame()
    {
        const qint64 currentFrame = TimestampGenerator::getInstance()->getCurrentFrameCount();

        std::vector<std::shared_ptr<AudioTimestampRingQueue>> inputs;
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> outputs;
        Params params;
        bool duckActive = false;
        {
            QMutexLocker locker(&_mutex);
            if (!_isProcessing || _inputBuffers.empty() || _outputBuffers.empty()) {
                return;
            }
            if (currentFrame == _lastProcessedTimestamp) {
                return;
            }
            inputs = _inputBuffers;
            outputs = _outputBuffers;
            params = _params;
            duckActive = _duckActive;
        }

        std::vector<AudioFrame> inputFrames(inputs.size());
        bool hasAny = false;
        for (size_t i = 0; i < inputs.size(); ++i) {
            if (inputs[i] && inputs[i]->isActive()
                && inputs[i]->getFrameByTimestamp(currentFrame, inputFrames[i])) {
                hasAny = true;
            }
        }
        if (!hasAny) {
            return;
        }

        performDucking(inputFrames, outputs, currentFrame, params, duckActive);

        QMutexLocker locker(&_mutex);
        _lastProcessedTimestamp = currentFrame;
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

    void AudioDuckingWorker::performDucking(const std::vector<AudioFrame> &inputFrames,
                                            const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &outputs,
                                            qint64 timestamp,
                                            const Params &params,
                                            bool duckActive)
    {
        if (inputFrames.empty() || outputs.empty()) {
            return;
        }

        int envFrames = 0;
        int sampleRate = 48000;
        for (const auto &frame : inputFrames) {
            const ChannelView view = viewOf(frame);
            if (view.valid) {
                envFrames = view.frames;
                sampleRate = view.sampleRate;
                break;
            }
        }
        if (envFrames <= 0) {
            envFrames = qMax(1, TimestampGenerator::getInstance()->getSamplesPerFrame(sampleRate));
        }

        const float dt = static_cast<float>(envFrames) / static_cast<float>(qMax(1, sampleRate));
        const float gainStartDb = _gainDb;
        if (duckActive) {
            _holdLeftSec = params.holdSec;
            _gainDb = onePoleDb(_gainDb, -params.depthDb, dt, params.attackSec);
        } else if (_holdLeftSec > 0.0f) {
            _holdLeftSec = std::max(0.0f, _holdLeftSec - dt);
            _gainDb = onePoleDb(_gainDb, -params.depthDb, dt, params.attackSec);
        } else {
            _gainDb = onePoleDb(_gainDb, 0.0f, dt, params.releaseSec);
        }
        const float gainEndDb = _gainDb;

        const int outCount = static_cast<int>(outputs.size());
        const int inCount = static_cast<int>(inputFrames.size());
        for (int outIndex = 0; outIndex < outCount; ++outIndex) {
            if (!outputs[static_cast<size_t>(outIndex)]) {
                continue;
            }

            ChannelView program;
            if (outIndex < inCount) {
                program = viewOf(inputFrames[static_cast<size_t>(outIndex)]);
            }

            int channels = program.valid ? program.channels : 1;
            int frames = program.valid ? program.frames : envFrames;
            if (channels <= 0) {
                channels = 1;
            }
            if (frames <= 0) {
                frames = envFrames;
            }

            std::vector<float> mixed(static_cast<size_t>(frames * channels), 0.0f);
            if (program.valid) {
                const int denom = qMax(1, frames - 1);
                const int total = frames * channels;
                for (int i = 0; i < frames; ++i) {
                    const float t = static_cast<float>(i) / static_cast<float>(denom);
                    const float gainDb = gainStartDb + (gainEndDb - gainStartDb) * t;
                    const float duck = std::pow(10.0f, gainDb / 20.0f);
                    for (int c = 0; c < channels; ++c) {
                        const int idx = i * channels + c;
                        if (idx < total && idx < program.frames * program.channels) {
                            mixed[static_cast<size_t>(idx)] = program.data[idx] * duck;
                        }
                    }
                }
            }

            AudioFrame outputFrame;
            outputFrame.sampleRate = program.valid ? program.sampleRate : sampleRate;
            outputFrame.channels = channels;
            outputFrame.bitsPerSample = 32;
            outputFrame.timestamp = timestamp + TimestampGenerator::getInstance()->getAudioOutputDelayFrames();
            outputFrame.data = QByteArray(reinterpret_cast<const char *>(mixed.data()),
                                          static_cast<int>(mixed.size() * sizeof(float)));
            outputs[static_cast<size_t>(outIndex)]->pushFrame(outputFrame);
        }
    }

    void AudioDuckingWorker::initializeBuffers(int audioChannelCount)
    {
        QMutexLocker locker(&_mutex);
        audioChannelCount = qMax(1, audioChannelCount);

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
    }

    void AudioDuckingWorker::setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer)
    {
        QMutexLocker locker(&_mutex);
        if (port >= 0 && port < static_cast<int>(_inputBuffers.size())) {
            _inputBuffers[static_cast<size_t>(port)] = buffer;
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
