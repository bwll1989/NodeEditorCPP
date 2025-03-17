#include <QVBoxLayout>
#include <QLineEdit>
#include "NodeListWidget.hpp"
#include <QMenu>
#include <QAction>
#include "QtNodes/internal/NodeGraphicsObject.hpp"

using QtNodes::NodeGraphicsObject;

NodeListWidget::NodeListWidget(CustomDataFlowGraphModel* model, CustomFlowGraphicsScene* scene, QWidget *parent)
    : QWidget(parent), dataFlowModel(model), dataFlowScene(scene) {
    searchBox = new QLineEdit(this);
    searchBox->setPlaceholderText("Search nodes...");
    nodeTree = new QTreeWidget(this);
    nodeTree->setColumnCount(1);
    nodeTree->setStyleSheet("QTreeView::item { padding: 4px; } QTreeView::item:selected { background-color: #0078D7; }");
    nodeTree->setHeaderLabels(QStringList() << "Nodes");
    auto layout = new QVBoxLayout(this);
    layout->addWidget(searchBox);
    layout->addWidget(nodeTree);
    setLayout(layout);

    populateNodeTree();

    // Connect signals to slots
    // node创建时，更新nodeList中的node
    connect(dataFlowModel, &CustomDataFlowGraphModel::nodeCreated, this, &NodeListWidget::onNodeCreated);
    // node创建时，更新nodeList中的node
    connect(dataFlowModel, &CustomDataFlowGraphModel::nodeDeleted, this, &NodeListWidget::onNodeDeleted);
    // node更新时，更新nodeList中的node
    connect(dataFlowModel, &CustomDataFlowGraphModel::nodeUpdated, this, &NodeListWidget::onNodeUpdated);
    // node位置更新时，更新nodeList中的位置
    connect(dataFlowModel, &CustomDataFlowGraphModel::nodePositionUpdated, this, &NodeListWidget::onNodeUpdated);
    // 搜索框文本变化时，过滤nodeList中的node
    connect(searchBox, &QLineEdit::textChanged, this, &NodeListWidget::filterNodes);
    connect(nodeTree, &QTreeWidget::itemSelectionChanged, this, &NodeListWidget::onTreeItemSelectionChanged);
    connect(dataFlowScene, &CustomFlowGraphicsScene::selectionChanged, this, &NodeListWidget::onSceneSelectionChanged);

    // Set context menu policy
    nodeTree->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(nodeTree, &QTreeWidget::customContextMenuRequested, this, &NodeListWidget::showContextMenu);
}

void NodeListWidget::onTreeItemSelectionChanged() {
    if (isUpdatingSelection) return;
    
    QList<QTreeWidgetItem*> selectedItems = nodeTree->selectedItems();
    if (selectedItems.isEmpty()) return;

    QTreeWidgetItem* item = selectedItems.first();
    // 如果选中的是子节点，找到其父节点
    if (item->parent()) {
        item = item->parent();
    }
    
    QString nodeText = item->text(0);
    NodeId nodeId = nodeText.section(':', 0, 0).toInt();
    
    isUpdatingSelection = true;
    dataFlowScene->clearSelection();
    if (auto nodeObj = dataFlowScene->nodeGraphicsObject(nodeId)) {
        nodeObj->setSelected(true);
    }
    isUpdatingSelection = false;
}

void NodeListWidget::onSceneSelectionChanged() {
    if (isUpdatingSelection) return;
    
    isUpdatingSelection = true;
    nodeTree->clearSelection();
    auto selectedNodes = dataFlowScene->selectedNodes();
    for (NodeId nodeId : selectedNodes) {
        if (QTreeWidgetItem* item = findNodeItem(nodeId)) {
            item->setSelected(true);
            nodeTree->scrollToItem(item);
        }
    }
    isUpdatingSelection = false;
}

QTreeWidgetItem* NodeListWidget::findNodeItem(NodeId nodeId) {
    for (int i = 0; i < nodeTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = nodeTree->topLevelItem(i);
        if (item->text(0).startsWith(QString::number(nodeId) + ":")) {
            return item;
        }
    }
    return nullptr;
}

void NodeListWidget::populateNodeTree() {
    nodeTree->clear();
    for (const auto& nodeId : dataFlowModel->allNodeIds()) {
        QString nodeName = dataFlowModel->nodeData(nodeId, NodeRole::Type).toString();
        QTreeWidgetItem* nodeItem = new QTreeWidgetItem(nodeTree);
        nodeItem->setIcon(0,QIcon(":/icons/icons/model_1.png"));
        nodeItem->setText(0, QString("%1: %2").arg(nodeId).arg(nodeName));

        // Add properties as child items
        QTreeWidgetItem* propertyItem = new QTreeWidgetItem(nodeItem);
        propertyItem->setIcon(0,QIcon(":/icons/icons/property.png"));
        propertyItem->setText(0, "Property: Value");
        nodeItem->addChild(propertyItem);

        // 添加位置信息
        QTreeWidgetItem* positionItem = new QTreeWidgetItem(nodeItem);
        positionItem->setIcon(0,QIcon(":/icons/icons/property.png"));
        positionItem->setText(0, QString("Position: "));
        nodeItem->addChild(positionItem);
    }
}

