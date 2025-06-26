#pragma once


#include <QTreeWidget>

/**
 * @brief A tree widget that allows for the dragging of its components to other widgets.
 */
class DraggableTreeWidget : public QTreeWidget {
public:
    /**
     * 创建一个新的空树形控件
     * @param parent 父控件
     */
    explicit DraggableTreeWidget(QWidget *parent = nullptr);

    QPixmap createDragPixmap(QTreeWidgetItem *item);
protected:
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
     * 开始拖拽
     * @param Qt::DropActions supportedActions 支持的动作
     */
    void startDrag(QTreeWidgetItem *item);
private:
    //拖拽的项
    QTreeWidgetItem *draggedItem_ = nullptr;
    //拖拽开始位置
    QPoint dragStartPosition_;
    //拖拽是否已开始
    bool isDragging_ = false;
};

