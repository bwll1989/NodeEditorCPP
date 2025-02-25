#include "timelinemodel.hpp"
#include "tracklistview.hpp"
#include <QDrag>
#include <QMimeData>
/**
 * @brief 删除轨道
 */
void TracklistView::onDeleteTrack() {
    if (Model&&selectionModel()->selectedIndexes().size()>=1) {
        Model->deleteTrack(selectionModel()->selectedIndexes().first().row());
    }
    viewport()->update();
    emit viewupdate();  // 发送信号
}
/**
 * @brief 上下文菜单事件
 */
void TracklistView::contextMenuEvent(QContextMenuEvent* event) {
        QMenu contextMenu(this);

        // Add track creation submenu
        QMenu* addTrackMenu = contextMenu.addMenu("Add Track");
        
        // Get available track types from PluginLoader
        QStringList availableTypes = Model->m_pluginLoader->getAvailableTypes();
        for (const QString& type : availableTypes) {
            QAction* addTrackAction = addTrackMenu->addAction("Add " + type + " Track");
            connect(addTrackAction, &QAction::triggered, [this, type]() {
                Model->createTrackForType(type);
            });
        }

        // Add delete track option if a track is selected
        QModelIndex index = indexAt(event->pos());
        if (index.isValid()) {
            contextMenu.addAction(m_deleteTrackAction);
        }

        contextMenu.exec(event->globalPos());
    }
    
/**
 * @brief 鼠标按下事件
 */
void TracklistView::mousePressEvent(QMouseEvent *event) {
    if (event->button() == Qt::LeftButton) {
        m_dragStartPosition = event->pos();
        QModelIndex item = indexAt(event->pos());
        if (item.isValid()) {
            selectionModel()->select(item, QItemSelectionModel::ClearAndSelect);
        } 
        viewport()->update();
    }
    
    QAbstractItemView::mousePressEvent(event);
}
/**
 * @brief 获取某一点的轨道索引
 */
QModelIndex TracklistView::indexAt(const QPoint &point) const {
    // Check if the point is above the ruler or in a blank area
    QRect rulerRect(0, 0, viewport()->width(), rulerHeight+toolbarHeight);
    if (point.y() < 0 || rulerRect.contains(point)) {
        return QModelIndex(); // Return an invalid index
    }

    // Iterate over the rows to find the index at the given point
    for (int i = 0; i < model()->rowCount(); ++i) {
        QModelIndex index = model()->index(i, 0);
        if (visualRect(index).contains(point)) {
            // qDebug() << "index"<<index.row()<<index.column()<<index.parent().row();
            return index; // Return the valid index if found
        }
    }

    // If no valid index is found, return an invalid QModelIndex
    return QModelIndex();
}

/**
 * @brief 选择改变
 */
void TracklistView::selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) {
    QAbstractItemView::selectionChanged(selected, deselected);
    // for (const QModelIndex &index : selected.indexes()) {
    //     qDebug() << index.row();
    // }
    viewport()->update();
}

/**
 * @brief 鼠标移动事件
 */
void TracklistView::mouseMoveEvent(QMouseEvent *event) {
    // 如果不是左键拖动，只更新悬停状态
    if (!(event->buttons() & Qt::LeftButton)) {
        m_hoverIndex = indexAt(event->pos());
        viewport()->update();
        return;
    }
    
    // 检查是否达到拖动启动距离
    if ((event->pos() - m_dragStartPosition).manhattanLength() < QApplication::startDragDistance()) {
        return;
    }

    // 获取拖动起始位置的轨道索引
    QModelIndex index = indexAt(m_dragStartPosition);
    if (!index.isValid()) {
        return;
    }

    // 创建拖动对象
    QDrag *drag = new QDrag(this);
    QMimeData *mimeData = new QMimeData;
    
    // 存储源轨道索引
    mimeData->setData("application/x-track-index", QByteArray::number(index.row()));
    drag->setMimeData(mimeData);

    // 开始拖动操作
    Qt::DropAction dropAction = drag->exec(Qt::MoveAction);
}

/**
 * @brief 离开事件
 */
