//
// Created by bwll1 on 2024/5/27.
//

#ifndef TIMELINEV2_TIMEWIDGET_HPP
#define TIMELINEV2_TIMEWIDGET_HPP
#include <QToolBar>
#include<QStyleFactory>
#include <QSplitter>
#include <QVBoxLayout>
#include "qtreeview.h"
#include "timelinemodel.hpp"
#include "timelineview.hpp"
#include "tracklistview.hpp"
#include "timelinetypes.h"
class timelinewidget : public QWidget{
Q_OBJECT

public:
    timelinewidget(QWidget *parent = nullptr){
        view->setModel(model);
        tracklist->setModel(model);

        toolbar->addWidget(slider);
        splitter->addWidget(tracklist);
        splitter->addWidget(view);
        splitter->setHandleWidth(0);
        QList<int> sizes({120,780});
        splitter->setMouseTracking(true);
        splitter->resize(880,230);
        splitter->setSizes(sizes);
        slider->setRange(1, 100);
        slider->setValue(1);
//        splitter->show();
        mainlayout->addWidget(splitter);
        mainlayout->addWidget(toolbar);

        QAction *actionStart = new QAction("start", this);
        actionStart->setShortcut(QKeySequence(Qt::Key_Space));
        connect(actionStart, &QAction::triggered, view, &TimelineView::timelinestart);
        toolbar->addAction(actionStart);


        QObject::connect(slider,&QSlider::valueChanged,view,&TimelineView::setScale);
        QObject::connect(view,&TimelineView::scrolled,tracklist,&TracklistView::scroll);
        QObject::connect(tracklist,&TracklistView::scrolled,view,&TimelineView::scroll);

        QObject::connect(model,&TimelineModel::newClip,view,&TimelineView::addClipToMap);
        QObject::connect(model,&TimelineModel::trackMoved,view,&TimelineView::TrackMoved);

        QObject::connect(model,&TimelineModel::playheadMoved,tracklist,&TracklistView::setTime);
        QObject::connect(model,&TimelineModel::tracksChanged,tracklist,&TracklistView::updateViewport);

    };
    ~timelinewidget()=default;


public:
    QVBoxLayout *mainlayout=new QVBoxLayout(this);
    QSplitter* splitter = new QSplitter(Qt::Horizontal,this);
    TimelineModel* model = new TimelineModel();
    TimelineView* view = new TimelineView(this);
    TracklistView* tracklist = new TracklistView(this);
    QToolBar* toolbar = new QToolBar("zoom slider",view);
    QSlider* slider = new QSlider(Qt::Horizontal);
};


#endif //TIMELINEV2_TIMEWIDGET_HPP
