//
// Created by WuBin on 2025/6/3.
//

#pragma once

#include "BaseTimeLineView.h"
#include "TimeLineModel.h"
class TimeLineView:public BaseTimelineView {
    Q_OBJECT
public:
    explicit TimeLineView(TimeLineModel* model, QWidget* parent = nullptr)
        : BaseTimelineView(model, parent) {}
    ~TimeLineView() override = default;
    void initToolBar(BaseTimelineToolbar *toolbar) override;
protected:
    QString isMimeAcceptable(const QMimeData *Mime) const override;

    void dropEvent(QDropEvent *event) override;
};
