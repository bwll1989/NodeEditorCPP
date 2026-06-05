#include "FfmpegWriters.hpp"

#include <QFileInfo>
#include <QDebug>

#include <opencv2/imgproc.hpp>

#include "Common/DataTypes/AudioTimestampRingQueue.h"

namespace
{
QString avErrorString(int err)
{
    char buf[AV_ERROR_MAX_STRING_SIZE] = {};
    av_make_error_string(buf, sizeof(buf), err);
    return QString::fromUtf8(buf);
}

AVCodecID imageCodecForPath(const QString& filePath)
{
    const QString ext = QFileInfo(filePath).suffix().toLower();
    if (ext == "png") {
        return AV_CODEC_ID_PNG;
    }
    if (ext == "bmp") {
        return AV_CODEC_ID_BMP;
    }
    return AV_CODEC_ID_MJPEG;
}

AVPixelFormat codecPixelFormat(AVCodecID codecId)
{
    switch (codecId) {
    case AV_CODEC_ID_PNG:
        return AV_PIX_FMT_RGB24;
    case AV_CODEC_ID_BMP:
        return AV_PIX_FMT_BGR24;
    case AV_CODEC_ID_MJPEG:
    default:
        return AV_PIX_FMT_YUVJ420P;
    }
}

AVCodecID audioCodecForPath(const QString& filePath)
{
    const QString ext = QFileInfo(filePath).suffix().toLower();
    if (ext == "mp3") {
        return AV_CODEC_ID_MP3;
    }
    if (ext == "aac" || ext == "m4a") {
        return AV_CODEC_ID_AAC;
    }
    return AV_CODEC_ID_PCM_S16LE;
}

AVSampleFormat audioSampleFormat(AVCodecID codecId)
{
    if (codecId == AV_CODEC_ID_PCM_S16LE) {
        return AV_SAMPLE_FMT_S16;
    }
    return AV_SAMPLE_FMT_FLTP;
}

void uninitChannelLayout(AVChannelLayout* layout)
{
    if (!layout) {
        return;
    }
    av_channel_layout_uninit(layout);
    *layout = {};
}

AVChannelLayout channelLayoutForCount(int channels)
{
    if (channels == 1) {
        return AV_CHANNEL_LAYOUT_MONO;
    }
    if (channels == 2) {
        return AV_CHANNEL_LAYOUT_STEREO;
    }

    AVChannelLayout layout{};
    av_channel_layout_default(&layout, channels);
    return layout;
}
} // namespace

