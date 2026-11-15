//
// Created by bwll1 on 2024/5/27.
//

#pragma once
#include <QToolBar>
#include<QStyleFactory>
#include <QSplitter>
#include <QVBoxLayout>
#include <QShortcut>
#include <QScreen>
#include <QItemSelectionModel>
#include "qtreeview.h"
#include "BaseTimeLineModel.h"
#include "BaseTimeLineView.h"
#include "BaseTrackListView.h"
#include "TimeLineDefines.h"
#include <QWidget>
#include <QFileDialog>
#include <QJsonDocument>
#include <QGuiApplication>
#include "DefaultTimeLineToolBar.h"



class NODE_TIMELINE_PUBLIC BaseTimelineWidget : public QWidget {
    Q_OBJECT

public:
    explicit BaseTimelineWidget(BaseTimeLineModel* model=nullptr, QWidget *parent = nullptr);
    ~BaseTimelineWidget() override;

public:
    // 主布局
    QVBoxLayout *mainlayout;
    // 分割器：左侧 TrackList，右侧 Timeline
    QSplitter* splitter ;
    // 数据模型（轨道/片段/播放头等状态的唯一来源）
    BaseTimeLineModel* model;
    // 时间线视图（绘制 clip）
    BaseTimelineView* view;
    // 轨道列表视图（绘制 track 行）
    BaseTracklistView* tracklist;
    // 默认工具栏（控制播放等）
    DefaultTimeLineToolBar* toolbar;

    /**
     * @brief 共享选择模型访问器（2026-11-15 画布选中分工配套）
     * @details
     * 选中状态唯一来源 = m_sharedSelectionModel（构造时创建，生命周期同
     * 本控件）。宿主（EditorWindow）经此访问器监听 currentChanged，实现
     * "时间轴选中片段 → 画布高亮该片段（活跃可见时画布可拖动/打键）"。
     * createComponents 在构造函数内完成，返回值恒非空（模型非空时）。
     */
    QItemSelectionModel* sharedSelectionModel() const { return m_sharedSelectionModel; }

private:
    /**
     * @brief 两个视图共享的选择模型
     * @details
     * 以往 TrackListView / TimelineView 各自创建 selectionModel，会出现"两套选择状态"，
     * 导致绑定链路复杂、状态不同步、逻辑越来越乱。
     * 这里将选择状态收敛为唯一来源：m_sharedSelectionModel。
     */
    QItemSelectionModel* m_sharedSelectionModel {nullptr};

    QObject* m_bindings {nullptr};

private:
    /**
     * 创建组件
     */
    virtual void createComponents();

};
