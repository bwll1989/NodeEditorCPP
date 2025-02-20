#include "timelineview.hpp"
#include <QMenu>
#include <QAction>

QRect TimelineView::visualRect(const QModelIndex &index) const
{
    
    return itemRect(index).translated(-m_scrollOffset);
}

 QModelIndex TimelineView::indexAt(const QPoint &point) const
    {
        // Check if the point is within the ruler area
        QRect rulerRect(-m_scrollOffset.x(), 0, viewport()->width() + m_scrollOffset.x(), rulerHeight);
        // qDebug()<<"point.y()"<<point.y();
        if (point.y() < rulerHeight) {
            return QModelIndex(); // Return an invalid index if the point is in the ruler area
        }

        // Iterate over each track
        for (int i = 0; i < model()->rowCount(); ++i) {
            QModelIndex trackIndex = model()->index(i, 0);
            QRect trackRect = visualRect(trackIndex);

            // Check if the point is within the track's rectangle
            if (trackRect.contains(point)) {
                // Iterate over each clip within the track
                for (int j = 0; j < model()->rowCount(trackIndex); ++j) {
                    QModelIndex clipIndex = model()->index(j, 0, trackIndex);
                    QRect clipRect = visualRect(clipIndex);

                    // Check if the point is within the clip's rectangle
                    if (clipRect.contains(point)) {
                        return clipIndex; // Return the clip index if the point is within a clip
                    }
                }
                return trackIndex; // Return the track index if the point is within a track but not within any clip
            }
        }

        return QModelIndex(); // Return an invalid index if the point is not within any track or clip
    }

QRect TimelineView::itemRect(const QModelIndex &index) const
{
    // qDebug()<<"index"<<index;
    // 如果索引无效，返回空矩形
    
    if (!index.isValid()) {
    return QRect();
    }

    // 获取轨道宽度
    int trackwidth = getTrackWdith()+ viewport()->width();
    if(index.parent()==QModelIndex())
    // 如果轨道索引
    {
        // 返回轨道矩形
        return QRect(0, (index.row() * trackHeight) + rulerHeight, trackwidth, trackHeight);
    }
    // 如果剪辑索引
    else{
        // 获取剪辑
        AbstractClipModel* clip = static_cast<AbstractClipModel*>(index.internalPointer());
        if (clip) {
            // 获取剪辑开始帧
            int startFrame = clip->start();
            // 获取剪辑结束帧
            int endFrame = clip->end()+1; 
            // 获取剪辑所在轨道行   
            int trackRow = index.parent().row();
            // 获取剪辑开始点x
            int clipStartX = frameToPoint(startFrame);
            // 获取剪辑开始点y
            int clipStartY = (trackRow*trackHeight + rulerHeight);
            // 获取剪辑宽度
            int clipWidth = frameToPoint(endFrame - startFrame);
            // 获取剪辑所在轨道行开始点
            QPoint topLeft(clipStartX,clipStartY);
            // 返回剪辑矩形
            // qDebug()<<"rect"<<QRect(topLeft,QSize(clipWidth,trackHeight));
            return QRect(topLeft,QSize(clipWidth,trackHeight));
    }

    }
    return QRect();
}

int TimelineView::getTrackWdith() const
{
    int timelineLength = model()->data(QModelIndex(), TimelineRoles::TimelineLengthRole).toInt();
    if (timelineLength <= 0) {
        return viewport()->width();  // 如果没有片段，返回视口宽度
    }
    return frameToPoint(timelineLength + 1);  // 否则返回基于时间轴长度的宽度
}

int TimelineView::getPlayheadPos()
{
    return frameToPoint(((TimelineModel*)model())->getPlayheadPos());
}

// 移动选定的剪辑
void TimelineView::moveSelectedClip(int dx, int dy,bool isMouse)
{
    QModelIndexList list = selectionModel()->selectedIndexes();
    if(list.isEmpty())
        return;
    int newPos;
    if(isMouse){
        newPos = dx;
    }else{
        newPos = model()->data(list[0],TimelineRoles::ClipInRole).toInt() + dx;
    }
    // 确保不会移动到负值位置
    newPos = std::max(0, newPos);

    // 更新模型数据
    Model->setData(list.first(), newPos, TimelineRoles::ClipInRole);

    Model->calculateLength();
    updateEditorGeometries();
    updateScrollBars();
    viewport()->update();
}