void NodeListWidget::onNodeCreated(NodeId nodeId) {
    QString nodeName = dataFlowModel->nodeData(nodeId, NodeRole::Type).toString();
    QTreeWidgetItem* nodeItem = new QTreeWidgetItem(nodeTree);
    nodeItem->setIcon(0,QIcon(":/icons/icons/model_1.png"));
    nodeItem->setText(0, QString("%1: %2").arg(nodeId).arg(nodeName));

    // Add properties as child items
    QTreeWidgetItem* propertyItem = new QTreeWidgetItem(nodeItem);
    propertyItem->setIcon(0,QIcon(":/icons/icons/property.png"));
    propertyItem->setText(0, "Property: Value");
    nodeItem->addChild(propertyItem);

    // Add position as a property
    QPointF nodePosition = dataFlowModel->nodeData(nodeId, NodeRole::Position).toPointF();
    QTreeWidgetItem* positionItem = new QTreeWidgetItem(nodeItem);
    positionItem->setIcon(0,QIcon(":/icons/icons/property.png"));
    positionItem->setText(0, QString("Position: (%1, %2)").arg(nodePosition.x()).arg(nodePosition.y()));
    nodeItem->addChild(positionItem);
}

void NodeListWidget::onNodeDeleted(NodeId nodeId) {
    for (int i = 0; i < nodeTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* nodeItem = nodeTree->topLevelItem(i);
        if (nodeItem->text(0).startsWith(QString::number(nodeId) + ":")) {
            delete nodeTree->takeTopLevelItem(i);
            break;
        }
    }
}

void NodeListWidget::onNodeUpdated(NodeId nodeId) {
    for (int i = 0; i < nodeTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* nodeItem = nodeTree->topLevelItem(i);
        if (nodeItem->text(0).startsWith(QString::number(nodeId) + ":")) {
            // Update node name
            QString nodeName = dataFlowModel->nodeData(nodeId, NodeRole::Type).toString();
            nodeItem->setText(0, QString("%1: %2").arg(nodeId).arg(nodeName));

            // Update position
            QPointF nodePosition = dataFlowModel->nodeData(nodeId, NodeRole::Position).toPointF();
            for (int j = 0; j < nodeItem->childCount(); ++j) {
                QTreeWidgetItem* childItem = nodeItem->child(j);
                if (childItem->text(0).startsWith("Position:")) {
                    childItem->setText(0, QString("Position: (%1, %2)").arg(nodePosition.x()).arg(nodePosition.y()));
                    break;
                }
            }
            break;
        }
    }
}

void NodeListWidget::filterNodes(const QString& query) {
    for (int i = 0; i < nodeTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* nodeItem = nodeTree->topLevelItem(i);
        bool matchInNode = nodeItem->text(0).contains(query, Qt::CaseInsensitive);
        bool matchInChildren = false;

        // Search through child items
        for (int j = 0; j < nodeItem->childCount(); ++j) {
            QTreeWidgetItem* childItem = nodeItem->child(j);
            if (childItem->text(0).contains(query, Qt::CaseInsensitive)) {
                matchInChildren = true;
                childItem->setHidden(false);
            } else {
                childItem->setHidden(!query.isEmpty());
            }
        }

        // Show the node if either the node itself or any of its children match
        nodeItem->setHidden(!(matchInNode || matchInChildren));
        
        // If the node matches, show all its children
        if (matchInNode) {
            for (int j = 0; j < nodeItem->childCount(); ++j) {
                nodeItem->child(j)->setHidden(false);
            }
        }
    }
}

void NodeListWidget::showContextMenu(const QPoint &pos) {
    QTreeWidgetItem* selectedItem = nodeTree->itemAt(pos);
    if (!selectedItem) return;

    QMenu contextMenu;
    QAction* focusAction = contextMenu.addAction("Focus Node");
    focusAction->setIcon(QIcon(":/icons/icons/focus.png"));
    QAction* deleteAction = contextMenu.addAction("Delete Node");
    deleteAction->setIcon(QIcon(":/icons/icons/clear.png"));
    connect(deleteAction, &QAction::triggered, [this, selectedItem]() {
        QString nodeText = selectedItem->text(0);
        NodeId nodeId = nodeText.section(':', 0, 0).toInt();
        dataFlowScene->undoStack().push(new QtNodes::DeleteCommand(dataFlowScene));
    });
    connect(focusAction, &QAction::triggered, [this, selectedItem]() {
        QString nodeText = selectedItem->text(0);
        NodeId nodeId = nodeText.section(':', 0, 0).toInt();
        dataFlowScene->centerOnNode(nodeId);
    });

    contextMenu.exec(nodeTree->viewport()->mapToGlobal(pos));
}
