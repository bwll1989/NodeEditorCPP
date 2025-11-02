#pragma once


#include <QObject>
#include <QTimer>
#include <QJsonObject>
#include <QJsonDocument>
#include <chrono>

class NodeTimeSync : public QObject {
    Q_OBJECT

private:
    using Clock = std::chrono::high_resolution_clock;
    using TimePoint = std::chrono::time_point<Clock>;

    QTimer *broadcastTimer;    // 广播定时器
    TimePoint startTime;       // 开始时间点
    double currentTime;        // 当前时间
    double speed;             // 播放速度
    bool isPaused;            // 是否暂停

public:
    NodeTimeSync(QObject *parent = nullptr);
    ~NodeTimeSync();
    double getTime() const;
    bool isPause() const{
        return isPaused;
    }
    
signals:
    void timeUpdated(double time);
public slots:
    void start();
    void stop();
    void pause();
    void resume();
    void setSpeed(double newSpeed);
    void setCurrentTime(double time);
    
private slots:
    void broadcastTime();
};

