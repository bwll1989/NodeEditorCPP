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
#include <QComboBox>
#include "Widget/NodeWidget/DataflowViewsManger.hpp"

using QtNodes::NodeGraphicsObject;

NodeListWidget::NodeListWidget(DataflowViewsManger* viewsManager, QWidget *parent)
    : QWidget(parent), viewsManager(viewsManager), dataFlowModel(nullptr), dataFlowScene(nullptr) {

    sceneSelector = new QComboBox(this);
    sceneSelector->addItems(viewsManager ? viewsManager->sceneTitles() : QStringList());
    sceneSelector->setEditable(false);

    // 刷新按钮：强制刷新显示
    refreshButton = new QPushButton(this);
    refreshButton->setIcon(QIcon(":/icons/icons/reload.png"));
    refreshButton->setFixedSize(25,25);
    refreshButton->setFlat(true);
    connect(refreshButton, &QPushButton::clicked, this, &NodeListWidget::onRefreshClicked);

    // 搜索框
    searchBox = new QLineEdit(this);
    searchBox->setPlaceholderText("Search nodes...");

    // 树形视图
    nodeTree = new QTreeWidget(this);
    nodeTree->setColumnCount(1);
    nodeTree->setHeaderLabels(QStringList() << "Nodes");

    // 顶部行：下拉框 + 刷新按钮
    QHBoxLayout* topRow = new QHBoxLayout();
    topRow->addWidget(sceneSelector,10);
    topRow->addWidget(refreshButton,1);
    topRow->addStretch();

    // 布局
    auto layout = new QVBoxLayout(this);
    layout->addLayout(topRow);
    layout->addWidget(searchBox);
    layout->addWidget(nodeTree);
    setLayout(layout);

    // 事件/行为设置
    nodeTree->setContextMenuPolicy(Qt::CustomContextMenu);
    nodeTree->setMouseTracking(true);
    nodeTree->setDragEnabled(false);
    nodeTree->viewport()->installEventFilter(this);

    // 连接：场景切换
    connect(sceneSelector, &QComboBox::currentTextChanged, this, &NodeListWidget::onSceneSwitched);
    // 连接：搜索过滤
    connect(searchBox, &QLineEdit::textChanged, this, &NodeListWidget::filterNodes);
    // 连接：右键菜单
    connect(nodeTree, &QTreeWidget::customContextMenuRequested, this, &NodeListWidget::showContextMenu);
    // 连接：树选择变化
    connect(nodeTree, &QTreeWidget::itemSelectionChanged, this, &NodeListWidget::onTreeItemSelectionChanged);

    connect(viewsManager, &DataflowViewsManger::createNewScene, this, [this](QString title) {
        onRefreshClicked();
        sceneSelector->setCurrentText(title);
    });
    // 连接：场景激活
    connect(viewsManager, &DataflowViewsManger::sceneIsActive, this, [this](QString title) {
        sceneSelector->setCurrentText(title);
    });

    // 连接：场景移除
    connect(viewsManager, &DataflowViewsManger::removeScene, this, [this](QString title) {
        int idx = sceneSelector->findText(title);
        if (idx >= 0) sceneSelector->removeItem(idx);
    });

    // 选择默认场景：优先当前聚焦场景，否则选择第一个
    // QString activeTitle;
    // if (viewsManager) {
    //     activeTitle = viewsManager->focusedSceneTitle();
    //     if (activeTitle.isEmpty() && sceneSelector->count() > 0) {
    //         activeTitle = sceneSelector->itemText(0);
    //     }
    // }
    // if (!activeTitle.isEmpty()) {
    //     int idx = sceneSelector->findText(activeTitle);
    //     if (idx >= 0) sceneSelector->setCurrentIndex(idx);
    //     onSceneSwitched(activeTitle);
    // }
}