void TimelineView::movePlayheadToFrame(int frame)
{
    Model->setPlayheadPos(frame);
}

void TimelineView::updateViewport(){
    updateEditorGeometries();
    updateScrollBars();
    viewport()->update();
}
// 定时器启动
void TimelineView::timelinestart(){
    if(timer->isActive()){
        timer->stop();
    }else{
        timer->start(40);
    }
};

// 定时器超时信号槽
void TimelineView::onTimeout()
{
   
    int currentPos = Model->getPlayheadPos();
    int timelineLength = Model->data(QModelIndex(), TimelineRoles::TimelineLengthRole).toInt();
    
    // 检查是否到达时间轴末尾
    if (currentPos >= timelineLength) {
        // 停止定时器
        timer->stop();
        // 重置游标到零位
        Model->setPlayheadPos(0);
    } else {
        // 继续正常播放
        Model->setPlayheadPos(currentPos + 1);
    }
    
    viewport()->update();
}

// 滚动视图
void TimelineView::scroll(int dx, int dy){
    
    m_scrollOffset -= QPoint(dx, dy);
    QAbstractItemView::scrollContentsBy(dx, dy);
    
    updateViewport();
}
void TimelineView::horizontalScroll(double position)
{
    // position 是 0-1 之间的值，表示滚动条的相对位置
    int maxScroll = horizontalScrollBar()->maximum();
    int newScrollValue = static_cast<int>(maxScroll * position);
    
    // 设置滚动条位置
    horizontalScrollBar()->setValue(newScrollValue);
    
    // 更新编辑器位置
    updateEditorGeometries();
    viewport()->update();
}
// 更新滚动条
void TimelineView::updateScrollBars()
{
    if (!model())
        return;
    int max = 0;
    max = getTrackWdith() -  viewport()->width();
    horizontalScrollBar()->setRange(0, max);
    verticalScrollBar()->setRange(0, model()->rowCount() * trackHeight + rulerHeight - viewport()->height());

    // 更新ZoomController
    emit timelineInfoChanged(getTrackWdith(), viewport()->width(), m_scrollOffset.x());
}

void TimelineView::scrollContentsBy(int dx, int dy)
{
    m_scrollOffset -= QPoint(dx, dy);
    QAbstractItemView::scrollContentsBy(dx, dy);
    updateEditorGeometries();
    
    // 更新ZoomController
    emit timelineInfoChanged(getTrackWdith(), viewport()->width(), m_scrollOffset.x());
}

int TimelineView::pointToFrame(int point) const
{
    return point/timescale ;
}

int TimelineView::frameToPoint(int frame) const
{
    return frame*timescale;
}

void TimelineView::contextMenuEvent(QContextMenuEvent* event) {
    QModelIndex index = indexAt(event->pos());

    // Check if the index is a track (i.e., it has no parent)
    if (index.isValid() && !index.parent().isValid()) {
        QMenu contextMenu(this);

        QAction* addClipAction = new QAction("Add Item", this);
        connect(addClipAction, &QAction::triggered, [this, index, event]() {
            addClipAtPosition(index, event->pos());
        });

        contextMenu.addAction(addClipAction);
        contextMenu.exec(event->globalPos());
    }
    if(index.isValid()&&selectionModel()->isSelected(index)){
   
        QMenu contextMenu(this);

        QAction* deleteClipAction = new QAction("Delete Clip", this);
        connect(deleteClipAction, &QAction::triggered, [this, index]() {
            Model->deleteClip(index);
            selectionModel()->clearSelection();
            viewport()->update();
        });

        contextMenu.addAction(deleteClipAction);
        contextMenu.exec(event->globalPos());
        QAbstractItemView::contextMenuEvent(event);
    }
}


