#include "CameraCapture.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavdevice/avdevice.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libavutil/log.h>
#include <libswscale/swscale.h>
}

#include <QDebug>
#include <QMutexLocker>

namespace {

QString avErrorString(int err)
{
    char buf[AV_ERROR_MAX_STRING_SIZE] = {0};
    av_strerror(err, buf, sizeof(buf));
    return QString::fromUtf8(buf);
}

/** 打开 dshow 时对设备名做简单转义（含冒号等特殊字符时用别名更稳，此处保留友好名） */
QString dshowVideoUrl(const QString& deviceName)
{
    return QStringLiteral("video=%1").arg(deviceName);
}

} // namespace

CameraCaptureThread::CameraCaptureThread(QObject* parent)
    : QThread(parent)
{
    avdevice_register_all();
}

CameraCaptureThread::~CameraCaptureThread()
{
    stop();
    if (isRunning()) {
        if (!wait(5000)) {
            qWarning() << "CameraCaptureThread: did not stop in time, terminating";
            terminate();
            wait();
        }
    }
}

QList<CameraDeviceInfo> CameraCaptureThread::listVideoDevices()
{
    avdevice_register_all();

    QList<CameraDeviceInfo> devices;
    const AVInputFormat* inputFormat = av_find_input_format("dshow");
    if (!inputFormat) {
        qWarning() << "CameraCapture: dshow input format not found";
        return devices;
    }

    AVDeviceInfoList* deviceList = nullptr;
    const int ret = avdevice_list_input_sources(inputFormat, nullptr, nullptr, &deviceList);
    if (ret < 0 || !deviceList) {
        qWarning() << "CameraCapture: avdevice_list_input_sources failed:" << avErrorString(ret);
        return devices;
    }

    auto isSystemPath = [](const QString& s) {
        return s.startsWith(QStringLiteral("@device_"), Qt::CaseInsensitive)
            || s.contains(QStringLiteral("\\\\?\\"))
            || s.contains(QStringLiteral("#{"));
    };

    for (int i = 0; i < deviceList->nb_devices; ++i) {
        const AVDeviceInfo* info = deviceList->devices[i];
        if (!info) {
            continue;
        }

        bool hasVideo = false;
        for (int t = 0; t < info->nb_media_types; ++t) {
            if (info->media_types[t] == AVMEDIA_TYPE_VIDEO) {
                hasVideo = true;
                break;
            }
        }
        // nb_media_types==0 时 dshow 可能未标注类型，仍尝试加入（打开时再校验）
        if (info->nb_media_types > 0 && !hasVideo) {
            continue;
        }

        // dshow：device_name 常为 @device_pnp_... 系统路径，device_description 才是可读名
        const QString rawName = info->device_name
            ? QString::fromUtf8(info->device_name)
            : QString();
        const QString rawDesc = info->device_description
            ? QString::fromUtf8(info->device_description)
            : QString();

        QString displayName;
        QString openName;
        if (!rawDesc.isEmpty() && !isSystemPath(rawDesc)) {
            displayName = rawDesc;
            openName = rawDesc;
        } else if (!rawName.isEmpty() && !isSystemPath(rawName)) {
            displayName = rawName;
            openName = rawName;
        } else if (!rawDesc.isEmpty()) {
            displayName = rawDesc;
            openName = rawDesc;
        } else if (!rawName.isEmpty()) {
            displayName = rawName;
            openName = rawName;
        } else {
            continue;
        }

        // 若 display 仍是系统路径而另一侧可读，互换优先显示可读名
        if (isSystemPath(displayName) && !rawDesc.isEmpty() && !isSystemPath(rawDesc)) {
            displayName = rawDesc;
            openName = rawDesc;
        } else if (isSystemPath(displayName) && !rawName.isEmpty() && !isSystemPath(rawName)) {
            displayName = rawName;
            openName = rawName;
        }

        bool exists = false;
        for (const auto& d : devices) {
            if (d.displayName == displayName || d.openName == openName) {
                exists = true;
                break;
            }
        }
        if (!exists) {
            devices.append({displayName, openName});
        }
    }

    avdevice_free_list_devices(&deviceList);
    return devices;
}

void CameraCaptureThread::startCapture(const QString& deviceName)
{
    if (isRunning()) {
        stop();
        if (!wait(5000)) {
            qWarning() << "CameraCaptureThread: previous session still running";
            terminate();
            wait();
        }
    }

    QMutexLocker locker(&m_mutex);
    m_deviceName = deviceName.trimmed();
    m_abort.store(false);
    m_running.store(true);
    start(QThread::HighPriority);
}

