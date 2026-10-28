//
// Created by WuBin on 2025/6/3.
//

#pragma once

#include "BaseTimeLineView.h"
#include "TimeLineNodeModel.h"
class TimeLineNodeView:public BaseTimelineView {
    Q_OBJECT
public:
    explicit TimeLineNodeView(TimeLineNodeModel* model, QWidget* parent = nullptr)
        : BaseTimelineView(model, parent) {}
    ~TimeLineNodeView() override = default;
    void initToolBar(BaseTimelineToolbar *toolbar) override;
protected:
    QString isMimeAcceptable(const QMimeData *Mime) const override;

    void dropEvent(QDropEvent *event) override;
};
