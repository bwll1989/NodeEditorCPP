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
#include <QListWidget>
#include <QStyledItemDelegate>
#include <QtCore/QBuffer>
#include <QtCore/QByteArray>
#include <QtCore/QDataStream>
#include <QtCore/QDebug>
#include <QtCore/QFile>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonObject>
#include <QtCore/QtGlobal>

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
     * 创建场景右键菜单（分级子菜单）
     * - 顶层仅包含一个文本搜索框与一级“标签”（分类）菜单项
     * - 第二级为对应分类下的节点列表，受搜索框过滤
     * - 点击节点动作后，在 scenePos 位置创建对应模型节点
     */
    QMenu *CustomFlowGraphicsScene::createSceneMenu(QPointF const scenePos)
    {
        QMenu *modelMenu = new QMenu();
        modelMenu->setWindowFlags(modelMenu->windowFlags() | Qt::NoDropShadowWindowHint);
        modelMenu->setAttribute(Qt::WA_TranslucentBackground, false);
        auto registry = _graphModel.dataModelRegistry();

        auto *txtBox = new QLineEdit(modelMenu);
        txtBox->setPlaceholderText(tr("搜索节点"));
        txtBox->setClearButtonEnabled(true);
        auto *txtBoxAction = new QWidgetAction(modelMenu);
        txtBoxAction->setDefaultWidget(txtBox);
        modelMenu->addAction(txtBoxAction);
        modelMenu->addSeparator();
        QHash<QString, QMenu *> categoryMenus;
        QHash<QString, QAction *> categoryActions;
        for (auto const &cat : registry->categories()) {
            QMenu *catMenu = modelMenu->addMenu(cat);
            categoryMenus.insert(cat, catMenu);
            categoryActions.insert(cat, catMenu->menuAction());
        }

        const auto associations = registry->registeredModelsCategoryAssociation();

        auto refreshMenus = [this, scenePos, categoryMenus, categoryActions, associations](const QString &filterText) {
            QHash<QString, int> categoryCount;
            for (auto it = categoryMenus.begin(); it != categoryMenus.end(); ++it) {
                categoryCount.insert(it.key(), 0);
            }

            for (auto menu : categoryMenus) {
                menu->clear();
            }

            for (auto const &assoc : associations) {
                const QString &modelName = assoc.first;
                const QString &category  = assoc.second;

                if (!categoryMenus.contains(category)) continue;
                if (!filterText.isEmpty() && !modelName.contains(filterText, Qt::CaseInsensitive)) continue;

                QAction *leafAction = categoryMenus[category]->addAction(modelName);
                QObject::connect(leafAction, &QAction::triggered, this, [this, modelName, scenePos]() {
                    this->undoStack().push(new QtNodes::CreateCommand(this, modelName, scenePos));
                });
                categoryCount[category] += 1;
            }

            for (auto it = categoryMenus.begin(); it != categoryMenus.end(); ++it) {
                bool hasItems = categoryCount[it.key()] > 0;
                it.value()->setEnabled(hasItems);
                if (categoryActions.contains(it.key())) {
                    categoryActions[it.key()]->setVisible(hasItems);
                }
            }
        };

        refreshMenus(QString());
        QObject::connect(txtBox, &QLineEdit::textChanged, modelMenu, [refreshMenus](const QString &text) {
            refreshMenus(text);
        });

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
