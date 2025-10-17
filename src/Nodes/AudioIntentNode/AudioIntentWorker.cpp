#include "AudioIntentWorker.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include <QDebug>
#include <algorithm>
#include <cmath>
#include <QFileInfo>
#include <QDir>

namespace Nodes
{
    // 重采样比例常量定义
    const double AudioIntentWorker::RESAMPLE_RATIO = static_cast<double>(INPUT_SAMPLE_RATE) / 16000.0;

    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    AudioIntentWorker::AudioIntentWorker(QObject *parent)
        : QObject(parent)
        , _processingTimer(new QTimer(this))
        , _isProcessing(false)
        , _lastProcessedTimestamp(0)
        , _frameSize(INPUT_FRAME_SIZE)
        , _sampleRate(INPUT_SAMPLE_RATE)
        , _porcupineHandle(nullptr)
        , _porcupineInitialized(false)
        , _currentState(STATE_IDLE)
    {
        // 设置处理定时器
        _processingTimer->setInterval(15);
        connect(_processingTimer, &QTimer::timeout, this, &AudioIntentWorker::processAudioData);
        
        qDebug() << "=== Porcupine Audio Analysis Worker Initialized ===";
        qDebug() << "Input format: 48kHz, 2048 samples per frame";
        qDebug() << "Target format: 16kHz int16 for Porcupine engine";
        
        // 预分配缓冲区
        _audioBuffer.reserve(16384);
        _porcupineBuffer.reserve(8192);
        
        qDebug() << "Audio buffers allocated";
    }
    
    /**
     * @brief 析构函数
     */
    AudioIntentWorker::~AudioIntentWorker()
    {
        cleanupPorcupine();
    }
    
    /**
     * @brief 初始化Porcupine唤醒词检测引擎
     */
    void AudioIntentWorker::initializePorcupine()
    {
        if (_porcupineInitialized) {
            cleanupPorcupine();
        }
        
        if (_porcupineAccessKey.isEmpty() || _porcupineModelPath.isEmpty() || _keywordPaths.isEmpty()) {
            qWarning() << "Porcupine configuration incomplete";
            return;
        }
        
        // 检查模型文件是否存在
        QFileInfo modelFile(_porcupineModelPath);
        if (!modelFile.exists()) {
            qWarning() << "Porcupine model file not found:" << _porcupineModelPath;
            return;
        }
        
        qDebug() << "=== Porcupine初始化调试信息 ===";
        qDebug() << "模型文件路径:" << _porcupineModelPath;
        qDebug() << "模型文件存在:" << modelFile.exists();
        qDebug() << "配置的关键词文件数量:" << _keywordPaths.size();
        
        // 检查关键词文件并构建有效的关键词列表
        QStringList validKeywords;
        QStringList validKeywordPaths;
        QList<float> validSensitivities;
        
        for (int i = 0; i < _keywordPaths.size(); ++i) {
            QString keywordPath = _keywordPaths[i];
            QFileInfo keywordFile(keywordPath);
            
            qDebug() << "检查关键词文件" << (i+1) << ":";
            qDebug() << "  路径:" << keywordPath;
            qDebug() << "  绝对路径:" << keywordFile.absoluteFilePath();
            qDebug() << "  文件存在:" << keywordFile.exists();
            qDebug() << "  文件大小:" << keywordFile.size() << "字节";
            qDebug() << "  提取的关键词名:" << keywordFile.baseName();
            
            if (keywordFile.exists()) {
                validKeywordPaths.append(keywordPath);
                validKeywords.append(keywordFile.baseName());
                if (i < _keywordSensitivities.size()) {
                    validSensitivities.append(_keywordSensitivities[i]);
                } else {
                    validSensitivities.append(0.5f); // 默认敏感度
                }
                qDebug() << "  ✓ 文件有效，已添加到列表";
            } else {
                qWarning() << "  ✗ 关键词文件不存在:" << keywordPath;
            }
        }
        
        if (validKeywordPaths.isEmpty()) {
            qWarning() << "没有找到有效的关键词文件";
            return;
        }
        
        qDebug() << "有效关键词文件数量:" << validKeywordPaths.size();
        qDebug() << "有效关键词列表:" << validKeywords;
        qDebug() << "关键词敏感度:" << validSensitivities;
        
        // 转换为C风格字符串数组
        std::vector<const char*> keywordPathsCStr;
        std::vector<QByteArray> keywordPathsBytes;
        for (const QString& path : validKeywordPaths) {
            keywordPathsBytes.push_back(path.toUtf8());
            keywordPathsCStr.push_back(keywordPathsBytes.back().constData());
        }
        
        qDebug() << "开始初始化Porcupine引擎...";
        
        porcupine_result_t result = porcupine_init_wrapper(
            _porcupineAccessKey.toUtf8().constData(),
            _porcupineModelPath.toUtf8().constData(),
            validKeywordPaths.size(),
            keywordPathsCStr.data(),
            validSensitivities.data()
        );
        
        if (result.status != PORCUPINE_STATUS_SUCCESS) {
            qWarning() << "Porcupine初始化失败, 状态码:" << result.status
                       << "错误信息:" << porcupine_status_to_string_wrapper(result.status);
            return;
        }
        
        _porcupineHandle = result.handle;
        _keywords = validKeywords;
        _porcupineInitialized = true;
        
        qDebug() << "=== Porcupine初始化成功 ===";
        qDebug() << "句柄:" << _porcupineHandle;
        qDebug() << "加载的关键词:" << _keywords;
        qDebug() << "帧长度:" << porcupine_frame_length_wrapper();
        qDebug() << "采样率:" << porcupine_sample_rate_wrapper();
        qDebug() << "版本:" << porcupine_version_wrapper();
        qDebug() << "==============================";
    }
    
