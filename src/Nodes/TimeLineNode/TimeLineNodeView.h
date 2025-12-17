//
// Created by WuBin on 2025/6/3.
//

#pragma once

#include "BaseTimeLineView.h"
#include "TimeLineNodeModel.h"
class TimeLineNodeView:public BaseTimelineView {
    Q_OBJECT
public:
    explicit TimeLineNodeView(TimeLineNodeModel* model, QWidget* parent=nullptr);
    ~TimeLineNodeView() override=default;
    void initToolBar(BaseTimelineToolbar *toolbar) override;
protected:
    /**
    * 鼠标移动事件
    * @param QMouseEvent *event 鼠标事件
    */
    void mouseMoveEvent(QMouseEvent *event) override;

    QString isMimeAcceptable(const QMimeData *Mime) const override;

    void dropEvent(QDropEvent *event) override;
private:
    void movePlayheadToFrame(int frame) override;

};
