#include "PluginsManagerWidget.hpp"

#include <QCoreApplication>
#include <QDesktopServices>
#include <QDir>
#include <QFileDialog>
#include <QGridLayout>
#include <QHeaderView>
#include <QPushButton>
#include <QTableView>
#include <QtNodes/NodeDelegateModelRegistry>
#include <QtNodes/PluginInterface>

using QtNodes::NodeDelegateModelRegistry;
using QtNodes::PluginInterface;

PluginsManagerWidget::PluginsManagerWidget(QWidget *parent)
    : QDialog(parent)
{
    initLayout();
}

PluginsManagerWidget::~PluginsManagerWidget()
{
    //
}
//初始化控件布局
void PluginsManagerWidget::initLayout()
{
    setMinimumSize(600, 250);
    awesome = new fa::QtAwesome(this);
    awesome->initFontAwesome();
    _pluginsFolder.setPath(QDir::cleanPath(QCoreApplication::applicationDirPath() + QDir::separator() + "plugins"));
    QGridLayout *layout = new QGridLayout();
    setLayout(layout);
//    layout->setContentsMargins(4,0,4,6);
    QTableView *pluginTable = new QTableView();
    pluginTable->setSelectionBehavior(QAbstractItemView::SelectRows);
    pluginTable->setEditTriggers(QAbstractItemView::NoEditTriggers);
    pluginTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    layout->addWidget(pluginTable, 0, 0, 1, 2);

    _model = new QStandardItemModel(pluginTable);
    _model->setColumnCount(3);
    _model->setHeaderData(0, Qt::Horizontal, "Name");
    _model->setHeaderData(1, Qt::Horizontal, "Version");
    _model->setHeaderData(2, Qt::Horizontal, "Describe");
    pluginTable->setModel(_model);
    pluginTable->selectRow(0);

    // add button
    QPushButton *addButton = new QPushButton(awesome->icon("fa-solid fa-plug-circle-plus"),"Add");
    layout->addWidget(addButton, 1, 0);
    connect(addButton, &QPushButton::clicked, this, [this]() {
        QString fileName =
                QFileDialog::getOpenFileName(this->parentWidget(),
                                             tr("Load Plugin"),
                                             QCoreApplication::applicationDirPath(),
                                             tr("Node Files(*.node);;Data Files(*.data)"));

        if (!QFileInfo::exists(fileName))
            return;

        QFileInfo f(fileName);

        QFileInfo newFile(
                QDir::cleanPath(_pluginsFolder.absolutePath() + QDir::separator() + f.fileName()));
        QString const newPath = newFile.absoluteFilePath();

        if (f.absoluteFilePath() == newPath)
            return;

        // Copy to the plug-in directory
        if (!QFile::copy(f.absoluteFilePath(), newPath))
            return;

        PluginsManager *pluginsManager = PluginsManager::instance();
        auto plugin = pluginsManager->loadPluginFromPath(newPath);
        if (!plugin) {
            QFile::remove(newPath);
            return;
        }
        QList<QStandardItem*> row;
        QStandardItem *name_item = new QStandardItem(plugin->name());
        name_item->setData(newPath);
        row.append(name_item);
        QStandardItem *version_item = new QStandardItem(plugin->version());
        row.append(version_item);
        QStandardItem *describe_item = new QStandardItem(plugin->describe());
        row.append(describe_item);
        version_item->setData(Qt::AlignCenter,Qt::TextAlignmentRole);
        name_item->setData(Qt::AlignCenter,Qt::TextAlignmentRole);
        describe_item->setData(Qt::AlignCenter,Qt::TextAlignmentRole);
        _model->appendRow(row);

        std::shared_ptr<NodeDelegateModelRegistry> reg = pluginsManager->registry();
        plugin->registerDataModels(reg);
    });

    // delete button
    QPushButton *deleteButton = new QPushButton(awesome->icon("fa-solid fa-plug-circle-minus"),"Delete", this);
    layout->addWidget(deleteButton, 1, 1);
    connect(deleteButton, &QPushButton::clicked, this, [this, pluginTable]() {
        QItemSelectionModel *selectionModel = pluginTable->selectionModel();

        int row = selectionModel->currentIndex().row();

        while (selectionModel->selectedRows().count() > 0) {
            auto rowIdx = selectionModel->selectedRows().first();
            row = std::min(row, rowIdx.row());

            QStandardItem *item = _model->itemFromIndex(rowIdx);

            PluginsManager *pluginsManager = PluginsManager::instance();

            // FIXME: Unload plugin successfully, but cannot delete the plugin file in windows
            if (!pluginsManager->unloadPluginFromName(item->text()) ||
                !QFile::remove(item->data().toString())) {
                selectionModel->select(rowIdx, QItemSelectionModel::Deselect);
                continue;
            }

            _model->removeRow(rowIdx.row());
        }

        pluginTable->selectRow(row);
    });
}
//打开插件目录
void PluginsManagerWidget::openPluginsFolder()
{
     QDesktopServices::openUrl(QUrl::fromLocalFile(pluginsFolderPath()));
//    QDesktopServices::openUrl(QUrl(_pluginsFolder.absolutePath()));
}

QString PluginsManagerWidget::pluginsFolderPath() const
{
    return _pluginsFolder.absolutePath();
}
//从目录加载所有插件
void PluginsManagerWidget::loadPluginsFromFolder()
{
    PluginsManager *pluginsManager = PluginsManager::instance();
    std::shared_ptr<NodeDelegateModelRegistry> registry = pluginsManager->registry();
//    registry->registerModel<PythonDataModel>("Controls");
//    加载内建插件
    pluginsManager->loadPlugins(_pluginsFolder.absolutePath(),
                                QStringList() << "*.node"
                                              << "*.data");
    for (auto l : pluginsManager->loaders()) {
        PluginInterface *plugin = qobject_cast<PluginInterface *>(l.second->instance());
        if (!plugin)
            continue;
        emit loadPluginStatus("loading "+plugin->name());
//        发送正在加载的插件
        QStandardItem *name_item = new QStandardItem(plugin->name());
//        qDebug()<<l.second->fileName();
        QList<QStandardItem*> row;
        name_item->setData(l.second->fileName());
        row.append(name_item);
        QStandardItem *version_item = new QStandardItem(plugin->version());
        row.append(version_item);
        QStandardItem *describe_item = new QStandardItem(plugin->describe());
        row.append(describe_item);
        version_item->setData(Qt::AlignCenter,Qt::TextAlignmentRole);
        name_item->setData(Qt::AlignCenter,Qt::TextAlignmentRole);
        describe_item->setData(Qt::AlignCenter,Qt::TextAlignmentRole);
        _model->appendRow(row);

        plugin->registerDataModels(registry);

    }
    emit loadPluginStatus("Plugins loading finished");
//    插件加载完成发送信号
}
