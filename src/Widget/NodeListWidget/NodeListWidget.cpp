#include <QVBoxLayout>
#include <QLineEdit>
#include "NodeListWidget.hpp"
#include <QMenu>
#include <QAction>
#include "QtNodes/internal/NodeGraphicsObject.hpp"
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include <QApplication>
#include <QPainter>
#include <QPixmap>

using QtNodes::NodeGraphicsObject;

NodeListWidget::NodeListWidget(CustomDataFlowGraphModel* model, CustomFlowGraphicsScene* scene, QWidget *parent)
    : QWidget(parent), dataFlowModel(model), dataFlowScene(scene) {
    searchBox = new QLineEdit(this);
    searchBox->setPlaceholderText("Search nodes...");
    nodeTree = new QTreeWidget(this);
    nodeTree->setColumnCount(1);
    nodeTree->setStyleSheet("QTreeView::item { padding: 2px; } QTreeView::item:selected { background-color: #0078D7; }");
    nodeTree->setHeaderLabels(QStringList() << "Nodes");
    auto layout = new QVBoxLayout(this);
    layout->addWidget(searchBox);
    layout->addWidget(nodeTree);
    setLayout(layout);

    // populateNodeTree();

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

    // 启用鼠标追踪
    nodeTree->setMouseTracking(true);
    // 允许拖拽
    nodeTree->setDragEnabled(false);

    // 修改拖拽设置
    nodeTree->viewport()->installEventFilter(this);  // 安装事件过滤器到视口
}

bool NodeListWidget::eventFilter(QObject* watched, QEvent* event)
{
    if (watched == nodeTree->viewport()) {
        switch (event->type()) {
            case QEvent::MouseButtonPress: {
                QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                if (mouseEvent->button() == Qt::LeftButton) {
                    QTreeWidgetItem* item = nodeTree->itemAt(mouseEvent->pos());
                    if (item && isCommand(item)) {
                        dragStartPosition = mouseEvent->pos();
                        isDragging = true;
                        return true;  // 拦截事件
                    }
                }
                break;
            }
            case QEvent::MouseMove: {
                QMouseEvent* mouseEvent = static_cast<QMouseEvent*>(event);
                if (isDragging && (mouseEvent->buttons() & Qt::LeftButton)) {
                    if ((mouseEvent->pos() - dragStartPosition).manhattanLength() 
                        >= QApplication::startDragDistance()) {
                        QTreeWidgetItem* item = nodeTree->itemAt(dragStartPosition);
                        if (item && isCommand(item)) {
                            startDrag(item);
                            isDragging = false;
                            return true;  // 拦截事件
                        }
                    }
                }
                break;
            }
            case QEvent::MouseButtonRelease: {
                if (isDragging) {
                    isDragging = false;
                }
                break;
            }
            default:
                break;
        }
    }
    return QWidget::eventFilter(watched, event);
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
        nodeItem->setIcon(0, QIcon(":/icons/icons/model_1.png"));
        nodeItem->setText(0, QString("%1: %2").arg(nodeId).arg(nodeName));

        // 添加 Properties 分组
        QTreeWidgetItem* propertiesGroup = new QTreeWidgetItem(nodeItem);
        propertiesGroup->setText(0, "Properties");
        
        // 添加位置属性到 Properties 分组
        QPointF nodePosition = dataFlowModel->nodeData(nodeId, NodeRole::Position).toPointF();
        QTreeWidgetItem* positionItem = new QTreeWidgetItem(propertiesGroup);
        positionItem->setIcon(0, QIcon(":/icons/icons/property.png"));
        positionItem->setText(0, QString("Position: (%1, %2)").arg(nodePosition.x()).arg(nodePosition.y()));
        // 添加备注属性到 Properties 分组
        QTreeWidgetItem* remarksItem = new QTreeWidgetItem(propertiesGroup);
        remarksItem->setIcon(0, QIcon(":/icons/icons/remarks.png"));
        remarksItem->setText(0, QString("Remarks: %1").arg(dataFlowModel->nodeData(nodeId, NodeRole::Remarks).toString()));
        // 添加 Controls 分组
        QTreeWidgetItem* controlsGroup = new QTreeWidgetItem(nodeItem);
        controlsGroup->setIcon(0, QIcon(":/icons/icons/cable.png"));
        controlsGroup->setText(0, "Controls");
        auto mapping = dataFlowModel->nodeData(nodeId, NodeRole::OSCAddress).value<std::unordered_map<QString, QWidget*>>();
        for(auto it:mapping){
            QTreeWidgetItem* controlItem = new QTreeWidgetItem(controlsGroup);
            controlItem->setIcon(0, QIcon(":/icons/icons/cable.png"));
            controlItem->setText(0, it.first);
        }
    }
}

