#pragma once

#include <QAbstractItemView>
#include <QMenu>
#include "BaseTrackDelegate.h"
#include <QEvent>
#include <QPaintEvent>
#include <QPainter>
#include <QScrollBar>
#include <QDrag>
#include <QApplication>
#include <QPersistentModelIndex>
#include <QVector>
#include <QMap>
#include <QPair>
#include <QSet>
#include <functional>
#include "BaseTimeLineModel.h"
#include "TimeLineStyle.h"
#include <QTimer>
#include <QMessageBox>
#include "TimelineExports.hpp"

class NODE_TIMELINE_PUBLIC BaseTracklistView : public QAbstractItemView
{
    Q_OBJECT
    Q_PROPERTY(QColor trackColor READ getTrackColor WRITE setTrackColor)
    Q_PROPERTY(QColor trackSelectedColor READ getTrackSelectedColor WRITE setTrackSelectedColor)
    Q_PROPERTY(QColor trackHoverColor READ getTrackHoverColor WRITE setTrackHoverColor)
    Q_PROPERTY(QColor trackBgColor READ trackBgColor WRITE setTrackBgColor)
    Q_PROPERTY(QColor trackTitleColor READ getTrackTitleColor WRITE setTrackTitleColor)
    // 标尺区域当前时间文字颜色（与播放头同色系），可由 QSS 配置
    Q_PROPERTY(QColor timelinePlayheadColor READ timelinePlayheadColor WRITE setTimelinePlayheadColor)
public:
    explicit BaseTracklistView(BaseTimeLineModel *viemModel, QWidget *parent = nullptr);

    QColor getTrackColor() const { return m_trackColor; }
    void setTrackColor(const QColor &color);

    QColor getTrackSelectedColor() const { return m_trackSelectedColor; }
    void setTrackSelectedColor(const QColor &color);

    QColor getTrackHoverColor() const { return m_trackHoverColor; }
    void setTrackHoverColor(const QColor &color);
    
    QColor trackBgColor() const { return m_trackBgColor; }
    void setTrackBgColor(const QColor& color){ m_trackBgColor=color;}

    QColor getTrackTitleColor() const { return m_trackTitleColor; }
    void setTrackTitleColor(const QColor &color);

    QColor timelinePlayheadColor() const { return m_timelinePlayheadColor; }
    void setTimelinePlayheadColor(const QColor& color);

    ~BaseTracklistView() override=default;
    /**
     * 更新滚动条
     */
    virtual void updateScrollBars();
    // 只刷新顶部时间文字，不重画轨道行
    void updatePlayheadHeader();
    /**
     * 获取可视区域
     * @param QModelIndex index 索引
     * @return QRect 可视区域
     */
    virtual QRect visualRect(const QModelIndex &index) const override;
    /**
     * 滚动到指定索引
     * @param QModelIndex index 索引
     * @param ScrollHint hint 提示
     */
    virtual void scrollTo(const QModelIndex &index, ScrollHint hint) override{}
    /**
     * 获取指定点的索引
     */
    virtual QModelIndex indexAt(const QPoint &point) const override;

    virtual BaseTimeLineModel * getModel();

    /// 行头展开按钮显隐开关（2026-11-09 合并关键帧模式：clip 内嵌 panel
    /// 退役后展开行无内容，宿主隐藏按钮——paint 不绘制、命中不拦截，轨道
    /// 行高恒 52px。默认 true 保持既有行为，宿主显式关闭）。
    void setExpandButtonVisible(bool on) { m_expandButtonVisible = on; }
    bool expandButtonVisible() const { return m_expandButtonVisible; }

public slots:
    /**
     * @brief 更新视图
     */
    virtual void onUpdateViewport();
    
protected:
    /**
     * @brief 确保可视范围内的轨道行存在 persistent editor
     */
    void ensureVisiblePersistentEditors();

    /**
     * 移动光标
     * @param CursorAction cursorAction 光标动作
     * @param Qt::KeyboardModifiers modifiers 键盘修饰符
     * @return QModelIndex 索引
     */
    virtual QModelIndex moveCursor(CursorAction cursorAction, Qt::KeyboardModifiers modifiers) override { return QModelIndex();}
    /**
     * 水平偏移
     * @return int 水平偏移
     */
    virtual int horizontalOffset() const override {return 0;}
    /**
     * 垂直偏移
     * @return int 垂直偏移
     */
    virtual int verticalOffset() const override {return 0;}
    /**
     * 是否隐藏索引
     * @param QModelIndex index 索引
     * @return bool 是否隐藏
     */ 
    virtual bool isIndexHidden(const QModelIndex &index) const override { return false; }
    /**
     * 设置选择
     * @param QRect rect 矩形
     * @param QItemSelectionModel::SelectionFlags command 命令
     */
    virtual void setSelection(const QRect &rect, QItemSelectionModel::SelectionFlags command) override ;
    /**
     * 获取选择区域
     * @param QItemSelection selection 选择
     * @return QRegion 区域
     */
    virtual QRegion visualRegionForSelection(const QItemSelection &selection) const override {return QRegion();}
    /**
     * 绘制事件
     * @param QPaintEvent *event 绘制事件
     */
    virtual void paintEvent(QPaintEvent *event) override;

