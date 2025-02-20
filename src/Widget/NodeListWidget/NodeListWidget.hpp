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
    QTreeWidget* nodeTree;
    QLineEdit* searchBox;
    CustomDataFlowGraphModel* dataFlowModel;
    CustomFlowGraphicsScene* dataFlowScene;
    bool isUpdatingSelection = false;
    void populateNodeTree();
    void onNodeCreated(NodeId nodeId);
    void onNodeDeleted(NodeId nodeId);
    void onNodeUpdated(NodeId nodeId);
    void filterNodes(const QString& query);
    void showContextMenu(const QPoint &pos);
    void onSceneSelectionChanged();
    QTreeWidgetItem* findNodeItem(NodeId nodeId);

private slots:
    void onTreeItemSelectionChanged();

signals:
    void focusOnNode(NodeId nodeId);
};



#endif // NODELISTWIDGET_HPP 