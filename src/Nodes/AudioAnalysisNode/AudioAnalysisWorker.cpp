#include "AudioAnalysisWorker.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include <QDebug>
#include <algorithm>
#include <cmath>

namespace Nodes
{
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    AudioAnalysisWorker::AudioAnalysisWorker(QObject *parent)
        : QObject(parent)
        , _processingTimer(new QTimer(this))
        , _isProcessing(false)
        , _lastProcessedTimestamp(0)
        , _frameSize(2048)  // 默认帧大小
        , _sampleRate(48000)  // 默认采样率
    {
        // 设置处理定时器
        _processingTimer->setInterval(15); // 15ms间隔处理音频数据
        connect(_processingTimer, &QTimer::timeout, this, &AudioAnalysisWorker::processAudioData);
        
        // 初始化GIST分析器
        initializeGist(_frameSize, _sampleRate);
    }
    
    /**
     * @brief 析构函数
     */
    AudioAnalysisWorker::~AudioAnalysisWorker()
    {
        stopProcessing();
    }
    
    /**
     * @brief 初始化GIST分析器
     * @param frameSize 帧大小
     * @param sampleRate 采样率
     */
    void AudioAnalysisWorker::initializeGist(int frameSize, int sampleRate)
    {
        _frameSize = frameSize;
        _sampleRate = sampleRate;
        
        try {
            _gistAnalyzer = std::make_unique<Gist<float>>(frameSize, sampleRate);
            qDebug() << "GIST analyzer initialized with frame size:" << frameSize << "sample rate:" << sampleRate;
        } catch (const std::exception& e) {
            qWarning() << "Failed to initialize GIST analyzer:" << e.what();
        }
    }
    
