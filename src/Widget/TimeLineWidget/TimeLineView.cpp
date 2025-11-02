//
// Created by WuBin on 2025/6/3.
//

#include "TimeLineView.h"
#include "DefaultTimeLineToolBar.h"
TimeLineView::TimeLineView(TimeLineModel* model, QWidget *parent) : BaseTimelineView(model, parent)
{
    //将时间游标的显示与模型中的时钟相绑定
    auto* derivedModel = dynamic_cast<TimeLineModel*>(getModel());
    // 连接时间码生成器信号
    // 使用lambda表达式来处理时间码变化信号
    connect(derivedModel->getClock(), &TimeLineClock::currentFrameChanged,
        [this](int frame) {
            viewport()->update();  // 触发viewport的更新
        });

    setAcceptDrops(true);
}
TimeLineView::~TimeLineView() = default;

void TimeLineView::movePlayheadToFrame(int frame)
{
    if (auto* derivedModel = dynamic_cast<TimeLineModel*>(getModel()))
    {
        derivedModel->getClock()->setCurrentFrame(frame);
        getModel()->onSetPlayheadPos(frame);
        viewport()->update();
    }
}


void TimeLineView::mouseMoveEvent(QMouseEvent *event)
{
    // 处理右键事件
    if (event->button() == Qt::RightButton) {
        QAbstractItemView::mouseMoveEvent(event);
        return;
    }

    // 处理鼠标按住拖动的情况
    if (mouseHeld) {
        handleMouseDrag(event);
        return;
    }

    // 处理鼠标悬停状态
    updateMouseHoverState(event);
    updateCursorShape();

    QAbstractItemView::mouseMoveEvent(event);
}


QString TimeLineView::isMimeAcceptable(const QMimeData *Mime) const
{
    // 获取拖拽的文件路径

    if (Mime->hasUrls())
    {
        QString filePath = Mime->urls().first().toLocalFile();
        QFileInfo fileInfo(filePath);
        QString suffix = fileInfo.suffix().toLower();
        // 根据后缀判断文件类型
        if(VideoTypes.contains(suffix, Qt::CaseInsensitive)) {
            return "Video";
        }
        else if(AudioTypes.contains(suffix, Qt::CaseInsensitive)) {
            return "Audio";
        }
        else if(ImageTypes.contains(suffix, Qt::CaseInsensitive)) {
            return "Image";
        }
        else if(ControlTypes.contains(suffix, Qt::CaseInsensitive)) {
            return "Control";
        }
    }else if (Mime->hasFormat("application/x-osc-address"))
    {
        return "Trigger";
    }
    return "";
}

void TimeLineView::initToolBar(BaseTimelineToolbar *toolbar)
{
    this->m_toolbar = toolbar;
    this->m_toolbar->setFixedHeight(toolbarHeight-4);
    // 设置工具栏位置
     this->m_toolbar->move(0, 2);
    // 连接工具栏播放按钮信号
    connect(dynamic_cast<DefaultTimeLineToolBar*>(this->m_toolbar), &DefaultTimeLineToolBar::playClicked, [this]() {
        getModel()->onStartPlay();
    });
    // 连接工具栏停止按钮信号
    connect(dynamic_cast<DefaultTimeLineToolBar*>(m_toolbar), &DefaultTimeLineToolBar::stopClicked, [this]() {
        getModel()->onStopPlay();
    });
    // 连接工具栏暂停按钮信号
    connect(dynamic_cast<DefaultTimeLineToolBar*>(m_toolbar), &DefaultTimeLineToolBar::pauseClicked, [this]() {
        getModel()->onPausePlay();
    });
    connect(dynamic_cast<DefaultTimeLineToolBar*>(m_toolbar),&DefaultTimeLineToolBar::prevFrameClicked,[this](){
       getModel()->onSetPlayheadPos(qMax(0,getModel()->getPlayheadPos()-1));
    });
    connect(dynamic_cast<DefaultTimeLineToolBar*>(m_toolbar),&DefaultTimeLineToolBar::nextFrameClicked,[this](){
        getModel()->onSetPlayheadPos(getModel()->getPlayheadPos()+1);
    });
    // 连接移动剪辑按钮信号
    connect(dynamic_cast<DefaultTimeLineToolBar*>(m_toolbar), &DefaultTimeLineToolBar::moveClipClicked, [this](int dx) {
        moveSelectedClip(dx,0,false);
    });
    // 连接删除剪辑按钮信号
    connect(dynamic_cast<DefaultTimeLineToolBar*>(m_toolbar), &DefaultTimeLineToolBar::deleteClipClicked, [this]() {
        if(selectionModel()->selectedIndexes().isEmpty())
            return;

        QModelIndex currentIndex = selectionModel()->currentIndex();
        getModel()->onDeleteClip(currentIndex);

        // 清除选择并发送 nullptr
        selectionModel()->clearSelection();
        emit currentClipChanged(nullptr);

        viewport()->update();
    });

    // 连接放大按钮信号
    connect(dynamic_cast<DefaultTimeLineToolBar*>(m_toolbar), &DefaultTimeLineToolBar::zoomInClicked, [this]() {
        currentScale = currentScale * 1.1;

                // 允许达到 0.0，从而触达 setScale 的 minTimescale
        currentScale = qBound(0.0, currentScale, 1.0);
       setScale(currentScale);
    });
    // 连接缩小按钮信号
    connect(dynamic_cast<DefaultTimeLineToolBar*>(m_toolbar), &DefaultTimeLineToolBar::zoomOutClicked, [this]() {
        currentScale = currentScale * 0.9;

        // 允许达到 0.0，从而触达 setScale 的 minTimescale
        currentScale = qBound(0.0, currentScale, 1.0);
        setScale(currentScale);
    });
    connect( dynamic_cast<TimeLineModel*>(getModel())->getClock(), &TimeLineClock::timecodePlayingChanged,
       dynamic_cast<DefaultTimeLineToolBar*>(this->m_toolbar),&DefaultTimeLineToolBar::setPlaybackState);

    // 连接循环按钮信号
    connect(dynamic_cast<DefaultTimeLineToolBar*>(m_toolbar), &DefaultTimeLineToolBar::loopToggled, [this](bool loop) {
        dynamic_cast<TimeLineModel*>(getModel())->onSetLoop(loop);
    });
    connect( dynamic_cast<TimeLineModel*>(getModel())->getClock(), &TimeLineClock::loopingChanged,
       dynamic_cast<DefaultTimeLineToolBar*>(this->m_toolbar),&DefaultTimeLineToolBar::setLoopState);
}
