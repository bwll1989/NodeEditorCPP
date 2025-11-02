//
// Created by bwll1 on 2024/5/27.
//
#pragma once

#include <QToolBar>


#include "TimeLineStyle.h"
#include <QWidget>
#include "TimeLineNodeModel.h"
#include "TimeLineNodeView.h"
#include "TrackListNodeView.h"
#include <QSplitter>
#include <QVBoxLayout>
#include "TimeLineNodeToolBar.h"

class TimelineNodeWidget : public QWidget {
    Q_OBJECT

public:
    explicit TimelineNodeWidget(TimeLineNodeModel* model=nullptr, QWidget *parent = nullptr);
    ~TimelineNodeWidget() override;

    //主布局
    QVBoxLayout *mainlayout;
    //分割器
    QSplitter *splitter;
    //模型
    TimeLineNodeModel* model;
    //视图
    TimeLineNodeView* view;
    //工具栏
    TimeLineNodeToolBar* toolbar;
    //轨道列表
    TrackListNodeView* tracklist;
signals:
    //初始化信号
    void initialized();

public slots:
    /**
     * 保存
     * @return QJsonObject 保存的json对象
     */
    QJsonObject save();
    /**
     * 加载
     * @param const QJsonObject& json 加载的json对象
     */
    void load(const QJsonObject& json);

private:
    /**
     * 创建组件
     */
    void createComponents();

};


