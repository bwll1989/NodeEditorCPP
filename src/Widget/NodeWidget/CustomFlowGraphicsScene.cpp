//
// Created by 吴斌 on 2024/1/16.
//

#include "CustomFlowGraphicsScene.h"

#include <QMessageBox>
#include <QTableWidget>

#include "QtNodes/internal/ConnectionGraphicsObject.hpp"
#include "CustomGraphicsView.h"
#include "QtNodes/internal/GraphicsView.hpp"
#include "QtNodes/internal/NodeDelegateModelRegistry.hpp"
#include "QtNodes/internal/NodeGraphicsObject.hpp"
#include "QtNodes/internal/UndoCommands.hpp"

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGraphicsSceneMoveEvent>
#include <QtWidgets/QHeaderView>
#include <QtWidgets/QLineEdit>
#include <QtWidgets/QTreeWidget>
#include <QtWidgets/QWidgetAction>
#include <QtWidgets/QHBoxLayout>
#include <QtWidgets/QVBoxLayout>
#include <QtWidgets/QListView>
#include <QStringListModel>
#include <QStyledItemDelegate>
#include <QIcon>

#include <QtCore/QBuffer>
#include <QtCore/QByteArray>
#include <QtCore/QDataStream>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QtGlobal>
#include <algorithm>

#include <stdexcept>
#include <utility>
using QtNodes::NodeGraphicsObject;
//using QtNodes::CreateCommand;

CustomFlowGraphicsScene::CustomFlowGraphicsScene(CustomDataFlowGraphModel &graphModel, QObject *parent)
            : BasicGraphicsScene(graphModel, parent)
            , _graphModel(graphModel)
    {

        connect(&_graphModel,
                &CustomDataFlowGraphModel::inPortDataWasSet,
                [this](NodeId const nodeId, PortType const, PortIndex const) { onNodeUpdated(nodeId); });
//        connect(this,&CustomFlowGraphicsScene::nodeContextMenu,this,&CustomFlowGraphicsScene::test);
//    connect(&_graphModel,&CustomDataFlowGraphModel::nodeDeleted,this,&CustomFlowGraphicsScene::test);

    }

