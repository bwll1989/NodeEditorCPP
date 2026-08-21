//
// Created by 吴斌 on 2024/1/16.
//

#include "CustomFlowGraphicsScene.h"
#include "NodeCreateSceneMenu.hpp"

#include <QMessageBox>

#include "QtNodes/internal/GraphicsView.hpp"
#include "QtNodes/internal/NodeGraphicsObject.hpp"

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGraphicsSceneMouseEvent>
#include <QtWidgets/QMenu>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QTransform>

using QtNodes::GraphicsView;
using QtNodes::NodeGraphicsObject;

namespace {

void addActionIfPresent(QMenu &menu, QAction *action)
{
    if (action && action->isVisible())
        menu.addAction(action);
}

GraphicsView *graphicsViewOf(QGraphicsScene *scene)
{
    if (!scene || scene->views().isEmpty())
        return nullptr;
    return qobject_cast<GraphicsView *>(scene->views().first());
}

} // namespace

CustomFlowGraphicsScene::CustomFlowGraphicsScene(CustomDataFlowGraphModel &graphModel, QObject *parent)
    : BasicGraphicsScene(graphModel, parent)
    , _graphModel(graphModel)
{
    connect(&_graphModel,
            &CustomDataFlowGraphModel::inPortDataWasSet,
            [this](NodeId const nodeId, PortType const, PortIndex const) { onNodeUpdated(nodeId); });
}

std::vector<NodeId> CustomFlowGraphicsScene::selectedNodes() const
{
    QList<QGraphicsItem *> graphicsItems = selectedItems();

    std::vector<NodeId> result;
    result.reserve(graphicsItems.size());

    for (QGraphicsItem *item : graphicsItems) {
        auto ngo = qgraphicsitem_cast<NodeGraphicsObject *>(item);
        if (ngo != nullptr) {
            result.push_back(ngo->nodeId());
        }
    }

    return result;
}

QMenu *CustomFlowGraphicsScene::createSceneMenu(QPointF const scenePos)
{
    return NodeCreateSceneMenu::create(this, _graphModel.dataModelRegistry(), scenePos);
}

void CustomFlowGraphicsScene::appendContextMenuActions(QMenu &menu, ContextMenuKind kind)
{
    auto *view = graphicsViewOf(this);
    if (!view)
        return;

    auto addSepIfNeeded = [&menu]() {
        QList<QAction *> const acts = menu.actions();
        if (acts.isEmpty())
            return;
        if (acts.last()->isSeparator())
            return;
        menu.addSeparator();
    };

    auto addUndoRedo = [&]() {
        bool const hasUndo = view->undoAction() && view->undoAction()->isVisible();
        bool const hasRedo = view->redoAction() && view->redoAction()->isVisible();
        if (!hasUndo && !hasRedo)
            return;
        addSepIfNeeded();
        addActionIfPresent(menu, view->undoAction());
        addActionIfPresent(menu, view->redoAction());
    };

    switch (kind) {
    case ContextMenuKind::Scene:
        // 空白画布右键：不依赖图元的操作；添加节点改由双击 createSceneMenu
        addActionIfPresent(menu, view->pasteAction());
        addActionIfPresent(menu, view->searchNodeAction());
        addActionIfPresent(menu, view->clearSelectionAction());
        addUndoRedo();
        break;

    case ContextMenuKind::Connection:
        // 连线：删除 + 撤销/重做
        addSepIfNeeded();
        addActionIfPresent(menu, view->deleteSelectionAction());
        addUndoRedo();
        break;

    case ContextMenuKind::Node:
        // 节点：编辑/剪贴板 → 布局 → 工具 → 撤销
        addSepIfNeeded();
        addActionIfPresent(menu, view->deleteSelectionAction());
        addActionIfPresent(menu, view->duplicateSelectionAction());
        addActionIfPresent(menu, view->copySelectionAction());
        addActionIfPresent(menu, view->pasteAction());
        addActionIfPresent(menu, view->createGroupAction());
        addActionIfPresent(menu, view->alignLayoutAction());
        addSepIfNeeded();
        addActionIfPresent(menu, view->clearSelectionAction());
        addActionIfPresent(menu, view->searchNodeAction());
        addUndoRedo();
        break;

    case ContextMenuKind::Group:
        // 分组：删除/分组操作 → 工具 → 撤销
        addSepIfNeeded();
        addActionIfPresent(menu, view->deleteSelectionAction());
        addActionIfPresent(menu, view->createGroupAction());
        addSepIfNeeded();
        addActionIfPresent(menu, view->clearSelectionAction());
        addActionIfPresent(menu, view->searchNodeAction());
        addUndoRedo();
        break;
    }
}

bool CustomFlowGraphicsScene::save() const
{
    QString fileName = QFileDialog::getSaveFileName(nullptr,
                                                    tr("Open Flow Scene"),
                                                    QDir::homePath(),
                                                    tr("Flow Scene Files (*.childflow)"));

    if (!fileName.isEmpty()) {
        if (!fileName.endsWith("childflow", Qt::CaseInsensitive))
            fileName += ".childflow";

        QFile file(fileName);
        if (file.open(QIODevice::WriteOnly)) {
            file.write(QJsonDocument(_graphModel.save()).toJson());
            return true;
        }
    }
    return false;
}

bool CustomFlowGraphicsScene::load()
{
    QString fileName = QFileDialog::getOpenFileName(nullptr,
                                                    tr("Open Child Flow "),
                                                    QDir::homePath(),
                                                    tr("Flow Scene Files (*.childflow)"));

    if (!QFileInfo::exists(fileName))
        return false;

    QFile file(fileName);

    if (!file.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(nullptr, tr("打开失败"),
                             tr("无法打开文件 %1:\n%2").arg(fileName).arg(file.errorString()));
        return false;
    }

    QByteArray const wholeFile = file.readAll();
    auto jsonDoc = QJsonDocument::fromJson(wholeFile);
    if (jsonDoc.isNull()) {
        QMessageBox::warning(nullptr, tr("打开失败"), tr("无法解析文件 %1").arg(fileName));
        return false;
    }
    clearScene();
    _graphModel.load(jsonDoc.object());

    Q_EMIT sceneLoaded();

    return true;
}

void CustomFlowGraphicsScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event)
{
    QPointF scenePos = event->scenePos();
    QGraphicsItem *item = itemAt(scenePos, QTransform());
    if (_graphModel.getNodesLocked()) {
        return;
    }
    if (!item) {
        // 空白处双击：添加节点
        if (auto *menu = createSceneMenu(event->scenePos()))
            menu->exec(event->screenPos());
        event->accept();
    } else {
        QGraphicsScene::mouseDoubleClickEvent(event);
    }
}
