//
// Created by bwll1 on 2024/10/5.
//

#ifndef NODEEDITORCPP_NODELIBRARYWIDGET_H
#define NODEEDITORCPP_NODELIBRARYWIDGET_H
#include "QWidget"
#include "QVBoxLayout"
#include "Widget/NodeWidget/CustomDataFlowGraphModel.h"
#include "draggabletreewidget.hpp"
class NodeLibraryWidget: public QWidget{
Q_OBJECT

public:
    NodeLibraryWidget(CustomDataFlowGraphModel *model,QWidget *parent = nullptr);
    /**
     * 初始化布局
     */
    void initLayout();
public slots:
    /**
     * 更新
     */
    void update();
    /**
     * 过滤器改变
     * @param const QString &text 文本
     */
    void filterChanged(const QString &text);
private:
    //主布局
    QVBoxLayout *mainLayout;
    //文本框
    QLineEdit *txtBox;
    //树形视图
    DraggableTreeWidget *treeView;
    //数据流模型
    CustomDataFlowGraphModel *_model;

};


#endif //NODEEDITORCPP_NODELIBRARYWIDGET_H
