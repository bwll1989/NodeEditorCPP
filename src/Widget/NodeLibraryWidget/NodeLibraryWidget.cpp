//
// Created by bwll1 on 2024/10/5.
//

#include <QtNodes/internal/UndoCommands.hpp>
#include "NodeLibraryWidget.h"
#include "QLineEdit"
NodeLibraryWidget::NodeLibraryWidget(CustomDataFlowGraphModel *model,
                               CustomGraphicsView *view,
                               CustomFlowGraphicsScene *scene,
                               QWidget *parent)
                               :_model(model),
                               _view(view) ,
                               _scene(scene){
    initLayout();
    this->update();
}
void NodeLibraryWidget::initLayout(){
    mainLayout=new QVBoxLayout(this);
//    // Add filterbox to the context menu
    txtBox = new QLineEdit(this);
    txtBox->setPlaceholderText(QStringLiteral("Filter"));
    txtBox->setClearButtonEnabled(true);
    treeView = new DraggableTreeWidget(this);
    treeView->setStyleSheet("QTreeView::item { padding: 4px; }");
    treeView->setHeaderHidden(true);
    mainLayout->addWidget(txtBox);
    mainLayout->addWidget(treeView);
}

void NodeLibraryWidget::update(){
    auto registry = _model->dataModelRegistry();

    for (auto const &cat : registry->categories()) {
        auto item = new QTreeWidgetItem(treeView);
        item->setText(0, cat);
        item->setIcon(0,QIcon(":/icons/icons/plugins.png"));
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
    }
    for (auto const &assoc : registry->registeredModelsCategoryAssociation()) {
        QList<QTreeWidgetItem *> parent = treeView->findItems(assoc.second, Qt::MatchExactly);

        if (parent.count() <= 0)
            continue;
        auto item = new QTreeWidgetItem(parent.first());
        item->setText(0, assoc.first);
        item->setIcon(0,QIcon(":/icons/icons/plugin.png"));
        auto *createNodeAction = new QAction(assoc.first, this);
        const QString &nodeType=item->text(0);
        QObject::connect(createNodeAction, &QAction::triggered, [this, nodeType]() {
            // Mouse position in scene coordinates.
            QPoint globalPos = _view->mapFromGlobal(QCursor::pos());
            if (_view->rect().contains(globalPos)) {
                QPointF posView = _view->mapToScene(globalPos);
                _scene->undoStack().push(new QtNodes::CreateCommand(_scene, nodeType, posView));
            }
        });
        treeView->registerAction(assoc.first, createNodeAction);
    }
    treeView->expandAll();
    txtBox->setFocus();
    //Setup filtering
    connect(txtBox, &QLineEdit::textChanged, this,&NodeLibraryWidget::filterChanged);
}
void NodeLibraryWidget::filterChanged(const QString &text){
    QTreeWidgetItemIterator categoryIt(treeView, QTreeWidgetItemIterator::HasChildren);
    while (*categoryIt)
        (*categoryIt++)->setHidden(true);
    QTreeWidgetItemIterator it(treeView, QTreeWidgetItemIterator::NoChildren);
    while (*it) {
        auto modelName = (*it)->text(0);
        const bool match = (modelName.contains(text, Qt::CaseInsensitive));
        (*it)->setHidden(!match);
        if (match) {
            QTreeWidgetItem *parent = (*it)->parent();
            while (parent) {
                parent->setHidden(false);
                parent = parent->parent();
            }
        }
        ++it;
    }
}