namespace Nodes
{
bool FfmpegImageWriter::saveImage(const cv::Mat& bgr, const QString& filePath, QString* error)
{
    if (bgr.empty()) {
        if (error) {
            *error = QStringLiteral("图像为空");
        }
        return false;
    }

    cv::Mat rgb;
    if (bgr.channels() == 4) {
        cv::cvtColor(bgr, rgb, cv::COLOR_BGRA2RGB);
    } else if (bgr.channels() == 3) {
        cv::cvtColor(bgr, rgb, cv::COLOR_BGR2RGB);
    } else if (bgr.channels() == 1) {
        cv::cvtColor(bgr, rgb, cv::COLOR_GRAY2RGB);
    } else {
        if (error) {
            *error = QStringLiteral("不支持的图像通道数");
        }
        return false;
    }

    const AVCodecID codecId = imageCodecForPath(filePath);
    const AVCodec* codec = avcodec_find_encoder(codecId);
    if (!codec) {
        if (error) {
            *error = QStringLiteral("找不到图像编码器");
        }
        return false;
    }

    AVCodecContext* codecCtx = avcodec_alloc_context3(codec);
    if (!codecCtx) {
        if (error) {
            *error = QStringLiteral("无法分配图像编码器");
        }
        return false;
    }

    codecCtx->width = rgb.cols;
    codecCtx->height = rgb.rows;
    codecCtx->pix_fmt = codecPixelFormat(codecId);
    codecCtx->time_base = {1, 25};

    int ret = avcodec_open2(codecCtx, codec, nullptr);
    if (ret < 0) {
        if (error) {
            *error = QStringLiteral("打开图像编码器失败: %1").arg(avErrorString(ret));
        }
        avcodec_free_context(&codecCtx);
        return false;
    }

    AVFrame* frame = av_frame_alloc();
    frame->format = codecCtx->pix_fmt;
    frame->width = codecCtx->width;
    frame->height = codecCtx->height;
    ret = av_frame_get_buffer(frame, 0);
    if (ret < 0) {
        if (error) {
            *error = QStringLiteral("分配图像帧失败: %1").arg(avErrorString(ret));
        }
        av_frame_free(&frame);
        avcodec_free_context(&codecCtx);
        return false;
    }

    SwsContext* swsCtx = sws_getContext(
        rgb.cols, rgb.rows, AV_PIX_FMT_RGB24,
        codecCtx->width, codecCtx->height, codecCtx->pix_fmt,
        SWS_BILINEAR, nullptr, nullptr, nullptr);
    if (!swsCtx) {
        if (error) {
            *error = QStringLiteral("图像色彩转换失败");
        }
        av_frame_free(&frame);
        avcodec_free_context(&codecCtx);
        return false;
    }

    const uint8_t* srcData[1] = {rgb.data};
    const int srcLinesize[1] = {static_cast<int>(rgb.step[0])};
    sws_scale(swsCtx, srcData, srcLinesize, 0, rgb.rows, frame->data, frame->linesize);
    sws_freeContext(swsCtx);

    ret = avcodec_send_frame(codecCtx, frame);
    av_frame_free(&frame);
    if (ret < 0) {
        if (error) {
            *error = QStringLiteral("编码图像失败: %1").arg(avErrorString(ret));
        }
        avcodec_free_context(&codecCtx);
        return false;
    }

    AVPacket* packet = av_packet_alloc();
    ret = avcodec_receive_packet(codecCtx, packet);
    if (ret < 0) {
        if (error) {
            *error = QStringLiteral("获取图像数据包失败: %1").arg(avErrorString(ret));
        }
        av_packet_free(&packet);
        avcodec_free_context(&codecCtx);
        return false;
    }

    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly)) {
        if (error) {
            *error = QStringLiteral("无法写入文件: %1").arg(filePath);
        }
        av_packet_free(&packet);
        avcodec_free_context(&codecCtx);
        return false;
    }

    const bool ok = file.write(reinterpret_cast<const char*>(packet->data), packet->size) == packet->size;
    file.close();

    av_packet_free(&packet);
    avcodec_free_context(&codecCtx);

    if (!ok && error) {
        *error = QStringLiteral("写入图像文件失败");
    }
    return ok;
}

FfmpegVideoEncoder::~FfmpegVideoEncoder()
{
    close();
}

bool FfmpegVideoEncoder::drainEncoderPackets(QString* error)
{
    while (true) {
        const int ret = avcodec_receive_packet(codecCtx_, packet_);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return true;
        }
        if (ret < 0) {
            if (error) {
                *error = QStringLiteral("接收视频包失败: %1").arg(avErrorString(ret));
            }
            return false;
        }
        if (!writePacket(packet_, error)) {
            av_packet_unref(packet_);
            return false;
        }
        av_packet_unref(packet_);
    }
}

bool FfmpegVideoEncoder::writePacket(AVPacket* packet, QString* error)
{
    if (packet->pts == AV_NOPTS_VALUE) {
        packet->pts = 0;
    }
    if (packet->duration <= 0) {
        packet->duration = 1;
    }
    av_packet_rescale_ts(packet, codecCtx_->time_base, stream_->time_base);
    packet->stream_index = stream_->index;
    const int ret = av_interleaved_write_frame(fmtCtx_, packet);
    if (ret < 0 && error) {
        *error = QStringLiteral("写入视频帧失败: %1").arg(avErrorString(ret));
    }
    return ret >= 0;
}

