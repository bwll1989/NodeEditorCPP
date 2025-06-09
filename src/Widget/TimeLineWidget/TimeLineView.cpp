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
        if (event->button() == Qt::RightButton) {
            // 处理左键按下的情况
            QAbstractItemView::mouseMoveEvent(event);
            return;
        }

        if(mouseHeld){
//            如果鼠标按住拖动
            m_mouseEnd = event->pos();
            if(!selectionModel()->selectedIndexes().isEmpty()&&m_mouseEnd.x()>=0){
                QModelIndex clipIndex = selectionModel()->selectedIndexes().first();
                AbstractClipModel* clip = static_cast<AbstractClipModel*>(clipIndex.internalPointer());

                if(m_mouseUnderClipEdge==hoverState::NONE){
                    moveSelectedClip(pointToFrame(m_mouseEnd.x()+m_mouseOffset.x()),m_mouseEnd.y()+m_mouseOffset.y());
                }else if(clip && clip->isResizable()){
                    if(m_mouseUnderClipEdge==hoverState::LEFT){
                        int newFrame = pointToFrame(m_mouseEnd.x() + m_scrollOffset.x());
                        getModel()->setData(clipIndex, newFrame, TimelineRoles::ClipInRole);
                        // getModel()->onTimelineLengthChanged();
                        updateEditorGeometries();
                    }else if(m_mouseUnderClipEdge==hoverState::RIGHT){
                        int newFrame = pointToFrame(m_mouseEnd.x() + m_scrollOffset.x());
                        getModel()->setData(clipIndex, newFrame, TimelineRoles::ClipOutRole);
                        // getModel()->onTimelineLengthChanged();
                        updateEditorGeometries();
                    }
                }
                viewport()->update();
            }
            else{
                movePlayheadToFrame(pointToFrame(std::max(0,m_mouseEnd.x() + m_scrollOffset.x())));
                viewport()->update();
            }

            if(m_playheadSelected){
                auto* derivedModel = dynamic_cast<TimeLineModel*>(getModel());
                emit derivedModel->onPausePlay();
                movePlayheadToFrame(pointToFrame(std::max(0,m_mouseEnd.x() + m_scrollOffset.x())));
                viewport()->update();
            }
            return QAbstractItemView::mouseMoveEvent(event);
       }

        QPoint pos = event->pos();
        m_hoverIndex = indexAt(event->pos());
        QRect rect = visualRect(m_hoverIndex);
        m_mouseUnderClipEdge = hoverState::NONE;
       //5 is hitbox size + -5px
       //see if item is a clip
        if((m_hoverIndex.isValid() && m_hoverIndex.parent().isValid())){
            AbstractClipModel* clip = static_cast<AbstractClipModel*>(m_hoverIndex.internalPointer());
            if(clip && clip->isResizable()){
                if(abs(pos.x() - rect.left())<=5){
                    m_mouseUnderClipEdge=hoverState::LEFT;
                }else if(abs(pos.x() - rect.right())<=5){
                    m_mouseUnderClipEdge=hoverState::RIGHT;
                }
            }
       }
        if (m_mouseUnderClipEdge != hoverState::NONE) {
           setCursor(Qt::SizeHorCursor);
        }else {
           unsetCursor();
        }

       QAbstractItemView::mouseMoveEvent(event);
    }


void TimeLineView::mouseReleaseEvent(QMouseEvent *event)
{
    if (event->button() == Qt::RightButton) {
        // 处理左键按下的情况
        QAbstractItemView::mousePressEvent(event);
        return;
    }
    mouseHeld = false;
    m_playheadSelected = false;
    m_mouseEnd = event->pos();

    //pressed outside of selection
    QAbstractItemView::mouseReleaseEvent(event);
}