//
// Created by 吴斌 on 2023/11/21.
//

#include <QMimeData>
#include "CustomGraphicsView.h"

#include <QClipboard>
#include <QDragEnterEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QFileInfo>
#include "CustomFlowGraphicsScene.h"
CustomGraphicsView::CustomGraphicsView(QWidget *parent) {
   setAcceptDrops(true);
}
/**
 * @brief 拖拽进入事件：支持库内节点添加与媒体库条目（标签+文件名）
 */
void CustomGraphicsView::dragEnterEvent(QDragEnterEvent *event) {

    const QMimeData* md = event->mimeData();
    if (!md) { QGraphicsView::dragEnterEvent(event); return; }

    if (md->hasFormat("application/add-node") || md->hasFormat("application/media-item"))
        event->acceptProposedAction();
    else
        QGraphicsView::dragEnterEvent(event);

}

/**
 * @brief 放置事件：根据来源创建节点
 * - application/add-node：沿用原逻辑
 * - application/media-item：根据标签创建对应节点（仅用标签与文件名，不用路径）
 */
void CustomGraphicsView::dropEvent(QDropEvent *event)
{
    const QMimeData* md = event->mimeData();
    if (!md) { QGraphicsView::dropEvent(event); return; }

    if(md->hasFormat("application/add-node"))
    {
        // 获取节点类型名称
        QByteArray nodeTypeData = md->data("application/add-node");
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
    else if (md->hasFormat("application/media-item"))
    {
        // 解析自定义媒体库拖拽数据：{"tag":"Image","name":"xxx.jpg"}
        const QByteArray payload = md->data("application/media-item");
        QJsonParseError err;
        const QJsonDocument doc = QJsonDocument::fromJson(payload, &err);
        if (err.error != QJsonParseError::NoError || !doc.isObject()) {
            QGraphicsView::dropEvent(event);
            return;
        }
        // 计算落点坐标，并填充剪贴板模板中的占位符
        QPointF scenePos = mapToScene(event->position().toPoint());
        if (QClipboard* cb = QGuiApplication::clipboard()) {
            QString clipText = cb->text();
            // 用于用户可视粘贴的同步更新：替换坐标占位符（若存在）
            if (!clipText.isEmpty()) {
                clipText.replace(QStringLiteral("%2"), QString::number(scenePos.x()))
                        .replace(QStringLiteral("%3"), QString::number(scenePos.y()));
                cb->setText(clipText);
            }
        }

        auto *scene = qobject_cast<CustomFlowGraphicsScene*>(this->scene());
        if (scene) {
            scene->undoStack().push(new QtNodes::PasteCommand(scene,scenePos));
            
            event->acceptProposedAction();
            return;
        }

        QGraphicsView::dropEvent(event);
    }
    else
    {
        // 其它所有情况都传递给父类处理
        QGraphicsView::dropEvent(event);
    }
}

/**
 * @brief 拖拽移动事件：保持两类来源的接受状态
 */
void CustomGraphicsView::dragMoveEvent(QDragMoveEvent *event) {
    const QMimeData* md = event->mimeData();
    if (md && (md->hasFormat("application/add-node") || md->hasFormat("application/media-item"))) {
        event->acceptProposedAction();
    } else {
        QGraphicsView::dragMoveEvent(event);
    }
}

void CustomGraphicsView::dragLeaveEvent(QDragLeaveEvent *event) {
    QGraphicsView::dragLeaveEvent(event);
}

