#pragma once


#include <QObject>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <chrono>

class NodeTimeSync : public QObject {
    Q_OBJECT
public:
    NodeTimeSync(QObject *parent = nullptr);
    ~NodeTimeSync();
    bool isPause() const{
        return isPaused;
    }

public slots:
    /**
     * @brief 设置时钟的 tick 间隔时间
     * @param intervalSec tick 间隔时间（秒）
     */
    void setTickInterval(double intervalSec);
    
    void start();
    void stop();
    void pause();
    void resume();
    void setSpeed(double newSpeed);
    
signals:
    void timeUpdated(double time);
private slots:
    void broadcastTime();
private:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    QTimer *broadcastTimer;
    double speed;
    bool isPaused;
    double tickIntervalSec;

    TimePoint m_lastTickTime;
    double m_errorAccumulator;
};
