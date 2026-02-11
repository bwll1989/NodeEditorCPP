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
#include <QClipboard>
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
    QAction *searchIconAction = new QAction(QIcon(":/icons/icons/search.png"), tr("搜索"), searchBox);
    searchBox->addAction(searchIconAction, QLineEdit::LeadingPosition);
    searchIconAction->setEnabled(false);
    // 树形视图（基于 QTreeView）
    nodeTree = new QTreeView(this);
    nodeModel = new QStandardItemModel(this);
    nodeModel->setHorizontalHeaderLabels(QStringList() << "Nodes");
    nodeModel->setColumnCount(1);
    nodeTree->setModel(nodeModel);
    // 禁用编辑，双击展开
    nodeTree->setEditTriggers(QAbstractItemView::NoEditTriggers);

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
    connect(nodeTree, &QTreeView::customContextMenuRequested, this, &NodeListWidget::showContextMenu);
    // 连接：树选择变化
    connect(nodeTree->selectionModel(), &QItemSelectionModel::selectionChanged, this, [this](const QItemSelection&, const QItemSelection&) {
        onTreeItemSelectionChanged();
    });
    // 连接：双击事件
    connect(nodeTree, &QTreeView::doubleClicked, this, &NodeListWidget::onTreeDoubleClicked);

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
        nodeModel->clear();
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
                    QModelIndex index = nodeTree->indexAt(mouseEvent->pos());
                    if (index.isValid() && isCommand(index)) {
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
                        QModelIndex index = nodeTree->indexAt(dragStartPosition);
                        if (index.isValid() && isCommand(index)) {
                            startDrag(index);
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

    QModelIndexList selectedIndexes = nodeTree->selectionModel()->selectedIndexes();
    if (selectedIndexes.isEmpty()) return;
    QModelIndex index = selectedIndexes.first();
    while (index.parent().isValid()) index = index.parent();
    QString nodeText = nodeModel->data(index, Qt::DisplayRole).toString();
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
    nodeTree->selectionModel()->clearSelection();
    auto selectedNodes = dataFlowScene->selectedNodes();
    for (NodeId nodeId : selectedNodes) {
        if (QStandardItem* item = findNodeItem(nodeId)) {
            QModelIndex idx = item->index();
            nodeTree->selectionModel()->select(idx, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);
            nodeTree->scrollTo(idx);
        }
    }
    isUpdatingSelection = false;
}

QStandardItem* NodeListWidget::findNodeItem(NodeId nodeId) {
    QStandardItem* root = nodeModel->invisibleRootItem();
    for (int i = 0; i < root->rowCount(); ++i) {
        QStandardItem* item = root->child(i, 0);
        if (item && item->text().startsWith(QString::number(nodeId) + ":")) {
            return item;
        }
    }
    return nullptr;
}

void NodeListWidget::populateNodeTree() {
    nodeModel->clear();
    nodeModel->setHorizontalHeaderLabels(QStringList() << "Nodes");
    if (!dataFlowModel) return;

    for (const auto& nodeId : dataFlowModel->allNodeIds()) {
        QString nodeName = dataFlowModel->nodeData(nodeId, NodeRole::Remarks).toString();
        auto* nodeItem = new QStandardItem(QIcon(":/icons/icons/model_1.png"), QString("%1: %2").arg(nodeId).arg(nodeName));
        nodeModel->invisibleRootItem()->appendRow(nodeItem);

        // 添加 Properties 分组
        auto* propertiesGroup = new QStandardItem(QIcon(":/icons/icons/property.png"), "Properties");
        nodeItem->appendRow(propertiesGroup);
        // 添加位置属性到 Properties 分组
        QPointF nodePosition = dataFlowModel->nodeData(nodeId, NodeRole::Position).toPointF();
        auto* positionItem = new QStandardItem(QString("Position: (%1, %2)").arg(nodePosition.x()).arg(nodePosition.y()));
        propertiesGroup->appendRow(positionItem);
        // 添加 Controls 分组
        auto mapping = dataFlowModel->nodeData(nodeId, NodeRole::OSCAddress).value<std::unordered_map<QString, QWidget*>>();
        if(mapping.size()<=0){
           continue;
        }
        auto* controlsGroup = new QStandardItem(QIcon(":/icons/icons/command.png"), "Commands");
        nodeItem->appendRow(controlsGroup);
        for(auto it:mapping){
            auto* controlItem = new QStandardItem("/dataflow/" + dataFlowModel->nodeData(nodeId, NodeRole::ModelAlias).toString() + "/" + QString::number(nodeId)+it.first);
            controlsGroup->appendRow(controlItem);
        }
    }
}

void NodeListWidget::onNodeCreated(NodeId nodeId) {
    QString nodeName = dataFlowModel->nodeData(nodeId, NodeRole::Remarks).toString();
    auto* nodeItem = new QStandardItem(QIcon(":/icons/icons/model_1.png"), QString("%1: %2").arg(nodeId).arg(nodeName));
    nodeModel->invisibleRootItem()->appendRow(nodeItem);
    // 添加 Properties 分组
    auto* propertiesGroup = new QStandardItem(QIcon(":/icons/icons/property.png"), "Properties");
    nodeItem->appendRow(propertiesGroup);
    //添加ID显示到Properties分组
    auto* idItem = new QStandardItem(QString("ID: %1").arg(nodeId));
    propertiesGroup->appendRow(idItem);
    // 添加位置属性到 Properties 分组
    QPointF nodePosition = dataFlowModel->nodeData(nodeId, NodeRole::Position).toPointF();
    auto* positionItem = new QStandardItem(QString("Position: (%1, %2)").arg(nodePosition.x()).arg(nodePosition.y()));
    propertiesGroup->appendRow(positionItem);
     // 添加备注属性到 Properties 分组
    auto* remarksItem = new QStandardItem(QString("Type: %1").arg(dataFlowModel->nodeData(nodeId, NodeRole::Type).toString()));
    propertiesGroup->appendRow(remarksItem);
    // 添加 Controls 分组
    auto mapping = dataFlowModel->nodeData(nodeId, NodeRole::OSCAddress).value<std::unordered_map<QString, QWidget*>>();
    if(mapping.size()<=0){
        return;
    }
    auto* controlsGroup = new QStandardItem(QIcon(":/icons/icons/command.png"), "Commands");
    nodeItem->appendRow(controlsGroup);
    for(auto it:mapping){
        auto* controlItem = new QStandardItem("/dataflow/"+dataFlowModel->nodeData(nodeId, NodeRole::ModelAlias).toString() + "/" + QString::number(nodeId)+it.first);
        controlsGroup->appendRow(controlItem);
    }
}

void NodeListWidget::onNodeDeleted(NodeId nodeId) {
    QStandardItem* root = nodeModel->invisibleRootItem();
    for (int i = 0; i < root->rowCount(); ++i) {
        QStandardItem* nodeItem = root->child(i, 0);
        if (nodeItem && nodeItem->text().startsWith(QString::number(nodeId) + ":")) {
            root->removeRow(i);
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
    QStandardItem* nodeItem = nullptr;
    QString nodeIdStr = QString::number(nodeId) + ":";

    // 使用线性查找顶层项
    nodeItem = findNodeItem(nodeId);

    if (!nodeItem) return; // 未找到节点，直接返回

    // 更新节点名称
    nodeItem->setText(QString("%1: %2").arg(nodeId).arg(nodeName));

    // 使用映射表存储子组，避免重复查找
    QStandardItem* propertiesGroup = nullptr;
    QStandardItem* commandsGroup = nullptr;

    // 一次性查找所有需要的组
    for (int j = 0; j < nodeItem->rowCount(); ++j) {
        QStandardItem* groupItem = nodeItem->child(j, 0);
        QString groupText = groupItem->text();

        if (groupText == "Properties") {
            propertiesGroup = groupItem;
        } else if (groupText == "Commands") {
            commandsGroup = groupItem;
        }
    }

    // 处理Properties组
    if (propertiesGroup) {
        // 使用映射表存储属性项，避免重复查找
        QStandardItem* positionItem = nullptr;
        QStandardItem* typeItem = nullptr;
        QStandardItem* idItem = nullptr;

        // 一次性查找所有需要的属性项
        for (int k = 0; k < propertiesGroup->rowCount(); ++k) {
            QStandardItem* propertyItem = propertiesGroup->child(k, 0);
            QString propertyText = propertyItem->text();

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
            positionItem->setText(QString("Position: (%1, %2)")
                .arg(nodePosition.x())
                .arg(nodePosition.y()));
        } else {
            positionItem = new QStandardItem(QString("Position: (%1, %2)")
                .arg(nodePosition.x())
                .arg(nodePosition.y()));
            propertiesGroup->appendRow(positionItem);
        }

        // 更新或创建类型项
        if (typeItem) {
            typeItem->setText(QString("Type: %1").arg(nodeType));
        } else {
            typeItem = new QStandardItem(QString("Type: %1").arg(nodeType));
            propertiesGroup->appendRow(typeItem);
        }

        // 更新或创建ID项
        if (idItem) {
            idItem->setText(QString("ID: %1").arg(nodeId));
        } else {
            idItem = new QStandardItem(QString("ID: %1").arg(nodeId));
            propertiesGroup->appendRow(idItem);
        }
    } else {
        // 创建Properties组
        propertiesGroup = new QStandardItem(QIcon(":/icons/icons/property.png"), "Properties");
        nodeItem->appendRow(propertiesGroup);

        // 一次性创建所有属性项
        QStandardItem* idItem = new QStandardItem(QString("ID: %1").arg(nodeId));
        propertiesGroup->appendRow(idItem);

        QStandardItem* positionItem = new QStandardItem(QString("Position: (%1, %2)")
            .arg(nodePosition.x())
            .arg(nodePosition.y()));
        propertiesGroup->appendRow(positionItem);

        QStandardItem* typeItem = new QStandardItem(QString("Type: %1").arg(nodeType));
        propertiesGroup->appendRow(typeItem);
    }

    // 处理Commands组
    bool hasCommands = !oscMapping.empty();

    if (commandsGroup) {
        if (hasCommands) {
            // 批量更新：先清除所有子项，然后一次性添加新的子项
            while (commandsGroup->rowCount() > 0) {
                commandsGroup->removeRow(0);
            }

            // 预先分配足够的空间
            QList<QStandardItem*> newItems;
            newItems.reserve(int(oscMapping.size()));

            QString prefix = "/dataflow/" + dataFlowModel->nodeData(nodeId, NodeRole::ModelAlias).toString() +"/"+QString::number(nodeId);
            for (const auto& it : oscMapping) {
                QStandardItem* controlItem = new QStandardItem(prefix + it.first);
                newItems.append(controlItem);
            }

            for (auto* it : newItems) commandsGroup->appendRow(it); // 批量添加子项
        } else {
            // 如果没有命令，移除Commands组
            for (int j = 0; j < nodeItem->rowCount(); ++j) {
                if (nodeItem->child(j, 0) == commandsGroup) {
                    nodeItem->removeRow(j);
                    break;
                }
            }
        }
    } else if (hasCommands) {
        // 创建Commands组
        commandsGroup = new QStandardItem(QIcon(":/icons/icons/command.png"), "Commands");
        nodeItem->appendRow(commandsGroup);

        // 预先分配足够的空间
        QList<QStandardItem*> newItems;
        newItems.reserve(int(oscMapping.size()));

        QString prefix = "/dataflow/" + dataFlowModel->nodeData(nodeId, NodeRole::ModelAlias).toString() + "/" + QString::number(nodeId);
        for (const auto& it : oscMapping) {
            QStandardItem* controlItem = new QStandardItem(prefix + it.first);
            newItems.append(controlItem);
        }

        for (auto* it : newItems) commandsGroup->appendRow(it); // 批量添加子项
    }
}

void NodeListWidget::filterNodes(const QString& query) {
    QStandardItem* root = nodeModel->invisibleRootItem();
    for (int i = 0; i < root->rowCount(); ++i) {
        QStandardItem* nodeItem = root->child(i, 0);
        if (!nodeItem) continue;
        bool matchInNode = nodeItem->text().contains(query, Qt::CaseInsensitive);
        bool matchInChildren = false;

        // 遍历组与子项
        for (int j = 0; j < nodeItem->rowCount(); ++j) {
            QStandardItem* groupItem = nodeItem->child(j, 0);
            if (!groupItem) continue;
            for (int k = 0; k < groupItem->rowCount(); ++k) {
                QStandardItem* childItem = groupItem->child(k, 0);
                if (!childItem) continue;
                bool match = childItem->text().contains(query, Qt::CaseInsensitive);
                nodeTree->setRowHidden(k, groupItem->index(), !match && !query.isEmpty());
                if (match) matchInChildren = true;
            }
        }

        // 顶层显示控制
        nodeTree->setRowHidden(i, QModelIndex(), !(matchInNode || matchInChildren));

        // 若顶层匹配，则显示所有子项
        if (matchInNode) {
            for (int j = 0; j < nodeItem->rowCount(); ++j) {
                QStandardItem* groupItem = nodeItem->child(j, 0);
                if (!groupItem) continue;
                for (int k = 0; k < groupItem->rowCount(); ++k) {
                    nodeTree->setRowHidden(k, groupItem->index(), false);
                }
            }
        }
    }
}

void NodeListWidget::showContextMenu(const QPoint &pos) {
    QModelIndex index = nodeTree->indexAt(pos);
    if (!index.isValid()) return;
    if (!dataFlowModel || !dataFlowScene) return;

    // 顶层索引
    while (index.parent().isValid()) index = index.parent();
    QString nodeText = nodeModel->data(index, Qt::DisplayRole).toString();
    NodeId nodeId = nodeText.section(':', 0, 0).toInt();

    QMenu contextMenu;
    contextMenu.setWindowFlags(contextMenu.windowFlags() | Qt::NoDropShadowWindowHint);
    contextMenu.setAttribute(Qt::WA_TranslucentBackground, false);
    QAction* focusAction = contextMenu.addAction("Focus Node");
    focusAction->setIcon(QIcon(":/icons/icons/focus.png"));
    QAction* expandAction = contextMenu.addAction("Switch expand");
    expandAction->setIcon(QIcon(":/icons/icons/expand.png"));
    QAction* updateAction = contextMenu.addAction("Update Node");
    updateAction->setIcon(QIcon(":/icons/icons/restore.png"));
    QAction* deleteAction = contextMenu.addAction("Delete Node");
    deleteAction->setIcon(QIcon(":/icons/icons/clear.png"));
    connect(deleteAction, &QAction::triggered, [this]() {
        if (dataFlowScene) {
            dataFlowScene->undoStack().push(new QtNodes::DeleteCommand(dataFlowScene));
        }
    });
    connect(focusAction, &QAction::triggered, [this, nodeId]() {
        if (dataFlowScene && !dataFlowScene->views().isEmpty()) {
            CustomGraphicsView view = dataFlowScene->views().first();
            //首先重置缩放
            view.resetTransform();
            //视图中心移至节点
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

bool NodeListWidget::isCommand(const QModelIndex& index) const {
    if (!index.isValid()) return false;
    QModelIndex parentIndex = index.parent();
    if (!parentIndex.isValid()) return false;
    return nodeModel->data(parentIndex, Qt::DisplayRole).toString() == "Commands";
}

void NodeListWidget::startDrag(const QModelIndex& index) {
    if (!index.isValid()) return;
    OSCMessage message;
    message.address = nodeModel->data(index, Qt::DisplayRole).toString();
    message.host = "127.0.0.1";
    message.port = 8991;
    message.value = 1;
    QClipboard *clipboard = QApplication::clipboard();
    clipboard->setText(message.address);
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

    drag->exec(Qt::CopyAction);
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

/**
 * @brief 双击项：展开顶层节点并聚焦到场景中的对应节点
 * @param index 被双击的模型索引
 */
void NodeListWidget::onTreeDoubleClicked(const QModelIndex& index)
{
    if (!index.isValid() || !dataFlowScene || !dataFlowModel) return;
    // 顶层索引
    QModelIndex rootIndex = index;
    while (rootIndex.parent().isValid()) rootIndex = rootIndex.parent();
    // 展开顶层节点
    nodeTree->setExpanded(rootIndex, true);
    // 获取节点ID并聚焦
    QString nodeText = nodeModel->data(rootIndex, Qt::DisplayRole).toString();
    NodeId nodeId = nodeText.section(':', 0, 0).toInt();
    if (auto nodeObj = dataFlowScene->nodeGraphicsObject(nodeId)) {
        // 重置缩放并居中
        if (!dataFlowScene->views().isEmpty()) {
            auto view = dataFlowScene->views().first();
            view->resetTransform();
        }
        dataFlowScene->centerOnNode(nodeId);
    }
}
