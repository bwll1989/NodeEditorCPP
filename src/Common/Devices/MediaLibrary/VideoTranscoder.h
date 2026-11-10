#pragma once
#include <QLoggingCategory>
#include <QObject>
#include <QQueue>
#include <QProcess>
#include <QString>
#include <QStringList>

Q_DECLARE_LOGGING_CATEGORY(logTranscoder)

/**
 * @class VideoTranscoder
 * @brief 视频自动转码器：将导入的视频统一转码为 HAP（Hap Q）无损编码的 .mov 文件。
 *
 * - 通过 QProcess 调用 ffmpeg.exe / ffprobe.exe（与 FFmpeg DLL 同目录或 appDir 下查找）
 * - 串行队列：同一时刻只运行一个转码进程，UI 线程无阻塞
 * - 探测阶段异步执行 ffprobe：判断源是否已是 HAP（是则跳过），并获取时长用于进度百分比
 * - 音频优先 stream copy；若 mov 容器不接受源音频编码导致失败，自动回退为 AAC 重编一次
 * - 视频统一加 scale 滤镜取整到 4 的倍数（DXT 纹理约束），已是 4 倍数时为无操作
 * - Hap Q 使用 snappy 无损压缩（FFmpeg 8 默认开启，无需额外参数）
 */
class VideoTranscoder : public QObject
{
    Q_OBJECT
public:
    static VideoTranscoder* instance() {
        static VideoTranscoder* s = nullptr;
        if (!s) s = new VideoTranscoder();
        return s;
    }

    explicit VideoTranscoder(QObject* parent = nullptr);
    ~VideoTranscoder() override;

    /**
     * @brief 将一个视频文件加入转码队列（队列内去重）
     * @return true 已入队；false 参数无效或已在队列中
     */
    bool enqueue(const QString& absPath);

    /**
     * @brief 根据源文件推导转码输出路径（与源同目录）
     *        扩展名非 mov 时为 <base>.mov；扩展名为 mov 时为 <base>_HAP.mov 临时名
     *        （转码成功删除原文件后会改名为最终名 finalPathFor，保持原始文件名）
     */
    static QString outputPathFor(const QString& srcPath);

    /**
     * @brief 转码产物的最终文件名（<base>.mov，与原始文件名保持一致，仅扩展名变 mov）
     */
    static QString finalPathFor(const QString& srcPath);

    /**
     * @brief 定位 ffmpeg.exe；返回空串表示未找到
     */
    static QString ffmpegExecutable();

    /**
     * @brief 定位 ffprobe.exe（与 ffmpeg.exe 同目录）；返回空串表示未找到
     */
    static QString ffprobeExecutable();

signals:
    /** 转码开始（srcPath 为源文件绝对路径） */
    void transcodeStarted(const QString& srcPath);
    /** 进度更新，percent 为 0-100；duration 未知时为 -1 */
    void transcodeProgress(const QString& srcPath, int percent);
    /** 单个任务结束（含跳过：success=true 且 outPath 为空表示源已是 HAP 跳过） */
    void transcodeFinished(const QString& srcPath, const QString& outPath, bool success, const QString& message);

private:
    struct Job {
        QString src;
        QString out;
    };

    enum class Phase { Probe, Transcode };

    void startNext();
    void startProbe(const Job& job);
    void startTranscode(const Job& job, bool audioCopy);
    /**
     * @brief 结束当前任务并启动下一个
     *        success=true 且 outPath 为空：跳过场景（源已是 HAP），不做任何文件操作
     *        success=true 且 outPath 非空：删除原始文件，并将产物改名为最终名（保持原始文件名）
     *        success=false：清理半成品输出文件，outPath 忽略
     */
    void finishCurrent(bool success, const QString& outPath, const QString& message);
    void cleanupProcess();

private:
    QQueue<Job> m_queue;
    QProcess* m_process = nullptr;
    Phase m_phase = Phase::Probe;
    Job m_current;
    bool m_audioCopyAttempted = false;  // 当前任务是否已尝试过音频 copy
    double m_durationSec = 0.0;         // ffprobe 探测到的时长（秒），<=0 表示未知
    int m_lastPercent = -1;
    QByteArray m_progressBuf;           // ffmpeg -progress 的 stdout 行缓冲
};
