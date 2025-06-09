//
// Created by WuBin on 2025/6/3.
//

#ifndef TRACKLISTVIEW_H
#define TRACKLISTVIEW_H
#include "BaseTrackListView.h"
#include "TimeLineModel.h"
class TrackListView: public BaseTracklistView {
Q_OBJECT
public:
    explicit TrackListView(TimeLineModel* model, QWidget *parent = nullptr);
    ~TrackListView() override;
protected:
    void drawTitle(QPainter *painter) override;
    
};



#endif //TRACKLISTVIEW_H
