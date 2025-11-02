//
// Created by WuBin on 2025/6/3.
//

#include "TimeLineModel.h"
#include <QMessageBox>
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
    try { // 基础数据加载
        BaseTimeLineModel::load(json);
    } catch (const std::exception& e) {
        qCritical() << tr("基础数据加载失败:\n%1").arg(e.what());
        return;
    }

    try { // 舞台数据加载
        m_stage->load(json["stage"].toObject());
    } catch (const std::exception& e) {
        qCritical() << tr("舞台数据加载失败:\n%1").arg(e.what());
        return;
    }

    try { // 时钟数据加载
        m_clock->load(json["clock"].toObject());
    } catch (const std::exception& e) {
         qCritical() << tr("时钟数据加载失败:\n%1").arg(e.what());
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

QVariant TimeLineModel::clipData(ClipId clipID, TimelineRoles role) const
{
    QVariant result;
    // 遍历所有轨道查找指定的片段
    for (const auto& track : m_tracks) {
        for (const auto& clip : track->clips) {
            if (clip->id() == clipID) {
                switch (role) {
                    case ClipIdRole:
                        result=clip->id();
                        break;
                    case ClipInRole:
                         result=clip->start();
                        break;
                    case ClipOutRole:
                         result=clip->end();
                        break;
                    case ClipLengthRole:
                        result=clip->end() - clip->start();
                    case ClipTypeRole:
                        result=clip->type();
                        break;
                    case ClipShowWidgetRole:
                        result=clip->isEmbedWidget();
                        break;
                    // case ClipResizableRole:
                    //     return QVariant::fromValue(clip->isResizeable());
                    case ClipShowBorderRole:
                        result=clip->isShowBorder();
                        break;
                    case ClipModelRole:
                        result= QVariant::fromValue(clip);
                        break;
                    case ClipOscWidgetsRole:
                        result=QVariant::fromValue(clip->getOscMapping());
                        break;
                    default:
                        break;
                }
            }
        }
    }
    return result;
}

bool TimeLineModel::setClipData(ClipId clipID, TimelineRoles role, QVariant value)
{
    // 遍历所有轨道查找指定的片段
    for (auto& track : m_tracks) {
        for (auto& clip : track->clips) {
            if (clip->id() == clipID) {
                bool success = false;
                switch (role) {
                    case ClipInRole:
                        clip->setStart(value.toInt());
                        success = true;
                        break;
                    case ClipOutRole:
                        clip->setEnd(value.toInt());
                        success = true;
                        break;
                    case ClipShowWidgetRole:
                        clip->setEmbedWidget(value.toBool());
                        success = true;
                        break;
                    // case ClipResizableRole:
                    //     clip->setResizeable(value.toBool());
                    //     success = true;
                    //     break;
                    case ClipShowBorderRole:
                        clip->setShowBorder(value.toBool());
                        success = true;
                        break;
                }
                if (success) {
                    // 发出数据改变信号
                    emit dataChanged(createIndex(0, 0), createIndex(rowCount(), 0));
                    return true;
                }
            }
        }
    }
    return false;
}