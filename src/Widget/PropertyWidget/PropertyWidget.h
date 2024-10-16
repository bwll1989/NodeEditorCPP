//
// Created by bwll1 on 2024/9/26.
//

#ifndef NODEEDITORCPP_PROPERTYWIDGET_H
#define NODEEDITORCPP_PROPERTYWIDGET_H
#include "QWidget"
#include "QLabel"
#include "src/Widget/NodeWidget/CustomDataFlowGraphModel.h"
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


#endif //NODEEDITORCPP_PROPERTYWIDGET_H
