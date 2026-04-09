//
// Created by bwll1 on 2024/9/26.
//

#include "PropertyWidget.hpp"

PropertyWidget::PropertyWidget(CustomDataFlowGraphModel *model,QWidget *parent)
    : QWidget(parent)
    , _model(model)
{
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

void PropertyWidget::setModel(CustomDataFlowGraphModel* model)
{
    _model = model;
    update(NodeId{});
}

void PropertyWidget::update(const QtNodes::NodeId nodeId) {

    if(!_model || !_model->nodeExists(nodeId)){
        QWidget *widget = (lay && lay->count() > 0) ? lay->itemAt(0)->widget() : nullptr;
        if (widget) {
            lay->removeWidget(widget);
            widget->setParent(nullptr);
        }
        lay->insertWidget(0,emptyProperty);
        return;
    }
    if( !_model->nodeData(nodeId, QtNodes::NodeRole::WidgetEmbeddable).value<bool>()) {
        auto p1 = _model->nodeData(nodeId, QtNodes::NodeRole::Widget).value<QWidget *>();
        QWidget *widget = (lay && lay->count() > 0) ? lay->itemAt(0)->widget() : nullptr;
        if (widget) {
            lay->removeWidget(widget);
            widget->setParent(nullptr);
        }
        if (p1) {
            lay->insertWidget(0,p1);
        } else {
            lay->insertWidget(0,emptyProperty);
        }
        return;
    }else{
        QWidget *widget = (lay && lay->count() > 0) ? lay->itemAt(0)->widget() : nullptr;
        if (widget) {
            lay->removeWidget(widget);
            widget->setParent(nullptr);
        }
        lay->insertWidget(0,emptyProperty);
        return;
    }

}