#include "RtspStreamReceiver.h"

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}

#include <QDebug>

namespace {

int interruptCallback(void* opaque)
{
    auto* worker = static_cast<RtspStreamReceiverWorker*>(opaque);
    return worker && worker->shouldAbort() ? 1 : 0;
}

} // namespace

RtspStreamReceiver::RtspStreamReceiver(QObject* parent)
    : QObject(parent)
    , m_worker(new RtspStreamReceiverWorker(this))
{
    qRegisterMetaType<cv::Mat>("cv::Mat");

    connect(m_worker, &RtspStreamReceiverWorker::frameReceived,
            this, &RtspStreamReceiver::frameReceived, Qt::QueuedConnection);
    connect(m_worker, &RtspStreamReceiverWorker::connectionStatusChanged,
            this, &RtspStreamReceiver::connectionStatusChanged, Qt::QueuedConnection);
    connect(m_worker, &RtspStreamReceiverWorker::errorOccurred,
            this, &RtspStreamReceiver::errorOccurred, Qt::QueuedConnection);
}

RtspStreamReceiver::~RtspStreamReceiver()
{
    stop();
}

void RtspStreamReceiver::start(const QString& url)
{
    setUrl(url);
    m_worker->startReceiving(url);
    if (!m_worker->isRunning()) {
        m_worker->start();
    }
}

void RtspStreamReceiver::stop()
{
    if (!m_worker) {
        return;
    }

    disconnect(m_worker, nullptr, this, nullptr);

    m_worker->stopReceiving();
    if (m_worker->isRunning()) {
        if (!m_worker->wait(5000)) {
            qWarning() << "RtspStreamReceiver: worker did not stop in time, terminating";
            m_worker->terminate();
            m_worker->wait();
        }
    }

    connect(m_worker, &RtspStreamReceiverWorker::frameReceived,
            this, &RtspStreamReceiver::frameReceived, Qt::QueuedConnection);
    connect(m_worker, &RtspStreamReceiverWorker::connectionStatusChanged,
            this, &RtspStreamReceiver::connectionStatusChanged, Qt::QueuedConnection);
    connect(m_worker, &RtspStreamReceiverWorker::errorOccurred,
            this, &RtspStreamReceiver::errorOccurred, Qt::QueuedConnection);
}

void RtspStreamReceiver::setUrl(const QString& url)
{
    if (m_worker) {
        m_worker->setUrl(url);
    }
}

RtspStreamReceiverWorker::RtspStreamReceiverWorker(QObject* parent)
    : QThread(parent)
{
}

RtspStreamReceiverWorker::~RtspStreamReceiverWorker()
{
    stopReceiving();
    if (isRunning()) {
        if (!wait(5000)) {
            qWarning() << "RtspStreamReceiverWorker: thread did not exit, terminating";
            terminate();
            wait();
        }
    }
    // cleanupFFmpeg 仅在工作线程 run() 退出时调用，避免跨线程释放 FFmpeg 上下文
}

bool RtspStreamReceiverWorker::shouldAbort() const
{
    return m_abort.load();
}

void RtspStreamReceiverWorker::startReceiving(const QString& url)
{
    QMutexLocker locker(&m_mutex);
    m_abort.store(false);
    m_url = url;
    m_running = true;
    m_reconnectRequested = true;
}

void RtspStreamReceiverWorker::stopReceiving()
{
    QMutexLocker locker(&m_mutex);
    m_abort.store(true);
    m_running = false;
    m_reconnectRequested = true;
}

void RtspStreamReceiverWorker::setUrl(const QString& url)
{
    QMutexLocker locker(&m_mutex);
    if (m_url != url) {
        m_url = url;
        m_reconnectRequested = true;
    }
}

void RtspStreamReceiverWorker::releaseScaler()
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

void RtspStreamReceiverWorker::cleanupFFmpeg()
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
        // 断开 interrupt 回调，避免 stop/析构后 FFmpeg 仍回调已销毁的 worker
        m_formatContext->interrupt_callback.callback = nullptr;
        m_formatContext->interrupt_callback.opaque = nullptr;
        avformat_close_input(&m_formatContext);
        m_formatContext = nullptr;
    }

    m_videoStreamIndex = -1;
    m_streamOpen = false;
}

