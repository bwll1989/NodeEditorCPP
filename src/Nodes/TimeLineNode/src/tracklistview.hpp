#ifndef TRACKLISTVIEW_H
#define TRACKLISTVIEW_H

#include <QAbstractItemView>

#include "trackdelegate.hpp"
#include <QEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollBar>
#include <QDrag>
#include "timelinemodel.hpp"
#include "timelinestyle.hpp"
#include <QTimer>
class TracklistView : public QAbstractItemView
{
    Q_OBJECT
public:
    explicit TracklistView(QWidget *parent = nullptr): QAbstractItemView{parent}{
        horizontalScrollBar()->setSingleStep(10);
        horizontalScrollBar()->setPageStep(100);
        verticalScrollBar()->setSingleStep(trackHeight);
        verticalScrollBar()->setPageStep(trackHeight * 5);
        viewport()->setMinimumHeight(trackHeight + rulerHeight);
        setAcceptDrops(true);
        setDragEnabled(true);
        setDragDropMode( QAbstractItemView::InternalMove );
        setDefaultDropAction( Qt::MoveAction );
        setDropIndicatorShown(true);
        setSelectionMode(SingleSelection);
        setSelectionBehavior(SelectItems);
        m_scrollOffset = QPoint(0,0);

        setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
        setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff ) ;

        setEditTriggers(EditTrigger::AllEditTriggers);
        setItemDelegateForColumn(0,new TrackDelegate);

        setMouseTracking(true);

    }

    ~TracklistView(){}

    void updateScrollBars(){
        if(!model())
            return;

        int max =0;


        verticalScrollBar()->setRange(0,model()->rowCount() * trackHeight + rulerHeight - viewport()->height());
    }

    void setModel(QAbstractItemModel *model) override{
        QAbstractItemView::setModel(model);
    }

    QRect visualRect(const QModelIndex &index) const override{
        return itemRect(index).translated(-m_scrollOffset);
    }

    void scrollTo(const QModelIndex &index, ScrollHint hint) override{}

    QModelIndex indexAt(const QPoint &point) const override{
        QModelIndex index;
        QModelIndex parent;

        QRect rullerRect(0,0,viewport()->width(),rulerHeight);
        if(point.y()<0)//if above the ruler
            return index;
        if(rullerRect.contains(point)){
            return index;
        }


        int columnIndex = model()->columnCount()-1;
        for(int i = 0; i < model()->rowCount(); i++){

            if (visualRect(model()->index(i, columnIndex,QModelIndex())).contains(point))
            {
                index = model()->index(i,columnIndex,QModelIndex());
                return index;
            }
        }
        index = ((TimelineModel*)model())->createFakeIndex();
        return index;
    }

    QString convertFramesToTimeString(int frames, double fps)
    {
        int totalMilliseconds = static_cast<int>((frames / fps) * 1000);
        int hours = totalMilliseconds / (1000 * 60 * 60);
        int minutes = (totalMilliseconds / (1000 * 60)) % 60;
        int seconds = (totalMilliseconds / 1000) % 60;
        int milliseconds = totalMilliseconds % 1000;

        return QString("%1:%2:%3.%4")
                .arg(hours, 2, 10, QChar('0'))
                .arg(minutes, 2, 10, QChar('0'))
                .arg(seconds, 2, 10, QChar('0'))
                .arg(milliseconds, 3, 10, QChar('0'));
    }

signals:
    void scrolled(int dx,int dy);

public slots:

    void scroll(int dx, int dy){
        m_scrollOffset -= QPoint(0, dy);
        updateEditorGeometries();
        QAbstractItemView::scrollContentsBy(dx, dy);
    }

    void setTime(int frame){
        m_time=frame;
        viewport()->update();
    }

    void updateViewport(){
        viewport()->update();
    }


