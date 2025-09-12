#include "NoiseGenerator.hpp"
#include <QDebug>
#include <QDateTime>
#include <cstring>
#include "TimestampGenerator/TimestampGenerator.hpp"

// PortAudio缓冲区大小（帧数）
static const int FRAMES_PER_BUFFER = 48000/(TimestampGenerator::getInstance()->getFrameRate());

/**
 * @brief 构造函数，初始化噪音生成器
 */
NoiseGenerator::NoiseGenerator(QObject *parent)
    : QObject(parent)
    , gen(rd())
    , whiteDist(-1.0f, 1.0f)
    , generating(false)
    , volume(0.0f)
    , noiseType(NoiseType::WHITE_NOISE)
    , audioFrameSequence(0)
{
    qRegisterMetaType<AudioFrame>("AudioFrame");
    // 初始化粉噪音状态
    memset(pinkState, 0, sizeof(pinkState));
    
    // 初始化PortAudio
    PaError err = Pa_Initialize();
    if (err != paNoError) {
        qDebug() << "PortAudio初始化失败:" << Pa_GetErrorText(err);
    }
}

/**
 * @brief 析构函数
 */
NoiseGenerator::~NoiseGenerator() {
    stopGeneration();
    Pa_Terminate();
}

/**
 * @brief 初始化噪音生成器
 */
QJsonObject* NoiseGenerator::initializeGenerator(int sampleRate, int channels, int bitsPerSample) {
    QMutexLocker locker(&mutex);
    
    this->sampleRate = sampleRate;
    this->channels = channels;
    this->bitsPerSample = bitsPerSample;
    
    // 创建音频缓冲区
    channelAudioBuffers = std::make_shared<AudioTimestampRingQueue>();
    channelAudioBuffers->setActive(true);
    
    // 返回音频参数
    QJsonObject* audioParams = new QJsonObject();
    (*audioParams)["sampleRate"] = sampleRate;
    (*audioParams)["channels"] = channels;
    (*audioParams)["bitsPerSample"] = bitsPerSample;
    
    return audioParams;
}

/**
 * @brief 开始生成噪音
 */
void NoiseGenerator::startGeneration() {
    QMutexLocker locker(&mutex);
    
    if (generating) return;
    
    // 配置PortAudio流参数
    PaStreamParameters outputParameters;
    outputParameters.device = Pa_GetDefaultOutputDevice();
    if (outputParameters.device == paNoDevice) {
        qDebug() << "错误：没有找到默认输出设备";
        return;
    }
    
    outputParameters.channelCount = channels;
    outputParameters.sampleFormat = paFloat32;  // 使用32位浮点格式
    outputParameters.suggestedLatency = Pa_GetDeviceInfo(outputParameters.device)->defaultLowOutputLatency;
    outputParameters.hostApiSpecificStreamInfo = nullptr;
    
    // 打开PortAudio流
    PaError err = Pa_OpenStream(
        &stream,
        nullptr,  // 无输入
        &outputParameters,
        sampleRate,
        FRAMES_PER_BUFFER,
        paClipOff,  // 不进行削波
        audioCallback,
        this  // 用户数据指针
    );
    
    if (err != paNoError) {
        qDebug() << "打开PortAudio流失败:" << Pa_GetErrorText(err);
        return;
    }
    
    // 启动流
    err = Pa_StartStream(stream);
    if (err != paNoError) {
        qDebug() << "启动PortAudio流失败:" << Pa_GetErrorText(err);
        Pa_CloseStream(stream);
        stream = nullptr;
        return;
    }
    
    generating = true;
    audioFrameSequence = TimestampGenerator::getInstance()->getCurrentFrameCount()+5;
}

/**
 * @brief 停止生成噪音
 */
void NoiseGenerator::stopGeneration() {
    QMutexLocker locker(&mutex);
    
    if (!generating) return;
    
    generating = false;
    
    if (stream) {
        PaError err = Pa_StopStream(stream);
        if (err != paNoError) {
            qDebug() << "停止PortAudio流失败:" << Pa_GetErrorText(err);
        }
        
        err = Pa_CloseStream(stream);
        if (err != paNoError) {
            qDebug() << "关闭PortAudio流失败:" << Pa_GetErrorText(err);
        }
        
        stream = nullptr;
    }

}

/**
 * @brief PortAudio回调函数（静态）
 */
