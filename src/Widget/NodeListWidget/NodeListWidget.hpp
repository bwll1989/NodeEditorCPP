#pragma once
#include <QWidget>
#include <QTreeWidget>
#include <QVBoxLayout>
#include <QLineEdit>
#include "Widget/NodeWidget/CustomDataFlowGraphModel.h"
#include "Widget/NodeWidget/CustomFlowGraphicsScene.h"
#include "Widget/NodeWidget/DataflowViewsManger.hpp"
#include <QDrag>
#include <QMimeData>
#include <QMouseEvent>
#include "OSCMessage.h"
class NodeListWidget : public QWidget {
    Q_OBJECT

public:
    /**
     * @brief 构造函数（支持多场景/多模型）
     * @param viewsManager DataflowViewsManger 指针（用于获取场景和模型）
     * @param parent 父对象
     */
    explicit NodeListWidget(DataflowViewsManger* viewsManager, QWidget *parent = nullptr);

protected:
    bool eventFilter(QObject* watched, QEvent* event) override;
    // void mouseReleaseEvent(QMouseEvent *event) override;
private:
    // 场景选择下拉框
    QComboBox* sceneSelector;
    // 刷新按钮（强制刷新显示）
    QPushButton* refreshButton;
    //树形视图
    QTreeWidget* nodeTree;
    //搜索框
    QLineEdit* searchBox;
    // 数据视图管理器（来源：DataflowViewsManger）
    DataflowViewsManger* viewsManager;
    //当前选择的数据流模型
    CustomDataFlowGraphModel* dataFlowModel;
    //当前选择的数据流场景
    CustomFlowGraphicsScene* dataFlowScene;
    //是否正在更新选择
    bool isUpdatingSelection = false;
    QPoint dragStartPosition;
    bool isDragging = false;
    /**
     * 填充节点树（从当前 dataFlowModel）
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
    /**
     * 是否为命令项
     * @param const QTreeWidgetItem* item 节点项
     */
    bool isCommand(const QTreeWidgetItem* item) const;
    /**
     * 开始拖动节点项
     * @param QTreeWidgetItem* item 节点项
     */
    void startDrag(QTreeWidgetItem* item);

private slots:
    /**
     * @brief 场景切换槽函数：根据选择的标题绑定新的模型与场景，并刷新视图
     * @param title 选中的场景标题
     */
    void onSceneSwitched(const QString& title);
    /**
     * @brief 刷新按钮点击：强制刷新场景列表与当前显示
     */
    void onRefreshClicked();
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