void NodeListWidget::onSceneSwitched(const QString& title)
{
    // 函数级注释：
    // 说明：当用户选择了不同的场景标题时，解绑旧的模型/场景信号，绑定到新的模型/场景，
    //       并重建节点树，保持 UI 同步。

    // 解绑旧信号
    if (dataFlowModel) {
        disconnect(dataFlowModel, nullptr, this, nullptr);
    }
    if (dataFlowScene) {
        disconnect(dataFlowScene, nullptr, this, nullptr);
    }

    // 绑定新模型/场景
    dataFlowModel = viewsManager ? viewsManager->modelByTitle(title) : nullptr;
    dataFlowScene = viewsManager ? viewsManager->sceneByTitle(title) : nullptr;

    // 安全检查
    if (!dataFlowModel || !dataFlowScene) {
        nodeTree->clear();
        return;
    }

    // 重新连接信号
    connect(dataFlowModel, &CustomDataFlowGraphModel::nodeCreated, this, &NodeListWidget::onNodeCreated);
    connect(dataFlowModel, &CustomDataFlowGraphModel::nodeDeleted, this, &NodeListWidget::onNodeDeleted);
    connect(dataFlowModel, &CustomDataFlowGraphModel::nodeUpdated, this, &NodeListWidget::onNodeUpdated);
    connect(dataFlowModel, &CustomDataFlowGraphModel::nodePositionUpdated, this, &NodeListWidget::onNodeUpdated);
    connect(dataFlowScene, &CustomFlowGraphicsScene::selectionChanged, this, &NodeListWidget::onSceneSelectionChanged);

    // 重建列表
    populateNodeTree();
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
    if (!dataFlowScene || !dataFlowModel) return;

    QList<QTreeWidgetItem*> selectedItems = nodeTree->selectedItems();
    if (selectedItems.isEmpty()) return;

    QTreeWidgetItem* item = selectedItems.first();
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
    if (!dataFlowScene) return;

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
    if (!dataFlowModel) return;

    for (const auto& nodeId : dataFlowModel->allNodeIds()) {
        QString nodeName = dataFlowModel->nodeData(nodeId, NodeRole::Remarks).toString();
        QTreeWidgetItem* nodeItem = new QTreeWidgetItem(nodeTree);
        nodeItem->setIcon(0, QIcon(":/icons/icons/model_1.png"));
        nodeItem->setText(0, QString("%1: %2").arg(nodeId).arg(nodeName));

        // 添加 Properties 分组
        QTreeWidgetItem* propertiesGroup = new QTreeWidgetItem(nodeItem);
        propertiesGroup->setText(0, "Properties");
        propertiesGroup->setIcon(0, QIcon(":/icons/icons/property.png"));
        // 添加位置属性到 Properties 分组
        QPointF nodePosition = dataFlowModel->nodeData(nodeId, NodeRole::Position).toPointF();
        QTreeWidgetItem* positionItem = new QTreeWidgetItem(propertiesGroup);
        positionItem->setText(0, QString("Position: (%1, %2)").arg(nodePosition.x()).arg(nodePosition.y()));
        // 添加 Controls 分组
        auto mapping = dataFlowModel->nodeData(nodeId, NodeRole::OSCAddress).value<std::unordered_map<QString, QWidget*>>();
        if(mapping.size()<=0){
           continue;
        }
        QTreeWidgetItem* controlsGroup = new QTreeWidgetItem(nodeItem);
        controlsGroup->setIcon(0, QIcon(":/icons/icons/command.png"));
        controlsGroup->setText(0, "Commands");
        for(auto it:mapping){
            QTreeWidgetItem* controlItem = new QTreeWidgetItem(controlsGroup);
            controlItem->setText(0, "/dataflow/" + dataFlowModel->nodeData(nodeId, NodeRole::ModelAlias).toString() + "/" + QString::number(nodeId)+it.first);
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
        controlItem->setText(0, "/dataflow/"+dataFlowModel->nodeData(nodeId, NodeRole::ModelAlias).toString() + "/" + QString::number(nodeId)+it.first);
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
    // 预先获取所有需要的数据，避免重复调用
    QString nodeName = dataFlowModel->nodeData(nodeId, NodeRole::Remarks).toString();
    QPointF nodePosition = dataFlowModel->nodeData(nodeId, NodeRole::Position).toPointF();
    QString nodeType = dataFlowModel->nodeData(nodeId, NodeRole::Type).toString();
    auto oscMapping = dataFlowModel->nodeData(nodeId, NodeRole::OSCAddress).value<std::unordered_map<QString, QWidget*>>();

    // 查找节点项
    QTreeWidgetItem* nodeItem = nullptr;
    QString nodeIdStr = QString::number(nodeId) + ":";

    // 使用二分查找或哈希表可以进一步优化，但这里先用简单的线性查找
    for (int i = 0; i < nodeTree->topLevelItemCount(); ++i) {
        QTreeWidgetItem* item = nodeTree->topLevelItem(i);
        if (item->text(0).startsWith(nodeIdStr)) {
            nodeItem = item;
            break;
        }
    }

    if (!nodeItem) return; // 未找到节点，直接返回

    // 更新节点名称
    nodeItem->setText(0, QString("%1: %2").arg(nodeId).arg(nodeName));

    // 使用映射表存储子组，避免重复查找
    QTreeWidgetItem* propertiesGroup = nullptr;
    QTreeWidgetItem* commandsGroup = nullptr;

    // 一次性查找所有需要的组
    for (int j = 0; j < nodeItem->childCount(); ++j) {
        QTreeWidgetItem* groupItem = nodeItem->child(j);
        QString groupText = groupItem->text(0);

        if (groupText == "Properties") {
            propertiesGroup = groupItem;
        } else if (groupText == "Commands") {
            commandsGroup = groupItem;
        }
    }

    // 处理Properties组
    if (propertiesGroup) {
        // 使用映射表存储属性项，避免重复查找
        QTreeWidgetItem* positionItem = nullptr;
        QTreeWidgetItem* typeItem = nullptr;
        QTreeWidgetItem* idItem = nullptr;

        // 一次性查找所有需要的属性项
        for (int k = 0; k < propertiesGroup->childCount(); ++k) {
            QTreeWidgetItem* propertyItem = propertiesGroup->child(k);
            QString propertyText = propertyItem->text(0);

            if (propertyText.startsWith("Position:")) {
                positionItem = propertyItem;
            } else if (propertyText.startsWith("Type:")) {
                typeItem = propertyItem;
            } else if (propertyText.startsWith("ID:")) {
                idItem = propertyItem;
            }
        }

        // 更新或创建位置项
        if (positionItem) {
            positionItem->setText(0, QString("Position: (%1, %2)")
                .arg(nodePosition.x())
                .arg(nodePosition.y()));
        } else {
            positionItem = new QTreeWidgetItem(propertiesGroup);
            positionItem->setText(0, QString("Position: (%1, %2)")
                .arg(nodePosition.x())
                .arg(nodePosition.y()));
        }

        // 更新或创建类型项
        if (typeItem) {
            typeItem->setText(0, QString("Type: %1").arg(nodeType));
        } else {
            typeItem = new QTreeWidgetItem(propertiesGroup);
            typeItem->setText(0, QString("Type: %1").arg(nodeType));
        }

        // 更新或创建ID项
        if (idItem) {
            idItem->setText(0, QString("ID: %1").arg(nodeId));
        } else {
            idItem = new QTreeWidgetItem(propertiesGroup);
            idItem->setText(0, QString("ID: %1").arg(nodeId));
        }
    } else {
        // 创建Properties组
        propertiesGroup = new QTreeWidgetItem(nodeItem);
        propertiesGroup->setText(0, "Properties");
        propertiesGroup->setIcon(0, QIcon(":/icons/icons/property.png"));

        // 一次性创建所有属性项
        QTreeWidgetItem* idItem = new QTreeWidgetItem(propertiesGroup);
        idItem->setText(0, QString("ID: %1").arg(nodeId));

        QTreeWidgetItem* positionItem = new QTreeWidgetItem(propertiesGroup);
        positionItem->setText(0, QString("Position: (%1, %2)")
            .arg(nodePosition.x())
            .arg(nodePosition.y()));

        QTreeWidgetItem* typeItem = new QTreeWidgetItem(propertiesGroup);
        typeItem->setText(0, QString("Type: %1").arg(nodeType));
    }

    // 处理Commands组
    bool hasCommands = !oscMapping.empty();

    if (commandsGroup) {
        if (hasCommands) {
            // 批量更新：先清除所有子项，然后一次性添加新的子项
            commandsGroup->takeChildren(); // 比逐个删除更高效

            // 预先分配足够的空间
            QList<QTreeWidgetItem*> newItems;
            newItems.reserve(oscMapping.size());

            QString prefix = "/dataflow/" + dataFlowModel->nodeData(nodeId, NodeRole::ModelAlias).toString() +"/"+QString::number(nodeId);
            for (const auto& it : oscMapping) {
                QTreeWidgetItem* controlItem = new QTreeWidgetItem();
                controlItem->setText(0, prefix + it.first);
                newItems.append(controlItem);
            }

            commandsGroup->addChildren(newItems); // 批量添加子项
        } else {
            // 如果没有命令，移除Commands组
            for (int j = 0; j < nodeItem->childCount(); ++j) {
                if (nodeItem->child(j) == commandsGroup) {
                    delete nodeItem->takeChild(j);
                    break;
                }
            }
        }
    } else if (hasCommands) {
        // 创建Commands组
        commandsGroup = new QTreeWidgetItem(nodeItem);
        commandsGroup->setText(0, "Commands");
        commandsGroup->setIcon(0, QIcon(":/icons/icons/command.png"));

        // 预先分配足够的空间
        QList<QTreeWidgetItem*> newItems;
        newItems.reserve(oscMapping.size());

        QString prefix = "/dataflow/" + dataFlowModel->nodeData(nodeId, NodeRole::ModelAlias).toString() + "/" + QString::number(nodeId);
        for (const auto& it : oscMapping) {
            QTreeWidgetItem* controlItem = new QTreeWidgetItem();
            controlItem->setText(0, prefix + it.first);
            newItems.append(controlItem);
        }

        commandsGroup->addChildren(newItems); // 批量添加子项
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
    if (!dataFlowModel || !dataFlowScene) return;

    QString nodeText = selectedItem->text(0);
    NodeId nodeId = nodeText.section(':', 0, 0).toInt();

    QMenu contextMenu;
    contextMenu.setWindowFlags(contextMenu.windowFlags() | Qt::NoDropShadowWindowHint);
    contextMenu.setAttribute(Qt::WA_TranslucentBackground, false);
    QAction* focusAction = contextMenu.addAction("Focus Node");
    focusAction->setIcon(QIcon(":/icons/icons/focus.png"));
    
    QAction* deleteAction = contextMenu.addAction("Delete Node");
    deleteAction->setIcon(QIcon(":/icons/icons/clear.png"));
    QAction* expandAction = contextMenu.addAction("Switch expand");
    expandAction->setIcon(QIcon(":/icons/icons/expand.png"));
     
    QAction* updateAction = contextMenu.addAction("Update Node");
    updateAction->setIcon(QIcon(":/icons/icons/restore.png"));
    
    connect(deleteAction, &QAction::triggered, [this, selectedItem]() {
        if (dataFlowScene) {
            dataFlowScene->undoStack().push(new QtNodes::DeleteCommand(dataFlowScene));
        }
    });
    connect(focusAction, &QAction::triggered, [this, nodeId]() {
        if (dataFlowScene) {
            dataFlowScene->centerOnNode(nodeId);
        }
    });
    connect(expandAction, &QAction::triggered, [this, nodeId]() {
        if (dataFlowModel) {
            dataFlowModel->setNodeData(nodeId,NodeRole::WidgetEmbeddable,!dataFlowModel->nodeData(nodeId, NodeRole::WidgetEmbeddable).toBool());
        }
    });
    if(dataFlowModel && dataFlowModel->getNodesLocked()){
        expandAction->setEnabled(false);
        deleteAction->setEnabled(false);
    }
    connect(updateAction, &QAction::triggered, [this, nodeId]() {
        onNodeUpdated(nodeId);
    });

    contextMenu.exec(nodeTree->viewport()->mapToGlobal(pos));
}

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

void NodeListWidget::onRefreshClicked()
{
    // 函数级注释：
    // 说明：刷新按钮点击后，重新获取场景标题列表，尽量保持当前标题选择（若仍存在），
    //       重新绑定模型与场景并重建节点树，强制更新显示。

    if (!viewsManager) {
        // 无管理器：仅重建列表
        populateNodeTree();
        nodeTree->viewport()->update();
        return;
    }

    const QString prevTitle = sceneSelector->currentText();
    const QStringList titles = viewsManager->sceneTitles();

    // 更新下拉框项目
    sceneSelector->blockSignals(true);
    sceneSelector->clear();
    sceneSelector->addItems(titles);

    // 保持当前选择（若仍存在），否则选择第一个
    QString newTitle = prevTitle;
    if (!titles.contains(prevTitle) && !titles.isEmpty()) {
        newTitle = titles.first();
    }
    int idx = sceneSelector->findText(newTitle);
    if (idx >= 0) sceneSelector->setCurrentIndex(idx);
    sceneSelector->blockSignals(false);

    // 触发场景重绑定与列表重建
    onSceneSwitched(newTitle);

    // 强制刷新显示
    nodeTree->viewport()->update();
    nodeTree->repaint();
}