int NoiseGenerator::audioCallback(const void* inputBuffer, void* outputBuffer,
                                unsigned long framesPerBuffer,
                                const PaStreamCallbackTimeInfo* timeInfo,
                                PaStreamCallbackFlags statusFlags,
                                void* userData) {
    NoiseGenerator* generator = static_cast<NoiseGenerator*>(userData);
    return generator->processAudioCallback(outputBuffer, framesPerBuffer);
}

/**
 * @brief 处理音频回调
 */
int NoiseGenerator::processAudioCallback(void* outputBuffer, unsigned long framesPerBuffer) {
     memset(outputBuffer, 0, framesPerBuffer * channels * sizeof(float));
    if (!generating) {
        // 如果不在生成状态，输出静音
        // memset(outputBuffer, 0, framesPerBuffer * channels * sizeof(float));
        return paContinue;
    }
    
    // float* output = static_cast<float*>(outputBuffer);
    
    // 使用连续的音频帧序列号，避免时间戳跳变
    qint64 currentTimestamp = audioFrameSequence++;

    // 创建音频帧
    AudioFrame frame;
    frame.sampleRate = sampleRate;
    frame.channels = channels;
    frame.bitsPerSample = 16;  // 使用16位浮点
    frame.timestamp = currentTimestamp;

    // 分配音频数据缓冲区
    frame.data.resize(framesPerBuffer * channels * sizeof(int16_t));
    int16_t* frameData = reinterpret_cast<int16_t*>(frame.data.data());
    
    // 生成噪音数据
    for (unsigned long i = 0; i < framesPerBuffer; ++i) {
        float sample;
        
        // 根据噪音类型生成样本
        switch (noiseType) {
            case NoiseType::WHITE_NOISE:
                sample = generateWhiteNoise();
                break;
            case NoiseType::PINK_NOISE:
                sample = generatePinkNoise();
                break;
            default:
                sample = 0.0f;
                break;
        }
         // 将dB转换为线性增益
    float linearGain = std::pow(10.0f, volume / 20.0f);
        // 应用音量并限制幅度
        sample *= linearGain;
        if (sample > 1.0f) sample = 1.0f;
        if (sample < -1.0f) sample = -1.0f;
        
         // 将float样本转换为16位整数
        int16_t intSample = static_cast<int16_t>(sample * 32767.0f);
        
        // 填充所有声道
        for (int ch = 0; ch < channels; ++ch) {
            frameData[i * channels + ch] = intSample;
        }
    
    }
    
    // 推送到音频缓冲区
    if (channelAudioBuffers) {
        channelAudioBuffers->pushFrame(frame);
    }
    
    return paContinue;
}

/**
 * @brief 设置音量
 */
void NoiseGenerator::setVolume(double vol) {
    QMutexLocker locker(&mutex);
    volume = static_cast<float>(vol);
}

float NoiseGenerator::getVolume() const {
    QMutexLocker locker(&mutex);
    return volume;
}

void NoiseGenerator::setNoiseType(NoiseType type) {
    QMutexLocker locker(&mutex);
    noiseType = type;
    
    // 重置粉噪音状态
    if (type == NoiseType::PINK_NOISE) {
        memset(pinkState, 0, sizeof(pinkState));
    }
}

NoiseType NoiseGenerator::getNoiseType() const {
    QMutexLocker locker(&mutex);
    return noiseType;
}

bool NoiseGenerator::isGenerating() const {
    QMutexLocker locker(&mutex);
    return generating;
}

std::shared_ptr<AudioTimestampRingQueue> NoiseGenerator::getAudioBuffer() {
    QMutexLocker locker(&mutex);
    return channelAudioBuffers;
}


/**
 * @brief 生成白噪音样本
 */
float NoiseGenerator::generateWhiteNoise() {
    return whiteDist(gen);
}

/**
 * @brief 生成粉噪音样本
 */
float NoiseGenerator::generatePinkNoise() {
    float white = whiteDist(gen);
    
    pinkState[0] = 0.99886f * pinkState[0] + white * 0.0555179f;
    pinkState[1] = 0.99332f * pinkState[1] + white * 0.0750759f;
    pinkState[2] = 0.96900f * pinkState[2] + white * 0.1538520f;
    pinkState[3] = 0.86650f * pinkState[3] + white * 0.3104856f;
    pinkState[4] = 0.55000f * pinkState[4] + white * 0.5329522f;
    pinkState[5] = -0.7616f * pinkState[5] - white * 0.0168980f;
    
    float pink = pinkState[0] + pinkState[1] + pinkState[2] + pinkState[3] + pinkState[4] + pinkState[5] + pinkState[6] + white * 0.5362f;
    pinkState[6] = white * 0.115926f;
    
    return pink * 0.11f;  // 调整音量
}