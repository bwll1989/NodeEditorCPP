
#include "QGridView.h"   
#include <QtWidgets>

#include "QGridView.h"   
#include <QStyleOptionViewItem>  
/**
 * 函数：QGridView::QGridView
 * 作用：构造网格视图，开启鼠标跟踪并使用默认滚动行为。
 */
QGridView::QGridView(QWidget* parent)
    : QAbstractItemView(parent)
{
    setMouseTracking(true);
    // 使用单选/多选由外部自行配置 selectionMode/behavior
}

/**
 * 函数：QGridView::~QGridView
 * 作用：默认析构即可。
 */
QGridView::~QGridView() = default;

/**
 * 函数：QGridView::setGridSize
 * 作用：设置固定单元格大小，触发几何更新与重绘。
 */
void QGridView::setGridSize(const QSize& size)
{
    if (m_gridSize == size) return;
    m_gridSize = size;
    updateGeometries();
    viewport()->update();
    scheduleDelayedItemsLayout();  // 触发延迟布局，确保缩放后重排
}

/**
 * 函数：QGridView::gridSize
 * 作用：返回当前单元格大小。
 */
QSize QGridView::gridSize() const
{
    return m_gridSize;
}

/**
 * 函数：QGridView::setSpacing
 * 作用：设置水平与垂直间距。
 */
void QGridView::setSpacing(int hSpacing, int vSpacing)
{
    if (m_hSpacing == hSpacing && m_vSpacing == vSpacing) return;
    m_hSpacing = qMax(0, hSpacing);
    m_vSpacing = qMax(0, vSpacing);
    updateGeometries();
    viewport()->update();
    scheduleDelayedItemsLayout();  // 触发延迟布局，确保缩放后重排
}

/**
 * 函数：QGridView::horizontalSpacing / verticalSpacing
 * 作用：返回当前间距。
 */
int QGridView::horizontalSpacing() const { return m_hSpacing; }
int QGridView::verticalSpacing() const   { return m_vSpacing; }

/**
 * 函数：QGridView::setUniformItemSizes
 * 作用：设置是否使用统一的固定单元格尺寸。
 */
void QGridView::setUniformItemSizes(bool on)
{
    if (m_uniform == on) return;
    m_uniform = on;
    updateGeometries();
    viewport()->update();
    scheduleDelayedItemsLayout();  // 触发延迟布局，确保缩放后重排
}

/**
 * 函数：QGridView::uniformItemSizes
 * 作用：返回统一尺寸模式开关。
 */
bool QGridView::uniformItemSizes() const
{
    return m_uniform;
}

/**
 * 函数：QGridView::columnsCount
 * 作用：基于视口宽度与单元格尺寸/间距计算列数（至少 1）。
 */
int QGridView::columnsCount() const
{
    const int vpw = viewport()->width();
    const int cellW = m_gridSize.width();
    const int step = cellW + m_hSpacing;
    if (vpw <= 0 || step <= 0) return 1;
    int cols = qMax(1, (vpw + m_hSpacing) / step);
    return cols;
}

/**
 * 函数：QGridView::rowsCount
 * 作用：根据模型行数与列数计算总行数。
 */
int QGridView::rowsCount() const
{
    if (!model()) return 0;
    const int rows = model()->rowCount(rootIndex());
    const int cols = columnsCount();
    if (rows == 0) return 0;
    return (rows + cols - 1) / cols;
}

/**
 * 函数：QGridView::itemRect
 * 作用：计算给定“线性序号”的项目矩形（内容坐标，不含滚动偏移）。
 * 说明：此视图将模型视为一维列表（使用 column=0），按行优先分布到网格。
 */
QRect QGridView::itemRect(int linearRow) const
{
    const int cols = columnsCount();
    if (cols <= 0) return QRect();

    const int row = linearRow / cols;
    const int col = linearRow % cols;

    const int x = col * (m_gridSize.width() + m_hSpacing);
    const int y = row * (m_gridSize.height() + m_vSpacing);
    return QRect(x, y, m_gridSize.width(), m_gridSize.height());
}

/**
 * 函数：QGridView::contentSize
 * 作用：返回完整内容区域尺寸，用于设置滚动条范围。
 */
QSize QGridView::contentSize() const
{
    const int rows = rowsCount();
    const int cols = qMin(columnsCount(), model() ? model()->rowCount(rootIndex()) : 0);

    const int width  = cols * m_gridSize.width() + qMax(0, cols - 1) * m_hSpacing;
    const int height = rows * m_gridSize.height() + qMax(0, rows - 1) * m_vSpacing;
    return QSize(width, height);
}

/**
 * 函数：QGridView::horizontalOffset/verticalOffset
 * 作用：返回滚动条的当前偏移。
 */
