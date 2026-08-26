//
// Created by 吴斌 on 2024/1/16.
//
#pragma once

#include "QtNodes/internal/BasicGraphicsScene.hpp"
#include "CustomDataFlowGraphModel.h"
#include "QtNodes/internal/Export.hpp"
#include "QtNodes/internal/UndoCommands.hpp"
#include "QPointF"

class QUndoStack;
class QMenu;
using QtNodes::BasicGraphicsScene;
using QtNodes::ContextMenuKind;
class CustomFlowGraphicsScene : public BasicGraphicsScene
{
    Q_OBJECT
public:
    CustomFlowGraphicsScene(CustomDataFlowGraphModel &graphModel, QObject *parent = nullptr);

    ~CustomFlowGraphicsScene() = default;

public:
    /**
     * 获取选中的节点
     */
    std::vector<NodeId> selectedNodes() const;

public:
    /**
     * 空白处双击：创建节点菜单（NodeCreateSceneMenu）
     */
    QMenu *createSceneMenu(QPointF const scenePos) override;

    /**
     * 按对象/画布类型追加共享编辑菜单项。
     * Scene：空白处右键（粘贴/搜索等）；Node/Connection/Group：图元右键尾部。
     */
    void appendContextMenuActions(QMenu &menu, ContextMenuKind kind) override;

Q_SIGNALS:
    /**
     * 端口编辑
     */
    void portEdit(QtNodes::NodeId nodeId);
private:
    //模型
    CustomDataFlowGraphModel &_graphModel;
protected:
    /**
     * 鼠标双击事件
     */
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override;
};
