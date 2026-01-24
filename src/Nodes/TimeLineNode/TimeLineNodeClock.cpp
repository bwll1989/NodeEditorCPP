#include "TimeLineNodeClock.hpp"
#include <QThread>
#include "TimeCodeDefines.h"
TimeLineNodeClock::TimeLineNodeClock(QObject* parent)
    : QObject(parent)
    , m_currentFrame(0)
    , m_maxFrames(0)
    , m_isLooping(false)
    , m_timecodeType(TimeCodeType::PAL)
    , m_clockSource(ClockSource::Internal)
{
    m_frameRate = timecode_frames_per_sec(m_timecodeType);
    initInternalClock();
}

TimeLineNodeClock::~TimeLineNodeClock()
{
    // 确保定时器停止
    if (m_timer) {
        // 使用 invokeMethod 安全地停止定时器
        QMetaObject::invokeMethod(m_timer, "stop", Qt::BlockingQueuedConnection);

        // 获取定时器所在线程
        QThread* timerThread = m_timer->thread();

        // 如果定时器不在主线程
        if (timerThread && timerThread != QThread::currentThread()) {
            // 断开所有连接，防止信号干扰
            m_timer->disconnect();
            disconnect(m_timer, nullptr, this, nullptr);

            // 安全停止线程
            timerThread->quit();
            if (!timerThread->wait(1000)) { // 等待1秒线程退出
                timerThread->terminate(); // 强制终止
                timerThread->wait(); // 等待终止完成
            }
        }

        // 安全删除定时器
        m_timer->deleteLater();
        m_timer = nullptr;
    }
}

void TimeLineNodeClock::initInternalClock()
{
    m_timer = new NodeTimeSync();
    double fps = timecode_frames_per_sec(m_timecodeType);
    if (fps > 0.0) {
        m_timer->setTickInterval(1.0 / fps);
    }
    QThread* timerThread = new QThread(this);
    timerThread->start(QThread::HighPriority);
    connect(m_timer, &NodeTimeSync::timeUpdated, this, &TimeLineNodeClock::onTick, Qt::DirectConnection);

    // 连接控制信号到 m_timer，确保跨线程调用安全
    connect(this, &TimeLineNodeClock::resumePlay, m_timer, &NodeTimeSync::resume);
    connect(this, &TimeLineNodeClock::pausePlay, m_timer, &NodeTimeSync::pause);
    connect(this, &TimeLineNodeClock::stopPlay, m_timer, &NodeTimeSync::stop);

    connect(timerThread, &QThread::finished, timerThread, &QObject::deleteLater);
    m_timer->moveToThread(timerThread);
}

void TimeLineNodeClock::closeInternalClock()
{
    if(m_timer) {
        m_timer->stop();
        delete m_timer;
        m_timer = nullptr;
    }
}


void TimeLineNodeClock::setCurrentFrame(qint64 frame)
{
    m_currentFrame = frame;
    m_currentTimecode = frames_to_timecode_frame(m_currentFrame, m_timecodeType);
    m_frameAccumulator = 0.0;
    updateTimecode();
}

void TimeLineNodeClock::setCurrentTimecode(const TimeCodeFrame& timecode)
{
    m_currentTimecode = timecode;
    m_currentFrame = timecode_frame_to_frames(timecode, m_timecodeType);
    updateTimecode();
}

void TimeLineNodeClock::onTick(double)
{
    if (m_clockSource != ClockSource::Internal) {
        return;
    }

    m_currentFrame++;
    if (m_maxFrames > 0 && m_currentFrame > m_maxFrames) {
        if (m_isLooping) {
            m_currentFrame = 0;
        } else {
            onStop();
            return;
        }
    }
    m_currentTimecode = frames_to_timecode_frame(m_currentFrame, m_timecodeType);
    updateTimecode();
}

void TimeLineNodeClock::setCurrentTimecodeFromTime(const double time)
{
    if (m_clockSource != ClockSource::Internal) {
        return;
    }
    if (time <= 0.0) {
        return;
    }
    double fps = timecode_frames_per_sec(m_timecodeType);
    if (fps <= 0.0) {
        return;
    }
    m_frameAccumulator += time * fps;
    qint64 step = static_cast<qint64>(m_frameAccumulator);
    if (step <= 0) {
        return;
    }
    m_frameAccumulator -= static_cast<double>(step);
    m_currentFrame += step;
    m_currentTimecode = frames_to_timecode_frame(m_currentFrame, m_timecodeType);
    if (m_maxFrames > 0 && m_currentFrame > m_maxFrames) {
        if (m_isLooping) {
            m_currentFrame = 0;
            m_currentTimecode = frames_to_timecode_frame(m_currentFrame, m_timecodeType);
            m_frameAccumulator = 0.0;
            updateTimecode();
            return;
        } else {
            onStop();
            return;
        }
    }

    updateTimecode();
}

double TimeLineNodeClock::getFrameRate() const
{
    return timecode_frames_per_sec(m_timecodeType);
}

void TimeLineNodeClock::setTimecodeType(TimeCodeType type)
{
   
    if (m_timecodeType != type) {
        // 判断时间码变换时，转换当前时间码到新的时间码类型
        m_timecodeType = type;
        m_frameRate = timecode_frames_per_sec(m_timecodeType);
        if (m_timer && m_frameRate > 0.0) {
            QMetaObject::invokeMethod(m_timer, "setTickInterval", Qt::QueuedConnection, Q_ARG(double, 1.0 / m_frameRate));
        }
        updateTimecode();
    }
    
}

