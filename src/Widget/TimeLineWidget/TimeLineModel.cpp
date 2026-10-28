//
// Created by WuBin on 2025/6/3.
//

#include "TimeLineModel.h"

#include "Common/BaseClass/AbstractClipDelegateModel.h"

TimeLineModel::TimeLineModel(QObject* parent)
    : BaseTimeLineModel(parent)
    , m_clock(new TimeLineClock(this))
{
    connect(m_clock, &TimeLineClock::currentFrameChanged, this, &TimeLineModel::onGetClipCurrentData);
    // 时钟是播放头的真值。统一发 S_playheadMoved，视图只刷新播放头，不再整页重画。
    connect(m_clock, &TimeLineClock::currentFrameChanged, this, [this](qint64 frame) {
        emit S_playheadMoved(static_cast<int>(frame));
    });
    // 变长只发 S_LengthChanged，不走 onUpdateTimeLineLength。时钟总长在这里跟上。
    connect(this, &BaseTimeLineModel::S_LengthChanged, this, [this](qint64 length) {
        m_clock->setMaxFrames(length);
    });
}
QJsonObject TimeLineModel::save() const
{
    auto res=BaseTimeLineModel::save();
    res["clock"]=m_clock->save();
    return res;
}

void TimeLineModel::load(const QJsonObject& json)
{
    try { // 时钟数据加载
        m_clock->load(json["clock"].toObject());
    } catch (const std::exception& e) {
        qCritical() << tr("时钟数据加载失败:\n%1").arg(e.what());
    }
    try { // 基础数据加载
        BaseTimeLineModel::load(json);
    } catch (const std::exception& e) {
        qCritical() << tr("基础数据加载失败:\n%1").arg(e.what());
        return;
    }
    for (TrackData* track : getTracks()) {
            if (!track) continue;
            for (AbstractClipModel* clip : track->clips) {
                if (auto* delegate = dynamic_cast<AbstractClipDelegateModel*>(clip)) {
                    delegate->onModelReady();
                }
            }
        }

}

void TimeLineModel::onStartPlay()
{

    getClock()->onStart();
}

void TimeLineModel::onStopPlay()
{
    getClock()->onStop();
}

void TimeLineModel::onSetLoop(bool loop)
{
    getClock()->onLoop(loop);
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

TimeCodeType TimeLineModel::getTimeCodeType() const
{
    return getClock()->getTimecodeType();
}

QList<QVariantMap> TimeLineModel::onGetClipCurrentData(qint64 currentFrame) const {
    QList<QVariantMap> clipDataList;
    // 遍历所有轨道
    // 帧序列校验：检测跳帧或重复帧（含倒退）
    // const qint64 delta = currentFrame - lastFrame;
    // if (lastFrame != 0) {
    //     if (delta > 1) {
    //         // 跳帧：当前帧与上一帧差值大于 1
    //         qDebug() << "skipped frames from " ;
    //     } else if (delta <= 0) {
    //         // 重复或倒退帧
    //         qDebug() << "duplicate/backward frame, last ";
    //     }
    // }
    // lastFrame = currentFrame;
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