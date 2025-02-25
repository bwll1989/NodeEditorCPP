#ifndef NODELISTWIDGET_HPP
#define NODELISTWIDGET_HPP

#include <QWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QLineEdit>
#include "Widget/NodeWidget/CustomDataFlowGraphModel.h"
#include "Widget/NodeWidget/CustomFlowGraphicsScene.h"
class NodeListWidget : public QWidget {
    Q_OBJECT

public:
    explicit NodeListWidget(CustomDataFlowGraphModel* model, CustomFlowGraphicsScene* scene, QWidget *parent = nullptr);

private:
    //树形视图
    QTreeWidget* nodeTree;
    //搜索框
    QLineEdit* searchBox;
    //数据流模型
    CustomDataFlowGraphModel* dataFlowModel;
    //数据流场景
    CustomFlowGraphicsScene* dataFlowScene;
    //是否正在更新选择
    bool isUpdatingSelection = false;
    /**
     * 填充节点树
     */
    void populateNodeTree();
    /**
     * 节点创建
     * @param NodeId nodeId 节点ID
     */
    void onNodeCreated(NodeId nodeId);
    /**
     * 节点删除
     * @param NodeId nodeId 节点ID
     */
    void onNodeDeleted(NodeId nodeId);
    /**
     * 节点更新
     * @param NodeId nodeId 节点ID
     */
    void onNodeUpdated(NodeId nodeId);
    /**
     * 过滤节点
     * @param const QString& query 查询
     */
    void filterNodes(const QString& query);
    /**
     * 显示上下文菜单
     * @param const QPoint &pos 位置
     */
    void showContextMenu(const QPoint &pos);
    /**
     * 场景选择改变
     */
    void onSceneSelectionChanged();
    /**
     * 查找节点项
     * @param NodeId nodeId 节点ID
     */
    QTreeWidgetItem* findNodeItem(NodeId nodeId);

private slots:
    /**
     * 树形视图项选择改变
     */
    void onTreeItemSelectionChanged();

signals:
    /**
     * 聚焦节点
     * @param NodeId nodeId 节点ID
     */
    void focusOnNode(NodeId nodeId);
};



#endif // NODELISTWIDGET_HPP 