protected:
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override {return QModelIndex();}

    int horizontalOffset() const override {return 0;}

    int verticalOffset() const override {return 0;}

    bool isIndexHidden(const QModelIndex &index) const override { return false; }

    void setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command) override{}

    QRegion visualRegionForSelection(const QItemSelection &selection) const override {return QRegion();}

    void paintEvent(QPaintEvent *event) override{
        QPainter painter(viewport());
        painter.setRenderHint(QPainter::Antialiasing,true);

        painter.setBrush(QBrush(bgColour));
        painter.drawRect(event->rect());

        painter.setBrush(QBrush(rulerColour));
        //draw tracklist

        int viewportWidth = viewport()->width();

        // QRect trackRect;

        QStyleOptionViewItem option;
        QAbstractItemView::initViewItemOption(&option);
        for (int i = 0; i < model()->rowCount(); ++i){
            QModelIndex trackIndex = model()->index(i,0);
            openPersistentEditor(model()->index(i,0));
            if(selectionModel()->isSelected(trackIndex)){
                option.state |= QStyle::State_Selected;
            }else{
                option.state &= ~QStyle::State_Selected;
            }
            if(m_hoverIndex==trackIndex && !selectionModel()->isSelected(trackIndex)){
                option.state |= QStyle::State_MouseOver;
                if(isDragging){
                    option.state |= QStyle::State_Raised;
                }
            }else{
                option.state &= ~QStyle::State_MouseOver;
                if(!isDragging){
                    option.state &=~QStyle::State_Raised;
                }
            }



            option.text = QString("" + QString::number(model()->data(model()->index(i,0),TimelineRoles::TrackNumberRole).toInt()));
            option.rect = visualRect(trackIndex);

            itemDelegateForIndex(trackIndex)->paint(&painter,option,trackIndex);


        }



        painter.setBrush(QBrush(bgColour));
        painter.drawRect(0,0,viewportWidth,rulerHeight);
        QRect ruler(0,  0, viewport()->width(), rulerHeight);
        painter.setPen(Qt::white);
        QFont font;
        font.setPixelSize(20);
        painter.setFont(font);
//        painter.drawText(ruler,Qt::AlignCenter,QString::number(m_time));
        painter.drawText(ruler,Qt::AlignCenter, convertFramesToTimeString(m_time,fps));


    }

    void mousePressEvent(QMouseEvent *event) override{
        m_mouseStart = event->pos();
        m_mouseEnd = m_mouseStart;
        m_mouseHeld = true;
        m_hoverIndex = QModelIndex();
        selectionModel()->clear();
        QModelIndex item = indexAt( event->pos());

        if(item.isValid() && model()->hasIndex(item.row(),item.column(),item.parent())){
            selectionModel()->select(item,QItemSelectionModel::Select);

        }

        QAbstractItemView::mousePressEvent(event);
    }

    void mouseReleaseEvent(QMouseEvent *event) override{
        m_mouseHeld = false;
        isDragging = false;
        m_mouseEnd = event->pos();


        QAbstractItemView::mouseReleaseEvent(event);
    }

    void mouseMoveEvent(QMouseEvent *event) override{
        m_mouseEnd = event->pos();
        m_hoverIndex = indexAt(m_mouseEnd);
        if(!selectionModel()->selectedIndexes().isEmpty() && m_mouseHeld){
            QDrag* drag = new QDrag(this);
            QMimeData *mimeData = model()->mimeData(selectionModel()->selectedIndexes());
            drag->setMimeData(mimeData);
            drag->exec(Qt::MoveAction);
            isDragging = true;
        }

        QAbstractItemView::mouseMoveEvent(event);
        m_hoverIndex = indexAt(m_mouseEnd);
        viewport()->update();
    }

    void leaveEvent(QEvent *event) override{
        m_mouseHeld = false;
        selectionModel()->clear();
        m_hoverIndex = QModelIndex();
        QAbstractItemView::leaveEvent(event);
    }

    void dropEvent(QDropEvent *event) override{
        m_hoverIndex = indexAt(m_mouseEnd);
        QModelIndex index = indexAt(event->position().toPoint());
        if (dragDropMode() == InternalMove) {
            if (event->source() != this || !(event->possibleActions() & Qt::MoveAction))
                return;
        }


        const Qt::DropAction action = dragDropMode() == InternalMove ? Qt::MoveAction : event->dropAction();
        if (model()->dropMimeData(event->mimeData(), action, index.row(), index.column(), index)) {
            if (action != event->dropAction()) {
                event->setDropAction(action);
                event->accept();
            } else {
                event->acceptProposedAction();
            }
        }

        stopAutoScroll();
        setState(NoState);
        viewport()->update();
        selectionModel()->clear();
        isDragging = false;
        m_mouseHeld = false;


    }

    void dragMoveEvent(QDragMoveEvent *event) override{
        m_mouseEnd = event->position().toPoint();
        m_hoverIndex = indexAt(m_mouseEnd);
        if (event->source() != this || !(event->possibleActions() & Qt::MoveAction))
            return;


        QAbstractItemView::dragMoveEvent(event);
    }

    void resizeEvent(QResizeEvent *event) override{
        updateScrollBars();
        QAbstractItemView::resizeEvent(event);
    }

    void showEvent(QShowEvent *event) override{
        updateScrollBars();
        QAbstractItemView::showEvent(event);
    }

    void scrollContentsBy(int dx, int dy) override{
        m_scrollOffset -= QPoint(dx, dy);
        QAbstractItemView::scrollContentsBy(dx, dy);
        updateEditorGeometries();

        emit scrolled(dx,dy);
    }

    QRect itemRect(const QModelIndex &index) const{
        if(!index.isValid())
            return QRect();
        return QRect(0, (index.row() * trackHeight) + rulerHeight, viewport()->width(), trackHeight);
    };

protected slots:

    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override{
        QAbstractItemView::selectionChanged(selected,deselected);
        viewport()->update();
    };

    void updateEditorGeometries() override{
        QAbstractItemView::updateEditorGeometries();

        for (int i = 0; i < model()->rowCount(); ++i){
            QModelIndex trackIndex = model()->index(i,0);

            QWidget *editor = indexWidget(trackIndex);
            if(!editor){
                continue;
            }

            QRect rect  = editor->rect();
            QPoint topInView = editor->mapToParent(rect.topLeft());
            if(topInView.y()<40){
                int offset = 40-topInView.y();
                editor->setMask(QRegion(0,offset,editor->width(),editor->height()));
            }

        }
}

private:

    TrackDelegate delegate;

    int m_time = 0;

    QPoint m_scrollOffset;

    QPoint m_mouseStart;

    QPoint m_mouseEnd;

    QPoint m_mouseOffset;

    bool m_mouseHeld = false;

    QItemSelectionModel* m_selectionmodel;

    QModelIndex m_hoverIndex = QModelIndex();

    bool isDragging = false;

    //QDrag* drag = nullptr;
};

#endif // TRACKLISTVIEW_H
