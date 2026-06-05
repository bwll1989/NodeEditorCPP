#pragma once

#include <QObject>
#include <QThread>
#include <QMutex>
#include <QString>
#include <atomic>
#include <opencv2/opencv.hpp>

struct AVFrame;

class RtspStreamReceiverWorker;

class RtspStreamReceiver : public QObject
{
    Q_OBJECT

public:
    explicit RtspStreamReceiver(QObject* parent = nullptr);
    ~RtspStreamReceiver() override;

    void start(const QString& url);
    void stop();
    void setUrl(const QString& url);

signals:
    void frameReceived(cv::Mat frame);
    void connectionStatusChanged(bool connected);
    void errorOccurred(const QString& message);

private:
    RtspStreamReceiverWorker* m_worker = nullptr;
};

class RtspStreamReceiverWorker : public QThread
{
    Q_OBJECT

public:
    explicit RtspStreamReceiverWorker(QObject* parent = nullptr);
    ~RtspStreamReceiverWorker() override;

    void startReceiving(const QString& url);
    void stopReceiving();
    void setUrl(const QString& url);
    bool shouldAbort() const;

signals:
    void frameReceived(cv::Mat frame);
    void connectionStatusChanged(bool connected);
    void errorOccurred(const QString& message);

protected:
    void run() override;

private:
    bool openStream(const QString& url);
    bool readFrame(cv::Mat& output);
    void convertToBgr(AVFrame* frame, cv::Mat& output);
    void cleanupFFmpeg();
    void releaseScaler();

    QMutex m_mutex;
    std::atomic<bool> m_abort{false};
    bool m_running = false;
    bool m_streamOpen = false;
    bool m_reconnectRequested = false;
    QString m_url;

    struct AVFormatContext* m_formatContext = nullptr;
    struct AVCodecContext* m_codecContext = nullptr;
    struct SwsContext* m_swsContext = nullptr;
    struct AVFrame* m_frame = nullptr;
    struct AVPacket* m_packet = nullptr;
    uint8_t* m_dstData[4] = {nullptr, nullptr, nullptr, nullptr};
    int m_dstLinesize[4] = {0, 0, 0, 0};
    int m_videoStreamIndex = -1;
    int m_frameWidth = 0;
    int m_frameHeight = 0;
};
