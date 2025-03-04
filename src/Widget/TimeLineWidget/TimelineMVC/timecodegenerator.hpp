#ifndef TIMECODEGENERATOR_HPP
#define TIMECODEGENERATOR_HPP

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QAtomicInteger>
#include "Widget/TimeLineWidget/timelinetypes.h"

class TimecodeGenerator : public QObject {
    Q_OBJECT
public:
    explicit TimecodeGenerator(QObject* parent = nullptr);
    ~TimecodeGenerator() override;

    // 启动生成器
    void start();
    // 停止生成器
    void stop();
    // 暂停生成器
    void pause();
    // 设置当前帧
    void setCurrentFrame(qint64 frame);
    // 获取当前帧
    qint64 getCurrentFrame() const;
    // 设置帧率
    void setFrameRate(double fps);
    // 获取帧率
    double getFrameRate() const;
    // 设置时间码类型
    void setTimecodeType(TimecodeType type);
    // 获取时间码类型
    TimecodeType getTimecodeType() const;
    // 设置是否循环
    void setLooping(bool loop);
    // 获取是否循环
    bool isLooping() const;
    // 设置最大帧数
    void setMaxFrames(qint64 maxFrames);
    // 获取最大帧数
    qint64 getMaxFrames() const;
    /**
     * 获取当前时间码
     * @return QString 当前时间码
     */
    QString getCurrentTimecode() const;
    // 移动到下一帧
    void moveToNextFrame();
    // 移动到上一帧
    void moveToPreviousFrame();
    // 获取当前帧的绝对时间
    QString getCurrentAbsoluteTime() const;
signals:
    /**
     * 当前帧改变
     * @param qint64 frame 当前帧
     */
    void currentFrameChanged(qint64 frame);
    /**
     * 时间码改变
     * @param QString timecode 时间码
     */
    void timecodeChanged(const QString& timecode);
    /**
     * 播放完成信号
     */
    void timecodeFinished();
    /**
     * 播放状态改变
     * @param bool isPlaying 是否播放
     */
    void timecodePlayingChanged(bool isPlaying);
    
public slots:
    /**
     * 定时器超时
     */
    void onTimeout();
    /**
     * 开始
     */
    void onStart();
    /**
     * 暂停
     */
    void onPause();
    /**
     * 停止
     */
    void onStop();

private:
    /**
     * 更新时间码
     */
    void updateTimecode();
private:
    /**
     * 定时器
     */
    QTimer* m_timer;
    /**
     * 互斥锁
     */
    QMutex m_mutex;
    /**
     * 当前帧
     */
    QAtomicInteger<qint64> m_currentFrame;
    /**
     * 最大帧数
     */
    QAtomicInteger<qint64> m_maxFrames;
    /**
     * 帧率
     */
    double m_frameRate;
    /**
     * 是否循环
     */
    QAtomicInteger<bool> m_isLooping;
    /**
     * 时间码类型
     */
    TimecodeType m_timecodeType;
    /**
     * 是否暂停
     */
    bool m_isPaused;
    // 添加时间戳相关成员
    qint64 m_startTime = 0;        // 开始播放的时间戳
    qint64 m_lastFrameTime = 0;    // 上一帧的时间戳
    double m_frameDuration = 0;    // 每帧持续时间(ms)
    // 当前时间码
    Timecode m_currentTimecode;
};

#endif // TIMECODEGENERATOR_HPP 