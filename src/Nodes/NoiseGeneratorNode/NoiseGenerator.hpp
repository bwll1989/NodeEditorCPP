#pragma once
#include <QDebug>
#include <QtCore/QObject>
#include "QJsonObject"
#include "DataTypes/AudioData.h"
#include <iostream>
#include <QObject>
#include <QMutex>
#include <QDateTime>
#include <random>
#include <cmath>
#include <portaudio.h>  // 添加PortAudio头文件

/**
 * @brief 噪音类型枚举
 */
enum class NoiseType {
    WHITE_NOISE,  // 白噪音
    PINK_NOISE    // 粉噪音
};

/**
 * @brief 基于PortAudio回调的白噪和粉噪音频生成器类
 * 支持实时生成白噪音和粉噪音，使用PortAudio回调确保时间戳稳定性
 */
class NoiseGenerator : public QObject {
    Q_OBJECT

public:
    /**
     * @brief 构造函数
     * @param parent 父对象
     */
    explicit NoiseGenerator(QObject *parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~NoiseGenerator();
    
    /**
     * @brief 初始化噪音生成器
     * @param sampleRate 采样率
     * @param channels 声道数
     * @param bitsPerSample 位深
     * @return 返回音频参数的JSON对象
     */
    QJsonObject* initializeGenerator(int sampleRate = 48000, int channels = 1, int bitsPerSample = 16);
    
    /**
     * @brief 开始生成噪音
     */
    void startGeneration();
    
    /**
     * @brief 停止生成噪音
     */
    void stopGeneration();
    
    /**
     * @brief 设置音量
     * @param vol 音量值 (线性值 0.0-1.0)
     */
    void setVolume(double vol);
    
    /**
     * @brief 获取当前音量
     * @return 当前音量值 (线性值 0.0-1.0)
     */
    float getVolume() const;
    
    /**
     * @brief 设置噪音类型
     * @param type 噪音类型（白噪音或粉噪音）
     */
    void setNoiseType(NoiseType type);
    
    /**
     * @brief 获取噪音类型
     * @return 当前噪音类型
     */
    NoiseType getNoiseType() const;
    
    /**
     * @brief 检查是否正在生成噪音
     * @return 生成状态
     */
    bool isGenerating() const;
    
    /**
     * @brief 获取音频缓冲区
     * @return 音频缓冲区的共享指针
     */
    std::shared_ptr<AudioTimestampRingQueue> getAudioBuffer();

signals:
    /**
     * @brief 音频帧准备就绪信号
     * @param frame 音频帧数据
     */
    void audioFrameReady(AudioFrame frame);

private:
    /**
     * @brief PortAudio回调函数
     * @param inputBuffer 输入缓冲区（未使用）
     * @param outputBuffer 输出缓冲区
     * @param framesPerBuffer 每个缓冲区的帧数
     * @param timeInfo 时间信息
     * @param statusFlags 状态标志
     * @param userData 用户数据指针
     * @return PortAudio回调状态
     */
    static int audioCallback(const void* inputBuffer, void* outputBuffer,
                           unsigned long framesPerBuffer,
                           const PaStreamCallbackTimeInfo* timeInfo,
                           PaStreamCallbackFlags statusFlags,
                           void* userData);
    
    /**
     * @brief 处理音频回调
     * @param outputBuffer 输出缓冲区
     * @param framesPerBuffer 每个缓冲区的帧数
     * @return PortAudio回调状态
     */
    int processAudioCallback(void* outputBuffer, unsigned long framesPerBuffer);
    
    /**
     * @brief 生成白噪音样本
     * @return 白噪音样本值
     */
    float generateWhiteNoise();
    
    /**
     * @brief 生成粉噪音样本
     * @return 粉噪音样本值
     */
    float generatePinkNoise();

    // 音频参数
    int sampleRate = 48000;
    int channels = 1;
    int bitsPerSample = 16;
    
    // 线程安全
    mutable QMutex mutex;
    bool generating = false;
    float volume = 0.0f;  // 音量控制 (0.0 - 1.0)
    NoiseType noiseType = NoiseType::WHITE_NOISE;
    
    // 随机数生成器
    std::random_device rd;
    std::mt19937 gen;
    std::uniform_real_distribution<float> whiteDist;
    
    // 粉噪音状态
    float pinkState[7] = {0};
    
    // 音频缓冲区
    std::shared_ptr<AudioTimestampRingQueue> channelAudioBuffers;
    
    // PortAudio相关
    PaStream* stream = nullptr;
    
    // 音频帧序列号（避免时间戳跳变）
    qint64 audioFrameSequence = 0;
};