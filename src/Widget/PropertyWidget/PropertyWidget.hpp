//
// Created by bwll1 on 2024/9/26.
//

#pragma once
#include "QWidget"
#include "QLabel"
#include <QStackedWidget>
#include <QVBoxLayout>
#include <QTabWidget>
#include "Widget/NodeWidget/CustomDataFlowGraphModel.h"

class PortEditAddRemoveWidget;

class PropertyWidget: public QWidget
{
    Q_OBJECT
public:
    PropertyWidget(CustomDataFlowGraphModel *model,QWidget *parent = nullptr);
    ~PropertyWidget();
    void initLayout();
    void setModel(CustomDataFlowGraphModel* model);

public slots:
    void update(const NodeId nodeId);

private:
    void setPropertyTabWidget(QWidget *widget);
    void setPortTabWidget(QWidget *widget);

    QLabel *emptyProperty;
    QLabel *emptyPortEdit;

    CustomDataFlowGraphModel *_model;

    NodeId _currentNodeId{};
    PortEditAddRemoveWidget *_portEditWidget = nullptr;
    QWidget *_currentPropertyWidget = nullptr;

    QVBoxLayout *lay = nullptr;
    QTabWidget *_tabWidget = nullptr;
    QWidget *_propertyPage = nullptr;
    QStackedWidget *_propertyStack = nullptr;
    QWidget *_portPage = nullptr;
    QStackedWidget *_portStack = nullptr;
};