void TimelineView::addClipAtPosition(const QModelIndex& index, const QPoint& pos) {
    if (!index.isValid() || index.parent().isValid()) {
        qDebug() << "Invalid index: Click was not on a valid track";
        return;
    }

    int trackIndex = index.row();
    if (trackIndex < 0 || trackIndex >= Model->m_tracks.size()) {
        qDebug() << "Invalid track index: " << trackIndex;
        return;
    }

    // Calculate the start frame based on the mouse position
    int startFrame = pointToFrame(pos.x() + m_scrollOffset.x());
    
    Model->m_tracks[trackIndex]->addClip(startFrame,Model->m_pluginLoader);
    // 计算时间线长度
    Model->calculateLength();
    // 更新视图
    updateViewport();
}

void TimelineView::mouseReleaseEvent(QMouseEvent *event)
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

void TimelineView::mouseDoubleClickEvent(QMouseEvent *event)
{
    // QModelIndex index = indexAt(event->pos());
    QAbstractItemView::mouseDoubleClickEvent(event);
}

void TimelineView::leaveEvent(QEvent *event)
{
    mouseHeld = false;
    // selectionModel()->clear();
    m_hoverIndex = QModelIndex();
    QAbstractItemView::leaveEvent(event);
}

// 设置缩放比例
void TimelineView::setScale(double value)
{
    // 保存当前焦点帧位置
    int focusFrame = Model->getPlayheadPos();
    
    // 计算旧焦点在点坐标系中的位置
    int oldPointFocus = frameToPoint(focusFrame);
    // 设置新的缩放比例 (value 范围是 0-1)
    timescale = (value * 99 + 1) * baseTimeScale / 100;  // 将 0-1 映射到 5%-100% 的缩放范围
    // timescale = value * baseTimeScale;
    // 计算新的焦点位置
    timescale=qMax(1,timescale);
    int newPointFocus = frameToPoint(focusFrame);
    
    // 计算位移差异以保持焦点位置不变
    int diff = newPointFocus - oldPointFocus;
    
    // 调整滚动偏移以保持焦点位置
    if (m_scrollOffset.x() + diff >= 0) {
        scrollContentsBy(-diff, 0);
    } else {
        m_scrollOffset.setX(0);
    }
    
    // 更新界面
    updateEditorGeometries();
    updateScrollBars();
    viewport()->update();
}

void TimelineView::resizeEvent(QResizeEvent *event)
{
    updateScrollBars();
    updateEditorGeometries();
    QAbstractItemView::resizeEvent(event);
    
    // 更新ZoomController
    emit timelineInfoChanged(getTrackWdith(), viewport()->width(), m_scrollOffset.x());
}

void TimelineView::showEvent(QShowEvent *event)
{
    updateScrollBars();
    QAbstractItemView::showEvent(event);
}

void TimelineView::mousePressEvent(QMouseEvent *event) 
{
    if (event->button() == Qt::RightButton) {
        // 处理左键按下的情况
        QAbstractItemView::mousePressEvent(event);
        return;
    }
    m_mouseStart = event->pos();
    m_mouseEnd = m_mouseStart;
    mouseHeld = true;
    m_playheadSelected = false;
    m_hoverIndex = QModelIndex();

    selectionModel()->clearSelection();
    // 获取播放头位置   
    int playheadPos = frameToPoint(((TimelineModel*)model())->getPlayheadPos());
    // 获取播放头矩形
    QRect playheadHitBox(QPoint(playheadPos-3,rulerHeight),QPoint(playheadPos+2,viewport()->height()));
    // 获取播放头矩形2
    QRect playheadHitBox2(QPoint(playheadPos-playheadwidth,-playheadheight + rulerHeight),QPoint(playheadPos+playheadwidth,rulerHeight));
    // 如果鼠标点击在播放头矩形内
    if(playheadHitBox.contains(m_mouseStart)||playheadHitBox2.contains(m_mouseStart)){
        m_playheadSelected = true;
        return QAbstractItemView::mousePressEvent(event);
    }
    // 设置播放头未选中
    m_playheadSelected = false;
    // 获取鼠标点击的索引
    QModelIndex item = indexAt(event->pos());
    // 如果鼠标点击的索引是有效的
    //item pressed was a clip
    if(item.parent().isValid()){
        selectionModel()->select(item,QItemSelectionModel::Select);
        m_mouseOffset.setX(frameToPoint(item.data(TimelineRoles::ClipInRole).toInt()) - m_mouseStart.x());

    }
    // 如果选中的索引为空   
    if(selectionModel()->selectedIndexes().isEmpty()){
        // 移动播放头到指定帧
        movePlayheadToFrame(pointToFrame(std::max(0,m_mouseEnd.x()+m_scrollOffset.x())));
        // 更新视口
        viewport()->update();
    }

    QAbstractItemView::mousePressEvent(event);
}

