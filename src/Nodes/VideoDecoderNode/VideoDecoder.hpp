#pragma once
#include <QDebug>
#include <queue>
#include <mutex>
#include <condition_variable>
#include <QtCore/QObject>
#include "QJsonObject"

#include "DataTypes/NodeDataList.hpp"
#include "Common/Devices/TimestampGenerator/TimestampGenerator.hpp"  // 添加时间戳生成器头文件

extern "C" {
#include <libavformat/avformat.h>
#include <libavcodec/avcodec.h>
#include <libswresample/swresample.h>
#include <libavutil/avutil.h>
#include <libavutil/opt.h>
#include <libswscale/swscale.h>
}
#include <iostream>
#include <QObject>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <portaudio.h>
#include <QDateTime>
#include <QByteArray>


/**
 * @brief 视频解码器类
 * 
 * 继承自 QThread，负责在单独的线程中进行音视频文件的解码。
 * 支持视频帧转 RGB 输出和音频帧重采样输出。
 * 使用 FFmpeg 进行解码。
 */
class VideoDecoder : public QThread {
Q_OBJECT

public:
    // 在构造函数中添加新的成员变量初始化
    explicit VideoDecoder(QObject *parent = nullptr);

    ~VideoDecoder();
    /**
     * @brief 初始化ffmpeg
     * @param filePath 视频文件路径
     * @return 返回包含媒体信息的 JSON 对象（如比特率、采样率、分辨率等），失败返回 nullptr
     */
    QJsonObject* initializeFFmpeg(const QString &filePath);

    /**
     * @brief 开始播放
     * 重置所有缓冲区状态并启动解码线程
     */
    void startPlay();
    
    /**
     * @brief 停止播放
     * 停止解码线程并清理资源
     */
    void stopPlay() ;
    
    /**
     * @brief 设置音量
     * @param vol 音量值 (dB)
     */
    void setVolume(double vol) ;
    
    /**
     * @brief 获取当前音量
     * @return 当前音量值 (dB)
     */
    float getVolume() const ;
    
    /**
     * @brief 设置循环播放
     * @param loop 是否循环播放
     */
    void setLooping(bool loop) ;

    /**
     * @brief 获取循环播放状态
     * @return 是否循环播放
     */
    bool getLooping() const ;

    /**
     * @brief 获取播放状态
     * @return true 表示正在播放，false 表示已停止
     */
    bool getPlaying() const;
       
    /**
     * @brief 播放主逻辑函数
     * 通常在 run() 方法中调用
     */
    void playAudio();

    /**
     * @brief 获取音频通道数
     * @return 通道数
     */
    int getChannels() const {
        if (codecContext) {
            return codecContext->ch_layout.nb_channels;
        }
        return 0;
    }

signals:
    // 发送解码后的音频数据
    void audioFrameReady(AudioFrame frame);
    // 发送解码后的视频数据
    void videoFrameReady(NodeDataTypes::ImageData frame);
    // 播放进度信号 (当前时间秒, 总时间秒)
    void playbackProgress(double currentSec, double totalSec);

public slots:
    /**
     * @brief 处理音频帧，支持动态通道数，使用内存块拷贝优化的音频通道分离
     * @param frame 音频帧数据
     */
    void handleAudioFrame(AudioFrame frame) ;

    /**
     * @brief 获取指定通道的音频环形缓冲区
     * @param index 通道索引
     * @return 音频环形缓冲区智能指针
     */
    std::shared_ptr<AudioTimestampRingQueue> getAudioBuffer(int index);
protected:
    /**
     * @brief 线程运行入口
     */
    void run() override ;

    // 辅助函数：处理视频帧
    void processVideoFrame();

private:
    /**
     * @brief 应用音量增益
     * @param data 音频数据指针
     * @param sampleCount 采样数
     * @param channels 通道数
     */
    void applyVolume(uint8_t* data, int sampleCount, int channels);

    /**
     * @brief 处理 PCM 数据并按固定帧大小发射
     * @param interleavedPcm 交错的 PCM 数据
     * @param samplesPerChannel 每通道采样数
     * @param channels 通道数
     * @param sampleRate 采样率
     * @return 处理结果
     */
    int processPcmAndEmitFixedFrames(const uint8_t* interleavedPcm, int samplesPerChannel, int channels, int sampleRate);
    
    /**
     * @brief 清理 FFmpeg 资源
     */
    void cleanupFFmpeg();

private:
    mutable QMutex mutex;       ///< 互斥锁，保护共享资源
    QWaitCondition condition;   ///< 条件变量，用于线程同步
    
    // FFmpeg 相关
    AVFormatContext *formatContext; ///< 格式上下文
    
    // 音频相关
    AVCodecContext *codecContext;   ///< 音频解码器上下文
    const AVCodec *codec;           ///< 音频解码器
    AVFrame *audioFrame;            ///< 音频帧缓冲区
    SwrContext *swrContext;         ///< 音频重采样上下文
    int audioStreamIndex;           ///< 音频流索引
    
    // 视频相关
    AVCodecContext *videoCodecContext;  ///< 视频解码器上下文
    const AVCodec *videoCodec;          ///< 视频解码器
    AVFrame *videoFrame;                ///< 视频帧缓冲区
    SwsContext *swsContext;             ///< 视频格式转换上下文（YUV -> RGB）
    int videoStreamIndex;               ///< 视频流索引
    uint8_t *videoDstData[4];           ///< 视频转换目标数据缓冲区
    int videoDstLinesize[4];            ///< 视频转换目标行大小
    int videoWidth;                     ///< 视频宽度
    int videoHeight;                    ///< 视频高度
    double videoClock;                  ///< 视频时钟

    AVPacket *packet;               ///< 数据包
    uint8_t *resampledBuffer;       ///< 重采样缓冲区
    
    // 播放状态
    bool isPlaying;     ///< 是否正在播放
    bool isLooping;     ///< 是否循环播放
    float volume;       ///< 音量 (dB)
    
    // 缓冲区管理
    // 使用map管理多通道缓冲区，key为通道索引(0,1,2...)
    std::map<int, std::shared_ptr<AudioTimestampRingQueue>> channelAudioBuffers;
    
    // 临时存储未发送的数据
    QByteArray pendingInterleavedPcm_;  // 存储交错的PCM数据
    int pendingSamplesPerChannel_ = 0;  // 当前存储的每通道采样数
    int lastChannels_ = 0;              // 上次处理的通道数
    
    // 时间戳相关
    TimestampGenerator* timestampGenerator_;
    uint64_t lastTimestamp_ = 0;
};


