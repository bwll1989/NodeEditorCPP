#include "draggabletreewidget.hpp"
#include <QMouseEvent>
#include <QScrollBar>
#include <QPixmap>
#include <QPainter>
#include <QApplication>
#include <QDrag>
#include <QMimeData>
#include <QCursor>

DraggableTreeWidget::DraggableTreeWidget(QWidget *parent) : QTreeWidget(parent) {
    setVerticalScrollMode(QAbstractItemView::ScrollPerPixel);
    // 允许拖拽
    // setDragEnabled(true);
}

void DraggableTreeWidget::mousePressEvent(QMouseEvent *event) {
    QTreeWidget::mousePressEvent(event);
    auto item = itemAt(event->pos());
    if (item && event->button() == Qt::LeftButton) {
        // if (actions_.contains(item->text(0))) {
            draggedItem_ = item;
            dragStartPosition_ = event->pos();
        // }
    }
}


void DraggableTreeWidget::mouseMoveEvent(QMouseEvent *event) {
    if (!draggedItem_) {
        QTreeWidget::mouseMoveEvent(event);
        return;
    }

    // 检查是否移动了足够的距离开始拖拽
    if ((event->pos() - dragStartPosition_).manhattanLength() < QApplication::startDragDistance()) {
        return;
    }
    // 设置拖拽状态
    isDragging_ = true;
    // 设置拖拽光标
    QCursor dragCursor(Qt::DragMoveCursor);
    // 开始拖拽
    startDrag(draggedItem_);
}

void DraggableTreeWidget::startDrag(QTreeWidgetItem *item) {
    if (!item) {
        return;
    }
    
    // 保存当前拖拽项的文本，用于后续触发action
    QString itemText = item->text(0);
    // bool hasAction = actions_.contains(itemText);
    
    // 创建拖拽对象
    QDrag *drag = new QDrag(this);
    
    // 创建MIME数据 - 只包含节点类型名称
    QMimeData *mimeData = new QMimeData;
    mimeData->setData("application/add-node", itemText.toUtf8());
    drag->setMimeData(mimeData);
    // 设置拖拽缩略图
    QPixmap pixmap = createDragPixmap(item);
    drag->setPixmap(pixmap);
    // 设置热点为图像中心
    drag->setHotSpot(QPoint(0,0));
    // 执行拖拽操作
    Qt::DropAction dropAction = drag->exec(Qt::CopyAction);
    // 重置状态
    draggedItem_ = nullptr;
    isDragging_ = false;
    // 恢复光标
    QApplication::restoreOverrideCursor();
}

QPixmap DraggableTreeWidget::createDragPixmap(QTreeWidgetItem *item) {
    if (!item) {
        return QPixmap();
    }
    
    // 获取项目图标和文本
    QIcon icon = item->icon(0);
    QString text = item->text(0);
    
    // 计算缩略图大小
    QFontMetrics fm(font());
    int textWidth = fm.horizontalAdvance(text);
    int textHeight = fm.height();
    int iconSize = 24; // 图标大小
    int padding = 8;   // 内边距
    
    int width = 100;
    int height =150;
    
    // 创建缩略图
    QPixmap pixmap(width, height);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 绘制背景
    QColor bgColor(70, 130, 180, 200); // 半透明的蓝色
    painter.setPen(Qt::NoPen);
    painter.setBrush(bgColor);
    painter.drawRoundedRect(0, 0, width, height, 2, 2);
    
    // 绘制图标
    if (!icon.isNull()) {
        QRect iconRect(padding, padding, iconSize, iconSize);
        icon.paint(&painter, iconRect);
    }
    
    // 绘制文本
    painter.setPen(Qt::white);
    QRect textRect((width - textWidth) / 2, (height - textHeight) / 2, textWidth, textHeight);
    painter.drawText(textRect, Qt::AlignVCenter, text);
    
    // 绘制边框
    painter.setPen(QPen(QColor(255, 255, 255, 100), 1));
    painter.setBrush(Qt::NoBrush);
    painter.drawRoundedRect(0, 0, width - 1, height - 1, 2, 2);
    
    return pixmap;
}