    /**
     * @brief 清理Porcupine资源
     */
    void AudioIntentWorker::cleanupPorcupine()
    {
        if (_porcupineInitialized && _porcupineHandle) {
            porcupine_delete_wrapper(_porcupineHandle);
            _porcupineHandle = nullptr;
            _porcupineInitialized = false;
            qDebug() << "Porcupine cleaned up";
        }
    }
    
    /**
     * @brief 设置Porcupine配置参数
     */
    void AudioIntentWorker::setPorcupineConfig(const QString& accessKey, const QString& modelPath,
                                                 const QStringList& keywordPaths, const QList<float>& sensitivities)
    {
        QMutexLocker locker(&_mutex);
        
        _porcupineAccessKey = accessKey;
        _porcupineModelPath = modelPath;
        _keywordPaths = keywordPaths;
        _keywordSensitivities = sensitivities;
        
        qDebug() << "Porcupine config updated:";
        qDebug() << "Model:" << modelPath;
        qDebug() << "Keywords:" << keywordPaths;
        qDebug() << "Sensitivities:" << sensitivities;
        
        // 如果正在处理，重新初始化
        if (_isProcessing) {
            initializePorcupine();
        }
    }
    
    /**
     * @brief 开始处理音频数据
     */
    void AudioIntentWorker::startProcessing()
    {
        QMutexLocker locker(&_mutex);

        if (_isProcessing) {
            return;
        }
        
        qDebug() << "Starting audio processing...";
        
        // 初始化Porcupine
        initializePorcupine();
        
        if (!_porcupineInitialized) {
            qWarning() << "Cannot start processing: Porcupine not initialized";
            return;
        }
        
        _isProcessing = true;
        setState(STATE_WAKE_DETECTION);
        _processingTimer->start();
        
        emit processingStatusChanged(true);
        qDebug() << "Audio processing started";
    }
    
