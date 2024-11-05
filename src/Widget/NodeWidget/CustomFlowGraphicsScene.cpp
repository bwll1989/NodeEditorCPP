//
// Created by 吴斌 on 2024/1/16.
//

#include "CustomFlowGraphicsScene.h"
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

    QMenu *CustomFlowGraphicsScene::createSceneMenu(QPointF const scenePos=QPointF(0,0))
{
    QMenu *modelMenu = new QMenu();

    // Add filterbox to the context menu
    auto *txtBox = new QLineEdit(modelMenu);
    txtBox->setPlaceholderText(QStringLiteral("Filter"));
    txtBox->setClearButtonEnabled(true);

    auto *txtBoxAction = new QWidgetAction(modelMenu);
    txtBoxAction->setDefaultWidget(txtBox);

    // 1.
    modelMenu->addAction(txtBoxAction);

    // Add result treeview to the context menu
    QTreeWidget *treeView = new QTreeWidget(modelMenu);
    treeView->header()->close();

    auto *treeViewAction = new QWidgetAction(modelMenu);
    treeViewAction->setDefaultWidget(treeView);

    // 2.
    modelMenu->addAction(treeViewAction);

    auto registry = _graphModel.dataModelRegistry();

    for (auto const &cat : registry->categories()) {
        auto item = new QTreeWidgetItem(treeView);
        item->setText(0, cat);
        item->setIcon(0,QIcon(":/icons/icons/plugins.png"));
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    }

    for (auto const &assoc : registry->registeredModelsCategoryAssociation()) {
        QList<QTreeWidgetItem *> parent = treeView->findItems(assoc.second, Qt::MatchExactly);

        if (parent.count() <= 0)
            continue;
        auto item = new QTreeWidgetItem(parent.first());
        item->setText(0, assoc.first);
        item->setIcon(0,QIcon(":/icons/icons/plugin.png"));
    }
    treeView->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    treeView->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    treeView->expandAll();

    connect(treeView,
            &QTreeWidget::itemClicked,
            [this, modelMenu, scenePos](QTreeWidgetItem *item, int) {
                if (!(item->flags() & (Qt::ItemIsSelectable))) {
                    return;
                }

                this->undoStack().push(new QtNodes::CreateCommand(this, item->text(0), scenePos));

                modelMenu->close();
            });

    //Setup filtering
    connect(txtBox, &QLineEdit::textChanged, [treeView](const QString &text) {
        QTreeWidgetItemIterator categoryIt(treeView, QTreeWidgetItemIterator::HasChildren);
        while (*categoryIt)
            (*categoryIt++)->setHidden(true);
        QTreeWidgetItemIterator it(treeView, QTreeWidgetItemIterator::NoChildren);
        while (*it) {
            auto modelName = (*it)->text(0);
            const bool match = (modelName.contains(text, Qt::CaseInsensitive));
            (*it)->setHidden(!match);
            if (match) {
                QTreeWidgetItem *parent = (*it)->parent();
                while (parent) {
                    parent->setHidden(false);
                    parent = parent->parent();
                }
            }
            ++it;
        }
    });

    // make sure the text box gets focus so the user doesn't have to click on it
    txtBox->setFocus();

    // QMenu's instance auto-destruction
    modelMenu->setAttribute(Qt::WA_DeleteOnClose);

    return modelMenu;
}

    bool CustomFlowGraphicsScene::save() const
    {
        QString fileName = QFileDialog::getSaveFileName(nullptr,
                                                        tr("Open Flow Scene"),
                                                        QDir::homePath(),
                                                        tr("Flow Scene Files (*.flow)"));

        if (!fileName.isEmpty()) {
            if (!fileName.endsWith("flow", Qt::CaseInsensitive))
                fileName += ".flow";

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
                                                        tr("Open Flow Scene"),
                                                        QDir::homePath(),
                                                        tr("Flow Scene Files (*.flow)"));

        if (!QFileInfo::exists(fileName))
            return false;

        QFile file(fileName);

        if (!file.open(QIODevice::ReadOnly))
            return false;

        clearScene();

        QByteArray const wholeFile = file.readAll();

        _graphModel.load(QJsonDocument::fromJson(wholeFile).object());

        Q_EMIT sceneLoaded();

        return true;
}

    void CustomFlowGraphicsScene::mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) {

    QPointF scenePos = event->scenePos();
    QGraphicsItem *item = itemAt(scenePos, QTransform());
    if (!item) {
        auto menu= createSceneMenu(event->scenePos());
        menu->exec(event->screenPos());}
    else
        QGraphicsScene::mouseDoubleClickEvent(event);


}
