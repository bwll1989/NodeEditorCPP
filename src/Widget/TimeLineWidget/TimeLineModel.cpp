//
// Created by WuBin on 2025/6/3.
//

#include "TimeLineModel.h"
TimeLineModel::TimeLineModel(QObject* parent):m_stage(new TimeLineStage()),m_clock(new TimeLineClock(this))
{
    connect(m_clock,&TimeLineClock::currentFrameChanged,this,&TimeLineModel::onGetClipCurrentData);
}
TimeLineModel::~TimeLineModel()
{
    delete m_stage;
}

QJsonObject TimeLineModel::save() const
{
    auto res=BaseTimeLineModel::save();
    res["stage"]=m_stage->save();
    res["clock"]=m_clock->save();
    return res;
}

void TimeLineModel::load(const QJsonObject& json)
{
    BaseTimeLineModel::load(json);
    m_stage->load(json["stage"].toObject());
    m_clock->load(json["clock"].toObject());
}

void TimeLineModel::onStartPlay()
{

    getClock()->onStart();
}

void TimeLineModel::onStopPlay()
{
    getClock()->onStop();
}

void TimeLineModel::onPausePlay()
{
    getClock()->onPause();
}

void TimeLineModel::onSetPlayheadPos(int frame)
{
    getClock()->setCurrentFrame(frame);
}

qint64 TimeLineModel::getPlayheadPos() const
{
    return getClock()->getCurrentFrame();
}

qint64 TimeLineModel::onUpdateTimeLineLength()
{
    m_clock->setMaxFrames(BaseTimeLineModel::onUpdateTimeLineLength());
    return m_clock->getMaxFrames();
}

TimeCodeType TimeLineModel::getTimeCodeType() const
{
    return getClock()->getTimecodeType();
}

QList<QVariantMap> TimeLineModel::onGetClipCurrentData(qint64 currentFrame) const {
    QList<QVariantMap> clipDataList;
    // 遍历所有轨道
    for (TrackData* track : getTracks()) {
        // 遍历轨道中的所有片段
        for (AbstractClipModel* clip : track->clips) {
            // 检查当前帧是否在片段范围内
            if (currentFrame >= clip->start() && currentFrame <= clip->end()) {
                // 获取片段在当前帧的数据
                QVariantMap data = clip->currentData(currentFrame);
                if(!data.isEmpty()) {
                    clipDataList.append(data);
                }
            }
        }
    }
    return clipDataList;
}