bool FfmpegVideoEncoder::open(const QString& filePath, int width, int height, double fps, QString* error)
{
    close();

    width = (width / 2) * 2;
    height = (height / 2) * 2;
    if (width <= 0 || height <= 0) {
        if (error) {
            *error = QStringLiteral("无效的视频尺寸");
        }
        return false;
    }

    const AVCodec* codec = avcodec_find_encoder_by_name("libx264");
    if (!codec) {
        codec = avcodec_find_encoder(AV_CODEC_ID_MPEG4);
    }
    if (!codec) {
        if (error) {
            *error = QStringLiteral("找不到视频编码器");
        }
        return false;
    }

    int ret = avformat_alloc_output_context2(&fmtCtx_, nullptr, nullptr, filePath.toUtf8().constData());
    if (ret < 0 || !fmtCtx_) {
        if (error) {
            *error = QStringLiteral("创建输出上下文失败: %1").arg(avErrorString(ret));
        }
        close();
        return false;
    }

    stream_ = avformat_new_stream(fmtCtx_, codec);
    if (!stream_) {
        if (error) {
            *error = QStringLiteral("创建视频流失败");
        }
        close();
        return false;
    }

    codecCtx_ = avcodec_alloc_context3(codec);
    codecCtx_->codec_id = codec->id;
    codecCtx_->width = width;
    codecCtx_->height = height;
    codecCtx_->pix_fmt = AV_PIX_FMT_YUV420P;
    codecCtx_->framerate = av_d2q(fps, 10000);
    codecCtx_->time_base = av_inv_q(codecCtx_->framerate);
    codecCtx_->gop_size = static_cast<int>(fps);
    codecCtx_->max_b_frames = 0;
    if (fmtCtx_->oformat->flags & AVFMT_GLOBALHEADER) {
        codecCtx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    if (codec->id == AV_CODEC_ID_H264) {
        av_opt_set(codecCtx_->priv_data, "preset", "veryfast", 0);
        av_opt_set(codecCtx_->priv_data, "tune", "zerolatency", 0);
        av_opt_set(codecCtx_->priv_data, "x264-params", "keyint=12:min-keyint=1:scenecut=0", 0);
    }

    if (QString(fmtCtx_->oformat->name) == QStringLiteral("mp4")) {
        av_opt_set(fmtCtx_->priv_data, "movflags", "+faststart", 0);
    }

    ret = avcodec_open2(codecCtx_, codec, nullptr);
    if (ret < 0) {
        if (error) {
            *error = QStringLiteral("打开视频编码器失败: %1").arg(avErrorString(ret));
        }
        close();
        return false;
    }

    ret = avcodec_parameters_from_context(stream_->codecpar, codecCtx_);
    if (ret < 0) {
        if (error) {
            *error = QStringLiteral("复制视频编码参数失败: %1").arg(avErrorString(ret));
        }
        close();
        return false;
    }

    stream_->time_base = codecCtx_->time_base;
    stream_->avg_frame_rate = codecCtx_->framerate;
    stream_->r_frame_rate = codecCtx_->framerate;

    if (!(fmtCtx_->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&fmtCtx_->pb, filePath.toUtf8().constData(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            if (error) {
                *error = QStringLiteral("打开输出文件失败: %1").arg(avErrorString(ret));
            }
            close();
            return false;
        }
    }

    ret = avformat_write_header(fmtCtx_, nullptr);
    if (ret < 0) {
        if (error) {
            *error = QStringLiteral("写入视频头失败: %1").arg(avErrorString(ret));
        }
        close();
        return false;
    }

    frame_ = av_frame_alloc();
    frame_->format = codecCtx_->pix_fmt;
    frame_->width = codecCtx_->width;
    frame_->height = codecCtx_->height;
    ret = av_frame_get_buffer(frame_, 0);
    if (ret < 0) {
        if (error) {
            *error = QStringLiteral("分配视频帧失败: %1").arg(avErrorString(ret));
        }
        close();
        return false;
    }

    packet_ = av_packet_alloc();
    srcWidth_ = 0;
    srcHeight_ = 0;
    pts_ = 0;
    return true;
}

bool FfmpegVideoEncoder::writeFrame(const cv::Mat& bgr, QString* error)
{
    if (!isOpen() || bgr.empty()) {
        if (error) {
            *error = QStringLiteral("视频编码器未打开或图像为空");
        }
        return false;
    }

    const AVPixelFormat srcPixFmt = bgr.channels() == 4 ? AV_PIX_FMT_BGRA : AV_PIX_FMT_BGR24;
    if (!swsCtx_ || srcWidth_ != bgr.cols || srcHeight_ != bgr.rows) {
        if (swsCtx_) {
            sws_freeContext(swsCtx_);
            swsCtx_ = nullptr;
        }
        swsCtx_ = sws_getContext(
            bgr.cols, bgr.rows, srcPixFmt,
            codecCtx_->width, codecCtx_->height, codecCtx_->pix_fmt,
            SWS_BILINEAR, nullptr, nullptr, nullptr);
        srcWidth_ = bgr.cols;
        srcHeight_ = bgr.rows;
        if (!swsCtx_) {
            if (error) {
                *error = QStringLiteral("视频色彩转换失败");
            }
            return false;
        }
    }

    const uint8_t* srcData[1] = {bgr.data};
    const int srcLinesize[1] = {static_cast<int>(bgr.step[0])};
    sws_scale(swsCtx_, srcData, srcLinesize, 0, bgr.rows, frame_->data, frame_->linesize);

    frame_->pts = pts_++;
    if (pts_ == 1) {
        frame_->pict_type = AV_PICTURE_TYPE_I;
    } else {
        frame_->pict_type = AV_PICTURE_TYPE_NONE;
    }
    int ret = avcodec_send_frame(codecCtx_, frame_);
    if (ret == AVERROR(EAGAIN)) {
        if (!drainEncoderPackets(error)) {
            return false;
        }
        ret = avcodec_send_frame(codecCtx_, frame_);
    }
    if (ret < 0) {
        if (error) {
            *error = QStringLiteral("发送视频帧失败: %1").arg(avErrorString(ret));
        }
        return false;
    }

    return drainEncoderPackets(error);
}

void FfmpegVideoEncoder::close()
{
    if (codecCtx_) {
        avcodec_send_frame(codecCtx_, nullptr);
        drainEncoderPackets(nullptr);
    }

    if (fmtCtx_) {
        av_write_trailer(fmtCtx_);
    }

    if (swsCtx_) {
        sws_freeContext(swsCtx_);
        swsCtx_ = nullptr;
    }
    av_packet_free(&packet_);
    av_frame_free(&frame_);
    if (codecCtx_) {
        avcodec_free_context(&codecCtx_);
        codecCtx_ = nullptr;
    }
    if (fmtCtx_) {
        if (!(fmtCtx_->oformat->flags & AVFMT_NOFILE) && fmtCtx_->pb) {
            avio_closep(&fmtCtx_->pb);
        }
        avformat_free_context(fmtCtx_);
        fmtCtx_ = nullptr;
    }
    stream_ = nullptr;
    pts_ = 0;
    srcWidth_ = 0;
    srcHeight_ = 0;
}

FfmpegAudioEncoder::~FfmpegAudioEncoder()
{
    close();
}

void FfmpegAudioEncoder::uninitChannelLayout(AVChannelLayout* layout)
{
    ::uninitChannelLayout(layout);
}

bool FfmpegAudioEncoder::drainEncoderPackets(QString* error)
{
    while (true) {
        const int ret = avcodec_receive_packet(codecCtx_, packet_);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            return true;
        }
        if (ret < 0) {
            if (error) {
                *error = QStringLiteral("接收音频包失败: %1").arg(avErrorString(ret));
            }
            return false;
        }
        if (!writePacket(packet_, error)) {
            av_packet_unref(packet_);
            return false;
        }
        av_packet_unref(packet_);
    }
}

bool FfmpegAudioEncoder::encodeFrame(QString* error)
{
    int ret = avcodec_send_frame(codecCtx_, frame_);
    if (ret == AVERROR(EAGAIN)) {
        if (!drainEncoderPackets(error)) {
            return false;
        }
        ret = avcodec_send_frame(codecCtx_, frame_);
    }
    if (ret < 0) {
        if (error) {
            *error = QStringLiteral("发送音频帧失败: %1").arg(avErrorString(ret));
        }
        return false;
    }
    return drainEncoderPackets(error);
}

bool FfmpegAudioEncoder::writePacket(AVPacket* packet, QString* error)
{
    av_packet_rescale_ts(packet, codecCtx_->time_base, stream_->time_base);
    packet->stream_index = stream_->index;
    const int ret = av_interleaved_write_frame(fmtCtx_, packet);
    if (ret < 0 && error) {
        *error = QStringLiteral("写入音频帧失败: %1").arg(avErrorString(ret));
    }
    return ret >= 0;
}

bool FfmpegAudioEncoder::ensureResampler(int sampleRate, int channels, int bitsPerSample, QString* error)
{
    if (swrCtx_ && sampleRate == configuredSampleRate_ && channels == configuredChannels_) {
        return true;
    }

    if (swrCtx_) {
        swr_free(&swrCtx_);
        swrCtx_ = nullptr;
    }

    AVSampleFormat srcFmt = AV_SAMPLE_FMT_FLT;
    if (bitsPerSample == 16) {
        srcFmt = AV_SAMPLE_FMT_S16;
    }

    AVChannelLayout outLayout = codecCtx_->ch_layout;
    AVChannelLayout inLayout = channelLayoutForCount(channels);

    const int ret = swr_alloc_set_opts2(
        &swrCtx_,
        &outLayout,
        codecCtx_->sample_fmt,
        codecCtx_->sample_rate,
        &inLayout,
        srcFmt,
        sampleRate,
        0,
        nullptr);
    if (channels > 2) {
        uninitChannelLayout(&inLayout);
    }
    if (ret < 0 || !swrCtx_) {
        if (error) {
            *error = QStringLiteral("创建音频重采样器失败: %1").arg(avErrorString(ret));
        }
        return false;
    }

    const int initRet = swr_init(swrCtx_);
    if (initRet < 0) {
        if (error) {
            *error = QStringLiteral("初始化音频重采样器失败: %1").arg(avErrorString(initRet));
        }
        swr_free(&swrCtx_);
        return false;
    }

    configuredSampleRate_ = sampleRate;
    configuredChannels_ = channels;
    return true;
}

bool FfmpegAudioEncoder::open(const QString& filePath, int sampleRate, int channels, QString* error)
{
    close();

    if (sampleRate <= 0 || channels <= 0) {
        if (error) {
            *error = QStringLiteral("无效的音频参数");
        }
        return false;
    }

    const AVCodecID codecId = audioCodecForPath(filePath);
    const AVCodec* codec = avcodec_find_encoder(codecId);
    if (!codec) {
        if (error) {
            *error = QStringLiteral("找不到音频编码器");
        }
        return false;
    }

    int ret = avformat_alloc_output_context2(&fmtCtx_, nullptr, nullptr, filePath.toUtf8().constData());
    if (ret < 0 || !fmtCtx_) {
        if (error) {
            *error = QStringLiteral("创建音频输出上下文失败: %1").arg(avErrorString(ret));
        }
        close();
        return false;
    }

    stream_ = avformat_new_stream(fmtCtx_, codec);
    if (!stream_) {
        if (error) {
            *error = QStringLiteral("创建音频流失败");
        }
        close();
        return false;
    }

    codecCtx_ = avcodec_alloc_context3(codec);
    codecCtx_->codec_id = codecId;
    codecCtx_->sample_rate = sampleRate;
    uninitChannelLayout(&codecCtx_->ch_layout);
    codecCtx_->ch_layout = channelLayoutForCount(channels);
    codecCtx_->sample_fmt = audioSampleFormat(codecId);
    codecCtx_->time_base = {1, sampleRate};
    codecCtx_->bit_rate = 192000;
    if (fmtCtx_->oformat->flags & AVFMT_GLOBALHEADER) {
        codecCtx_->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }

    ret = avcodec_open2(codecCtx_, codec, nullptr);
    if (ret < 0) {
        if (error) {
            *error = QStringLiteral("打开音频编码器失败: %1").arg(avErrorString(ret));
        }
        close();
        return false;
    }

    ret = avcodec_parameters_from_context(stream_->codecpar, codecCtx_);
    if (ret < 0) {
        if (error) {
            *error = QStringLiteral("复制音频编码参数失败: %1").arg(avErrorString(ret));
        }
        close();
        return false;
    }

    if (!(fmtCtx_->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&fmtCtx_->pb, filePath.toUtf8().constData(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            if (error) {
                *error = QStringLiteral("打开音频输出文件失败: %1").arg(avErrorString(ret));
            }
            close();
            return false;
        }
    }

    ret = avformat_write_header(fmtCtx_, nullptr);
    if (ret < 0) {
        if (error) {
            *error = QStringLiteral("写入音频头失败: %1").arg(avErrorString(ret));
        }
        close();
        return false;
    }

    outputChunkSamples_ = codecCtx_->frame_size > 0 ? codecCtx_->frame_size : 1024;

    frame_ = av_frame_alloc();
    frame_->format = codecCtx_->sample_fmt;
    frame_->sample_rate = codecCtx_->sample_rate;
    av_channel_layout_copy(&frame_->ch_layout, &codecCtx_->ch_layout);
    frame_->nb_samples = outputChunkSamples_;
    ret = av_frame_get_buffer(frame_, 0);
    if (ret < 0) {
        if (error) {
            *error = QStringLiteral("分配音频帧失败: %1").arg(avErrorString(ret));
        }
        close();
        return false;
    }

    packet_ = av_packet_alloc();
    configuredSampleRate_ = sampleRate;
    configuredChannels_ = channels;
    pts_ = 0;
    return true;
}

bool FfmpegAudioEncoder::writeFrame(const AudioFrame& audioFrame, QString* error)
{
    if (!isOpen() || audioFrame.data.isEmpty()) {
        if (error) {
            *error = QStringLiteral("音频编码器未打开或数据为空");
        }
        return false;
    }

    if (!ensureResampler(audioFrame.sampleRate, audioFrame.channels, audioFrame.bitsPerSample, error)) {
        return false;
    }

    const int bytesPerSample = qMax(1, audioFrame.bitsPerSample / 8);
    const int totalSamples = audioFrame.data.size() / bytesPerSample;
    const int samplesPerChannel = totalSamples / qMax(1, audioFrame.channels);
    if (samplesPerChannel <= 0) {
        if (error) {
            *error = QStringLiteral("音频样本数无效");
        }
        return false;
    }

    const uint8_t* srcData[1] = {reinterpret_cast<const uint8_t*>(audioFrame.data.constData())};
    int ret = swr_convert(swrCtx_, nullptr, 0, srcData, samplesPerChannel);
    if (ret < 0) {
        if (error) {
            *error = QStringLiteral("音频重采样失败: %1").arg(avErrorString(ret));
        }
        return false;
    }

    while (true) {
        ret = swr_convert(swrCtx_, frame_->data, outputChunkSamples_, nullptr, 0);
        if (ret <= 0) {
            break;
        }

        frame_->nb_samples = ret;
        frame_->pts = pts_;
        pts_ += ret;

        if (!encodeFrame(error)) {
            return false;
        }
    }
    return true;
}

void FfmpegAudioEncoder::close()
{
    if (swrCtx_ && frame_) {
        while (true) {
            const int ret = swr_convert(swrCtx_, frame_->data, outputChunkSamples_, nullptr, 0);
            if (ret <= 0) {
                break;
            }
            frame_->nb_samples = ret;
            frame_->pts = pts_;
            pts_ += ret;
            encodeFrame(nullptr);
        }
    }

    if (codecCtx_) {
        avcodec_send_frame(codecCtx_, nullptr);
        drainEncoderPackets(nullptr);
    }

    if (fmtCtx_) {
        av_write_trailer(fmtCtx_);
    }

    if (swrCtx_) {
        swr_free(&swrCtx_);
        swrCtx_ = nullptr;
    }
    av_packet_free(&packet_);
    av_frame_free(&frame_);
    if (codecCtx_) {
        avcodec_free_context(&codecCtx_);
        codecCtx_ = nullptr;
    }
    if (fmtCtx_) {
        if (!(fmtCtx_->oformat->flags & AVFMT_NOFILE) && fmtCtx_->pb) {
            avio_closep(&fmtCtx_->pb);
        }
        avformat_free_context(fmtCtx_);
        fmtCtx_ = nullptr;
    }
    stream_ = nullptr;
    pts_ = 0;
    outputChunkSamples_ = 0;
    configuredSampleRate_ = 0;
    configuredChannels_ = 0;
}
} // namespace Nodes