int QGridView::horizontalOffset() const { return horizontalScrollBar()->value(); }
int QGridView::verticalOffset() const   { return verticalScrollBar()->value(); }

/**
 * 函数：QGridView::isIndexHidden
 * 作用：索引隐藏判断（此实现不隐藏任何索引）。
 */
bool QGridView::isIndexHidden(const QModelIndex& index) const
{
    Q_UNUSED(index);
    return false;
}

/**
 * 函数：QGridView::visualRect
 * 作用：返回索引在视口中的矩形（扣除滚动偏移）。
 */
QRect QGridView::visualRect(const QModelIndex& index) const
{
    if (!index.isValid() || index.parent() != rootIndex()) return QRect();
    const int linear = index.row();
    QRect r = itemRect(linear);
    r.translate(-horizontalOffset(), -verticalOffset());
    return r;
}

/**
 * 函数：QGridView::indexAt
 * 作用：根据视口坐标命中索引。
 */
QModelIndex QGridView::indexAt(const QPoint& point) const
{
    if (!model()) return QModelIndex();
    QPoint contentPt = point + QPoint(horizontalOffset(), verticalOffset());

    const int stepW = m_gridSize.width() + m_hSpacing;
    const int stepH = m_gridSize.height() + m_vSpacing;
    if (stepW <= 0 || stepH <= 0) return QModelIndex();

    const int col = contentPt.x() / stepW;
    const int row = contentPt.y() / stepH;
    const int cols = columnsCount();
    const int linear = row * cols + col;

    if (col < 0 || row < 0) return QModelIndex();
    const int total = model()->rowCount(rootIndex());
    if (linear < 0 || linear >= total) return QModelIndex();

    // 命中检测需在单元格内
    QRect cell = itemRect(linear);
    if (!cell.contains(contentPt)) return QModelIndex();

    return model()->index(linear, 0, rootIndex());
}

/**
 * 函数：QGridView::scrollTo
 * 作用：将指定索引滚动至可见区域。
 */
void QGridView::scrollTo(const QModelIndex& index, ScrollHint hint)
{
    if (!index.isValid() || !model()) return;

    const QRect vr = visualRect(index);
    const QRect vp = viewport()->rect();

    int hVal = horizontalOffset();
    int vVal = verticalOffset();

    switch (hint) {
    case PositionAtTop:
        vVal = vr.top();
        break;
    case PositionAtBottom:
        vVal = vr.bottom() - vp.height() + 1;
        break;
    case PositionAtCenter:
        vVal = vr.center().y() - vp.height() / 2;
        hVal = vr.center().x() - vp.width() / 2;
        break;
    case EnsureVisible:
    default:
        if (vr.top() < 0) vVal += vr.top();
        if (vr.bottom() > vp.height() - 1) vVal += (vr.bottom() - (vp.height() - 1));
        if (vr.left() < 0) hVal += vr.left();
        if (vr.right() > vp.width() - 1) hVal += (vr.right() - (vp.width() - 1));
        break;
    }

    horizontalScrollBar()->setValue(qMax(0, hVal));
    verticalScrollBar()->setValue(qMax(0, vVal));
}

/**
 * 函数：QGridView::setSelection
 * 作用：根据矩形与选择标志进行框选。
 */
void QGridView::setSelection(const QRect& rect, QItemSelectionModel::SelectionFlags flags)
{
    if (!model()) return;

    QRect contentRect = rect.translated(horizontalOffset(), verticalOffset());
    QItemSelection selection;

    const int total = model()->rowCount(rootIndex());
    const int cols = columnsCount();
    const int rows = rowsCount();

    // 约束起止行列，避免遍历全部元素
    const int stepW = m_gridSize.width() + m_hSpacing;
    const int stepH = m_gridSize.height() + m_vSpacing;

    const int startCol = qMax(0, contentRect.left() / stepW);
    const int endCol   = qMin(cols - 1, contentRect.right() / stepW);
    const int startRow = qMax(0, contentRect.top() / stepH);
    const int endRow   = qMin(rows - 1, contentRect.bottom() / stepH);

    for (int r = startRow; r <= endRow; ++r) {
        for (int c = startCol; c <= endCol; ++c) {
            const int linear = r * cols + c;
            if (linear < 0 || linear >= total) continue;
            QRect cell = itemRect(linear);
            if (cell.intersects(contentRect)) {
                QModelIndex idx = model()->index(linear, 0, rootIndex());
                selection.select(idx, idx);
            }
        }
    }

    selectionModel()->select(selection, flags);
}

/**
 * 函数：QGridView::visualRegionForSelection
 * 作用：返回选择集对应的可视区域联合。
 */
