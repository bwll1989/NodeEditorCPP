#ifndef TIMECODEGENERATOR_HPP
#define TIMECODEGENERATOR_HPP

#include <QObject>
#include <QTimer>
#include <QMutex>
#include <QAtomicInteger>
#include "TimeLineDefines.h"
#include "TimeCodeMessage.h"
#include "../../Common/Devices/LtcReceiver/ltcreceiver.h"
#include <QJsonObject>
#include "TimeSyncServer.hpp"
class TimeLineClock : public QObject {
    Q_OBJECT
public:
    explicit TimeLineClock(QObject* parent = nullptr);
    ~TimeLineClock() override;
    // 设置当前帧
    void setCurrentFrame(qint64 frame);
    // 获取当前帧
    qint64 getCurrentFrame() const;
    // 从时间码设置当前时间码
    void setCurrentTimecode(const TimeCodeFrame& timecode);
    // 从时间设置当前时间码
    void setCurrentTimecodeFromTime(const double time);
    // 获取帧率
    double getFrameRate() const;
    // 设置时间码类型
    void setTimecodeType(TimeCodeType type);
    // 获取时间码类型
    TimeCodeType getTimecodeType() const;
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
    TimeCodeFrame getCurrentTimecode() const;
    // 移动到下一帧
    void moveToNextFrame();
    // 移动到上一帧
    void moveToPreviousFrame();
    // 获取当前帧的绝对时间
    QString getCurrentAbsoluteTime() const;
    // 设置时钟源
    void closeCurrentClockSource();
    // 获取时钟源
    ClockSource getClockSource() const;

    void initInternalClock();

    void closeInternalClock();
    
    void initLTCClock(QString device,int channelIndex);

    void closeLTCClock();

    QJsonObject  save();

    void load(const QJsonObject& json);
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
    void timecodeChanged(const TimeCodeFrame& timecode);
    /**
     * 播放完成信号
     */
    void timecodeFinished();
    /**
     * 播放状态改变
     * @param bool isPlaying 是否播放
     */
    void timecodePlayingChanged(bool isPlaying);
    
    void startPlay();

    void pausePlay();

    void stopPlay();

    void resumePlay();

public slots:
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

    void onLoop(bool loop);
private:
    /**
     * 更新时间码
     */
    void updateTimecode();
private:
    /**
     * 定时器
     */
    TimeSyncServer* m_timer;
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
    TimeCodeType m_timecodeType;
    // 当前时间码
    TimeCodeFrame m_currentTimecode;
    /**
     * 时钟源
     */
    ClockSource m_clockSource;
    /**
     * LTC接收器
     */
    LTCReceiver* m_ltcReceiver {nullptr};
};

#endif // TIMECODEGENERATOR_HPP 