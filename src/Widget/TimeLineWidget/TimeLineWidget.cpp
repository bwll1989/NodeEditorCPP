//
// Created by bwll1 on 2024/5/27.
//

#ifndef TIMELINEWIDGET_CPP
#define TIMELINEWIDGET_CPP

#include "timelinewidget.hpp"

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
    // 连接选择变化信号
    connect(view->selectionModel(), &QItemSelectionModel::currentChanged,
            this, &TimelineWidget::onClipSelected);
    // 创建视频播放器和窗口
    setupVideoWindow();
}

QJsonObject TimelineWidget::save() {
    return model->save();
}

void TimelineWidget::load(const QJsonObject& json) {
    model->load(json);
}

void TimelineWidget::start(){
    view->timelinestart();
}


void TimelineWidget::createComponents() {
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

void TimelineWidget::onClipSelected(const QModelIndex& index) {
    // 如果选中了有效的片段
    if (index.isValid() && index.parent().isValid()) {
        auto* clip = static_cast<AbstractClipModel*>(index.internalPointer());
        if (clip) {
            emit clipSelected(clip);
        }
    }
}

void TimelineWidget::setupVideoWindow()
{
    // 创建视频播放器
    videoPlayer = new VideoPlayerWidget;
    videoPlayer->setWindowFlags(Qt::Window | Qt::FramelessWindowHint);  // 设置为独立窗口
    videoPlayer->setKeepAspectRatio(true);
    
    // 添加 Escape 键关闭功能
    auto* shortcut = new QShortcut(QKeySequence(Qt::Key_Escape), videoPlayer);
    connect(shortcut, &QShortcut::activated, [this]() {
        showVideoWindow(false);
    });
}

void TimelineWidget::showVideoWindow(bool show)
{
    if (show) {
        if (!videoPlayer) {
            setupVideoWindow();
        }

        videoPlayer->showFullScreen();
        
        // 获取所有屏幕
        QList<QScreen*> screens = QGuiApplication::screens();
        
        // 如果有第二个屏幕，确保窗口在第二个屏幕上
        if (screens.size() > 1) {
            QScreen* secondScreen = screens[1];
            videoPlayer->setGeometry(secondScreen->geometry());
        }
    } else {
        if (videoPlayer) {
            videoPlayer->close();  // 关闭窗口
            delete videoPlayer;    // 销毁对象
            videoPlayer = nullptr; // 清空指针
            emit videoWindowClosed();
        }
    }
}


TimelineWidget::~TimelineWidget()
{
    if (videoPlayer) {
        videoPlayer->close();  // 确保窗口关闭
        delete videoPlayer;
        videoPlayer = nullptr;
    }
}

#endif //TIMELINEV2_TIMEWIDGET_HPP