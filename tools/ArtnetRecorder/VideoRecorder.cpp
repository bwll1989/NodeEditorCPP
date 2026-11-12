#include "VideoRecorder.h"
#include <QDebug>
#include <QDir>
#include <QStandardPaths>

VideoRecorder::VideoRecorder(QObject *parent)
    : QObject(parent)
    , m_isRecording(false)
    , m_frameCount(0)
    , m_formatContext(nullptr)
    , m_codecContext(nullptr)
    , m_videoStream(nullptr)
    , m_frame(nullptr)
    , m_packet(nullptr)
    , m_swsContext(nullptr)
{
}

VideoRecorder::~VideoRecorder()
{
    if (m_isRecording) {
        stopRecording();
    }
    cleanup();
}

/**
 * @brief 开始录制视频
 * @param filename 输出视频文件名
 * @param width 视频宽度
 * @param height 视频高度
 * @param fps 帧率（默认25fps）
 * @return 是否成功开始录制
 */
/**
 * @brief 开始录制视频
 */
bool VideoRecorder::startRecording(const QString &filename, int width, int height)
{
    QMutexLocker locker(&m_mutex);
    
    if (m_isRecording) {
        qWarning() << "Already recording!";
        return false;
    }
    m_frameCount = 0;
    
    if (!initializeEncoder(filename, width, height, FPS)) {
        cleanup();
        return false;
    }
    
    m_isRecording = true;
    emit recordingStarted();
    qDebug() << "Started recording to" << filename << "at" << FPS << "fps";
    
    return true;
}

/**
 * @brief 获取录制时间（毫秒）
 */
qint64 VideoRecorder::getRecordingFrames() const
{
    if (m_isRecording ) {
        return m_frameCount;
    }
    return 0;
}

/**
 * @brief 停止录制视频
 */
void VideoRecorder::stopRecording()
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_isRecording) {
        return;
    }
    
    m_isRecording = false;
    
    // 写入文件尾部
    if (m_formatContext) {
        av_write_trailer(m_formatContext);
    }
    
    cleanup();
    emit recordingStopped();
    qDebug() << "Stopped recording. Total frames:" << m_frameCount;
}

/**
 * @brief 添加一帧图像到视频
 * @param image 要录制的图像
 */
void VideoRecorder::addFrame(const QImage &image)
{
    QMutexLocker locker(&m_mutex);
    
    if (!m_isRecording || !m_codecContext) {
        return;
    }
    
    if (!encodeFrame(image)) {
        emit recordingError("Failed to encode frame");
        stopRecording();
    }
}

/**
 * @brief 初始化FFmpeg编码器
 * @param filename 输出文件名
 * @param width 视频宽度
 * @param height 视频高度
 * @param fps 帧率
 * @return 初始化是否成功
 */
bool VideoRecorder::initializeEncoder(const QString &filename, int width, int height, int fps)
{
    
    /**
     * @brief 初始化FFmpeg编码器（无损灰度，支持512*N）
     * 使用 matroska 封装 + FFV1 编码，像素格式为 GRAY8（mono）
     * 说明：强制封装器为 matroska，避免按扩展名推断导致不兼容；FFV1 为无损编码
     */
    int ret;
    // 分配输出格式上下文：强制使用 matroska 封装器（扩展名仍可为 .dmx）
    ret = avformat_alloc_output_context2(&m_formatContext, nullptr, "matroska", filename.toUtf8().constData());
    if (ret < 0) {
        qWarning() << "Could not allocate output format context";
        return false;
    }
    
    // 查找无损编码器：FFV1
    const AVCodec *codec = avcodec_find_encoder(AV_CODEC_ID_FFV1);
    if (!codec) {
        qWarning() << "FFV1 codec not found";
        return false;
    }
    
    // 创建视频流
    m_videoStream = avformat_new_stream(m_formatContext, codec);
    if (!m_videoStream) {
        qWarning() << "Could not create video stream";
        return false;
    }
    
    // 分配编码器上下文
    m_codecContext = avcodec_alloc_context3(codec);
    if (!m_codecContext) {
        qWarning() << "Could not allocate codec context";
        return false;
    }
    
    // 设置编码器参数（无损灰度）
    m_codecContext->width = width;
    m_codecContext->height = height;
    m_codecContext->time_base = {1, fps};
    m_codecContext->framerate = {fps, 1};
    m_codecContext->pix_fmt = AV_PIX_FMT_GRAY8;   // 使用灰度（mono）
    m_codecContext->bit_rate = 0;                 // 无损无需设置比特率
    m_codecContext->gop_size = 1;                 // 关键帧（帧间无损）
    m_codecContext->max_b_frames = 0;
    
    // matroska 常为全局头格式
    if (m_formatContext->oformat->flags & AVFMT_GLOBALHEADER) {
        m_codecContext->flags |= AV_CODEC_FLAG_GLOBAL_HEADER;
    }
    
    // 打开编码器
    ret = avcodec_open2(m_codecContext, codec, nullptr);
    if (ret < 0) {
        qWarning() << "Could not open codec";
        return false;
    }
    
    // 拷贝编码器参数到流
    ret = avcodec_parameters_from_context(m_videoStream->codecpar, m_codecContext);
    if (ret < 0) {
        qWarning() << "Could not copy codec parameters";
        return false;
    }
    
    // 分配帧
    m_frame = av_frame_alloc();
    if (!m_frame) {
        qWarning() << "Could not allocate frame";
        return false;
    }
    m_frame->format = m_codecContext->pix_fmt;
    m_frame->width = width;
    m_frame->height = height;
    
    ret = av_frame_get_buffer(m_frame, 0);
    if (ret < 0) {
        qWarning() << "Could not allocate frame buffer";
        return false;
    }
    
    // 分配数据包
    m_packet = av_packet_alloc();
    if (!m_packet) {
        qWarning() << "Could not allocate packet";
        return false;
    }
    
    /**
     * @brief 初始化图像转换上下文
     * 源：RGBA（QImage），目标：GRAY8（单通道），使用最近邻避免插值平滑
     */
    m_swsContext = sws_getContext(
        width, height, AV_PIX_FMT_RGBA,   // 源格式：RGBA
        width, height, AV_PIX_FMT_GRAY8,  // 目标格式：GRAY8（mono）
        SWS_POINT, nullptr, nullptr, nullptr
    );
    if (!m_swsContext) {
        qWarning() << "Could not initialize sws context";
        return false;
    }
    
    // 打开输出文件
    if (!(m_formatContext->oformat->flags & AVFMT_NOFILE)) {
        ret = avio_open(&m_formatContext->pb, filename.toUtf8().constData(), AVIO_FLAG_WRITE);
        if (ret < 0) {
            qWarning() << "Could not open output file";
            return false;
        }
    }
    
    // 写入文件头
    ret = avformat_write_header(m_formatContext, nullptr);
    if (ret < 0) {
        qWarning() << "Could not write header";
        return false;
    }

    return true;
}

