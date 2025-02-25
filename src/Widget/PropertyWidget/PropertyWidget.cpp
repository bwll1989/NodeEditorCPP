//
// Created by bwll1 on 2024/9/26.
//

#include "PropertyWidget.hpp"

PropertyWidget::PropertyWidget(CustomDataFlowGraphModel *model,QWidget *parent):_model(model) {
    initLayout();
}

PropertyWidget::~PropertyWidget() {

}

void PropertyWidget::initLayout() {


    emptyProperty=new QLabel("No properties can be edited");
    emptyProperty->setAlignment(Qt::AlignCenter);

    lay=new QVBoxLayout(this);
    lay->setContentsMargins(0,0,0,0);
    lay->addWidget(emptyProperty,1);
}

void PropertyWidget::update(const QtNodes::NodeId nodeId) {

//    bool isEmbeded = _model->nodeData(nodeId, QtNodes::NodeRole::WidgetEmbeddable).value<bool>();
    if(!_model->nodeExists(nodeId)){
        QWidget *widget = lay->itemAt(0)->widget();
        if (widget) {
            lay->removeWidget(widget);  // 只移除不删除
            widget->setParent(nullptr);
        }
        lay->insertWidget(0,emptyProperty);
        return;
    }
    if( !_model->nodeData(nodeId, QtNodes::NodeRole::WidgetEmbeddable).value<bool>()) {
        auto p1 = _model->nodeData(nodeId, QtNodes::NodeRole::Widget).value<QWidget *>();
        QWidget *widget = lay->itemAt(0)->widget();
        if (widget) {
            lay->removeWidget(widget);  // 只移除不删除
            widget->setParent(nullptr);
        }
        lay->insertWidget(0,p1);
        return;
    }else{
        QWidget *widget = lay->itemAt(0)->widget();
        if (widget) {
            lay->removeWidget(widget);  // 只移除不删除
            widget->setParent(nullptr);
        }
        lay->insertWidget(0,emptyProperty);
//            widget = lay_property->itemAt(1)->widget();
        return;
    }

}