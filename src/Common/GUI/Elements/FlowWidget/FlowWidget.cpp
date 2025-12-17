#include <QtWidgets>

#include "FlowWidget.h"
//! [1]
FlowLayout::FlowLayout(QWidget *parent, int margin, int hSpacing, int vSpacing)
        : QLayout(parent), m_hSpace(hSpacing), m_vSpace(vSpacing)
{
    setContentsMargins(margin, margin, margin, margin);
}

FlowLayout::FlowLayout(int margin, int hSpacing, int vSpacing)
        : m_hSpace(hSpacing), m_vSpace(vSpacing)
{
    setContentsMargins(margin, margin, margin, margin);
}
//! [1]

//! [2]
FlowLayout::~FlowLayout()
{
    QLayoutItem *item;
    while ((item = takeAt(0)))
        delete item;
}
//! [2]

//! [3]
void FlowLayout::addItem(QLayoutItem *item)
{
    itemList.append(item);
}
//! [3]

//! [4]
int FlowLayout::horizontalSpacing() const
{
    if (m_hSpace >= 0) {
        return m_hSpace;
    } else {
        return smartSpacing(QStyle::PM_LayoutHorizontalSpacing);
    }
}

int FlowLayout::verticalSpacing() const
{
    if (m_vSpace >= 0) {
        return m_vSpace;
    } else {
        return smartSpacing(QStyle::PM_LayoutVerticalSpacing);
    }
}
//! [4]

//! [5]
int FlowLayout::count() const
{
    return itemList.size();
}

QLayoutItem *FlowLayout::itemAt(int index) const
{
    return itemList.value(index);
}

QLayoutItem *FlowLayout::takeAt(int index)
{
    if (index >= 0 && index < itemList.size())
        return itemList.takeAt(index);
    else
        return 0;
}
//! [5]

//! [6]
Qt::Orientations FlowLayout::expandingDirections() const
{
    /**
     * 函数：FlowLayout::expandingDirections
     * 作用：声明布局在水平与垂直两个方向均可扩展，
     *       以允许父容器根据需要进行缩放。
     */
    return Qt::Horizontal | Qt::Vertical;
}
//! [6]

//! [7]
bool FlowLayout::hasHeightForWidth() const
{
    return true;
}

int FlowLayout::heightForWidth(int width) const
{
    int height = doLayout(QRect(0, 0, width, 0), true);
    return height;
}
//! [7]

//! [8]
void FlowLayout::setGeometry(const QRect &rect)
{
    QLayout::setGeometry(rect);
    doLayout(rect, false);
}

QSize FlowLayout::sizeHint() const
{
    return minimumSize();
}

QSize FlowLayout::minimumSize() const
{
    QSize size;
    QLayoutItem *item;
            foreach (item, itemList)
            size = size.expandedTo(item->minimumSize());

    size += QSize(2*contentsMargins().left(), 2*contentsMargins().top());
    return size;
}
//! [8]

//! [9]
int FlowLayout::doLayout(const QRect &rect, bool testOnly) const
{
    /**
     * 函数：FlowLayout::doLayout
     * 作用：执行流式布局。相较原版：
     *  - 若子控件的水平 SizePolicy 为 Expanding/MinimumExpanding，
     *    则让其在当前行内占用可用剩余宽度，以支持缩放。
     *  - 仍保留换行逻辑：当预计超出行宽则换到下一行。
     */
    int left, top, right, bottom;
    getContentsMargins(&left, &top, &right, &bottom);
    QRect effectiveRect = rect.adjusted(+left, +top, -right, -bottom);
    int x = effectiveRect.x();
    int y = effectiveRect.y();
    int lineHeight = 0;

    QLayoutItem *item;
    foreach (item, itemList) {
        QWidget *wid = item->widget();
        int spaceX = horizontalSpacing();
        if (spaceX == -1)
            spaceX = wid->style()->layoutSpacing(
                    QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Horizontal);
        int spaceY = verticalSpacing();
        if (spaceY == -1)
            spaceY = wid->style()->layoutSpacing(
                    QSizePolicy::PushButton, QSizePolicy::PushButton, Qt::Vertical);

        // 计算当前行剩余宽度
        int availWidth = effectiveRect.right() - x + 1;

        // 初始使用控件的建议宽度
        int itemWidth = item->sizeHint().width();
        int itemHeight = item->sizeHint().height();

        // 若控件声明可水平扩展，则让其占用当前行的剩余宽度（在最小/最大范围内）
        QSizePolicy::Policy hp = wid->sizePolicy().horizontalPolicy();
        bool canExpand = (hp == QSizePolicy::Expanding || hp == QSizePolicy::MinimumExpanding);
        if (canExpand) {
            int minW = wid->minimumSizeHint().width();
            int maxW = wid->maximumSize().width();
            if (maxW <= 0 || maxW >= QWIDGETSIZE_MAX) {
                // 没有限制时，允许占满行剩余宽度
                maxW = availWidth;
            }
            itemWidth = qBound(minW, availWidth, maxW);
        }

        // 如果预计超出行宽，且当前行已有内容，则换行重算
        int nextX = x + itemWidth + spaceX;
        if (nextX - spaceX > effectiveRect.right() && lineHeight > 0) {
            x = effectiveRect.x();
            y = y + lineHeight + spaceY;
            lineHeight = 0;

            // 换行后重新计算行剩余宽度与扩展宽度
            availWidth = effectiveRect.right() - x + 1;
            if (canExpand) {
                int minW = wid->minimumSizeHint().width();
                int maxW = wid->maximumSize().width();
                if (maxW <= 0 || maxW >= QWIDGETSIZE_MAX) {
                    maxW = availWidth;
                }
                itemWidth = qBound(minW, availWidth, maxW);
            } else {
                itemWidth = item->sizeHint().width();
            }
            nextX = x + itemWidth + spaceX;
        }

        if (!testOnly) {
            // 关键：使用计算后的 itemWidth，而非固定的 sizeHint 宽度
            item->setGeometry(QRect(x, y, itemWidth, itemHeight));
        }

        x = nextX;
        lineHeight = qMax(lineHeight, itemHeight);
    }
    return y + lineHeight - rect.y() + bottom;
}
//! [9]
//! [12]
int FlowLayout::smartSpacing(QStyle::PixelMetric pm) const
{
    QObject *parent = this->parent();
    if (!parent) {
        return -1;
    } else if (parent->isWidgetType()) {
        QWidget *pw = static_cast<QWidget *>(parent);
        return pw->style()->pixelMetric(pm, 0, pw);
    } else {
        return static_cast<QLayout *>(parent)->spacing();
    }
}