QRegion QGridView::visualRegionForSelection(const QItemSelection& selection) const
{
    QRegion region;
    const auto ranges = selection.indexes();
    for (const QModelIndex& idx : ranges) {
        region += visualRect(idx);
    }
    return region;
}

/**
 * 函数：QGridView::paintEvent
 * 作用：绘制视口内可见的网格项，使用委托进行项渲染。
 * 修复点：
 *  - 使用 initViewItemOption(&option) 替代 viewOptions() 初始化项绘制选项
 *  - 使用 itemDelegateForIndex(index) 替代已弃用的 itemDelegate()，必要时回退
 */
void QGridView::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);
    QPainter painter(viewport());
    painter.setClipRect(viewport()->rect());

    if (!model()) return;

    const QRect vp = viewport()->rect();
    const int total = model()->rowCount(rootIndex());
    const int cols = columnsCount();
    const int stepH = m_gridSize.height() + m_vSpacing;

    // 估算可见行范围，减少遍历
    const int firstVisibleRow = qMax(0, (verticalOffset()) / stepH);
    const int lastVisibleRow  = qMin(rowsCount() - 1, (verticalOffset() + vp.height()) / stepH + 1);

    /**
     * 函数：QGridView::paintEvent
     * 作用：绘制视口内可见的网格项，使用委托进行项渲染。
     * 修复点：
     *  - 使用 initViewItemOption(&option) 替代 viewOptions() 初始化项绘制选项
     *  - 使用 itemDelegateForIndex(index) 替代已弃用的 itemDelegate()，必要时回退
     */
    for (int r = firstVisibleRow; r <= lastVisibleRow; ++r) {
        for (int c = 0; c < cols; ++c) {
            const int linear = r * cols + c;
            if (linear >= total) break;

            QModelIndex index = model()->index(linear, 0, rootIndex());
            QRect rect = visualRect(index);
            if (!rect.intersects(vp)) continue;

            QStyleOptionViewItem option;
            initViewItemOption(&option);   // 替代 viewOptions()
            option.rect = rect;

            if (selectionModel() && selectionModel()->isSelected(index)) {
                option.state |= QStyle::State_Selected;
            }
            if (hasFocus()) {
                option.state |= QStyle::State_HasFocus;
            }

            QAbstractItemDelegate* delegate = itemDelegateForIndex(index); // 新：推荐 API
            if (!delegate) delegate = itemDelegate();                      // 兼容回退
            delegate->paint(&painter, option, index);
        }
    }
}

/**
 * 函数：QGridView::resizeEvent
 * 作用：在视口尺寸变化时更新滚动条与重绘。
 */
void QGridView::resizeEvent(QResizeEvent* event)
{
    QAbstractItemView::resizeEvent(event);
    updateGeometries();
    viewport()->update();
    scheduleDelayedItemsLayout();  // 视口尺寸变化后立刻重新布局
}

/**
 * 函数：QGridView::updateGeometries
 * 作用：更新滚动条范围与步长，保证内容区完整可滚动。
 */
void QGridView::updateGeometries()
{
    const QSize content = contentSize();
    const QSize vp = viewport()->size();

    horizontalScrollBar()->setRange(0, qMax(0, content.width() - vp.width()));
    horizontalScrollBar()->setPageStep(vp.width());
    horizontalScrollBar()->setSingleStep(m_gridSize.width() + m_hSpacing);

    verticalScrollBar()->setRange(0, qMax(0, content.height() - vp.height()));
    verticalScrollBar()->setPageStep(vp.height());
    verticalScrollBar()->setSingleStep(m_gridSize.height() + m_vSpacing);
}