void TimelineView::mouseMoveEvent(QMouseEvent *event)
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
                        Model->setData(clipIndex, newFrame, TimelineRoles::ClipInRole);
                        Model->calculateLength();
                        updateEditorGeometries();
                    }else if(m_mouseUnderClipEdge==hoverState::RIGHT){
                        int newFrame = pointToFrame(m_mouseEnd.x() + m_scrollOffset.x());
                        Model->setData(clipIndex, newFrame, TimelineRoles::ClipOutRole);
                        Model->calculateLength();
                        updateEditorGeometries();
                    }
                }
                viewport()->update();
            }else{
                movePlayheadToFrame(pointToFrame(std::max(0,m_mouseEnd.x() + m_scrollOffset.x())));
                viewport()->update();
            }

            if(m_playheadSelected){
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

void TimelineView::dragEnterEvent(QDragEnterEvent *event)
{


}

void TimelineView::dragMoveEvent(QDragMoveEvent *event) 
{
    if(m_isDroppingMedia){
        m_lastDragPos = event->position().toPoint();
        event->acceptProposedAction();
    }else{
        QAbstractItemView::dragMoveEvent(event);
    }
}

void TimelineView::dropEvent(QDropEvent *event) 
{
    if(m_isDroppingMedia){
        m_lastDragPos = event->position().toPoint();
        QModelIndex trackIndex;
        TimelineModel* timelineModel = ((TimelineModel*)model());
        QRect rullerRect(-m_scrollOffset.x(),0,viewport()->width() + m_scrollOffset.x(),rulerHeight);
        /* If above or on the ruler drop on the first track*/
        if(m_lastDragPos.y()<0 || rullerRect.contains(m_lastDragPos)){
            if(timelineModel->rowCount()>0)
                trackIndex = model()->index(0, 0);
        }else{
            /* Find track at drop point */
            for(int i = 0; i < timelineModel->rowCount(); i++){
                if (visualRect(timelineModel->index(i, 0)).contains(m_lastDragPos)){
                    trackIndex = timelineModel->index(i,0);
                }
            }
        }
        /* If dropped out side of tracks */
        if(!trackIndex.isValid()){
            trackIndex =QModelIndex();
        }
        int pos = pointToFrame(m_lastDragPos.x());


        viewport()->update();

    }else{
        QAbstractItemView::dropEvent(event);
    }
}

bool TimelineView::eventFilter(QObject *watched, QEvent *event)
    {
        
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);
            QModelIndexList list = selectionModel()->selectedIndexes();
            TimelineModel* timelinemodel = (TimelineModel*)model();
            if (watched == this) {
                switch (keyEvent->key()){
                    case Qt::Key_Right :
                        moveSelectedClip(1,0,false);
                        break;
                    case Qt::Key_Left:
                        moveSelectedClip(-1,0,false);
                        return true;  // 表示事件已处理，不再向其他控件传递
                    case Qt::Key_Delete:
                        if(list.isEmpty())
                            break;
                        timelinemodel->deleteClip(list[0]);
                        clearSelection();
                        return true;  // 表示事件已处理，不再向其他控件传递
                    defualt:
                        break;
                }

            }
        }
        return QObject::eventFilter(watched, event);
    }

