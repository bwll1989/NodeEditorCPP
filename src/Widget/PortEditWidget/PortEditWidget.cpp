//
// Created by bwll1 on 2024/9/26.
//

#include "PortEditWidget.h"

PortEditWidget::PortEditWidget(CustomDataFlowGraphModel *model, QWidget *parent):_model(model) {
    initLayout();
}

void PortEditWidget::initLayout() {
    lay=new QVBoxLayout(this);
    emptyPortEdit=new QLabel("Ports cannot be edited");
    emptyPortEdit->setAlignment(Qt::AlignCenter);
    lay->addWidget(emptyPortEdit);
}

PortEditWidget::~PortEditWidget() {

}


void PortEditWidget::update(const NodeId nodeId){
//    bool isPortEditable = _model->nodeData(nodeId, QtNodes::NodeRole::PortEditable).value<bool>();
    if(!_model->nodeExists(nodeId)){
        //nodeId不存在时
        QWidget *widget = lay->itemAt(0)->widget();
        if (widget) {
            lay->removeWidget(widget);  // 只移除不删除
            widget->setParent(nullptr);
        }
        lay->insertWidget(0,emptyPortEdit);
        return;
    }
    if(_model->nodeData(nodeId, QtNodes::NodeRole::PortEditable).value<bool>()){
        auto p2 = _model->nodeData(nodeId, QtNodes::NodeRole::PortEditableWidget).value<QWidget *>();
        QWidget *widget = lay->itemAt(0)->widget();
        if (widget) {
            lay->removeWidget(widget);  // 只移除不删除
            widget->setParent(nullptr);
        }
        lay->insertWidget(0,p2);
    }else{
        QWidget *widget = lay->itemAt(0)->widget();
        if (widget) {
            lay->removeWidget(widget);  // 只移除不删除
            widget->setParent(nullptr);
        }
        lay->insertWidget(0,emptyPortEdit);
    }
}