QModelIndex QGridView::moveCursor(CursorAction action, Qt::KeyboardModifiers modifiers)
{
    /**
     * 函数：QGridView::moveCursor
     * 作用：根据键盘操作在网格中移动当前索引。
     * 规则：
     *  - 左右：在同一行移动，行首/行末跨行移动
     *  - 上下：按列对齐，跨行时在目标行末尾夹取可用列
     *  - 翻页：按视口高度估算行数步进
     *  - Home/End/Next/Previous：跳到首项/末项/前一项/后一项
     */
    Q_UNUSED(modifiers);

    const QAbstractItemModel* mdl = model();
    if (!mdl) return QModelIndex();

    const int total = mdl->rowCount(rootIndex());
    if (total <= 0) return QModelIndex();

    const int cols = qMax(1, columnsCount());
    const int rows = rowsCount();

    QModelIndex cur = currentIndex();
    int linear = cur.isValid() ? cur.row() : 0;

    auto itemsInRow = [&](int r) {
        if (r < 0 || r >= rows) return 0;
        if (r == rows - 1) {
            // 最后一行可能不满
            return total - r * cols;
        }
        return cols;
    };

    auto clampLinear = [&](int r, int c) {
        r = qBound(0, r, qMax(0, rows - 1));
        int colsInTarget = itemsInRow(r);
        c = qBound(0, c, qMax(0, colsInTarget - 1));
        int lin = r * cols + c;
        lin = qBound(0, lin, total - 1);
        return lin;
    };

    int row = linear / cols;
    int col = linear % cols;

    switch (action) {
    case MoveLeft:
        if (col > 0) {
            col -= 1;
        } else if (linear > 0) {
            // 跨到上一行的最后一列（若存在）
            row -= 1;
            col = itemsInRow(row) - 1;
        }
        linear = clampLinear(row, col);
        break;
    case MoveRight:
        if (col + 1 < itemsInRow(row)) {
            col += 1;
        } else if (linear + 1 < total) {
            // 跨到下一行的第一列
            row += 1;
            col = 0;
        }
        linear = clampLinear(row, col);
        break;
    case MoveUp:
        if (row > 0) {
            row -= 1;
            col = qMin(col, itemsInRow(row) - 1);
        }
        linear = clampLinear(row, col);
        break;
    case MoveDown:
        if (row + 1 < rows) {
            row += 1;
            col = qMin(col, itemsInRow(row) - 1);
        }
        linear = clampLinear(row, col);
        break;
    case MoveHome:
        linear = 0;
        break;
    case MoveEnd:
        linear = total - 1;
        break;
    case MovePageUp: {
        const int stepH = m_gridSize.height() + m_vSpacing;
        int pageRows = stepH > 0 ? qMax(1, viewport()->height() / stepH) : 1;
        row = qMax(0, row - pageRows);
        col = qMin(col, itemsInRow(row) - 1);
        linear = clampLinear(row, col);
        break;
    }
    case MovePageDown: {
        const int stepH = m_gridSize.height() + m_vSpacing;
        int pageRows = stepH > 0 ? qMax(1, viewport()->height() / stepH) : 1;
        row = qMin(rows - 1, row + pageRows);
        col = qMin(col, itemsInRow(row) - 1);
        linear = clampLinear(row, col);
        break;
    }
    case MoveNext:
        if (linear + 1 < total) linear += 1;
        break;
    case MovePrevious:
        if (linear - 1 >= 0) linear -= 1;
        break;
    default:
        break;
    }

    return mdl->index(linear, 0, rootIndex());
}


/**
 * 函数：QGridView::updateEditorGeometries
 * 作用：为每个索引的 indexWidget 设置几何位置，确保窗口缩放后摆放正确。
 * 说明：此实现遍历模型行，若该索引存在由 setIndexWidget 绑定的部件，
 *       则将其几何设置为 visualRect(index)。
 */
void QGridView::updateEditorGeometries()
{
    // 1) 基类负责持久编辑器定位（openPersistentEditor 的控件）
    QAbstractItemView::updateEditorGeometries();

    // 2) 额外支持：若存在 indexWidget，则仅定位当前可视区域内的控件
    const QAbstractItemModel* mdl = model();
    if (!mdl) return;

    const QRect vp = viewport()->rect();
    const int cols = columnsCount();
    const int total = mdl->rowCount(rootIndex());
    const int stepH = m_gridSize.height() + m_vSpacing;

    const int firstVisibleRow = qMax(0, verticalOffset() / stepH);
    const int lastVisibleRow  = qMin(rowsCount() - 1, (verticalOffset() + vp.height()) / stepH + 1);

    for (int gridRow = firstVisibleRow; gridRow <= lastVisibleRow; ++gridRow) {
        for (int c = 0; c < cols; ++c) {
            const int linear = gridRow * cols + c;
            if (linear >= total) break;

            const QModelIndex idx = mdl->index(linear, 0, rootIndex());
            QWidget* w = indexWidget(idx);
            if (!w) continue;

            const QRect rect = visualRect(idx);
            if (!rect.isNull())
                w->setGeometry(rect);
        }
    }
}

/**
 * 函数：QGridView::scrollContentsBy
 * 作用：内容滚动（滚动条、鼠标滚轮、触控拖动等）后，
 *       在基类完成视口滚动后，立即刷新编辑器几何，使控件跟随移动。
 */
void QGridView::scrollContentsBy(int dx, int dy)
{
    QAbstractItemView::scrollContentsBy(dx, dy);
    updateEditorGeometries();
}

/**
 * 函数：QGridView::doItemsLayout
 * 作用：当视图需要重新布局项目（由 scheduleDelayedItemsLayout 触发），
 *       更新滚动条范围、编辑器几何并请求重绘。
 */
void QGridView::doItemsLayout()
{
    QAbstractItemView::doItemsLayout();
    updateGeometries();
    updateEditorGeometries();
    viewport()->update();
}