/**
 * @brief 清理FFmpeg资源
 */
void VideoRecorder::cleanup()
{
    if (m_swsContext) {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }
    
    if (m_packet) {
        av_packet_free(&m_packet);
    }
    
    if (m_frame) {
        av_frame_free(&m_frame);
    }
    
    if (m_codecContext) {
        avcodec_free_context(&m_codecContext);
    }
    
    if (m_formatContext) {
        if (!(m_formatContext->oformat->flags & AVFMT_NOFILE)) {
            avio_closep(&m_formatContext->pb);
        }
        avformat_free_context(m_formatContext);
        m_formatContext = nullptr;
    }
}

/**
 * @brief 编码一帧图像
 * @param image 要编码的图像
 * @return 是否编码成功
 */
bool VideoRecorder::encodeFrame(const QImage &image)
{
    if (!m_frame || !m_codecContext || !m_swsContext) {
        return false;
    }
    
    // 确保图像格式为RGBA8888（保持Alpha通道）
    QImage rgbaImage = image.convertToFormat(QImage::Format_RGBA8888);
    
    // 从图像获取实际尺寸
    int imageHeight = rgbaImage.height();
    
    // 准备源数据
    const uint8_t *srcData[1] = { rgbaImage.constBits() };
    int srcLinesize[1] = { rgbaImage.bytesPerLine() };
    
    // 直接复制RGBA数据到帧缓冲区，使用图像的实际高度
    sws_scale(m_swsContext, srcData, srcLinesize, 0, imageHeight,
              m_frame->data, m_frame->linesize);
    // 设置帧的时间戳
    m_frame->pts = m_frameCount++;
    // 设置关键帧标志（FFmpeg 5.1+ 使用 flags，旧字段 key_frame 已移除）
    m_frame->flags |= AV_FRAME_FLAG_KEY;
    m_frame->pict_type = AV_PICTURE_TYPE_I;
    
    // 发送帧到编码器
    int ret = avcodec_send_frame(m_codecContext, m_frame);
    if (ret < 0) {
        qWarning() << "Error sending frame to encoder";
        return false;
    }
    
    // 接收编码后的数据包
    while (ret >= 0) {
        ret = avcodec_receive_packet(m_codecContext, m_packet);
        if (ret == AVERROR(EAGAIN) || ret == AVERROR_EOF) {
            break;
        } else if (ret < 0) {
            qWarning() << "Error receiving packet from encoder";
            return false;
        }
        
        // 设置数据包的流索引
        m_packet->stream_index = m_videoStream->index;
        
        // 设置关键帧标志
        m_packet->flags |= AV_PKT_FLAG_KEY;
        
        // 重新计算时间戳
        av_packet_rescale_ts(m_packet, m_codecContext->time_base, m_videoStream->time_base);
        
        // 写入数据包到文件
        ret = av_interleaved_write_frame(m_formatContext, m_packet);
        if (ret < 0) {
            qWarning() << "Error writing packet";
            av_packet_unref(m_packet);
            return false;
        }
        
        av_packet_unref(m_packet);
    }
    
    return true;
}
