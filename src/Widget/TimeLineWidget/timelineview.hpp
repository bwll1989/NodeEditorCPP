#ifndef TIMELINEVIEW_HPP
#define TIMELINEVIEW_HPP

#include <QAbstractItemView>
//#include "mediaclips/meidaclipdelegate.hpp"
#include <unordered_map>
#include "timelinetypes.h"
#include <QEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollBar>
#include <QFileInfo>
#include <QMimeData>
#include "timelinemodel.hpp"
#include "timelinestyle.hpp"
#include "trackdelegate.hpp"
#include "timelinetoolbar.hpp"
//#include "mediaclips/mediaclipmodel.hpp"
#include "QTimer"
#include "QMenu"
// #include "zoomcontroller.hpp"
#include "AbstractClipDelegate.hpp"
#include "videoplayerwidget.hpp"
// #include "pluginloader.hpp"
// TimelineView类继承自QAbstractItemView
class TimelineView : public QAbstractItemView
{
Q_OBJECT
public:
    // 构造函数，初始化视图模型
    explicit TimelineView(TimelineModel *viewModel, QWidget *parent = nullptr);
    ~TimelineView() override ;
    // 返回给定索引的可视矩形
    QRect visualRect(const QModelIndex &index) const override;
    // 滚动到指定索引
    void scrollTo(const QModelIndex &index, ScrollHint hint) override{};

    // 返回给定点的索引
    QModelIndex indexAt(const QPoint &point) const override;

    TimelineToolbar* toolbar;

    QTimer *timer = new QTimer(this);

signals:
    void scrolled(int dx,int dy);
    void timelineInfoChanged(int totalWidth, int viewportWidth, int scrollPosition);
    void videoWindowClosed();
    void currentClipChanged(AbstractClipModel* clip);
public slots:
    void showVideoWindow(bool show = true);
    // 更新可视区域
    void updateViewport();
    // 定时器启动
    void timelinestart();
    // 定时器超时信号槽
    void onTimeout();
    // 滚动视图
    void scroll(int dx, int dy);
    // 水平滚动
    void horizontalScroll(double dx);
    // 更新滚动条
    void updateScrollBars();

    void scrollContentsBy(int dx, int dy) override;

    void setScale(double value);

    // 在指定位置添加剪辑
    void addClipAtPosition(const QModelIndex& index, const QPoint& pos);

protected:
    QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override {return QModelIndex();}

    int horizontalOffset() const override{return 0;}

    int verticalOffset() const override{return 0;}

    bool isIndexHidden(const QModelIndex &index) const override{ return false; }

    void setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command) override{}

    QRegion visualRegionForSelection(const QItemSelection &selection) const override {return QRegion();}

    void paintEvent(QPaintEvent *event) override;

    void mousePressEvent(QMouseEvent *event) override;

    void mouseMoveEvent(QMouseEvent *event) override;

    void mouseReleaseEvent(QMouseEvent *event) override;
    
    void mouseDoubleClickEvent(QMouseEvent *event) override;
   
    void leaveEvent(QEvent *event) override;

    // bool eventFilter(QObject *watched, QEvent *event) override;

    void contextMenuEvent(QContextMenuEvent* event) override;

    void resizeEvent(QResizeEvent *event) override;

    void showEvent(QShowEvent *event) override;

    void dragEnterEvent(QDragEnterEvent *event) override;

    void dragMoveEvent(QDragMoveEvent *event) override;

    void dropEvent(QDropEvent *event) override;

    void wheelEvent(QWheelEvent *event) override;

protected slots:
    // 选择更改
    void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;

    QAbstractItemDelegate* itemDelegateForIndex(const QModelIndex &index) const override ;

private:

    double currentScale = 1.0;
    TimelineModel *Model;
    QRect itemRect(const QModelIndex &index) const;

    QPoint m_scrollOffset;
   
    int pointToFrame(int point) const;

    int frameToPoint(int frame) const;

    
    
    QPoint m_mouseStart;
    QPoint m_mouseEnd;
    QPoint m_mouseOffset;
    bool mouseHeld = false;
    bool m_playheadSelected = false;
    bool m_isDroppingMedia = false;
    QPoint m_lastDragPos;
    VideoPlayerWidget* videoPlayer = nullptr;
    hoverState m_mouseUnderClipEdge = NONE;

    QModelIndex m_hoverIndex = QModelIndex();

    int getTrackWdith() const;
 
    int getPlayheadPos();

    // 移动选定的剪辑
    void moveSelectedClip(int dx, int dy,bool isMouse = true);

    // 移动播放头到指定帧
    void movePlayheadToFrame(int frame);

    void setupVideoWindow();
};

#endif // TIMELINEVIEW_HPP
