//
// Created by WuBin on 2024/12/5.
//

#ifndef NODEEDITORCPP_MLTPLAYERWIDGET_H
#define NODEEDITORCPP_MLTPLAYERWIDGET_H

#include <QApplication>
#include <QWidget>
#include <QVBoxLayout>
#include <QTimer>
#include <QtWidgets>

#include <mlt++/Mlt.h>  // 引入 Mlt++ 的头文件

class MLTPlayerWidget : public QMainWindow {
Q_OBJECT

public:
    MLTPlayerWidget(QWidget *parent = nullptr);
    ~MLTPlayerWidget();

public slots:
    void updatePlayer() ;

private:
    Mlt::Producer *producer;
    Mlt::Consumer *consumer;
    Mlt::Profile *profile; // defaults to dv_pal
    Mlt::Playlist *playlist;
    QTimer *timer;
};





#endif //NODEEDITORCPP_MLTPLAYERWIDGET_H
