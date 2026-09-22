#include "AudioRouterWorker.hpp"

#include <QtGlobal>
#include <algorithm>
#include <cstring>

namespace Nodes
{
    AudioRouterWorker::AudioRouterWorker(QObject *parent)
        : QObject(parent)
    {}

    AudioRouterWorker::~AudioRouterWorker()
    {
        stopProcessing();
    }

    void AudioRouterWorker::startProcessing()
    {
        {
            QMutexLocker locker(&_mutex);
            if (_isProcessing) {
                return;
            }
            _isProcessing = true;
            _lastProcessedTimestamp = 0;
        }

        _stopRequested.store(false, std::memory_order_release);
        _tickWaiter.reset();
        TimestampGenerator::getInstance()->registerAudioTickWaiter(&_tickWaiter);
        _audioThread = std::thread([this]() { audioLoop(); });
        emit processingStatusChanged(true);
    }

    void AudioRouterWorker::stopProcessing()
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

    void AudioRouterWorker::audioLoop()
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

    void AudioRouterWorker::updateRouting(QVector<int> map)
    {
        QMutexLocker locker(&_mutex);
        _routingMap = std::move(map);
    }

    void AudioRouterWorker::processCurrentFrame()
    {
        // 忽略积压唤醒，始终锚定当前全局时钟
        const qint64 currentFrame = TimestampGenerator::getInstance()->getCurrentFrameCount();

        std::vector<std::shared_ptr<AudioTimestampRingQueue>> inputs;
        std::vector<std::shared_ptr<AudioTimestampRingQueue>> outputs;
        QVector<int> map;
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
            map = _routingMap;
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

        performRouting(inputFrames, outputs, map, currentFrame);

        QMutexLocker locker(&_mutex);
        _lastProcessedTimestamp = currentFrame;
    }

    void AudioRouterWorker::performRouting(const std::vector<AudioFrame> &inputFrames,
                                           const std::vector<std::shared_ptr<AudioTimestampRingQueue>> &outputs,
                                           const QVector<int> &map,
                                           qint64 timestamp)
    {
        constexpr int kSampleRate = 48000;
        const size_t frameSize = static_cast<size_t>(
            TimestampGenerator::getInstance()->getSamplesPerFrame(kSampleRate));
        if (frameSize == 0 || outputs.empty()) {
            return;
        }

        int sampleRate = kSampleRate;
        for (const auto &frame : inputFrames) {
            if (frame.sampleRate > 0) {
                sampleRate = frame.sampleRate;
                break;
            }
        }

        const int delayFrames = TimestampGenerator::getInstance()->getAudioOutputDelayFrames();
        const int outCount = static_cast<int>(outputs.size());
        for (int out = 0; out < outCount; ++out) {
            if (!outputs[static_cast<size_t>(out)]) {
                continue;
            }

            const int src = (out < map.size()) ? map[out] : -1;
            AudioFrame outputFrame;
            outputFrame.timestamp = timestamp + delayFrames;
            outputFrame.sampleRate = sampleRate;
            outputFrame.channels = 1;
            outputFrame.bitsPerSample = 32;
            outputFrame.data.resize(static_cast<int>(frameSize * sizeof(float)));
            float *dst = reinterpret_cast<float *>(outputFrame.data.data());

            if (src >= 0 && src < static_cast<int>(inputFrames.size())
                && !inputFrames[static_cast<size_t>(src)].data.isEmpty()) {
                const AudioFrame &in = inputFrames[static_cast<size_t>(src)];
                if (in.sampleRate > 0) {
                    outputFrame.sampleRate = in.sampleRate;
                }
                const float *srcPtr = reinterpret_cast<const float *>(in.data.constData());
                const size_t available = in.data.size() / sizeof(float);
                const size_t copyCount = std::min(frameSize, available);
                std::memcpy(dst, srcPtr, copyCount * sizeof(float));
                for (size_t i = copyCount; i < frameSize; ++i) {
                    dst[i] = 0.0f;
                }
            } else {
                std::memset(dst, 0, frameSize * sizeof(float));
            }

            outputs[static_cast<size_t>(out)]->pushFrame(outputFrame);
        }
    }

    void AudioRouterWorker::initializeBuffers(int inputCount, int outputCount, QVector<int> map)
    {
        QMutexLocker locker(&_mutex);
        inputCount = qMax(1, inputCount);
        outputCount = qMax(1, outputCount);

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
    }

    void AudioRouterWorker::setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer)
    {
        QMutexLocker locker(&_mutex);
        if (port >= 0 && port < static_cast<int>(_inputBuffers.size())) {
            _inputBuffers[static_cast<size_t>(port)] = buffer;
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
