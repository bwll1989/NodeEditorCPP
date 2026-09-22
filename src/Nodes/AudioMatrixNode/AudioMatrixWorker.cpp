#include "AudioMatrixWorker.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include <QDebug>
#include <QtGlobal>
#include <utility>

namespace Nodes
{
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    AudioMatrixWorker::AudioMatrixWorker(QObject *parent)
        : QObject(parent)
    {}
    
    /**
     * @brief 析构函数
     */
    AudioMatrixWorker::~AudioMatrixWorker()
    {
        stopProcessing();
    }
    
   
    
    /**
     * @brief 开始处理音频数据
     */
    void AudioMatrixWorker::startProcessing()
    {
        {
            QMutexLocker locker(&_mutex);
            if (_isProcessing) {
                qDebug() << "AudioMatrixWorker: Already processing, ignoring duplicate call";
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

    void AudioMatrixWorker::stopProcessing()
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

    void AudioMatrixWorker::audioLoop()
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
    
    /**
     * @brief 更新矩阵数据
     * @param matrix 新的矩阵数据
     */
    void AudioMatrixWorker::updateMatrix(const Eigen::MatrixXd& matrix)
    {
        QMutexLocker locker(&_mutex);
        _matrix = matrix;
    }
    
    /**
     * @brief 处理音频数据的主循环
     */
    void AudioMatrixWorker::processAudioData()
    {
        if (!_isProcessing || _inputBuffers.empty() || _outputBuffers.empty()) {
            return;
        }
        
        auto currentTime = TimestampGenerator::getInstance()->getCurrentFrameCount();
        
        if (currentTime == _lastProcessedTimestamp) {
            return;
        }
        
        // 收集所有输入通道的音频帧
        std::vector<AudioFrame> inputFrames(_inputBuffers.size());
        bool hasValidInput = false;

        for (size_t i = 0; i < _inputBuffers.size(); ++i) {
            if (_inputBuffers[i] && _inputBuffers[i]->isActive()) {
                if (_inputBuffers[i]->getFrameByTimestamp(currentTime, inputFrames[i])) {
                    hasValidInput = true;
                }
            }
        }

        if (!hasValidInput) {
            return;
        }

        // 执行矩阵运算
        performMatrixOperation(inputFrames, currentTime);
        
        _lastProcessedTimestamp = currentTime;
        
        // 发送处理完成信号
        emit audioProcessed(_outputBuffers);
    }

    void AudioMatrixWorker::processCurrentFrame()
    {
        if (!_isProcessing || _inputBuffers.empty() || _outputBuffers.empty()) {
            return;
        }
        const qint64 currentFrame = TimestampGenerator::getInstance()->getCurrentFrameCount();
        if (currentFrame == _lastProcessedTimestamp) {
            return;
        }
        std::vector<AudioFrame> inputFrames(_inputBuffers.size());
        bool hasValidInput = false;
        for (size_t i = 0; i < _inputBuffers.size(); ++i) {
            if (_inputBuffers[i] && _inputBuffers[i]->isActive()) {
                if (_inputBuffers[i]->getFrameByTimestamp(currentFrame, inputFrames[i])) {
                    hasValidInput = true;
                }
            }
        }
        if (!hasValidInput) {
            return;
        }
        performMatrixOperation(inputFrames, currentFrame);
        _lastProcessedTimestamp = currentFrame;
        emit audioProcessed(_outputBuffers);
    }
    
    /**
     * @brief 使用矩阵运算执行音频矩阵操作
     * @param inputFrames 输入音频帧向量
     * @param timestamp 时间戳
     */
    void AudioMatrixWorker::performMatrixOperation(const std::vector<AudioFrame>& inputFrames, qint64 timestamp)
    {
        if (inputFrames.empty()) {
            return;
        }

        constexpr int kSampleRate = 48000;
        const size_t frameSize = static_cast<size_t>(
            TimestampGenerator::getInstance()->getSamplesPerFrame(kSampleRate));
        if (frameSize == 0) {
            return;
        }

        const int inputChannels = _matrix.rows();
        const int outputChannels = _matrix.cols();

        int sampleRate = kSampleRate;
        for (const auto& frame : inputFrames) {
            if (frame.sampleRate > 0) {
                sampleRate = frame.sampleRate;
                break;
            }
        }
        
        // 创建输入矩阵：inputChannels × frameSize
        Eigen::MatrixXf inputMatrix(inputChannels, frameSize);
        
        // 填充输入矩阵数据
        for (int inChannel = 0; inChannel < inputChannels; ++inChannel) {
            if (inChannel < static_cast<int>(inputFrames.size())) {
                const auto& frameData = inputFrames[inChannel].data;
                const float* audioSamples = reinterpret_cast<const float*>(frameData.constData());
                const size_t availableSamples = frameData.size() / sizeof(float);
                const size_t samplesToCopy = std::min(frameSize, availableSamples);
                
                // 直接填充到矩阵中
                for (size_t sample = 0; sample < samplesToCopy; ++sample) {
                    inputMatrix(inChannel, sample) = audioSamples[sample];
                }
                
                // 如果数据不足，填充零
                for (size_t sample = samplesToCopy; sample < frameSize; ++sample) {
                    inputMatrix(inChannel, sample) = 0.0f;
                }
            } else {
                // 通道不存在，填充零
                inputMatrix.row(inChannel).setZero();
            }
        }
        
        // 执行矩阵乘法：outputMatrix = _matrix^T × inputMatrix
        // 结果矩阵维度：outputChannels × frameSize
        Eigen::MatrixXf outputMatrix = _matrix.transpose().cast<float>() * inputMatrix;
        
        // 提取每个输出通道的数据并创建音频帧
        for (int outChannel = 0; outChannel < outputChannels; ++outChannel) {
            if (outChannel < static_cast<int>(_outputBuffers.size()) && _outputBuffers[outChannel]) {
                AudioFrame outputFrame;
                outputFrame.timestamp = timestamp + TimestampGenerator::getInstance()->getAudioOutputDelayFrames();
                outputFrame.sampleRate = sampleRate;
                outputFrame.channels = 1;
                outputFrame.bitsPerSample = 32;
                
                // 分配输出数据内存
                outputFrame.data.resize(frameSize * sizeof(float));
                float* outputData = reinterpret_cast<float*>(outputFrame.data.data());
                
                // 从输出矩阵提取当前通道的数据
                for (size_t sample = 0; sample < frameSize; ++sample) {
                    float outputSample = outputMatrix(outChannel, sample);
                    
                    // 应用幅度限制（削波保护）
                    if (outputSample > 1.0f) outputSample = 1.0f;
                    else if (outputSample < -1.0f) outputSample = -1.0f;
                    
                    outputData[sample] = outputSample;
                }
                
                // 将处理后的音频帧添加到输出缓冲区
                _outputBuffers[outChannel]->pushFrame(outputFrame);
            }
        }
    }
    /**
 * @brief 初始化缓冲区
 * @param inputCount 输入端口数量
 * @param outputCount 输出端口数量
 * @param matrix 矩阵数据
 */
void AudioMatrixWorker::initializeBuffers(int inputCount, int outputCount, const Eigen::MatrixXd& matrix)
{
    QMutexLocker locker(&_mutex);

    inputCount = qMax(1, inputCount);
    outputCount = qMax(1, outputCount);

    // 输入：保留已有范围内的连接；超出部分自然丢弃，新增为空（由 setInData 填入）
    _inputBuffers.resize(static_cast<size_t>(inputCount));

    // 输出：尽量保留已有 ring queue 的 shared_ptr，避免下游断链
    std::vector<std::shared_ptr<AudioTimestampRingQueue>> oldOutputs = std::move(_outputBuffers);
    _outputBuffers.resize(static_cast<size_t>(outputCount));
    for (int i = 0; i < outputCount; ++i) {
        if (i < static_cast<int>(oldOutputs.size()) && oldOutputs[static_cast<size_t>(i)]) {
            _outputBuffers[static_cast<size_t>(i)] = oldOutputs[static_cast<size_t>(i)];
        } else {
            _outputBuffers[static_cast<size_t>(i)] = std::make_shared<AudioTimestampRingQueue>();
        }
    }

    _matrix = matrix;
}

/**
 * @brief 设置指定末端端口缓冲区
 * @param port 端口索引
 * @param buffer 音频缓冲区
 */
void AudioMatrixWorker::setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer)
{
    QMutexLocker locker(&_mutex);
    
    if (port >= 0 && port < static_cast<int>(_inputBuffers.size())) {
        _inputBuffers[port] = buffer;

    } else {
        qWarning() << "AudioMatrixWorker: Invalid input port index:" << port 
                   << "(valid range: 0 -" << (_inputBuffers.size() - 1) << ")";
    }
}

/**
 * @brief 获取指定输出端口的缓冲区
 * @param port 端口索引
 * @return 输出音频缓冲区
 */
std::shared_ptr<AudioTimestampRingQueue> AudioMatrixWorker::getOutputBuffer(int port)
{

    if (port >= 0 && port < static_cast<int>(_outputBuffers.size())) {
        return _outputBuffers[port];
    }
        return nullptr;
    
}
}