void TimelineView::paintEvent(QPaintEvent *event) 
{
    
    // 获取可见区域
    int trackwidth = getTrackWdith();
    QPainter painter(viewport());
    painter.save();  // 保存状态
    QRect visibleRect = viewport()->rect();
    int visibleLeft = m_scrollOffset.x();
    int visibleRight = visibleLeft + visibleRect.width();

    // 计算可见的帧范围
    int startFrame = pointToFrame(visibleLeft);
    int endFrame = pointToFrame(visibleRight) + 1;

    // 1. 绘制背景
    painter.fillRect(visibleRect, bgColour);
    // 画轨道
    painter.setPen(seperatorColour);
    for (int i = 0; i < model()->rowCount(); ++i) {
        QModelIndex trackIndex = model()->index(i, 0);
        int trackSplitter = (i+1) * trackHeight + rulerHeight - m_scrollOffset.y();
        painter.drawLine(0, trackSplitter, event->rect().width(), trackSplitter);
    }

        // 画垂直线
        int lineheight = model()->rowCount() * trackHeight + rulerHeight;
        int frameStep;

        if (timescale >= 25) {
            frameStep = 1;// Draw text at every frame
        } else if (timescale > 15) {
            frameStep = 5;  // Draw text every 10 frames
        } else if (timescale > 5) {
            frameStep = 15; // Draw text every 25 frames
        } else {
            frameStep = 30; // Draw text every 50 frames
        }

        int startMarker = static_cast<int>(pointToFrame(m_scrollOffset.x()))*timescale+1 ;
        int endMarker = event->rect().width() + m_scrollOffset.x();
        startMarker = pointToFrame(startMarker);
        startMarker-= (startMarker%frameStep);
        startMarker = frameToPoint(startMarker);

        for (int i = startMarker; i<endMarker; i += timescale*frameStep)
        {
            painter.drawLine(i - m_scrollOffset.x(), std::max(rulerHeight, event->rect().top()),
                             i - m_scrollOffset.x(), lineheight);
        }

    for (int i = 0; i < model()->rowCount(); ++i) {
        QModelIndex trackIndex = model()->index(i, 0);
       
        for (int j = 0; j < model()->rowCount(trackIndex); ++j) {

            QModelIndex clipIndex = model()->index(j, 0, trackIndex);
           // 设置画笔和画刷
            if (selectionModel()->isSelected(clipIndex)) {
                painter.setBrush(ClipSelectedColor);
                
            } else if (m_hoverIndex == clipIndex) {
                painter.setBrush(ClipHoverColor);
               
            } else {
                painter.setBrush(ClipColor);
               
            }            
            QStyleOptionViewItem option;
            option.rect = visualRect(clipIndex);
            option.state = QStyle::State_Selected | QStyle::State_MouseOver;

            // 使用圆角矩形绘制clip
            if(clipIndex.data(TimelineRoles::ClipShowBorderRole).toBool()){
                painter.setPen(QPen(ClipBorderColour, ClipBorderWidth));
                QRect clipRect = visualRect(clipIndex).adjusted(0, clipoffset, 0, -clipoffset);
                painter.drawRoundedRect(clipRect, clipround, clipround);  // 设置水平和垂直圆角半径为5像素
            }
            if (!indexWidget(clipIndex)&&clipIndex.data(TimelineRoles::ClipShowWidgetRole).toBool()) {
                
                openPersistentEditor(clipIndex);
            }
            else{
                // closePersistentEditor(clipIndex);
                itemDelegateForIndex(clipIndex)->paint(&painter, option, clipIndex);
            }
           
        }
    }
        // 画时间轴
        painter.setPen(rulerColour);
        painter.setBrush(QBrush(bgColour));
        painter.drawRect(-m_scrollOffset.x(),0,event->rect().width() + m_scrollOffset.x(),rulerHeight);

        
        static int jump = 1;
        if(  baseTimeScale%timescale == 0){
            jump =  baseTimeScale/timescale;
        }

        for(int i = startMarker;i < endMarker; i+=timescale*frameStep){

            int number = pointToFrame(i);
            QString text     = tr("%1").arg(number);
            QRect   textRect = painter.fontMetrics().boundingRect(text);

            textRect.translate(-m_scrollOffset.x(), 0);
            textRect.translate(i - textRect.width() / 2, rulerHeight - textoffset);

        
            painter.drawLine(i  - m_scrollOffset.x(),textRect.bottom(),
                             i  - m_scrollOffset.x(),rulerHeight);

            painter.drawText(textRect, text);
        }

//         // 画中间线
//         for(int i = startMarker;i < endMarker; i+=timescale){
//             int number = pointToFrame(i);
//             int boost = 0 ;

//             if (timescale > 20) {
//                 break;  // Draw text at every frame
//             } else if (timescale > 10) {
//                 if(number%5==0)
//                     continue;// skip every 5th marker

//             } else if (timescale > 5) {
//                 if(number%10==0)
//                     continue; //skip every 10th marker

//                 if(number%5==0)
//                     boost =5; //hilight every 5th
//             } else {
//                 if(number%25==0)
//                     continue; //skip every 25th marker

//                 if(number%5==0)
//                     boost =5; //hilight every 5th

//             }
// //
//             painter.drawLine(i  - m_scrollOffset.x(),rulerHeight - textoffset +10 -boost ,
//                              i  - m_scrollOffset.x(),rulerHeight);
//         }

//        画时间轴游标

        QPoint kite[5]{
                QPoint(0,0),QPoint(-playheadwidth,-playheadCornerHeight),QPoint(-playheadwidth,-playheadheight),QPoint(playheadwidth,-playheadheight),QPoint(playheadwidth,-playheadCornerHeight)
        };
        int playheadPos = frameToPoint(Model->getPlayheadPos()) -m_scrollOffset.x();
        for(QPoint &p:kite){
            p.setX(p.x()+playheadPos);
            p.setY(p.y()+rulerHeight);
        }

        // 设置画笔和画刷颜色
        painter.setPen(playheadColour);
        painter.setBrush(playheadColour);  // 设置填充颜色

        // 绘制播放头三角形
        painter.drawConvexPolygon(kite,5);

        // 绘制播放头竖线
        painter.drawLine(QPoint(playheadPos,rulerHeight),QPoint(playheadPos,viewport()->height()));
        painter.restore();  // 恢复状态
    }

