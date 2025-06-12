//
// Created by WuBin on 2025/6/3.
//

#include "TimeLineView.h"
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
    connect(derivedModel->getClock(), &TimeLineClock::timecodePlayingChanged,this->toolbar,&BaseTimelineToolbar::setPlaybackState);
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
//
//
// void TimeLineView::mouseReleaseEvent(QMouseEvent *event)
// {
//     if (event->button() == Qt::RightButton) {
//         // 处理左键按下的情况
//         QAbstractItemView::mousePressEvent(event);
//         return;
//     }
//     mouseHeld = false;
//     m_playheadSelected = false;
//     m_mouseEnd = event->pos();
//
//     //pressed outside of selection
//     QAbstractItemView::mouseReleaseEvent(event);
// }

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