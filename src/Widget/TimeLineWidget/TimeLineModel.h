//
// Created by WuBin on 2025/6/3.
//

#ifndef TIMELINEMODEL_H
#define TIMELINEMODEL_H
#include "BaseTimeLineModel.h"
#include  "./TimelineStageWidget/TimeLineStage.hpp"
#include  "TimeCodeMessage.h"
#include "./TimeLineClock/TimeLineClock.hpp"
class TimeLineModel : public BaseTimeLineModel {
    Q_OBJECT

public:
    explicit TimeLineModel(QObject* parent = nullptr);
    ~TimeLineModel() override;
    /**
     * 获取舞台对象
     * @return TimelineStage* 舞台对象
     */
    TimeLineStage* getStage() const { return m_stage; }

    QJsonObject save() const override;

    void load(const QJsonObject &modelJson)  override;

    TimeLineClock* getClock() const { return m_clock; }

    qint64 getPlayheadPos() const override;

    TimeCodeType getTimeCodeType() const;
public slots:
    //开始播放槽函数
    void onStartPlay() override;
    //暂停播放槽函数
    void onPausePlay() override;
    //停止播放槽函数
    void onStopPlay() override;
    //设置播放位置槽函数
    void onSetPlayheadPos(int newPlayheadPos) override;
    //时间轴长度变化槽函数
    qint64 onUpdateTimeLineLength() override;
private:
    // 舞台对象
    TimeLineStage* m_stage;
    //时钟对象
    TimeLineClock* m_clock;
};



#endif //TIMELINEMODEL_H