    /**
     * @brief 停止处理音频数据
     */
    void AudioIntentWorker::stopProcessing()
    {
        QMutexLocker locker(&_mutex);
        
        if (!_isProcessing) {
            return;
        }
        
        qDebug() << "Stopping audio processing...";
        
        _isProcessing = false;
        _processingTimer->stop();
        
        // 清理资源
        cleanupPorcupine();
        
        setState(STATE_IDLE);
        
        emit processingStatusChanged(false);
        qDebug() << "Audio processing stopped";
    }
    
    /**
     * @brief 设置处理状态
     * @param state 新状态
     */
    void AudioIntentWorker::setState(ProcessingState state)
    {
        if (_currentState != state) {
            ProcessingState oldState = _currentState;
            _currentState = state;
            
            qDebug() << "State changed from" << oldState << "to" << state;
            emit stateChanged(state);
        }
    }
    
    /**
     * @brief 将音频数据转换为Picovoice格式
     * @param inputFrame 输入音频帧
     * @param outputBuffer 输出缓冲区
     * @return 转换后的样本数
     */
    int AudioIntentWorker::convertAudioForPicovoice(const AudioFrame& inputFrame, std::vector<int16_t>& outputBuffer)
    {
        // 从QByteArray获取音频数据指针
        const int16_t* audioData = reinterpret_cast<const int16_t*>(inputFrame.data.constData());
        int sampleCount = inputFrame.data.size() / sizeof(int16_t);
        
        // 如果需要float格式，需要进行类型转换
        for (int i = 0; i < sampleCount; ++i) {
            // 将int16_t转换为float（归一化到-1.0到1.0范围）
            float floatSample = static_cast<float>(audioData[i]) / 32767.0f;
            _audioBuffer.push_back(floatSample);
        }
        
        // 计算需要输出的样本数（降采样到16kHz）
        int targetSamples = static_cast<int>(_audioBuffer.size() / RESAMPLE_RATIO);
        
        if (targetSamples == 0) {
            return 0;
        }
        
        // 简单的降采样：每RESAMPLE_RATIO个样本取一个
        outputBuffer.clear();
        outputBuffer.reserve(targetSamples);
        
        for (int i = 0; i < targetSamples; ++i) {
            int sourceIndex = static_cast<int>(i * RESAMPLE_RATIO);
            if (sourceIndex < _audioBuffer.size()) {
                // 转换为int16并限制范围
                float sample = _audioBuffer[sourceIndex];
                sample = std::max(-1.0f, std::min(1.0f, sample));
                int16_t intSample = static_cast<int16_t>(sample * 32767.0f);
                outputBuffer.push_back(intSample);
            }
        }
        
        // 移除已处理的样本
        int samplesToRemove = static_cast<int>(targetSamples * RESAMPLE_RATIO);
        if (samplesToRemove > 0 && samplesToRemove <= _audioBuffer.size()) {
            _audioBuffer.erase(_audioBuffer.begin(), _audioBuffer.begin() + samplesToRemove);
        }
        
        return outputBuffer.size();
    }
    
    /**
     * @brief 处理唤醒词检测
     * @param audioData 音频数据
     * @param dataSize 数据大小
     */
    void AudioIntentWorker::processWakewordDetection(const int16_t* audioData, int dataSize)
    {
        if (!_porcupineInitialized || !_porcupineHandle) {
            return;
        }

        int frameLength = porcupine_frame_length_wrapper();
        
        // 将新数据添加到Porcupine缓冲区
        for (int i = 0; i < dataSize; ++i) {
            _porcupineBuffer.push_back(audioData[i]);
        }
        
        // 处理完整的帧
        while (_porcupineBuffer.size() >= frameLength) {
            int keywordIndex = -1;
            int result = porcupine_process_wrapper(_porcupineHandle, _porcupineBuffer.data(), &keywordIndex);
            
            if (result != PORCUPINE_STATUS_SUCCESS) {
                qWarning() << "Porcupine processing failed, error code:" << result;
                break;
            }
            
            if (keywordIndex >= 0 && keywordIndex < _keywords.size()) {
                QString detectedKeyword = _keywords[keywordIndex];
                qDebug() << "Wake word detected:" << detectedKeyword;
                emit wakewordDetected(detectedKeyword);
            }
            
            // 移除已处理的帧
            _porcupineBuffer.erase(_porcupineBuffer.begin(), _porcupineBuffer.begin() + frameLength);
        }
    }
    
