//
// Created by bwll1 on 2024/10/5.
//

#ifndef NODEEDITORCPP_NODELIBRARYWIDGET_H
#define NODEEDITORCPP_NODELIBRARYWIDGET_H
#include "QWidget"
#include "QVBoxLayout"
#include "Widget/NodeWidget/CustomDataFlowGraphModel.h"
#include "draggabletreewidget.hpp"
#include "Widget/NodeWidget/CustomGraphicsView.h"
#include "Widget/NodeWidget/CustomFlowGraphicsScene.h"
class NodeLibraryWidget: public QWidget{
Q_OBJECT

public:
    NodeLibraryWidget(CustomDataFlowGraphModel *model,CustomGraphicsView *view,CustomFlowGraphicsScene *scene,QWidget *parent = nullptr);

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


#endif //NODEEDITORCPP_NODELIBRARYWIDGET_H
