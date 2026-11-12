#ifndef VIDEORECORDER_H
#define VIDEORECORDER_H

#include <QString>
#include <QOpenGLWidget>
#include <QImage>
#include <QMutex>
#include <QThread>
#include <memory>
#include "Defines.h"
extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/opt.h>
#include <libavutil/imgutils.h>
#include <libswscale/swscale.h>
}
#include <QElapsedTimer>  // 添加头文件

/**
 * @brief 视频录制器类
 * 使用FFmpeg将OpenGL渲染的图像录制为视频文件
 */
class VideoRecorder : public QObject
{
    Q_OBJECT

public:
    explicit VideoRecorder(QObject *parent = nullptr);
    ~VideoRecorder();

    /**
     * @brief 开始录制视频
     * @param filename 输出视频文件名
     * @param width 视频宽度
     * @param height 视频高度
     * @param fps 帧率（默认25fps）
     * @return 是否成功开始录制
     */
    bool startRecording(const QString &filename, int width, int height);

    /**
     * @brief 停止录制视频
     */
    void stopRecording();

    /**
     * @brief 添加一帧图像到视频
     * @param image 要录制的图像
     */
    void addFrame(const QImage &image);

    /**
     * @brief 检查是否正在录制
     * @return 是否正在录制
     */
    bool isRecording() const { return m_isRecording; }
    /**
        * @brief 获取录制帧数
        * @return 当前录制的帧数
        */
    int getFrameCount() const { return m_frameCount; }
    /**
 * @brief 获取录制时间（毫秒）
 * @return 录制时间，如果未录制则返回0
 */
    qint64 getRecordingFrames() const;
signals:
    /**
     * @brief 录制开始信号
     */
    void recordingStarted();

    /**
     * @brief 录制停止信号
     */
    void recordingStopped();

    /**
     * @brief 录制错误信号
     * @param error 错误信息
     */
    void recordingError(const QString &error);


    


private:
    /**
     * @brief 初始化FFmpeg编码器
     * @param filename 输出文件名
     * @param width 视频宽度
     * @param height 视频高度
     * @param fps 帧率
     * @return 是否初始化成功
     */
    bool initializeEncoder(const QString &filename, int width, int height, int fps);

    /**
     * @brief 清理FFmpeg资源
     */
    void cleanup();

    /**
     * @brief 编码一帧图像
     * @param image 要编码的图像
     * @return 是否编码成功
     */
    bool encodeFrame(const QImage &image);

private:
    bool m_isRecording;
    int m_frameCount;
    // FFmpeg相关变量
    AVFormatContext *m_formatContext;
    AVCodecContext *m_codecContext;
    AVStream *m_videoStream;
    AVFrame *m_frame;
    AVPacket *m_packet;
    SwsContext *m_swsContext;
    
    QMutex m_mutex;
};

#endif // VIDEORECORDER_H