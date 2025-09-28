#include "AudioMatrixWorker.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include <QDebug>

namespace Nodes
{
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    AudioMatrixWorker::AudioMatrixWorker(QObject *parent)
        : QObject(parent)
        , _processingTimer(new QTimer(this))
        , _isProcessing(false)
        , _lastProcessedTimestamp(0)
    {

        // 设置处理定时器
        _processingTimer->setInterval(15); // 15ms间隔处理音频数据
        connect(_processingTimer, &QTimer::timeout, this, &AudioMatrixWorker::processAudioData);

    }
    
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
        QMutexLocker locker(&_mutex);
        // 如果已经在处理，直接返回
        if (_isProcessing) {
            qDebug() << "AudioMatrixWorker: Already processing, ignoring duplicate call";
            return;
        }
        _isProcessing = true;
        _lastProcessedTimestamp = 0;
        // 启动处理定时器
        _processingTimer->start();
        emit processingStatusChanged(true);
    }
    
    /**
     * @brief 停止处理音频数据
     */
    void AudioMatrixWorker::stopProcessing()
    {
        QMutexLocker locker(&_mutex);
        
        if (_isProcessing) {
            _isProcessing = false;
            
            // 停止处理定时器
            if (_processingTimer->isActive()) {
                _processingTimer->stop();
            }
            
            emit processingStatusChanged(false);

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
        
        const size_t frameSize = inputFrames[0].data.size() / sizeof(int16_t);
        const int inputChannels = _matrix.rows();
        const int outputChannels = _matrix.cols();
        
        // 创建输入矩阵：inputChannels × frameSize
        Eigen::MatrixXf inputMatrix(inputChannels, frameSize);
        
        // 填充输入矩阵数据
        for (int inChannel = 0; inChannel < inputChannels; ++inChannel) {
            if (inChannel < static_cast<int>(inputFrames.size())) {
                const auto& frameData = inputFrames[inChannel].data;
                const int16_t* audioSamples = reinterpret_cast<const int16_t*>(frameData.constData());
                const size_t availableSamples = frameData.size() / sizeof(int16_t);
                const size_t samplesToCopy = std::min(frameSize, availableSamples);
                
                // 将int16_t转换为float并填充到矩阵中
                for (size_t sample = 0; sample < samplesToCopy; ++sample) {
                    inputMatrix(inChannel, sample) = static_cast<float>(audioSamples[sample]) / 32768.0f;
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
                outputFrame.timestamp = timestamp + 2;
                outputFrame.sampleRate = inputFrames[0].sampleRate;
                outputFrame.channels = 1;
                outputFrame.bitsPerSample = 16;
                
                // 分配输出数据内存
                outputFrame.data.resize(frameSize * sizeof(int16_t));
                int16_t* outputData = reinterpret_cast<int16_t*>(outputFrame.data.data());
                
                // 从输出矩阵提取当前通道的数据并转换为int16_t
                for (size_t sample = 0; sample < frameSize; ++sample) {
                    float outputSample = outputMatrix(outChannel, sample);
                    
                    // 应用幅度限制（削波保护）
                    if (outputSample > 1.0f) outputSample = 1.0f;
                    else if (outputSample < -1.0f) outputSample = -1.0f;
                    
                    // 转换为int16_t格式
                    outputData[sample] = static_cast<int16_t>(outputSample * 32767.0f);
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
    // 停止当前处理
    if (_isProcessing) {
        stopProcessing();
    }
    
    // 初始化输入缓冲区
    _inputBuffers.clear();
    _inputBuffers.resize(inputCount);
    for (int i = 0; i < inputCount; ++i) {
        _inputBuffers[i] = std::make_shared<AudioTimestampRingQueue>();
    }
    
    // 初始化输出缓冲区
    _outputBuffers.clear();
    _outputBuffers.resize(outputCount);
    for (int i = 0; i < outputCount; ++i) {
        _outputBuffers[i] = std::make_shared<AudioTimestampRingQueue>();
    }
    
    // 设置矩阵
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

