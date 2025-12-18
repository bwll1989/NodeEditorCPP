//
// Created by bwll1 on 2024/10/5.
//


#include "NodeLibraryWidget.h"
#include "QLineEdit"
#include <QStandardItemModel>
#include <QStandardItem>
#include <QSortFilterProxyModel>
#include <QAction>

NodeLibraryWidget::NodeLibraryWidget(QWidget *parent):QWidget(parent){
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

    // 设置模型
    QStandardItemModel *model = new QStandardItemModel(treeView);
    QSortFilterProxyModel *proxy = new QSortFilterProxyModel(treeView);
    proxy->setSourceModel(model);
    proxy->setFilterCaseSensitivity(Qt::CaseInsensitive);
    proxy->setRecursiveFilteringEnabled(true);
    treeView->setModel(proxy);

    treeView->setHeaderHidden(true);
    mainLayout->addWidget(txtBox);
    mainLayout->addWidget(treeView);

}

void NodeLibraryWidget::update(){
    auto proxy = static_cast<QSortFilterProxyModel*>(treeView->model());
    auto model = static_cast<QStandardItemModel*>(proxy->sourceModel());
    model->clear();

    auto registry = QtNodes::PluginsManager::instance()->registry();

    for (auto const &cat : registry->categories()) {
        auto item = new QStandardItem(cat);
        item->setIcon(QIcon(":/icons/icons/plugins.png"));
        item->setFlags(item->flags() & ~Qt::ItemIsSelectable);
        model->appendRow(item);
    }
    for (auto const &assoc : registry->registeredModelsCategoryAssociation()) {
        QList<QStandardItem *> parents = model->findItems(assoc.second);

        if (parents.isEmpty())
            continue;
        
        auto parentItem = parents.first();
        auto item = new QStandardItem(assoc.first);
        item->setIcon(QIcon(":/icons/icons/plugin.png"));
        parentItem->appendRow(item);

        auto *createNodeAction = new QAction(assoc.first, this);
        const QString &nodeType=item->text();
        // QObject::connect(createNodeAction, &QAction::triggered, [this, nodeType]() {
        //     // Mouse position in scene coordinates.
        //     QPoint globalPos = _view->mapFromGlobal(QCursor::pos());
        //     if (_view->rect().contains(globalPos)) {
        //         QPointF posView = _view->mapToScene(globalPos);
        //         _scene->undoStack().push(new QtNodes::CreateCommand(_scene, nodeType, posView));
        //     }
        // });
        // treeView->registerAction(assoc.first, createNodeAction);
    }
    treeView->expandAll();
    txtBox->setFocus();
    //Setup filtering
    connect(txtBox, &QLineEdit::textChanged, this,&NodeLibraryWidget::filterChanged);
}
void NodeLibraryWidget::filterChanged(const QString &text){
    auto proxy = static_cast<QSortFilterProxyModel*>(treeView->model());
    proxy->setFilterFixedString(text);
    if (!text.isEmpty()) {
        treeView->expandAll();
    }
}