bool RtspStreamReceiverWorker::openStream(const QString& url)
{
    cleanupFFmpeg();

    if (url.trimmed().isEmpty()) {
        return false;
    }

    avformat_network_init();

    m_formatContext = avformat_alloc_context();
    if (!m_formatContext) {
        return false;
    }
    m_formatContext->interrupt_callback.callback = interruptCallback;
    m_formatContext->interrupt_callback.opaque = this;

    AVDictionary* options = nullptr;
    av_dict_set(&options, "rtsp_transport", "tcp", 0);
    av_dict_set(&options, "stimeout", "5000000", 0);
    av_dict_set(&options, "max_delay", "500000", 0);

    const QByteArray urlBytes = url.toUtf8();
    if (avformat_open_input(&m_formatContext, urlBytes.constData(), nullptr, &options) < 0) {
        av_dict_free(&options);
        if (m_formatContext) {
            avformat_free_context(m_formatContext);
            m_formatContext = nullptr;
        }
        return false;
    }
    av_dict_free(&options);

    if (avformat_find_stream_info(m_formatContext, nullptr) < 0) {
        cleanupFFmpeg();
        return false;
    }

    m_videoStreamIndex = av_find_best_stream(m_formatContext, AVMEDIA_TYPE_VIDEO, -1, -1, nullptr, 0);
    if (m_videoStreamIndex < 0) {
        cleanupFFmpeg();
        return false;
    }

    AVCodecParameters* codecpar = m_formatContext->streams[m_videoStreamIndex]->codecpar;
    const AVCodec* codec = avcodec_find_decoder(codecpar->codec_id);
    if (!codec) {
        cleanupFFmpeg();
        return false;
    }

    m_codecContext = avcodec_alloc_context3(codec);
    if (!m_codecContext) {
        cleanupFFmpeg();
        return false;
    }

    if (avcodec_parameters_to_context(m_codecContext, codecpar) < 0) {
        cleanupFFmpeg();
        return false;
    }

    if (avcodec_open2(m_codecContext, codec, nullptr) < 0) {
        cleanupFFmpeg();
        return false;
    }

    m_packet = av_packet_alloc();
    m_frame = av_frame_alloc();
    if (!m_packet || !m_frame) {
        cleanupFFmpeg();
        return false;
    }

    m_streamOpen = true;
    return true;
}

void RtspStreamReceiverWorker::convertToBgr(AVFrame* frame, cv::Mat& output)
{
    if (!frame || !m_codecContext) {
        return;
    }

    const int frameW = m_codecContext->width;
    const int frameH = m_codecContext->height;
    if (frameW <= 0 || frameH <= 0) {
        return;
    }

    if (!m_swsContext || m_frameWidth != frameW || m_frameHeight != frameH) {
        releaseScaler();
        m_frameWidth = frameW;
        m_frameHeight = frameH;

        m_swsContext = sws_getContext(frameW, frameH,
                                      m_codecContext->pix_fmt,
                                      frameW, frameH,
                                      AV_PIX_FMT_BGR24,
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

bool RtspStreamReceiverWorker::readFrame(cv::Mat& output)
{
    if (!m_formatContext || !m_codecContext || !m_packet || !m_frame) {
        return false;
    }

    while (true) {
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
}

void RtspStreamReceiverWorker::run()
{
    struct RunCleanup {
        RtspStreamReceiverWorker* self = nullptr;
        explicit RunCleanup(RtspStreamReceiverWorker* worker)
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

    bool connected = false;

    while (true) {
        bool running = false;
        QString currentUrl;
        bool reconnect = false;

        {
            QMutexLocker locker(&m_mutex);
            running = m_running;
            if (!running) {
                break;
            }
            currentUrl = m_url.trimmed();
            reconnect = m_reconnectRequested || !m_streamOpen;
            m_reconnectRequested = false;
        }

        if (m_abort.load()) {
            break;
        }

        if (currentUrl.isEmpty()) {
            if (connected) {
                cleanupFFmpeg();
                connected = false;
                emit connectionStatusChanged(false);
            }
            for (int i = 0; i < 1 && !m_abort.load(); ++i) {
                msleep(100);
            }
            continue;
        }

        if (reconnect) {
            cleanupFFmpeg();
            connected = false;

            if (!openStream(currentUrl)) {
                if (m_abort.load()) {
                    break;
                }
                emit connectionStatusChanged(false);
                emit errorOccurred(QStringLiteral("无法连接 RTSP: %1").arg(currentUrl));
                for (int i = 0; i < 20 && !m_abort.load(); ++i) {
                    msleep(100);
                }
                continue;
            }

            connected = true;
            emit connectionStatusChanged(true);
        }

        cv::Mat frame;
        if (!readFrame(frame)) {
            if (m_abort.load()) {
                break;
            }
            cleanupFFmpeg();
            connected = false;
            emit connectionStatusChanged(false);
            emit errorOccurred(QStringLiteral("RTSP 流中断: %1").arg(currentUrl));
            for (int i = 0; i < 10 && !m_abort.load(); ++i) {
                msleep(100);
            }
            continue;
        }

        if (!frame.empty()) {
            emit frameReceived(frame);
        }
    }

    if (connected) {
        emit connectionStatusChanged(false);
    }
}
