//
// Created by bwll1 on 2024/5/27.
//

#ifndef TIMELINEWIDGET_CPP
#define TIMELINEWIDGET_CPP

#include "timelinewidget.hpp"
#include "timelinesettingsdialog.hpp"

TimelineWidget::TimelineWidget(QWidget *parent) : QWidget(parent) {
    // 首先创建模型
    model = new TimelineModel();
    // 创建组件
    createComponents();
    // 连接信号
    connect(tracklist, &TracklistView::scrolled, view, &TimelineView::scroll);
    connect(model, &TimelineModel::playheadMoved, tracklist, &TracklistView::setTime);
    connect(model, &TimelineModel::tracksChanged, tracklist, &TracklistView::updateViewport);
    connect(model, &TimelineModel::tracksChanged, view, &TimelineView::updateViewport);
    connect(tracklist, &TracklistView::viewupdate, view, &TimelineView::updateViewport);    
    setupConnections();
}

void TimelineWidget::setupConnections()
{
    // Connect toolbar settings button to show settings dialog
    connect(view->toolbar, &TimelineToolbar::settingsClicked, this, &TimelineWidget::showSettingsDialog);
    
    // Connect settings dialog changes to timeline
   
}

void TimelineWidget::showSettingsDialog()
{
    if (!m_settingsDialog) {
        m_settingsDialog = new TimelineSettingsDialog(this);
        
        // Connect settings changes to timeline
        connect(m_settingsDialog, &TimelineSettingsDialog::settingsChanged, this, [this]() {
            // 应用设置到timelineview
            int fps = m_settingsDialog->getFPS();
           
            // TODO: Apply video settings
        });
    }
    
    m_settingsDialog->show();
}

QJsonObject TimelineWidget::save() {
    return model->save();
}

void TimelineWidget::load(const QJsonObject& json) {
    model->load(json);
}



void TimelineWidget::createComponents() {
    // 创建工具栏
    view = new TimelineView(model, this);
    tracklist = new TracklistView(model, this);
    
    // 创建主布局
    mainlayout = new QVBoxLayout(this);
    mainlayout->setContentsMargins(0, 0, 0, 0);
    mainlayout->setSpacing(0);
    
    // 创建水平分割器
    auto* horizontalSplitter = new QSplitter(Qt::Horizontal, this);
    
    // 创建左侧面板（轨道列表和时间线）
    auto* leftPanel = new QWidget(this);
    auto* leftLayout = new QVBoxLayout(leftPanel);
    leftLayout->setContentsMargins(0, 0, 0, 0);
    leftLayout->setSpacing(0);
    
    splitter->addWidget(tracklist);
    splitter->addWidget(view);
    splitter->setHandleWidth(0);
    QList<int> sizes({200,800});
    splitter->setMouseTracking(true);
    splitter->setSizes(sizes);
    leftLayout->addWidget(splitter);
    
    // 添加到水平分割器
    horizontalSplitter->addWidget(leftPanel);
    
    // 设置分割器大小
    horizontalSplitter->setSizes({700, 300});  // 左侧面板和属性面板的初始大小比例
    
    // 添加到主布局
    mainlayout->addWidget(horizontalSplitter);
}

TimelineWidget::~TimelineWidget()
{
    delete view;
    delete tracklist;
    delete model;
}

#endif //TIMELINEV2_TIMEWIDGET_HPP