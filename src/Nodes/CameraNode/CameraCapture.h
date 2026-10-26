#pragma once

#include <QList>
#include <QMutex>
#include <QString>
#include <QThread>

#include <atomic>

#include <opencv2/core.hpp>

/** 摄像头枚举项：界面显示友好名，打开时用 openName（dshow video=） */
struct CameraDeviceInfo {
    QString displayName; ///< 可读名称，如 "USB2.0 HD UVC WebCam"
    QString openName;    ///< 传给 FFmpeg 的设备名（优先友好名，必要时为备用路径）
};

/**
 * @brief FFmpeg dshow 摄像头采集线程（项目自带 FFmpeg，不依赖 Qt Multimedia / OpenCV VideoCapture）
 *
 * 数据流：avformat(dshow) → avcodec → sws(BGR24) → frameAvailable(cv::Mat)
 */
class CameraCaptureThread : public QThread {
    Q_OBJECT

public:
    explicit CameraCaptureThread(QObject* parent = nullptr);
    ~CameraCaptureThread() override;

    /** 枚举本机 DirectShow 视频输入设备 */
    static QList<CameraDeviceInfo> listVideoDevices();

    void startCapture(const QString& deviceName);
    void stop();

signals:
    void frameAvailable(const cv::Mat& frame);
    void captureStateChanged(bool isCapturing);
    void errorOccurred(const QString& message);

protected:
    void run() override;

private:
    static int interruptCallback(void* opaque);
    bool shouldAbort() const;

    bool openDevice(const QString& deviceName);
    void cleanupFFmpeg();
    void releaseScaler();
    void convertToBgr(struct AVFrame* frame, cv::Mat& output);
    bool readFrame(cv::Mat& output);

    QMutex m_mutex;
    std::atomic<bool> m_running{false};
    std::atomic<bool> m_abort{false};
    QString m_deviceName;

    // 仅在采集线程内使用
    struct AVFormatContext* m_formatContext = nullptr;
    struct AVCodecContext* m_codecContext = nullptr;
    struct AVFrame* m_frame = nullptr;
    struct AVPacket* m_packet = nullptr;
    struct SwsContext* m_swsContext = nullptr;
    uint8_t* m_dstData[4] = {nullptr, nullptr, nullptr, nullptr};
    int m_dstLinesize[4] = {0, 0, 0, 0};
    int m_videoStreamIndex = -1;
    int m_frameWidth = 0;
    int m_frameHeight = 0;
};
