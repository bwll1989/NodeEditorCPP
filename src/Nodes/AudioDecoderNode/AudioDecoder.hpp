#pragma once
#include <QDebug>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <QtCore/QObject>
#include "QJsonObject"
#include "DataTypes/AudioData.h"  // 确保包含此头文件
extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
}
#include <iostream>
#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <portaudio.h>
#include <QDateTime>


class AudioDecoder : public QThread {
Q_OBJECT

public:
    // 在构造函数中添加新的成员变量初始化
    explicit AudioDecoder(QObject *parent = nullptr);

    ~AudioDecoder();
    /**
     * @brief 初始化ffmpeg
     * @param filePath
     * @return 返回音频参数
     */
    QJsonObject* initializeFFmpeg(const QString &filePath);

    /**
     * @brief 开始播放音频，重置所有缓冲区状态
     */
    void startPlay();
    
    /**
     * @brief 停止解码
     */
    void stopPlay() ;
    
    /**
     * @brief 设置音量
     * @param vol 音量值 (0.0 - 1.0)
     */
    void setVolume(float vol) ;
    
    /**
     * @brief 获取当前音量
     * @return 当前音量值 (0.0 - 1.0)
     */
    float getVolume() const ;
    
    /**
     * @brief 设置循环播放
     * @param loop 是否循环播放
     */
    void setLooping(bool loop) ;
   /**
     * @brief 获取所有通道缓冲区中的平均填充率
     * @return 所有通道缓冲区的平均填充率，如果没有缓冲区则返回-1
     */
    double getBufferUsedRatio() const;
    /**
     * @brief 获取循环播放状态
     * @return 是否循环播放
     */
    bool getLooping() const ;

    /**
     *
     */
    bool getPlaying() const;
       

signals:
    // 发送解码后的音频数据
    void audioFrameReady(AudioFrame frame);
public slots:
    /**
     * @brief 处理音频帧，支持动态通道数，使用内存块拷贝优化的音频通道分离
     * @param frame 音频帧数据
     */
    void handleAudioFrame(AudioFrame frame) ;

    /**
     *
     * @param index
     * @return
     */
    std::shared_ptr<AudioTimestampRingQueue> getAudioBuffer(int index);
protected:
    void run() override ;

private:
    /**
     * @brief 播放音频（支持预缓冲策略）
     * 前20帧快速填充，之后保持正常播放速度
     */
    void playAudio() ;

    void cleanupFFmpeg();
    /**
     * @brief 应用音量到音频数据
     * @param data 音频数据指针
     * @param sampleCount 采样数量
     * @param channels 声道数
     */
    void applyVolume(uint8_t* data, int sampleCount, int channels) ;
    /**
         * @brief 重置时间戳计算器（用于开始播放或循环播放）
         */
    void resetTimestamp() ;

    /**
     * @brief 根据系统播放开始时间和音频PTS计算精确的时间戳
     * @param audioFrame FFmpeg音频帧
     * @param sampleCount 当前帧的采样数（用于重采样情况）
     * @return 精确的时间戳（毫秒），反映系统时间什么时候这帧音频开始播放
     */
    qint64 calculatePreciseTimestamp(AVFrame* audioFrame, int sampleCount) ;
    // 添加新的成员变量
    AVFormatContext *formatContext = nullptr;
    AVCodecContext *codecContext = nullptr;
    const AVCodec *codec;
    AVFrame *audioFrame;
    AVPacket *packet;
    SwrContext *swrContext;
    qint64 baseTimestamp = 0;  // 基准时间戳（回退方案）
    qint64 totalSamples = 0;   // 累计采样数（回退方案）
    
    // 新增成员变量用于PTS时间戳计算
    qint64 systemStartTime = 0;      // 系统播放开始时间（毫秒）
    int64_t firstFramePts = AV_NOPTS_VALUE;  // 第一帧的PTS
    double timeBase = 0.0;           // 时间基准（毫秒）
    
    int audioStreamIndex = -1;
    int64_t decodedFrames;
    uint8_t *resampledBuffer;
    QMutex mutex;
    QWaitCondition condition;
    bool isPlaying;             //播放标志位
    bool isLooping = false;  // 循环播放标志
    float volume = 0.5f;     // 音量控制 (0.0 - 1.0)
    std::map<int, std::shared_ptr<AudioTimestampRingQueue>> channelAudioBuffers;  // 动态通道环形缓冲区
};