void TimeLineNodeClock::setLooping(bool loop)
{
    m_isLooping = loop;
}

void TimeLineNodeClock::setMaxFrames(qint64 maxFrames)
{
    m_maxFrames = maxFrames;
}

void TimeLineNodeClock::updateTimecode()
{
    const qint64 frame = m_currentFrame;            // 把当前帧号拷贝出来
    const TimeCodeFrame tc = m_currentTimecode;     // 把当前时间码拷贝出来
    QMetaObject::invokeMethod(this, [this, frame, tc]() {
        emit currentFrameChanged(frame);
        emit timecodeChanged(tc);
    }, Qt::QueuedConnection);
}

TimeCodeFrame TimeLineNodeClock::getCurrentTimecode() const
{
    return m_currentTimecode;
} 

qint64 TimeLineNodeClock::getCurrentFrame() const
{
    return m_currentFrame;
}

void TimeLineNodeClock::onStart()
{

    if(m_clockSource != ClockSource::Internal || m_maxFrames <= 0)
    {
        emit timecodePlayingChanged(false);
        return;
    }
    emit resumePlay();
    emit timecodePlayingChanged(true);
  
    
}

void TimeLineNodeClock::onPause()
{
    if(m_clockSource != ClockSource::Internal)
    {
        return;
    }
    // QMutexLocker locker(&m_mutex);
    emit pausePlay();
    // emit timecodePlayingChanged(false);
    
}

void TimeLineNodeClock::onStop()
{   
    if(m_clockSource != ClockSource::Internal)
    {
        return;
    }
    emit stopPlay();
    emit timecodePlayingChanged(false);
    m_currentFrame = 0;
    m_currentTimecode = frames_to_timecode_frame(m_currentFrame, m_timecodeType);
    updateTimecode();
}

void TimeLineNodeClock::moveToNextFrame()
{
//    QMutexLocker locker(&m_mutex);
//    m_currentFrame++;
//    updateTimecode();
}

void TimeLineNodeClock::moveToPreviousFrame()
{
//    QMutexLocker locker(&m_mutex);
//    m_currentFrame--;
//    if (m_currentFrame < 0) {
//        m_currentFrame = 0;
//    }
//    updateTimecode();
}

qint64 TimeLineNodeClock::getMaxFrames() const
{
    return m_maxFrames;
}   

bool TimeLineNodeClock::isLooping() const
{
    return m_isLooping;
}

TimeCodeType TimeLineNodeClock::getTimecodeType() const
{
    return m_timecodeType;
}

QString TimeLineNodeClock::getCurrentAbsoluteTime() const
{
    // 计算当前帧对应的总毫秒数
    double frameTimeInMs = (static_cast<double>(m_currentFrame) / timecode_frames_per_sec(m_timecodeType)) * 1000.0;
    
    // 计算小时、分钟、秒和毫秒
    int hours = static_cast<int>(frameTimeInMs / (1000 * 60 * 60));
    frameTimeInMs -= hours * (1000 * 60 * 60);
    
    int minutes = static_cast<int>(frameTimeInMs / (1000 * 60));
    frameTimeInMs -= minutes * (1000 * 60);
    
    int seconds = static_cast<int>(frameTimeInMs / 1000);
    frameTimeInMs -= seconds * 1000;
    
    int milliseconds = static_cast<int>(frameTimeInMs);
    
    // 格式化为 HH:MM:SS:MS
    return QString("%1:%2:%3:%4")
        .arg(hours, 2, 10, QChar('0'))
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'))
        .arg(milliseconds, 3, 10, QChar('0'));
}

void TimeLineNodeClock::closeCurrentClockSource()
{
    // 根据当前时钟源类型进行清理
    switch (m_clockSource) {
        case ClockSource::Internal:
            closeInternalClock();
            break;
        case ClockSource::LTC:
            break;
        case ClockSource::MTC:
            // 如果有 MTC 相关清理代码，在这里添加
            break;
    }
}

ClockSource TimeLineNodeClock::getClockSource() const
{ 
    return m_clockSource;
}

QJsonObject TimeLineNodeClock::save()
{
    QJsonObject json;

    // 保存基本设置
    json["clockSource"] = static_cast<int>(m_clockSource);
    json["timecodeType"] = static_cast<int>(m_timecodeType);
    json["isLooping"] = m_isLooping.loadRelaxed();
    return json;
}

void TimeLineNodeClock::load(const QJsonObject& json)
{
    // 停止当前播放
    // 加载基本设置

    if (json.contains("timecodeType")) {
        setTimecodeType(static_cast<TimeCodeType>(json["timecodeType"].toInt()));
        qDebug()<<"timecodeType"<<json["timecodeType"].toInt();
    }
    
    // 加载时钟源设置
    if (json.contains("clockSource")) {
        ClockSource source = static_cast<ClockSource>(json["clockSource"].toInt());
        
        switch (source) {
            case ClockSource::Internal:
                if(m_clockSource != ClockSource::Internal)
                {
                    closeCurrentClockSource();
                    initInternalClock();
                    m_clockSource = source;
                }
                break;
                
            case ClockSource::LTC:
                break;
            case ClockSource::MTC:
                break;
            default:
                break;
        }
    }
    // 加载循环设置
    if (json.contains("isLooping")) {
        setLooping(json["isLooping"].toBool());
        emit loopingChanged(m_isLooping.loadRelaxed());
    }
}

void TimeLineNodeClock::onLoop(bool loop)
{
    m_isLooping.storeRelaxed(loop);
}
