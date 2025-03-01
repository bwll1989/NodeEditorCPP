//
// Created by bwll1 on 2024/5/27.
//

#ifndef TIMELINEWIDGET_CPP
#define TIMELINEWIDGET_CPP

#include "timelinewidget.hpp"
#include "timelinesettingsdialog.hpp"
#include "imageprovider.hpp"
TimelineWidget::TimelineWidget(QWidget *parent) : QWidget(parent) {
    // 首先创建模型
    model = new TimelineModel();
    // 创建组件
    createComponents();
    // 连接轨道列表竖向滚动到时间线竖向滚动
    connect(tracklist, &TracklistView::scrolled, view, &TimelineView::onScroll);
    // 连接模型轨道变化到时间线更新视图
    connect(model, &TimelineModel::S_trackChanged, view, &TimelineView::onUpdateViewport);
    // 连接轨道列表更新到时间线更新视图
    connect(tracklist, &TracklistView::viewupdate, view, &TimelineView::onUpdateViewport);    
    // 连接工具栏设置按钮到显示设置对话框
    connect(view->toolbar, &TimelineToolbar::settingsClicked, this, &TimelineWidget::showSettingsDialog);
}

void TimelineWidget::showSettingsDialog()
{
    if (!m_settingsDialog) {
        m_settingsDialog = new TimelineSettingsDialog(model,this);
    }
    // 同步模型中设置，保证设置对话框中的设置与模型中的设置一致
    m_settingsDialog->syncSettings();
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

    // 连接模型的帧图像更新信号到舞台
    connect(model, &TimelineModel::frameImageUpdated,
            model->getStage(), &TimelineStage::updateCurrentFrame);

    // 注册图像提供者
    auto engine = qmlEngine(model->getStage());
    if (engine) {
        engine->addImageProvider(QLatin1String("timeline"), ImageProvider::instance());
    }
}

TimelineWidget::~TimelineWidget()
{
    delete view;
    delete tracklist;
    delete model;
    delete m_settingsDialog;
}

#endif //TIMELINEV2_TIMEWIDGET_HPP