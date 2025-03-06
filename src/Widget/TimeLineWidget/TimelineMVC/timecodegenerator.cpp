#include "timecodegenerator.hpp"
#include <QThread>
TimecodeGenerator::TimecodeGenerator(QObject* parent)
    : QObject(parent)
    , m_currentFrame(0)
    , m_maxFrames(0)
    , m_isLooping(false)
    , m_timecodeType(TimecodeType::PAL)
    , m_isPaused(false)
{
    // 创建定时器并保持在主线程
    m_timer = new QTimer(this);
    m_frameRate = Timecode::getFrameRate(m_timecodeType);
    m_frameDuration = 1000.0 / m_frameRate;  // 计算每帧持续时间
    
    // 使用高精度计时器
    m_timer->setTimerType(Qt::PreciseTimer);
    // 设置较短的间隔以提高精度
    m_timer->setInterval(qRound(m_frameDuration / 2));
    
    // 设置定时器优先级
    m_timer->setTimerType(Qt::PreciseTimer);
    
    // 将定时器移动到高优先级线程
    QThread* timerThread = new QThread(this);
    timerThread->start(QThread::HighPriority);
    m_timer->moveToThread(timerThread);
    
    // 使用直接连接确保及时处理定时器事件
    connect(m_timer, &QTimer::timeout, this, &TimecodeGenerator::onTimeout, 
            Qt::DirectConnection);
}

TimecodeGenerator::~TimecodeGenerator()
{
    m_timer->stop();
    delete m_timer;
}

void TimecodeGenerator::start()
{
    QMutexLocker locker(&m_mutex);
    if (!m_timer->isActive() && !m_isPaused) {
        m_timer->start(1000 / m_frameRate);
    }
}

void TimecodeGenerator::stop()
{
    QMutexLocker locker(&m_mutex);
    m_timer->stop();
    m_currentFrame = 0;
    m_isPaused = false;
    updateTimecode();
}

void TimecodeGenerator::pause()
{
    QMutexLocker locker(&m_mutex);
    if (m_timer->isActive()) {
        m_timer->stop();
        m_isPaused = true;
    }
}


void TimecodeGenerator::setCurrentFrame(qint64 frame)
{
    m_currentFrame = frame;  // QAtomicInteger 是线程安全的，不需要互斥锁
    updateTimecode();
}

void TimecodeGenerator::setFrameRate(double fps)
{
    QMutexLocker locker(&m_mutex);
    if (m_frameRate != fps) {
        bool wasActive = m_timer->isActive();
        if (wasActive) {
            m_timer->stop();
        }
        
        m_frameRate = fps;
        m_frameDuration = 1000.0 / fps;
        m_timer->setInterval(qRound(m_frameDuration / 2));
        
        if (wasActive) {
            m_startTime = QDateTime::currentMSecsSinceEpoch();
            m_lastFrameTime = m_startTime;
            m_timer->start();
        }
        
        updateTimecode();
    }
}

void TimecodeGenerator::setTimecodeType(TimecodeType type)
{
   
    if (m_timecodeType != type) {
        m_timecodeType = type;
        setFrameRate(Timecode::getFrameRate(type));
    }
}

void TimecodeGenerator::setLooping(bool loop)
{
    m_isLooping = loop;
}

void TimecodeGenerator::setMaxFrames(qint64 maxFrames)
{
    m_maxFrames = maxFrames;
}

void TimecodeGenerator::onTimeout()
{
    QMutexLocker locker(&m_mutex);
    
    // 使用高精度时间戳
    qint64 currentTime = QDateTime::currentMSecsSinceEpoch();
    
    // 如果是第一次调用，初始化时间戳
    if (m_lastFrameTime == 0) {
        m_startTime = currentTime;
        m_lastFrameTime = currentTime;
        return;
    }
    
    qint64 elapsedTime = currentTime - m_lastFrameTime;
    
    // 计算理论上应该前进的帧数
    double theoreticalFrames = elapsedTime / m_frameDuration;
    
    // 使用累积误差来提高精度
    static double accumulatedError = 0.0;
    theoreticalFrames += accumulatedError;
    
    // 计算实际应该前进的帧数
    int framesToAdvance = static_cast<int>(theoreticalFrames);
    accumulatedError = theoreticalFrames - framesToAdvance;
    
    if (framesToAdvance > 0) {
        qint64 nextFrame = m_currentFrame + framesToAdvance;
        
        // 处理循环和结束条件
        if (nextFrame >= m_maxFrames) {
            if (m_isLooping) {
                nextFrame = nextFrame % m_maxFrames;
            } else {
                nextFrame = 0;
                m_timer->stop();
                m_isPaused = false;
                emit timecodePlayingChanged(false);
            }
        }
        
        // 更新时间戳和帧位置
        m_lastFrameTime = currentTime;
        setCurrentFrame(nextFrame);
    }
}

void TimecodeGenerator::updateTimecode()
{
    // 使用队列连接发送信号，避免阻塞定时器线程
    QMetaObject::invokeMethod(this, [this]() {
        emit currentFrameChanged(m_currentFrame);
        emit timecodeChanged(getCurrentTimecode());
    }, Qt::QueuedConnection);
}

QString TimecodeGenerator::getCurrentTimecode() const
{
    if (m_timecodeType == TimecodeType::NTSC_DF || 
        m_timecodeType == TimecodeType::HD_60_DF) {
        return Timecode::fromFramesDF(m_currentFrame, 
            Timecode::getFrameRate(m_timecodeType)).toStringDF();
    }
    return Timecode::fromFrames(m_currentFrame, m_frameRate).toString();
} 

qint64 TimecodeGenerator::getCurrentFrame() const
{
    return m_currentFrame;
}

void TimecodeGenerator::onStart()
{
    QMutexLocker locker(&m_mutex);
    if (!m_timer->isActive() && m_maxFrames > 0) {
        m_startTime = QDateTime::currentMSecsSinceEpoch();
        m_lastFrameTime = m_startTime;
        m_timer->start();
        emit timecodePlayingChanged(true);
        return;
    }
    emit timecodePlayingChanged(false);
}

void TimecodeGenerator::onPause()
{
    QMutexLocker locker(&m_mutex);
    if (m_timer->isActive()) {
        m_timer->stop();
        emit timecodePlayingChanged(false);
    }
}

void TimecodeGenerator::onStop()
{   
    QMutexLocker locker(&m_mutex);
    m_timer->stop();
    m_currentFrame = 0;
    updateTimecode();
    emit timecodePlayingChanged(false);
}

void TimecodeGenerator::moveToNextFrame()
{
    QMutexLocker locker(&m_mutex);
    m_currentFrame++;
    updateTimecode();
}

void TimecodeGenerator::moveToPreviousFrame()
{
    QMutexLocker locker(&m_mutex);
    m_currentFrame--;
    if (m_currentFrame < 0) {
        m_currentFrame = 0;
    }
    updateTimecode();
}
qint64 TimecodeGenerator::getMaxFrames() const
{
    return m_maxFrames;
}   

bool TimecodeGenerator::isLooping() const
{
    return m_isLooping;
}

double TimecodeGenerator::getFrameRate() const
{
    return m_frameRate;
}

TimecodeType TimecodeGenerator::getTimecodeType() const
{
    return m_timecodeType;
}


QString TimecodeGenerator::getCurrentAbsoluteTime() const
{
    // 计算当前帧对应的总毫秒数
    double frameTimeInMs = (static_cast<double>(m_currentFrame) / m_frameRate) * 1000.0;
    
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