void TimelineView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected)
{
    QAbstractItemView::selectionChanged(selected,deselected);
    viewport()->update();
}

QAbstractItemDelegate* TimelineView::itemDelegateForIndex(const QModelIndex &index) const {
        if (!index.isValid() || !index.parent().isValid()) {
            return QAbstractItemView::itemDelegateForIndex(index);
        }

        QString clipType = index.data(TimelineRoles::ClipTypeRole).toString();
        return Model->m_pluginLoader->createDelegateForType(clipType);
    }

void TimelineView::wheelEvent(QWheelEvent *event){
        if (event->modifiers() & Qt::ControlModifier) {
            // 缩放操作，以鼠标位置为中心
            QPoint pos = event->position().toPoint();
            double delta = event->angleDelta().y() / 120.0; // 标准滚轮步长
            double scaleChange = (delta > 0 ? 1.1 : 0.9);
            currentScale = currentScale*scaleChange;
            currentScale=qBound(0.01,currentScale,1.0);
            setScale(currentScale);
        } else {
            // 水平滚动
            QPoint numPixels = event->angleDelta();
            int dx = -numPixels.y() / 2; // 减小滚动速度
            
            // 计算新的滚动位置
            int newX = m_scrollOffset.x() + dx;
            
            // 限制滚动范围
            int maxScroll = qMax(0, getTrackWdith() - viewport()->width());
            newX = qBound(0, newX, maxScroll);
            
            // 应用新的滚动位置
            if (newX != m_scrollOffset.x()) {
                m_scrollOffset.setX(newX);
                updateEditorGeometries();
                viewport()->update();
                emit scrolled(dx, 0);
            }
        }
        event->accept();
    }