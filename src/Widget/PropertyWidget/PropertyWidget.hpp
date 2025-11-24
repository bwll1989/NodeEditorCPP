//
// Created by bwll1 on 2024/9/26.
//

#pragma once
#include "QWidget"
#include "QLabel"
#include "Widget/NodeWidget/CustomDataFlowGraphModel.h"
class PropertyWidget: public QWidget
{
    Q_OBJECT
public:
    PropertyWidget(CustomDataFlowGraphModel *model,QWidget *parent = nullptr);
    ~PropertyWidget();
    void initLayout();
//    void setObject(QObject *object);
    QVBoxLayout *lay;
public slots:
    void update(const NodeId nodeId);
private:

    QLabel *emptyProperty;

    CustomDataFlowGraphModel *_model;
};
