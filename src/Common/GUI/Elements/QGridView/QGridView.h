#pragma once

#include <QAbstractItemView>
#include <QScrollBar>
#ifdef GUI_ELEMENTS_LIBRARY
#define GUI_ELEMENTS_EXPORT Q_DECL_EXPORT
#else
#define GUI_ELEMENTS_EXPORT Q_DECL_IMPORT
#endif

class GUI_ELEMENTS_EXPORT QGridView : public QAbstractItemView
{
    Q_OBJECT
public:
    /**
     * 函数：QGridView::QGridView
     * 作用：构造网格视图，初始化单元格尺寸、间距与统一尺寸模式。
     */
    explicit QGridView(QWidget* parent = nullptr);
    /**
     * 函数：QGridView::~QGridView
     * 作用：析构函数，默认行为即可。
     */
    ~QGridView() override;

    /**
     * 函数：QGridView::setGridSize
     * 作用：设置每个单元格（项目）绘制区域的尺寸（固定网格大小）。
     */
    void setGridSize(const QSize& size);
    /**
     * 函数：QGridView::gridSize
     * 作用：获取当前单元格尺寸。
     */
    QSize gridSize() const;

    /**
     * 函数：QGridView::setSpacing
     * 作用：设置网格的水平/垂直间距（像素）。
     */
    void setSpacing(int hSpacing, int vSpacing);
    /**
     * 函数：QGridView::horizontalSpacing / verticalSpacing
     * 作用：获取当前网格的水平/垂直间距。
     */
    int horizontalSpacing() const;
    int verticalSpacing() const;

    /**
     * 函数：QGridView::setUniformItemSizes
     * 作用：设置是否使用统一的固定单元格尺寸（推荐开启）。
     *       若关闭，可根据委托 sizeHint 动态调整，但此实现以统一尺寸为主。
     */
    void setUniformItemSizes(bool on);
    /**
     * 函数：QGridView::uniformItemSizes
     * 作用：获取当前统一尺寸模式。
     */
    bool uniformItemSizes() const;

protected:
    /**
     * 函数：QGridView::scrollTo
     * 作用：滚动视口，以便指定索引项目可见。
     */
    void scrollTo(const QModelIndex& index, ScrollHint hint = EnsureVisible) override;
    /**
     * 函数：QGridView::indexAt
     * 作用：返回视口坐标处的模型索引（若没有命中则返回无效索引）。
     */
    QModelIndex indexAt(const QPoint& point) const override;
    /**
     * 函数：QGridView::visualRect
     * 作用：返回指定索引在视口中的可视矩形（已考虑滚动偏移）。
     */
    QRect visualRect(const QModelIndex& index) const override;

    /**
     * 函数：QGridView::horizontalOffset/verticalOffset
     * 作用：返回当前水平/垂直滚动偏移。
     */
    int horizontalOffset() const override;
    int verticalOffset() const override;
    /**
     * 函数：QGridView::isIndexHidden
     * 作用：返回索引是否被隐藏（此实现始终返回 false）。
     */
    bool isIndexHidden(const QModelIndex& index) const override;

    /**
     * 函数：QGridView::setSelection
     * 作用：根据矩形与选择标志进行选择（框选）。
     */
    void setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags flags) override;
    /**
     * 函数：QGridView::visualRegionForSelection
     * 作用：返回选择集对应的可视区域联合。
     */
    QRegion visualRegionForSelection(const QItemSelection& selection) const override;

    /**
     * 函数：QGridView::paintEvent
     * 作用：绘制视口内可见项目，委托负责具体项绘制。
     */
    void paintEvent(QPaintEvent* event) override;
    /**
     * 函数：QGridView::resizeEvent
     * 作用：响应视口尺寸变化，更新滚动条几何与布局。
     */
    void resizeEvent(QResizeEvent* event) override;

    /**
     * 函数：QGridView::updateGeometries
     * 作用：在模型或尺寸变更时更新滚动条范围。
     */
    void updateGeometries() override;

    /**
     * 函数：QGridView::moveCursor
     * 作用：实现键盘光标移动（上下左右、翻页、Home/End），在网格中定位目标索引。
     */
    QModelIndex moveCursor(CursorAction action, Qt::KeyboardModifiers modifiers) override;

    /**
     * 函数：QGridView::doItemsLayout
     * 作用：在需要重新布局项目时，更新滚动条、编辑器几何并请求重绘。
     */
    void doItemsLayout() override;
    /**
     * 函数：QGridView::updateEditorGeometries
     * 作用：先让基类为所有持久编辑器定位几何，
     *       再补充定位通过 setIndexWidget 设置的部件（若存在）。
     */
    void updateEditorGeometries() override;
    /**
     * 函数：QGridView::scrollContentsBy
     * 作用：滚动发生后，立即刷新编辑器几何，保证控件随内容移动。
     */
    void scrollContentsBy(int dx, int dy) override;

private:
    /**
     * 函数：QGridView::itemRect
     * 作用：根据索引序号计算该元素的内容坐标系中的矩形（未扣除滚动）。
     */
    QRect itemRect(int row) const;
    /**
     * 函数：QGridView::columnsCount
     * 作用：根据视口宽度与网格尺寸/间距，计算当前可容纳的列数。
     */
    int columnsCount() const;
    /**
     * 函数：QGridView::rowsCount
     * 作用：根据模型行数与列数，计算总行数。
     */
    int rowsCount() const;
    /**
     * 函数：QGridView::contentSize
     * 作用：计算整个内容区域大小，用于设置滚动条范围。
     */
    QSize contentSize() const;

private:
    QSize m_gridSize {120, 90};
    int m_hSpacing {8};
    int m_vSpacing {8};
    bool m_uniform {true};
};