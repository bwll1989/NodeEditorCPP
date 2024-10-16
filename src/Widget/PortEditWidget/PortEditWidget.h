//
// Created by bwll1 on 2024/9/26.
//

#ifndef NODEEDITORCPP_PORTEDITWIDGET_H
#define NODEEDITORCPP_PORTEDITWIDGET_H
#include "QWidget"
#include "QLabel"
#include "src/Widget/NodeWidget/CustomDataFlowGraphModel.h"
#include "QVBoxLayout"
class PortEditWidget:public QWidget
{
Q_OBJECT
public:
    PortEditWidget(CustomDataFlowGraphModel *model,QWidget *parent = nullptr);
    ~PortEditWidget();
    void initLayout();
public slots:
//    void update(const NodeId nodeId);
    void update(const NodeId nodeId);
private:
    QVBoxLayout *lay;
    QLabel *emptyPortEdit;
    CustomDataFlowGraphModel *_model;
};


#endif //NODEEDITORCPP_PORTEDITWIDGET_H