void CameraCaptureThread::stop()
{
    QMutexLocker locker(&m_mutex);
    m_abort.store(true);
    m_running.store(false);
}

int CameraCaptureThread::interruptCallback(void* opaque)
{
    auto* self = static_cast<CameraCaptureThread*>(opaque);
    return self && self->shouldAbort() ? 1 : 0;
}

bool CameraCaptureThread::shouldAbort() const
{
    return m_abort.load();
}

void CameraCaptureThread::releaseScaler()
{
    if (m_swsContext) {
        sws_freeContext(m_swsContext);
        m_swsContext = nullptr;
    }
    if (m_dstData[0]) {
        av_freep(&m_dstData[0]);
        for (int i = 0; i < 4; ++i) {
            m_dstData[i] = nullptr;
            m_dstLinesize[i] = 0;
        }
    }
    m_frameWidth = 0;
    m_frameHeight = 0;
}

void CameraCaptureThread::cleanupFFmpeg()
{
    releaseScaler();

    if (m_packet) {
        av_packet_free(&m_packet);
        m_packet = nullptr;
    }
    if (m_frame) {
        av_frame_free(&m_frame);
        m_frame = nullptr;
    }
    if (m_codecContext) {
        avcodec_free_context(&m_codecContext);
        m_codecContext = nullptr;
    }
    if (m_formatContext) {
        m_formatContext->interrupt_callback.callback = nullptr;
        m_formatContext->interrupt_callback.opaque = nullptr;
        avformat_close_input(&m_formatContext);
        m_formatContext = nullptr;
    }

    m_videoStreamIndex = -1;
}

bool CameraCaptureThread::openDevice(const QString& deviceName)
{
    cleanupFFmpeg();

    if (deviceName.isEmpty()) {
        return false;
    }

    const AVInputFormat* inputFormat = av_find_input_format("dshow");
    if (!inputFormat) {
        emit errorOccurred(QStringLiteral("未找到 dshow 输入格式（请确认已链接项目 FFmpeg avdevice）"));
        return false;
    }

    m_formatContext = avformat_alloc_context();
    if (!m_formatContext) {
        return false;
    }
    m_formatContext->interrupt_callback.callback = interruptCallback;
    m_formatContext->interrupt_callback.opaque = this;

    const QByteArray urlBytes = dshowVideoUrl(deviceName).toUtf8();

    auto tryOpen = [&](AVDictionary** options) -> int {
        return avformat_open_input(&m_formatContext, urlBytes.constData(), inputFormat, options);
    };

    // 先尝试常见分辨率；失败则回退为设备默认参数
    // 注意：avformat_open_input 失败时会释放 *ps 并置空
    AVDictionary* options = nullptr;
    av_dict_set(&options, "video_size", "1920x1080", 0);
    av_dict_set(&options, "framerate", "30", 0);
    int openRet = tryOpen(&options);
    av_dict_free(&options);

    if (openRet < 0) {
        m_formatContext = avformat_alloc_context();
        if (!m_formatContext) {
            emit errorOccurred(
                QStringLiteral("无法打开摄像头 \"%1\": %2").arg(deviceName, avErrorString(openRet)));
            return false;
        }
        m_formatContext->interrupt_callback.callback = interruptCallback;
        m_formatContext->interrupt_callback.opaque = this;
        options = nullptr;
        openRet = tryOpen(&options);
        av_dict_free(&options);
    }

    if (openRet < 0) {
        emit errorOccurred(
            QStringLiteral("无法打开摄像头 \"%1\": %2").arg(deviceName, avErrorString(openRet)));
        m_formatContext = nullptr;
        return false;
    }

    if (avformat_find_stream_info(m_formatContext, nullptr) < 0) {
        cleanupFFmpeg();
        emit errorOccurred(QStringLiteral("无法读取摄像头流信息"));
        return false;
    }

    m_videoStreamIndex = av_find_best_stream(m_formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (m_videoStreamIndex < 0) {
        cleanupFFmpeg();
        emit errorOccurred(QStringLiteral("摄像头无视频流"));
        return false;
    }

    AVCodecParameters* codecpar = m_formatContext->streams[m_videoStreamIndex]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        cleanupFFmpeg();
        emit errorOccurred(QStringLiteral("找不到视频解码器"));
        return false;
    }

    m_codecContext = avcodec_alloc_context3(codec);
    if (!m_codecContext) {
        cleanupFFmpeg();
        return false;
    }

    if (avcodec_parameters_to_context(m_codecContext, codecpar) < 0
        || avcodec_open2(m_codecContext, codec, nullptr) < 0) {
        cleanupFFmpeg();
        emit errorOccurred(QStringLiteral("打开视频解码器失败"));
        return false;
    }

    m_packet = av_packet_alloc();
    m_frame = av_frame_alloc();
    if (!m_packet || !m_frame) {
        cleanupFFmpeg();
        return false;
    }

    qInfo() << "Camera FFmpeg dshow opened:" << deviceName
            << m_codecContext->width << "x" << m_codecContext->height
            << "pix_fmt" << m_codecContext->pix_fmt;
    return true;
}

