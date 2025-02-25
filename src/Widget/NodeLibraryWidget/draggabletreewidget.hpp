#pragma once

#include <QMap>
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

    /**
     * 注册一个新的动作到树形控件中
     * @param key 动作的唯一ID
     * @param action 当项目在拖拽结束时触发的动作
     */
    void registerAction(const QString &key, QAction *action);

protected:
    /**
     * 鼠标按下事件
     * @param QMouseEvent *event 鼠标事件
     */
    void mousePressEvent(QMouseEvent *event) override;
    /**
     * 鼠标释放事件
     * @param QMouseEvent *event 鼠标事件
     */
    void mouseReleaseEvent(QMouseEvent *event) override;
    /**
     * 鼠标移动事件
     * @param QMouseEvent *event 鼠标事件
     */
    void mouseMoveEvent(QMouseEvent *event) override;

private:
    //动作映射
    QMap<QString, QAction *> actions_;
    //拖拽的项
    QTreeWidgetItem *draggedItem_ = nullptr;
};

