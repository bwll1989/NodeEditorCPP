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
using QtNodes::BasicGraphicsScene;
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
     * 创建场景菜单
     */
    QMenu *createSceneMenu(QPointF const scenePos) override;



public Q_SLOTS:
    /**
     * 保存
     * @return bool 是否保存
     */
    bool save() const ;
    /**
     * 加载
     * @return bool 是否加载
     */
    bool load();
    /**
     * 测试
     * @param NodeId node 节点
     */
	void test(const NodeId node);


Q_SIGNALS:
    /**
     * 场景加载
     */
    void sceneLoaded();
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