void CameraCaptureThread::convertToBgr(AVFrame* frame, cv::Mat& output)
{
    if (!frame || !m_codecContext) {
        return;
    }

    const int frameW = frame->width > 0 ? frame->width : m_codecContext->width;
    const int frameH = frame->height > 0 ? frame->height : m_codecContext->height;
    if (frameW <= 0 || frameH <= 0) {
        return;
    }

    const AVPixelFormat srcFmt = static_cast<AVPixelFormat>(frame->format);
    if (srcFmt == AV_PIX_FMT_NONE) {
        return;
    }

    if (!m_swsContext || m_frameWidth != frameW || m_frameHeight != frameH) {
        releaseScaler();
        m_frameWidth = frameW;
        m_frameHeight = frameH;

        m_swsContext = sws_getContext(frameW, frameH, srcFmt,
                                      frameW, frameH, AV_PIX_FMT_BGR24,
                                      SWS_BILINEAR, nullptr, nullptr, nullptr);
        if (!m_swsContext) {
            return;
        }
        if (av_image_alloc(m_dstData, m_dstLinesize, frameW, frameH, AV_PIX_FMT_BGR24, 1) < 0) {
            releaseScaler();
            return;
        }
    }

    sws_scale(m_swsContext, frame->data, frame->linesize, 0, frameH,
              m_dstData, m_dstLinesize);

    cv::Mat img(frameH, frameW, CV_8UC3, m_dstData[0], m_dstLinesize[0]);
    output = img.clone();
}

bool CameraCaptureThread::readFrame(cv::Mat& output)
{
    if (!m_formatContext || !m_codecContext || !m_packet || !m_frame) {
        return false;
    }

    while (!shouldAbort()) {
        const int ret = av_read_frame(m_formatContext, m_packet);
        if (ret < 0) {
            return false;
        }

        if (m_packet->stream_index != m_videoStreamIndex) {
            av_packet_unref(m_packet);
            continue;
        }

        if (avcodec_send_packet(m_codecContext, m_packet) < 0) {
            av_packet_unref(m_packet);
            continue;
        }
        av_packet_unref(m_packet);

        const int decodeRet = avcodec_receive_frame(m_codecContext, m_frame);
        if (decodeRet == AVERROR(EAGAIN) || decodeRet == AVERROR_EOF) {
            continue;
        }
        if (decodeRet < 0) {
            return false;
        }

        convertToBgr(m_frame, output);
        av_frame_unref(m_frame);
        return !output.empty();
    }
    return false;
}

void CameraCaptureThread::run()
{
    struct RunCleanup {
        CameraCaptureThread* self = nullptr;
        explicit RunCleanup(CameraCaptureThread* worker)
            : self(worker)
        {
        }
        ~RunCleanup()
        {
            if (self) {
                self->cleanupFFmpeg();
            }
        }
    } runCleanup(this);

    QString deviceName;
    {
        QMutexLocker locker(&m_mutex);
        deviceName = m_deviceName;
    }

    if (deviceName.isEmpty() || shouldAbort() || !m_running.load()) {
        emit captureStateChanged(false);
        return;
    }

    if (!openDevice(deviceName)) {
        emit captureStateChanged(false);
        return;
    }

    emit captureStateChanged(true);

    while (m_running.load() && !shouldAbort()) {
        cv::Mat frame;
        if (!readFrame(frame)) {
            if (shouldAbort() || !m_running.load()) {
                break;
            }
            qWarning() << "CameraCapture: readFrame failed, retrying";
            msleep(50);
            continue;
        }

        if (!m_running.load() || shouldAbort()) {
            break;
        }

        emit frameAvailable(frame);
    }

    emit captureStateChanged(false);
}
