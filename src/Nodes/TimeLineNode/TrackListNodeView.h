//
// Created by WuBin on 2025/6/3.
//

#pragma once

#include "BaseTrackListView.h"
#include "TimeLineNodeModel.h"
class TrackListNodeView: public BaseTracklistView {
Q_OBJECT
public:
    explicit TrackListNodeView(TimeLineNodeModel* model, QWidget *parent = nullptr);
    ~TrackListNodeView() override;
protected:
    void drawTitle(QPainter *painter) override;
    
};




