//
// Created by bwll1 on 2024/10/5.
//

#ifndef NODEEDITORCPP_NODELISTWIDGET_H
#define NODEEDITORCPP_NODELISTWIDGET_H
#include "QWidget"
#include "QVBoxLayout"
#include "src/Widget/NodeWidget/CustomDataFlowGraphModel.h"
#include "draggabletreewidget.hpp"
#include "src/Widget/NodeWidget/CustomGraphicsView.h"
#include "src//Widget/NodeWidget/CustomFlowGraphicsScene.h"
class NodeListWidget: public QWidget{
Q_OBJECT

public:
    NodeListWidget(CustomDataFlowGraphModel *model,CustomGraphicsView *view,CustomFlowGraphicsScene *scene,QWidget *parent = nullptr);

    void initLayout();
public slots:

    void update();
    void filterChanged(const QString &text);
private:
    QVBoxLayout *mainLayout;
    QLineEdit *txtBox;
    DraggableTreeWidget *treeView;
    CustomDataFlowGraphModel *_model;
    CustomGraphicsView *_view;
    CustomFlowGraphicsScene *_scene;
};


#endif //NODEEDITORCPP_NODELISTWIDGET_H