void TracklistView::leaveEvent(QEvent *event) {
    // selectionModel()->clear();
    m_hoverIndex = QModelIndex();
    QAbstractItemView::leaveEvent(event);
}
/**
 * @brief 大小改变事件
 */
void TracklistView::resizeEvent(QResizeEvent *event) {
    updateScrollBars();
    QAbstractItemView::resizeEvent(event);
}

/**
 * @brief 显示事件
 */
void TracklistView::showEvent(QShowEvent *event) {
    updateScrollBars();
    QAbstractItemView::showEvent(event);
}

/**
 * @brief 滚动事件
 */
void TracklistView::scrollContentsBy(int dx, int dy) {
    m_scrollOffset -= QPoint(dx, dy);
    QAbstractItemView::scrollContentsBy(dx, dy);
    updateEditorGeometries();
    updateViewport();
    emit scrolled(dx,dy);
}

/**
 * @brief 滚动事件
 */
void TracklistView::scroll(int dx, int dy){
    m_scrollOffset -= QPoint(0, dy);
    updateEditorGeometries();
    QAbstractItemView::scrollContentsBy(dx, dy);
}

/**
 * @brief 设置时间
 */
void TracklistView::setTime(int frame){
    m_time=frame;
    viewport()->update();
}

/**
 * @brief 更新视图
 */
void TracklistView::updateViewport(){
    
    updateScrollBars();
    viewport()->update();
}

/**
 * @brief 更新滚动条
 */
void TracklistView::updateScrollBars(){
    if(!model())
        return;
    int max =0;
    verticalScrollBar()->setRange(0,model()->rowCount() * trackHeight + rulerHeight+toolbarHeight - viewport()->height());
}

/**
 * @brief 获取视图矩形
 */
QRect TracklistView::visualRect(const QModelIndex &index) const{
    return itemRect(index).translated(-m_scrollOffset);
}
/**
 * @brief 获取项目矩形
 */
QRect TracklistView::itemRect(const QModelIndex &index) const{
    if(!index.isValid())
        return QRect();
    return QRect(0, (index.row() * trackHeight) + rulerHeight+toolbarHeight, viewport()->width(), trackHeight);
};
/**
 * @brief 更新编辑几何
 */
void TracklistView::updateEditorGeometries() {
    QAbstractItemView::updateEditorGeometries();

    for (int i = 0; i < model()->rowCount(); ++i){
        QModelIndex trackIndex = model()->index(i,0);

        QWidget *editor = indexWidget(trackIndex);
        if(!editor){
            continue;
        }

        QRect rect  = editor->rect();
        QPoint topInView = editor->mapToParent(rect.topLeft());
        if(topInView.y()<rulerHeight+toolbarHeight){
            int offset = rulerHeight+toolbarHeight-topInView.y();
            editor->setMask(QRegion(0,offset,editor->width(),rulerHeight+toolbarHeight));
        }

    }
}
/**
 * @brief 事件过滤
 */
bool TracklistView::eventFilter(QObject *watched, QEvent *event)
{

    if (event->type() == QEvent::KeyPress) {
        QKeyEvent *keyEvent = static_cast<QKeyEvent*>(event);

        if (watched == this) {
            // 处理 view1 的 keyPressEvent
            // 注意：可以添加其他逻辑来处理特定的键盘输入
            
            if(keyEvent->key()==Qt::Key_Delete){
                onDeleteTrack();
            }
            return true;  // 表示事件已处理，不再向其他控件传递
        }
    }
    return QObject::eventFilter(watched, event);
}
/**
 * @brief 设置选择
 */
void TracklistView::setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command) { 
    if (!model() || !selectionModel())
    return;

    // 将视图坐标转换为模型索引围
    QModelIndex topLeft = indexAt(rect.topLeft());
    QModelIndex bottomRight = indexAt(rect.bottomRight());

    if (!topLeft.isValid() || !bottomRight.isValid())
        return;

    // 创建选择范围
    QItemSelection selection(topLeft, bottomRight);

    // 更新选择模型
    selectionModel()->select(selection, command);
}
/**
 * @brief 绘制事件
 */
