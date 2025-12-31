//
// Created by bwll1 on 2024/5/27.
//
#pragma once
#include "TimeLineToolbar.h"
#include "TimeLineStyle.h"
#include <QWidget>
#include "TimeLineModel.h"
#include "TimeLineView.h"
#include "TrackListView.h"
#include <QSplitter>
#include <QVBoxLayout>
#include "./TimelineSettingWidget/TimeLineSetting.hpp"


class TimelineWidget : public QWidget {
    Q_OBJECT

public:
    explicit TimelineWidget(TimeLineModel* model=nullptr, QWidget *parent = nullptr);
    ~TimelineWidget() override;

public:
    //主布局
    QVBoxLayout *mainlayout;
    //分割器
    QSplitter *splitter;
    //模型
    TimeLineModel* model;
    //视图
    TimeLineView* view;
    //轨道列表
    TrackListView* tracklist;
    //工具栏
    TimeLineToolBar* toolbar;
    //设置窗口
    TimeLineSetting* settingsDialog;
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
    /**
     * 显示设置对话框
     */
    void showSettingsDialog(); // Add new slot


private:
    /**
     * 创建组件
     */
    void createComponents();

};


