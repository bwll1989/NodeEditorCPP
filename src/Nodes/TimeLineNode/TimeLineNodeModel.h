//
// Created by WuBin on 2025/6/3.
//

#pragma once
#include "BaseTimeLineModel.h"
#include  "TimeCodeDefines.h"
#include "TimeLineNodeClock.hpp"
#include "TimeLineDefines.h"
class TimeLineNodeModel : public BaseTimeLineModel {
    Q_OBJECT

public:
    explicit TimeLineNodeModel(QObject* parent = nullptr);
    ~TimeLineNodeModel() override;

    QJsonObject save() const override;

    void load(const QJsonObject &modelJson)  override;

    TimeLineNodeClock* getClock() const { return m_clock; }

    qint64 getPlayheadPos() const override;

    TimeCodeType getTimeCodeType() const;

    QVariant clipData(ClipId clipID, TimelineRoles role) const override;

    bool setClipData(ClipId clipID, TimelineRoles role, QVariant value) override;
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
    void onSetLoop(bool loop);

    QList<QVariantMap> onGetClipCurrentData(qint64 currentFrame) const;
private:
    //时钟对象
    TimeLineNodeClock* m_clock;
};