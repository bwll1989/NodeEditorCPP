//
// Created by 吴斌 on 2024/1/16.
//

#include "CustomFlowGraphicsScene.h"
#include "NodeCreateSceneMenu.hpp"

#include <QMessageBox>

#include "QtNodes/internal/NodeGraphicsObject.hpp"

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QGraphicsSceneMouseEvent>

#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QFileInfo>
#include <QtCore/QJsonDocument>
#include <QTransform>

using QtNodes::NodeGraphicsObject;

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
        auto *menu = createSceneMenu(event->scenePos());
        menu->exec(event->screenPos());
    } else {
        QGraphicsScene::mouseDoubleClickEvent(event);
    }
}
