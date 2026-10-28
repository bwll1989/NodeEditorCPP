//
// Created by WuBin on 2025/6/3.
//

#include "TimeLineNodeModel.h"
TimeLineNodeModel::TimeLineNodeModel(QObject* parent)
    : BaseTimeLineModel(parent)
    , m_clock(new TimeLineNodeClock(this))
{
    connect(m_clock, &TimeLineNodeClock::currentFrameChanged, this, &TimeLineNodeModel::onGetClipCurrentData);
    connect(m_clock, &TimeLineNodeClock::currentFrameChanged, this, [this](qint64 frame) {
        emit S_playheadMoved(static_cast<int>(frame));
    });
    // 变长只发 S_LengthChanged，不走 onUpdateTimeLineLength。时钟总长在这里跟上。
    connect(this, &BaseTimeLineModel::S_LengthChanged, this, [this](qint64 length) {
        m_clock->setMaxFrames(length);
    });
}

QJsonObject TimeLineNodeModel::save() const
{
    auto res=BaseTimeLineModel::save();

    res["clock"]=m_clock->save();
    return res;
}
void TimeLineNodeModel::load(const QJsonObject& json)
{
    try {
        m_clock->load(json["clock"].toObject());
    } catch (const std::exception& e) {
        qCritical() << tr("时钟数据加载失败:\n%1").arg(e.what());
    }
    try {
        BaseTimeLineModel::load(json);
    } catch (const std::exception& e) {
        qCritical() << tr("基础数据加载失败:\n%1").arg(e.what());
    }
}

void TimeLineNodeModel::onStartPlay()
{

    getClock()->onStart();
}

void TimeLineNodeModel::onStopPlay()
{
    getClock()->onStop();
}

void TimeLineNodeModel::onPausePlay()
{
    getClock()->onPause();
}

void TimeLineNodeModel::onSetLoop(bool loop)
{
    getClock()->onLoop(loop);
}

void TimeLineNodeModel::onSetPlayheadPos(int frame)
{
    getClock()->setCurrentFrame(frame);
}

qint64 TimeLineNodeModel::getPlayheadPos() const
{
    return getClock()->getCurrentFrame();
}

TimeCodeType TimeLineNodeModel::getTimeCodeType() const
{
    return getClock()->getTimecodeType();
}

QList<QVariantMap> TimeLineNodeModel::onGetClipCurrentData(qint64 currentFrame) const {
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