// TODO constructor for an empyt scene?

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

    void CustomFlowGraphicsScene::test(const NodeId node){
        qDebug()<<node;
}

    /**
     * 创建场景右键菜单（双列持久展示）
     * - 顶部为带图标的搜索框，支持即时过滤
     * - 左列为分类列表，右列为节点项列表
     * - 点击节点后在 scenePos 位置创建对应模型节点
     */
    QMenu *CustomFlowGraphicsScene::createSceneMenu(QPointF const scenePos)
    {
        QMenu *modelMenu = new QMenu();
        modelMenu->setWindowFlags(modelMenu->windowFlags() | Qt::NoDropShadowWindowHint);
        modelMenu->setAttribute(Qt::WA_TranslucentBackground, false);
        
        // 创建容器 Widget
        QWidget *container = new QWidget;
        QVBoxLayout *mainLayout = new QVBoxLayout(container);
        mainLayout->setContentsMargins(0, 0, 0, 0);
        mainLayout->setSpacing(4);

        // 搜索框
        QLineEdit *txtBox = new QLineEdit;
        txtBox->setPlaceholderText(tr("搜索节点"));
        txtBox->setClearButtonEnabled(true);
        // 使用资源图标
        QAction *searchIconAction = new QAction(QIcon(":/icons/icons/search.png"), tr("搜索"), txtBox);
        txtBox->addAction(searchIconAction, QLineEdit::LeadingPosition);
        searchIconAction->setEnabled(false);
        mainLayout->addWidget(txtBox);

        // 双列布局：左侧分类，右侧节点
        QWidget *columnsWidget = new QWidget;
        QHBoxLayout *columnsLayout = new QHBoxLayout(columnsWidget);
        columnsLayout->setContentsMargins(0, 0, 0, 0);
        columnsLayout->setSpacing(4);
        
        QListView *categoryView = new QListView;    
        QListView *nodeView = new QListView;

        // 设置委托以固定行高
        class FixedHeightDelegate : public QStyledItemDelegate {
        public:
            explicit FixedHeightDelegate(QObject *parent = nullptr) : QStyledItemDelegate(parent) {}
            QSize sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const override {
                QSize size = QStyledItemDelegate::sizeHint(option, index);
                size.setHeight(28);
                return size;
            }
        };
        categoryView->setItemDelegate(new FixedHeightDelegate(categoryView));
        nodeView->setItemDelegate(new FixedHeightDelegate(nodeView));

        auto *categoriesModel = new QStringListModel(categoryView);
        auto *nodesModel = new QStringListModel(nodeView);
        
        categoryView->setFixedWidth(120); 
        nodeView->setMinimumWidth(200);
        nodeView->setMinimumHeight(300);
        categoryView->setSelectionMode(QAbstractItemView::SingleSelection);
        nodeView->setSelectionMode(QAbstractItemView::SingleSelection);
        categoryView->setModel(categoriesModel);
        nodeView->setModel(nodesModel);
        
        columnsLayout->addWidget(categoryView);
        columnsLayout->addWidget(nodeView);
        mainLayout->addWidget(columnsWidget);

        // 获取数据
        auto registry = _graphModel.dataModelRegistry();
        auto associations = registry->registeredModelsCategoryAssociation(); 
        auto categories = registry->categories(); 
        QStringList catList;
        for (const auto &cat : categories) {
            catList << cat;
        }
        categoriesModel->setStringList(catList);
        
        /**
         * 更新右侧节点列表（支持分类或全文检索）
         * - 当有搜索文本时忽略分类，展示命中项
         * - 对最终列表进行本地化排序，提升浏览体验
         */
        auto updateNodeList = [nodesModel, associations](const QString& category, const QString& filter) {
            QStringList items;
            bool isSearching = !filter.isEmpty();
            
            for(const auto& pair : associations) {
                QString name = pair.first;
                QString cat = pair.second;
                
                if (isSearching) {
                    if (name.contains(filter, Qt::CaseInsensitive)) {
                        items << name;
                    }
                } else {
                    if (cat == category) {
                        items << name;
                    }
                }
            }
            std::sort(items.begin(), items.end(),
                      [](const QString& a, const QString& b){
                          return QString::localeAwareCompare(a, b) < 0;
                      });
            nodesModel->setStringList(items);
        };
        
        QObject::connect(categoryView, &QListView::clicked,
                         [categoriesModel, updateNodeList, txtBox](const QModelIndex &index) {
                             if (!txtBox->text().isEmpty()) {
                                 txtBox->blockSignals(true);
                                 txtBox->clear();
                                 txtBox->blockSignals(false);
                             }
                             QString cat = categoriesModel->data(index, Qt::DisplayRole).toString();
                             updateNodeList(cat, QString());
                         });
        
        QObject::connect(txtBox, &QLineEdit::textChanged, [categoryView, categoriesModel, nodeView, nodesModel, updateNodeList](const QString &text) {
            if (text.isEmpty()) {
                 categoryView->setVisible(true);
                 categoryView->setEnabled(true);
                 QModelIndex current = categoryView->currentIndex();
                 if (current.isValid()) {
                     QString cat = categoriesModel->data(current, Qt::DisplayRole).toString();
                     updateNodeList(cat, QString());
                 } else if (categoriesModel->rowCount() > 0) {
                     QModelIndex first = categoriesModel->index(0, 0);
                     categoryView->setCurrentIndex(first);
                     QString cat = categoriesModel->data(first, Qt::DisplayRole).toString();
                     updateNodeList(cat, QString());
                 } else {
                     nodesModel->setStringList(QStringList());
                 }
            } else {
                 categoryView->setVisible(false);
                 updateNodeList(QString(), text);
            }
        });
        
        QObject::connect(nodeView, &QListView::clicked, [this, nodesModel, modelMenu, scenePos](const QModelIndex &index) {
            QString name = nodesModel->data(index, Qt::DisplayRole).toString();
            if (!name.isEmpty()) {
                this->undoStack().push(new QtNodes::CreateCommand(this, name, scenePos));
                modelMenu->close();
            }
        });

        if(categoriesModel->rowCount() > 0) {
            QModelIndex first = categoriesModel->index(0, 0);
            categoryView->setCurrentIndex(first);
            QString cat = categoriesModel->data(first, Qt::DisplayRole).toString();
            updateNodeList(cat, QString());
        }

        // 将 Widget 放入菜单
        QWidgetAction *action = new QWidgetAction(modelMenu);
        action->setDefaultWidget(container);
        modelMenu->addAction(action);
        
        modelMenu->setAttribute(Qt::WA_DeleteOnClose);
        txtBox->setFocus();
        return modelMenu;
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

        if (!file.open(QIODevice::ReadOnly))
        {
            QMessageBox::warning(nullptr, tr("打开失败"),
                tr("无法打开文件 %1:\n%2").arg(fileName).arg(file.errorString()));
            return false;
        }



        QByteArray const wholeFile = file.readAll();
        auto jsonDoc = QJsonDocument::fromJson(wholeFile);
        if (jsonDoc.isNull()) {
            QMessageBox::warning(nullptr, tr("打开失败"),
                tr("无法解析文件 %1").arg(fileName));
            return false;
        }
        clearScene();
        _graphModel.load(jsonDoc.object());

        Q_EMIT sceneLoaded();

        return true;
}

    void CustomFlowGraphicsScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {

        QPointF scenePos = event->scenePos();
        QGraphicsItem *item = itemAt(scenePos, QTransform());
        if(_graphModel.getNodesLocked()){
            return;
        }
        if (!item) {
            auto menu= createSceneMenu(event->scenePos());
            menu->exec(event->screenPos());
        }else {
            QGraphicsScene::mouseDoubleClickEvent(event);
        }
}
