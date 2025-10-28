//
// Created by WuBin on 2025/6/3.
//

#pragma once

#include "BaseTimeLineView.h"
#include "TimeLineModel.h"
class TimeLineView:public BaseTimelineView {
    Q_OBJECT
public:
    explicit TimeLineView(TimeLineModel* model, QWidget* parent=nullptr);
    ~TimeLineView() override;
protected:
    /**
    * 鼠标移动事件
    * @param QMouseEvent *event 鼠标事件
    */
    void mouseMoveEvent(QMouseEvent *event) override;

    QString isMimeAcceptable(const QMimeData *Mime) const override;
private:
    void movePlayheadToFrame(int frame) override;

};