    /**
     * @brief 开始处理音频数据
     */
    void AudioAnalysisWorker::startProcessing()
    {
        QMutexLocker locker(&_mutex);
        // 如果已经在处理，直接返回
        if (_isProcessing) {
            qDebug() << "AudioAnalysisWorker: Already processing, ignoring duplicate call";
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
    void AudioAnalysisWorker::stopProcessing()
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
     * @brief 处理音频数据的主循环
     */
    void AudioAnalysisWorker::processAudioData()
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
        
        _lastProcessedTimestamp = currentTime;

    }
    
    /**
     * @brief 使用矩阵运算执行音频矩阵操作
     * @param inputFrame 输入音频帧
     * @param timestamp 时间戳
     */
    void AudioAnalysisWorker::performAnalysisOperation(const AudioFrame& inputFrame, qint64 timestamp)
    {

        if (!_gistAnalyzer || inputFrame.data.isEmpty()) {
            return;
        }

        // 遍历每个输入通道
       
        const AudioFrame& frame = inputFrame;
        
        // 检查音频帧是否有效
        if (frame.data.isEmpty()) {
            return;
        }
        
        // 准备音频数据用于GIST分析
        std::vector<float> audioBuffer;
        
        // 将QByteArray转换为16位整数指针
        const int16_t* samples = reinterpret_cast<const int16_t*>(frame.data.constData());
        int totalSamples = frame.data.size() / sizeof(int16_t);
        int samplesPerChannel = totalSamples / frame.channels;
        
        // 根据音频帧的通道数处理数据
        if (frame.channels == 1) {
            // 单声道：直接转换为float
            audioBuffer.reserve(samplesPerChannel);
            for (int i = 0; i < samplesPerChannel; i++) {
                audioBuffer.push_back(static_cast<float>(samples[i]) / 32768.0f);
            }
        } else if (frame.channels == 2) {
            // 立体声：混合左右声道为单声道
            audioBuffer.reserve(samplesPerChannel);
            for (int i = 0; i < samplesPerChannel; i++) {
                float leftSample = static_cast<float>(samples[i * 2]) / 32768.0f;
                float rightSample = static_cast<float>(samples[i * 2 + 1]) / 32768.0f;
                float mixedSample = (leftSample + rightSample) * 0.5f;
                audioBuffer.push_back(mixedSample);
            }
        } else {
            // 多声道：提取第一个声道
            audioBuffer.reserve(samplesPerChannel);
            for (int i = 0; i < samplesPerChannel; i++) {
                audioBuffer.push_back(static_cast<float>(samples[i * frame.channels]) / 32768.0f);
            }
        }
        
        // 确保有足够的数据进行分析
        if (audioBuffer.size() < static_cast<size_t>(_frameSize)) {
            // 如果数据不足，用零填充
            audioBuffer.resize(_frameSize, 0.0f);
        } else if (audioBuffer.size() > static_cast<size_t>(_frameSize)) {
            // 如果数据过多，截取前面的部分
            audioBuffer.resize(_frameSize);
        }
        QVariantMap analysisValues;
        try {
            // 使用GIST进行音频分析
            _gistAnalyzer->processAudioFrame(audioBuffer);
            
            // 获取RMS值
            float rmsValue = _gistAnalyzer->rootMeanSquare();
            analysisValues["Rms"] = rmsValue;
            // 获取Peak值
            float peakValue = _gistAnalyzer->peakEnergy();
            analysisValues["Peak"] = peakValue;
            // 获取零交叉率
            // 零交叉率：人声通常高于乐器
            float zcr = _gistAnalyzer->zeroCrossingRate();
            analysisValues["Zcr"] = zcr;
            
            // 频谱质心：乐器通常高于人声
            float spectralCentroid = _gistAnalyzer->spectralCentroid();
            analysisValues["SpectralCentroid"] = spectralCentroid;
            
            // 频谱滚降点：人声通常低于乐器
            float spectralRolloff = _gistAnalyzer->spectralRolloff();
            analysisValues["SpectralRolloff"] = spectralRolloff;
            
            // 频谱通量：人声变化更剧烈
            float spectralFlux = _gistAnalyzer->spectralDifference();
            analysisValues["SpectralFlux"] = spectralFlux;
             // 音高检测：人声通常高于乐器
            float pitch = _gistAnalyzer->pitch();
            analysisValues["Pitch"] = pitch;
            
            /**
             * @brief 分析10段频率能量分布
             * 将频谱分为10个等对数频段，计算每段能量在总能量中的归一化分布
             * 频段划分：20Hz-63Hz, 63Hz-200Hz, 200Hz-630Hz, 630Hz-2kHz, 2kHz-6.3kHz, 
             *          6.3kHz-20kHz等（基于1/3倍频程划分）
             */
            const std::vector<float>& magnitudeSpectrum = _gistAnalyzer->getMagnitudeSpectrum();
            int spectrumSize = magnitudeSpectrum.size();
            float nyquistFreq = _sampleRate / 2.0f;
            
            // 计算频率分辨率
            float freqResolution = nyquistFreq / (spectrumSize - 1);
            
            // 定义10个频段的边界频率 (Hz) - 基于对数分布
            std::vector<float> freqBands = {
                20.0f, 63.0f, 200.0f, 630.0f, 2000.0f, 
                6300.0f, 12500.0f, 16000.0f, 18000.0f, 20000.0f
            };
            
            // 计算每个频段对应的频谱索引
            std::vector<int> bandIndices;
            for (float freq : freqBands) {
                int index = static_cast<int>(freq / freqResolution);
                index = std::min(index, spectrumSize - 1);
                bandIndices.push_back(index);
            }
            
            // 计算10个频段的能量
            std::vector<float> bandEnergies(10, 0.0f);
            
            // 第1段：0 - bandIndices[0]
            for (int i = 0; i <= bandIndices[0]; ++i) {
                bandEnergies[0] += magnitudeSpectrum[i] * magnitudeSpectrum[i];
            }
            
            // 第2-9段：bandIndices[i-1]+1 - bandIndices[i]
            for (int band = 1; band < 9; ++band) {
                for (int i = bandIndices[band-1] + 1; i <= bandIndices[band]; ++i) {
                    bandEnergies[band] += magnitudeSpectrum[i] * magnitudeSpectrum[i];
                }
            }
            
            // 第10段：bandIndices[8]+1 - end
            for (int i = bandIndices[8] + 1; i < spectrumSize; ++i) {
                bandEnergies[9] += magnitudeSpectrum[i] * magnitudeSpectrum[i];
            }
            
            // 计算总能量
            float totalEnergy = 0.0f;
            for (float energy : bandEnergies) {
                totalEnergy += energy;
            }
            
            // 输出归一化能量分布 (避免除零)
            if (totalEnergy > 0.0f) {
                for (int i = 0; i < 10; ++i) {
                    QString key = "Band" + QString::number(i + 1) + "EnergyRatio";
                    analysisValues[key] = bandEnergies[i] / totalEnergy;
                }
            } else {
                for (int i = 0; i < 10; ++i) {
                    QString key = "Band" + QString::number(i + 1) + "EnergyRatio";
                    analysisValues[key] = 0.0f;
                }
            }

        } catch (const std::exception& e) {
            qWarning() << "GIST analysis error for channel" << ":" << e.what();
        }
        emit analysisValueChanged(analysisValues);
    }


/**
 * @brief 设置指定末端端口缓冲区
 * @param port 端口索引
 * @param buffer 音频缓冲区
 */
void AudioAnalysisWorker::setInputBuffer(int port, std::shared_ptr<AudioTimestampRingQueue> buffer)
{
    QMutexLocker locker(&_mutex);
    
    if (port >= 0 ) {
        _inputBuffers = buffer;
    } else {
        qWarning() << "AudioAnalysisWorker: Invalid input port index:" << port
                   << "(valid range: 0 -";
    }
}

}