    virtual void drawTitle(QPainter *painter);
    /**
     * 鼠标按下事件
     * @param QMouseEvent *event 鼠标事件
     */
    void mousePressEvent(QMouseEvent *event) override;


    /**
     * 鼠标移动事件
     * @param QMouseEvent *event 鼠标事件
     */
    void mouseMoveEvent(QMouseEvent *event) override;
    /**
     * 离开事件
     * @param QEvent *event 事件
     */
    void leaveEvent(QEvent *event) override;
    /**
     * 调整事件
     * @param QResizeEvent *event 调整事件
     */
    void resizeEvent(QResizeEvent *event) override;
    /**
     * 显示事件
     * @param QShowEvent *event 显示事件
     */
    void showEvent(QShowEvent *event) override;
    /**
     * 样式变化事件（QSS qproperty- 生效后重绘视口）
     * @param QEvent *event 事件
     */
    void changeEvent(QEvent *event) override;
    /**
     * 滚动内容
     * @param int dx 水平滚动
     * @param int dy 垂直滚动
     */
    void scrollContentsBy(int dx, int dy) override;
    /**
     * 获取项目矩形
     * @param QModelIndex index 索引
     * @return QRect 矩形
     */
    virtual QRect itemRect(const QModelIndex &index) const;
    /**
     * 轨道行头展开按钮矩形（2026-11-06 轨道展开；paintEvent 绘制与
     * mousePressEvent 命中共用同一公式——同源纪律，否则画在哪、点在哪各说各话）
     * @param rowRect 轨道行矩形（visualRect）
     * @return QRect 按钮矩形（行头左侧 18px 方块，对齐 clip 本体行内居中）
     */
    QRect trackExpandButtonRect(const QRect& rowRect) const;
    /**
     * 事件过滤
     * @param QObject *watched 观察对象
     * @param QEvent *event 事件
     * @return bool 是否过滤
     */
    bool eventFilter(QObject *watched, QEvent *event) override;
    /**
     * 上下文菜单事件
     * @param QContextMenuEvent* event 上下文菜单事件
     */
    void contextMenuEvent(QContextMenuEvent* event) override ;
protected slots:
    /**
     * 删除轨道
     */
    virtual void onDeleteTrack();
    /**
     * 选择改变
     * @param QItemSelection selected 选择
     * @param QItemSelection deselected 取消选择
     */
    virtual void selectionChanged(const QItemSelection &selected, const QItemSelection &deselected) override;
    /**
     * 更新编辑几何
     */
    virtual void updateEditorGeometries() override;
    /**
     * 拖入事件
     * @param QDragEnterEvent *event 拖入事件
     */
    virtual void dragEnterEvent(QDragEnterEvent *event) override;
    /**
     * 拖动移动事件
     * @param QDragMoveEvent *event 拖动移动事件
     */
    virtual void dragMoveEvent(QDragMoveEvent *event) override;
    /**
     * 拖动释放事件
     * @param QDropEvent *event 拖动释放事件
     */
    virtual void dropEvent(QDropEvent *event) override;

public:
    /**
     * 删除轨道动作
     */
    QAction* m_deleteTrackAction;
protected:
    //拖动开始位置
    QPoint m_dragStartPosition;
    //悬停索引
    QModelIndex m_hoverIndex = QModelIndex();
    //模型
    BaseTimeLineModel *Model;

    // 当前已打开的 persistent editor（用于可视范围内按需开关，避免滚动时只增不减）
    QVector<QPersistentModelIndex> m_openPersistentEditors;

private:
   // 颜色默认值来自 TimeLineStyle.h，运行时由 QSS qproperty- 覆盖
    QColor m_trackColor = trackColour;
    QColor m_trackTitleColor = trackTitleColor;
    QColor m_trackSelectedColor = trackSelectedColour;
    QColor m_trackHoverColor = trackHoverColour;
    QColor m_trackBgColor       = bgColour;
    QColor m_timelinePlayheadColor = playheadColour; // 标尺时间文字颜色

    // 行头展开按钮显隐（2026-11-09 合并关键帧模式；默认显示保持既有行为）
    bool m_expandButtonVisible = true;
};