    /**
     * @brief 处理音频数据的主循环
     */
    void AudioIntentWorker::processAudioData()
    {

        if (!_isProcessing || !_inputBuffers) {

            return;
        }

        auto currentTime = TimestampGenerator::getInstance()->getCurrentFrameCount();

        if (currentTime == _lastProcessedTimestamp) {
            return;
        }

        // 尝试从输入缓冲区获取音频帧
        AudioFrame inputFrame;
        bool hasValidInput = false;

        if (_inputBuffers && _inputBuffers->isActive()) {
            if (_inputBuffers->getFrameByTimestamp(currentTime, inputFrame)) {
                hasValidInput = true;
            }
        }


        if (!hasValidInput) {
            return;
        }

        // 将单个音频帧转换为向量格式，以兼容现有的performAnalysisOperation方法
        // std::vector<AudioFrame> inputFrames;
        // inputFrames.push_back(inputFrame);

        // 执行音频分析操作
        performAnalysisOperation(inputFrame, currentTime);
    }
    
    /**
     * @brief 执行音频分析操作
     * @param inputFrame 输入音频帧
     * @param timestamp 时间戳
     */
    void AudioIntentWorker::performAnalysisOperation(const AudioFrame& inputFrame, qint64 timestamp)
    {

        if (!_isProcessing) {
            return;
        }
        // 转换音频格式为Picovoice兼容格式
        std::vector<int16_t> convertedAudio;
        int convertedSamples = convertAudioForPicovoice(inputFrame, convertedAudio);
        
        if (convertedSamples == 0) {
            return;
        }
        
        // // 计算RMS值用于音频监控
        // float rms = 0.0f;
        // for (int16_t sample : convertedAudio) {
        //     float normalizedSample = sample / 32767.0f;
        //     rms += normalizedSample * normalizedSample;
        // }
        // rms = std::sqrt(rms / convertedAudio.size());
        //
        // // 发送分析值
        // QVariantMap values;
        // values["rms"] = rms;
        // values["timestamp"] = timestamp;
        // emit analysisValueChanged(values);

        // 根据当前状态处理音频
        switch (_currentState) {
            case STATE_WAKE_DETECTION:
                processWakewordDetection(convertedAudio.data(), convertedAudio.size());
                break;
            case STATE_IDLE:
            default:
                break;
        }
    }
    
    /**
     * @brief 设置输入缓冲区
     * @param port 端口索引
     * @param buffer 音频缓冲区
     */
    void AudioIntentWorker::setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer)
    {
        QMutexLocker locker(&_mutex);
        _inputBuffers = buffer;
        qDebug() << "Input buffer set for port" << port;
    }

    /**
     * @brief 打印版本信息
     */
    void AudioIntentWorker::printVersionInfo()
    {
        qDebug() << "=== Porcupine Version Information ===";
        
        // Porcupine版本信息
        const char* porcupineVersion = porcupine_version_wrapper();
        if (porcupineVersion) {
            qDebug() << "Porcupine Version:" << porcupineVersion;
        } else {
            qDebug() << "Porcupine Version: Unable to get version";
        }
        
        int frameLength = porcupine_frame_length_wrapper();
        int sampleRate = porcupine_sample_rate_wrapper();
        
        qDebug() << "Porcupine Frame Length:" << frameLength;
        qDebug() << "Porcupine Sample Rate:" << sampleRate;
        
        qDebug() << "=== End Version Information ===";
    }
}