void TracklistView::paintEvent(QPaintEvent *event) {
    QPainter painter(viewport());
    painter.save();  // 保存状态
    painter.setRenderHint(QPainter::Antialiasing,true);
    painter.setBrush(QBrush(bgColour));
    painter.drawRect(event->rect());
    painter.setBrush(QBrush(rulerColour));
    // Draw tracklist
    int viewportWidth = viewport()->width();
    QStyleOptionViewItem option;
    QAbstractItemView::initViewItemOption(&option);
    
    for (int i = 0; i < model()->rowCount(); ++i){
        QModelIndex trackIndex = model()->index(i,0);
        openPersistentEditor(model()->index(i,0));
        if(selectionModel()->isSelected(trackIndex)){
           painter.setBrush(QBrush(trackSelectedColour));
        }else
        if(m_hoverIndex==trackIndex && !selectionModel()->isSelected(trackIndex)){
            painter.setBrush(QBrush(trackHoverColour));
        }
        else{
            painter.setBrush(QBrush(trackColour));
        }
        painter.drawRect(visualRect(trackIndex));
        itemDelegateForIndex(trackIndex)->paint(&painter,option,trackIndex);
        
        
    }

    // Draw the ruler and time display separately
    painter.setBrush(QBrush(bgColour));
    painter.drawRect(0,0,viewportWidth,rulerHeight+toolbarHeight);
        
    QRect ruler(0,  0, viewport()->width(), rulerHeight+toolbarHeight);
    painter.setPen(fontColor);
    QFont font;
    font.setPixelSize(fontSize);
    painter.setFont(font);
    painter.drawText(ruler,Qt::AlignCenter, FramesToTimeString(m_time,fps));
    painter.restore();  // 恢复状态
}

void TracklistView::dragEnterEvent(QDragEnterEvent *event) {
    // 检查是否是我们的自定义MIME类型
    if (event->mimeData()->hasFormat("application/x-track-index")) {
        event->acceptProposedAction();
    }
}

void TracklistView::dragMoveEvent(QDragMoveEvent *event) {
    if (event->mimeData()->hasFormat("application/x-track-index")) {
        event->acceptProposedAction();
        
        // 获取当前拖动位置的轨道索引
        QModelIndex dropIndex = indexAt(event->position().toPoint());
        if (dropIndex.isValid()) {
            // 只高亮显示目标位置，不进行选择
            m_hoverIndex = dropIndex;
            viewport()->update();
        }
    }
}

void TracklistView::dropEvent(QDropEvent *event) {
    if (event->mimeData()->hasFormat("application/x-track-index")) {
        // 获取源轨道和目标轨道的索引
        QByteArray data = event->mimeData()->data("application/x-track-index");
        int sourceRow = data.toInt();
        int targetRow = indexAt(event->position().toPoint()).row();

        // 只在源和目标不同且都有效时进行移动
        if (sourceRow != targetRow && targetRow >= 0 && sourceRow >= 0) {
            TimelineModel* timelineModel = qobject_cast<TimelineModel*>(model());
            if (timelineModel) {
                // 关闭所有持久化编辑器
                for (int i = 0; i < timelineModel->rowCount(); ++i) {
                    QModelIndex index = timelineModel->index(i, 0);
                    closePersistentEditor(index);
                }

                // 移动轨道
                timelineModel->m_tracks.move(sourceRow, targetRow);
                
                // 更新所有轨道的索引
                for (int i = 0; i < timelineModel->m_tracks.size(); ++i) {
                    timelineModel->m_tracks[i]->setTrackIndex(i);
                }

                // 清除选择和悬停状态
                selectionModel()->clearSelection();
                m_hoverIndex = QModelIndex();

                // 发送信号通知更新
                emit timelineModel->tracksChanged();
                emit timelineModel->trackMoved(sourceRow, targetRow);
                
                // 重新打开所有持久化编辑器
                for (int i = 0; i < timelineModel->rowCount(); ++i) {
                    QModelIndex index = timelineModel->index(i, 0);
                    openPersistentEditor(index);
                }

                // 更新编辑器几何形状和视图
                updateEditorGeometries();
                viewport()->update();

                emit viewupdate();  // 发送信号
            }
        }
        event->acceptProposedAction();
    }
}