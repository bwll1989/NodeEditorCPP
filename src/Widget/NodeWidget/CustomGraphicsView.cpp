//
// Created by 吴斌 on 2023/11/21.
//

#include <QMimeData>
#include "CustomGraphicsView.h"
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileInfo>
#include "CustomFlowGraphicsScene.h"
CustomGraphicsView::CustomGraphicsView(QWidget *parent) {
   setAcceptDrops(true);
}

void CustomGraphicsView::dragEnterEvent(QDragEnterEvent *event) {

    if(event->mimeData()->hasFormat("application/add-node"))
        event->acceptProposedAction();
    else
        QGraphicsView::dragEnterEvent(event);

}

void CustomGraphicsView::dropEvent(QDropEvent *event)
{
    if(event->mimeData()->hasFormat("application/add-node"))
    {
        // 获取节点类型名称
        QByteArray nodeTypeData = event->mimeData()->data("application/add-node");
        QString nodeType = QString::fromUtf8(nodeTypeData);
        // 获取鼠标位置并转换为场景坐标
        QPointF scenePos = mapToScene(event->position().toPoint());
        // 向场景中添加节点
        auto *scene = qobject_cast<CustomFlowGraphicsScene*>(this->scene());
        if (scene) {
            scene->undoStack().push(new QtNodes::CreateCommand(scene, nodeType, scenePos));
        }
        event->acceptProposedAction();
    }
    else
    {
        // 其它所有情况都传递给父类处理
        QGraphicsView::dropEvent(event);
    }
}

void CustomGraphicsView::dragMoveEvent(QDragMoveEvent *event) {
    if(event->mimeData()->hasFormat("application/add-node")) {
        event->acceptProposedAction();
    } else {
        QGraphicsView::dragMoveEvent(event);
    }
}

void CustomGraphicsView::dragLeaveEvent(QDragLeaveEvent *event) {
    QGraphicsView::dragLeaveEvent(event);
}