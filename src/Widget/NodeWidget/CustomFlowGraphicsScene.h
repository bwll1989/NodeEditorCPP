//
// Created by 吴斌 on 2024/1/16.
//

#ifndef NODEEDITORCPP_CUSTOMFLOWGRAPHICSSCENE_H
#define NODEEDITORCPP_CUSTOMFLOWGRAPHICSSCENE_H

#pragma once

#include "QtNodes/internal/BasicGraphicsScene.hpp"
#include "CustomDataFlowGraphModel.h"
#include "QtNodes/internal/Export.hpp"
#include "QtNodes/internal/UndoCommands.hpp"
#include "QPointF"

class QUndoStack;
using QtNodes::BasicGraphicsScene;
class CustomFlowGraphicsScene : public BasicGraphicsScene
{
    Q_OBJECT
public:
    CustomFlowGraphicsScene(CustomDataFlowGraphModel &graphModel, QObject *parent = nullptr);

    ~CustomFlowGraphicsScene() = default;

public:
    std::vector<NodeId> selectedNodes() const;

public:
    QMenu *createSceneMenu(QPointF const scenePos) override;

public Q_SLOTS:
    bool save() const ;

    bool load();

	void test(const NodeId node);
Q_SIGNALS:
    	void sceneLoaded();

    	void portEdit(QtNodes::NodeId nodeId);
    private:
    	CustomDataFlowGraphModel &_graphModel;
protected:
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
};





#endif //NODEEDITORCPP_CUSTOMFLOWGRAPHICSSCENE_H
