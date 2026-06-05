#pragma once

#include <QString>
#include <QByteArray>
#include <opencv2/core.hpp>

#include "Common/DataTypes/AudioTimestampRingQueue.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/opt.h>
#include <libswresample/swresample.h>
#include <libswscale/swscale.h>
}

namespace Nodes
{
class FfmpegImageWriter
{
public:
    static bool saveImage(const cv::Mat& bgr, const QString& filePath, QString* error = nullptr);
};

class FfmpegVideoEncoder
{
public:
    FfmpegVideoEncoder() = default;
    ~FfmpegVideoEncoder();

    bool open(const QString& filePath, int width, int height, double fps, QString* error = nullptr);
    bool writeFrame(const cv::Mat& bgr, QString* error = nullptr);
    void close();
    bool isOpen() const { return fmtCtx_ != nullptr; }

private:
    bool drainEncoderPackets(QString* error);
    bool writePacket(AVPacket* packet, QString* error);

    AVFormatContext* fmtCtx_ = nullptr;
    AVCodecContext* codecCtx_ = nullptr;
    AVStream* stream_ = nullptr;
    SwsContext* swsCtx_ = nullptr;
    AVFrame* frame_ = nullptr;
    AVPacket* packet_ = nullptr;
    int64_t pts_ = 0;
    int srcWidth_ = 0;
    int srcHeight_ = 0;
};

class FfmpegAudioEncoder
{
public:
    FfmpegAudioEncoder() = default;
    ~FfmpegAudioEncoder();

    bool open(const QString& filePath, int sampleRate, int channels, QString* error = nullptr);
    bool writeFrame(const AudioFrame& frame, QString* error = nullptr);
    void close();
    bool isOpen() const { return fmtCtx_ != nullptr; }

private:
    bool ensureResampler(int sampleRate, int channels, int bitsPerSample, QString* error);
    bool drainEncoderPackets(QString* error);
    bool encodeFrame(QString* error);
    bool writePacket(AVPacket* packet, QString* error);
    void uninitChannelLayout(AVChannelLayout* layout);

    AVFormatContext* fmtCtx_ = nullptr;
    AVCodecContext* codecCtx_ = nullptr;
    AVStream* stream_ = nullptr;
    SwrContext* swrCtx_ = nullptr;
    AVFrame* frame_ = nullptr;
    AVPacket* packet_ = nullptr;
    int64_t pts_ = 0;
    int outputChunkSamples_ = 0;
    int configuredSampleRate_ = 0;
    int configuredChannels_ = 0;
};
} // namespace Nodes