void NodeListWidget::onNodeCreated(NodeId nodeId) {
    QString nodeName = dataFlowModel->nodeData(nodeId, NodeRole::Remarks).toString();
    QTreeWidgetItem* nodeItem = new QTreeWidgetItem(nodeTree);
    nodeItem->setIcon(0, QIcon(":/icons/icons/model_1.png"));
    nodeItem->setText(0, QString("%1: %2").arg(nodeId).arg(nodeName));

    // 添加 Properties 分组
    QTreeWidgetItem* propertiesGroup = new QTreeWidgetItem(nodeItem);
    propertiesGroup->setText(0, "Properties");
    propertiesGroup->setIcon(0, QIcon(":/icons/icons/property.png"));
    //添加ID显示到Properties分组
    QTreeWidgetItem* idItem = new QTreeWidgetItem(propertiesGroup);
    idItem->setText(0, QString("ID: %1").arg(nodeId));
    // 添加位置属性到 Properties 分组
    QPointF nodePosition = dataFlowModel->nodeData(nodeId, NodeRole::Position).toPointF();
    QTreeWidgetItem* positionItem = new QTreeWidgetItem(propertiesGroup);
    positionItem->setText(0, QString("Position: (%1, %2)").arg(nodePosition.x()).arg(nodePosition.y()));
     // 添加备注属性到 Properties 分组
    QTreeWidgetItem* remarksItem = new QTreeWidgetItem(propertiesGroup);
    remarksItem->setText(0, QString("Type: %1").arg(dataFlowModel->nodeData(nodeId, NodeRole::Type).toString()));

    // 添加 Controls 分组
   
    auto mapping = dataFlowModel->nodeData(nodeId, NodeRole::OSCAddress).value<std::unordered_map<QString, QWidget*>>();
    if(mapping.size()<=0){
        return;
    }
    QTreeWidgetItem* controlsGroup = new QTreeWidgetItem(nodeItem);
    controlsGroup->setText(0, "Commands");
    controlsGroup->setIcon(0, QIcon(":/icons/icons/command.png"));
    for(auto it:mapping){
        QTreeWidgetItem* controlItem = new QTreeWidgetItem(controlsGroup);
        controlItem->setText(0, "/dataflow/"+QString::number(nodeId)+it.first);
    }
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
            // 更新节点名称
            QString nodeName = dataFlowModel->nodeData(nodeId, NodeRole::Remarks).toString();
            nodeItem->setText(0, QString("%1: %2").arg(nodeId).arg(nodeName));

            // 更新位置信息
            QPointF nodePosition = dataFlowModel->nodeData(nodeId, NodeRole::Position).toPointF();
            
            // 查找 Properties 分组
            for (int j = 0; j < nodeItem->childCount(); ++j) {
                QTreeWidgetItem* groupItem = nodeItem->child(j);
                if (groupItem->text(0) == "Properties") {
                    // 更新位置属性
                    for (int k = 0; k < groupItem->childCount(); ++k) {
                        QTreeWidgetItem* propertyItem = groupItem->child(k);
                        if (propertyItem->text(0).startsWith("Position:")) {
                            propertyItem->setText(0, QString("Position: (%1, %2)")
                                .arg(nodePosition.x())
                                .arg(nodePosition.y()));
                            break;
                        }
                    }
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
    QString nodeText = selectedItem->text(0);
    NodeId nodeId = nodeText.section(':', 0, 0).toInt();

    QMenu contextMenu;
    QAction* focusAction = contextMenu.addAction("Focus Node");
    focusAction->setIcon(QIcon(":/icons/icons/focus.png"));
    
    QAction* deleteAction = contextMenu.addAction("Delete Node");
    deleteAction->setIcon(QIcon(":/icons/icons/clear.png"));
    QAction* expandAction = contextMenu.addAction("Switch expand");
    expandAction->setIcon(QIcon(":/icons/icons/expand.png"));
    connect(deleteAction, &QAction::triggered, [this, selectedItem]() {
        dataFlowScene->undoStack().push(new QtNodes::DeleteCommand(dataFlowScene));
    });
    connect(focusAction, &QAction::triggered, [this, nodeId]() {
        dataFlowScene->centerOnNode(nodeId);
    });
    connect(expandAction, &QAction::triggered, [this, nodeId]() {
        dataFlowModel->setNodeData(nodeId,NodeRole::WidgetEmbeddable,!dataFlowModel->nodeData(nodeId, NodeRole::WidgetEmbeddable).toBool());
    });
    if(dataFlowModel->getNodesLocked()){
        expandAction->setEnabled(false);
        deleteAction->setEnabled(false);
    }
    contextMenu.exec(nodeTree->viewport()->mapToGlobal(pos));
}

// 添加辅助函数实现
bool NodeListWidget::isCommand(const QTreeWidgetItem* item) const {
    return item->parent() && item->parent()->text(0) == "Commands";
}

void NodeListWidget::startDrag(QTreeWidgetItem* item) {
    OSCMessage message;
    message.address = item->text(0);
    message.host = "127.0.0.1";
    message.port = 8991;
    message.value = 1;

    QByteArray itemData;
    QDataStream dataStream(&itemData, QIODevice::WriteOnly);
    dataStream << message.host << message.port << message.address << message.value;

    QMimeData* mimeData = new QMimeData;
    mimeData->setData("application/x-osc-address", itemData);

    QDrag* drag = new QDrag(nodeTree->viewport());  // 从视口创建拖拽
    drag->setMimeData(mimeData);

    QPixmap pixmap(200, 30);
    pixmap.fill(Qt::transparent);
    
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // 绘制背景
    QColor bgColor(40, 40, 40, 200);  // 半透明深灰色
    painter.setBrush(bgColor);
    painter.setPen(Qt::NoPen);
    painter.drawRoundedRect(pixmap.rect(), 5, 5);  // 圆角矩形
    // 绘制文本
    painter.setPen(Qt::white);
    QFont font = painter.font();
    font.setPointSize(9);
    painter.setFont(font);
    QRect textRect = pixmap.rect().adjusted(30, 0, -8, 0);  // 图标右侧的文本区域
    painter.drawText(textRect, Qt::AlignVCenter | Qt::AlignLeft, message.address);

    // 设置拖拽预览
    drag->setPixmap(pixmap);
    drag->setHotSpot(QPoint(pixmap.width()/2, pixmap.height()/2));  // 热点在中心

    Qt::DropAction dropAction = drag->exec(Qt::CopyAction);
}
