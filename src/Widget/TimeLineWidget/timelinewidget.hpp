//
// Created by bwll1 on 2024/5/27.
//

#ifndef TIMELINEWIDGET_HPP
#define TIMELINEWIDGET_HPP
#if defined(_MSC_VER) || defined(WIN64) || defined(_WIN64) || defined(__WIN64__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32__) || defined(__NT__)
#  define Q_DECL_EXPORT __declspec(dllexport)
#  define Q_DECL_IMPORT __declspec(dllimport)
#else
#  define Q_DECL_EXPORT     __attribute__((visibility("default")))
#  define Q_DECL_IMPORT     __attribute__((visibility("default")))
#endif

#if defined(TIMELINEWIDGET_LIBRARY)
#  define TIMELINEWIDGET_EXPORT Q_DECL_EXPORT
#else
#  define TIMELINEWIDGET_EXPORT Q_DECL_IMPORT
#endif


#include <QToolBar>
#include<QStyleFactory>
#include <QSplitter>
#include <QVBoxLayout>
#include <QShortcut>
#include <QScreen>
#include "qtreeview.h"
#include "timelinemodel.hpp"
#include "timelineview.hpp"
#include "tracklistview.hpp"
#include "timelinetypes.h"
#include <QFileDialog>
#include <QJsonDocument>
#include <QGuiApplication>
#include "clipproperty.hpp"
#include "videoplayerwidget.hpp"
// #include "zoomcontroller.hpp"

class TimelineWidget : public QWidget{
Q_OBJECT

public:
    explicit TimelineWidget(QWidget *parent = nullptr);
    ~TimelineWidget();

public:
    QVBoxLayout *mainlayout;
    QSplitter* splitter = new QSplitter(Qt::Horizontal,this);
    TimelineModel* model;
    TimelineView* view;
    TracklistView* tracklist;
    VideoPlayerWidget* videoPlayer = nullptr;  // 只保留 videoPlayer

Q_SIGNALS:
    void initialized();
    void videoWindowClosed();
    void clipSelected(AbstractClipModel* clip); // Add signal for clip selection

public slots:
    QJsonObject save();
    void load(const QJsonObject& json);
    void start();
    void onClipSelected(const QModelIndex& index);
    void showVideoWindow(bool show = true);

private:
    void createComponents();
    void setupVideoWindow();
};


#endif //TIMELINEV2_TIMEWIDGET_HPP