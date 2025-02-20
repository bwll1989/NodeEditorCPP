#ifndef TRACKLISTVIEW_HPP
#define TRACKLISTVIEW_HPP

#include <QAbstractItemView>
#include <QMenu>
#include "trackdelegate.hpp"
#include <QEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollBar>
#include <QDrag>
#include <QApplication>
#include "timelinemodel.hpp"
#include "timelinestyle.hpp"
#include <QTimer>
#include <QMessageBox>

class TracklistView : public QAbstractItemView
{
    Q_OBJECT
public:
    explicit TracklistView(TimelineModel *viemModel,QWidget *parent = nullptr): Model(viemModel),QAbstractItemView{parent}{
        setModel(Model);
        setSelectionMode(QAbstractItemView::SingleSelection);
        setSelectionBehavior(QAbstractItemView::SelectRows);
        setSelectionModel(new QItemSelectionModel(Model, this));
        horizontalScrollBar()->setSingleStep(10);
        horizontalScrollBar()->setPageStep(100);
        verticalScrollBar()->setSingleStep(trackHeight);
        verticalScrollBar()->setPageStep(trackHeight );
        viewport()->setMinimumHeight(trackHeight + rulerHeight+zoomHeight);
        setAcceptDrops(true);
        setDragEnabled(true);
        setDragDropMode(QAbstractItemView::InternalMove);
        setDefaultDropAction(Qt::MoveAction);
        setDropIndicatorShown(true);
        m_scrollOffset = QPoint(0,0);
        
        // Create delete track action
        m_deleteTrackAction = new QAction("Delete track", this);
        m_deleteTrackAction->setShortcut(QKeySequence(Qt::Key_Delete));
        QObject::connect(m_deleteTrackAction, &QAction::triggered, this, &TracklistView::onDeleteTrack);
        QObject::connect(Model, &TimelineModel::tracksChanged, this, &TracklistView::updateViewport);

        setItemDelegate(new TrackDelegate(this));
        setMouseTracking(true);
        m_scrollOffset = QPoint(0,0);
        setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded ) ;
        setVerticalScrollBarPolicy( Qt::ScrollBarAsNeeded) ;
        installEventFilter(this);
    }

    ~TracklistView(){}

    void updateScrollBars();

    QRect visualRect(const QModelIndex &index) const override;
    
    void scrollTo(const QModelIndex &index, ScrollHint hint) override{}

    QModelIndex indexAt(const QPoint &point) const override;


signals:
    void scrolled(int dx,int dy);
    void viewupdate();

public slots:
    // void onDeleteTrack();
    void scroll(int dx, int dy);

    void setTime(int frame);

    void updateViewport();
    
protected:
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override {return QModelIndex();}

    int horizontalOffset() const override {return 0;}

    int verticalOffset() const override {return 0;}

    bool isIndexHidden(const QModelIndex &index) const override { return false; }
/**
 * @brief 设置选择
 */
    void setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command) override ;
/**
 * @brief 获取选择区域
 */
    QRegion visualRegionForSelection(const QItemSelection &selection) const override {return QRegion();}

    void paintEvent(QPaintEvent *event) override;
    void mousePressEvent(QMouseEvent *event) override;

    // void mouseReleaseEvent(QMouseEvent *event) override;
    void mouseMoveEvent(QMouseEvent *event) override;

    void leaveEvent(QEvent *event) override;

    void resizeEvent(QResizeEvent *event) override;

    void showEvent(QShowEvent *event) override;

    void scrollContentsBy(int dx, int dy) override;

    QRect itemRect(const QModelIndex &index) const;

    bool eventFilter(QObject *watched, QEvent *event) override;

    // 重载 contextMenuEvent 事件
    void contextMenuEvent(QContextMenuEvent* event) override ;
protected slots:
    void onDeleteTrack();
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;
    void updateEditorGeometries() override;
    void dragEnterEvent(QDragEnterEvent *event) override;
    void dragMoveEvent(QDragMoveEvent *event) override;
    void dropEvent(QDropEvent *event) override;

public:
    QAction* m_deleteTrackAction;
private:
    int m_time = 0;
    //滚动偏移  
    QPoint m_scrollOffset;
    //拖动开始位置
    QPoint m_dragStartPosition;
    //悬停索引
    QModelIndex m_hoverIndex = QModelIndex();
    //模型
    TimelineModel *Model;
};

#endif // TRACKLISTVIEW_HPP
