// 文件：ViewsTabWidget.cpp（类 ViewsTabWidget 的成员实现）
// 依赖：QtNodes 的 DataFlowGraphicsScene / GraphicsView；QJsonObject / QJsonArray

#include "DataflowViewsManger.hpp"
#include "GraphSnapshotBridge/GraphSnapshotBridge.hpp"


#include "CustomFlowGraphicsScene.h"
#include "CustomGraphicsView.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QSignalBlocker>
#include "DockWidget.h"
#include "QtNodes/internal/PluginsManager.hpp"

using namespace QtNodes;

using QtNodes::NodeId;

namespace {

void registerSnapshotHooks(DataflowViewsManger *self, const QString &title, CustomDataFlowGraphModel *model)
{
    if (!self || !model || title.isEmpty()) {
        return;
    }

    GraphSnapshotBridge::instance()->registerScene(
        title,
        [model](const QString &, const QVector<NodeId> &nodeIds) {
            return model->captureSnapshotNodes(nodeIds);
        },
        [model](const QString &, const QJsonArray &nodesJson) {
            return model->applySnapshotNodes(nodesJson);
        },
        [self, title](const QString &) {
            QVector<NodeId> ids;
            if (CustomFlowGraphicsScene *scene = self->sceneByTitle(title)) {
                const auto selected = scene->selectedNodes();
                ids.reserve(static_cast<int>(selected.size()));
                for (NodeId id : selected) {
                    ids.append(id);
                }
            }
            return ids;
        });
}

} // namespace

DataflowViewsManger::DataflowViewsManger(ads::CDockManager* dockManager,
                                         QObject* parent,
                                         DataflowPresentationMode mode)
    : QObject(parent),
      m_DockManager(dockManager),
      _mode(mode)
{
    if (m_DockManager && wantsUi()) {
        connect(m_DockManager,
                &ads::CDockManager::focusedDockWidgetChanged,
                this,
                &DataflowViewsManger::focusedSceneTitle);
    }
}

DataflowViewsManger::~DataflowViewsManger()
{
    // 函数级注释：
    // 说明：析构时确保场景和 DockWidget 全部关闭并释放，避免外部仍持有指向模型的引用导致悬挂指针。
    clearAllScenes();
}

void DataflowViewsManger::setDockManager(ads::CDockManager* dockManager)
{
    if (m_DockManager) {
        disconnect(m_DockManager, nullptr, this, nullptr);
    }
    m_DockManager = dockManager;
    if (m_DockManager && wantsUi()) {
        connect(m_DockManager,
                &ads::CDockManager::focusedDockWidgetChanged,
                this,
                &DataflowViewsManger::focusedSceneTitle);
    }
}

CustomDataFlowGraphModel* DataflowViewsManger::ensureModel(const QString& title)
{
    auto mit = _models.find(title);
    if (mit == _models.end()) {
        _models.emplace(title, std::make_unique<CustomDataFlowGraphModel>(PluginsManager::instance()->registry()));
        mit = _models.find(title);
        if (mit != _models.end() && mit->second) {
            mit->second->setModelAlias(title);
            registerSnapshotHooks(this, title, mit->second.get());
        }
    }
    if (mit == _models.end() || !mit->second) {
        return nullptr;
    }
    return mit->second.get();
}

void DataflowViewsManger::createSceneUi(const QString& title)
{
    if (!m_DockManager) {
        return;
    }

    auto mit = _models.find(title);
    if (mit == _models.end() || !mit->second) {
        return;
    }

    auto dit = _DockWidget.find(title);
    if (dit != _DockWidget.end() && dit->second) {
        return;
    }

    auto& model = *mit->second;
    ads::CDockWidget* DockWidget = m_DockManager->createDockWidget(title);

    // 以 DockWidget 为父对象，确保其关闭时 View/Scene 一并销毁
    auto view  = new CustomGraphicsView(DockWidget);
    auto scene = new CustomFlowGraphicsScene(model, view);
    // 函数级注释：
    // 说明：确保 Scene 作为 QObject 的父对象是 View，从而在 DockWidget 关闭时按父子链自动析构
    scene->setParent(view);
    view->setScene(scene);

    // 便捷行为：加载后居中显示
    // QObject::connect(scene, &CustomFlowGraphicsScene::sceneLoaded, view, &GraphicsView::centerScene);

    // 仅保存弱引用，避免二次释放
    _DockWidget[title] = DockWidget;

    DockWidget->setWidget(view);
    DockWidget->setIcon(QIcon(":/icons/icons/genealogy.png"));

    // addAction(DockWidget->toggleViewAction());

    QMenu* OptionsMenu = new QMenu(DockWidget);
    OptionsMenu->setTitle(QObject::tr("Options"));
    OptionsMenu->setToolTip(OptionsMenu->title());
    OptionsMenu->setIcon(QIcon(":/ads_icons/ads_icons/custom-menu-button.svg"));
    auto MenuAction = OptionsMenu->menuAction();
    // The object name of the action will be set for the QToolButton that
    // is created in the dock area title bar. You can use this name for CSS
    // styling
    MenuAction->setObjectName("optionsMenu");
    DockWidget->setTitleBarActions({OptionsMenu->menuAction()});
    auto a = OptionsMenu->addAction(QIcon(":/icons/icons/clear.png"),QObject::tr("Clear Dataflow"));
    // c->connect(a, SIGNAL(triggered()), SLOT(clear()));
    QObject::connect(a, &QAction::triggered, scene, &CustomFlowGraphicsScene::clearScene);
    auto b = OptionsMenu->addAction(QIcon(":/icons/icons/save.png"),QObject::tr("Save Child Dataflow"));
    QObject::connect(b, &QAction::triggered, scene, &CustomFlowGraphicsScene::save);
    auto c = OptionsMenu->addAction(QIcon(":/icons/icons/delete_database.png"),QObject::tr("Delete Dataflow"));
    QObject::connect(c, &QAction::triggered, this, [this, DockWidget, title](){
        GraphSnapshotBridge::instance()->unregisterScene(title);
        _DockWidget.erase(title);
        emit removeScene(title);

        if (m_DockManager && DockWidget) {
            m_DockManager->removeDockWidget(DockWidget);
            delete DockWidget;
        }

        _models.erase(title);
    });

    auto d = OptionsMenu->addAction(QIcon(":/icons/icons/folder.png"),QObject::tr("Load Child Dataflow"));
    QObject::connect(d, &QAction::triggered, scene, &CustomFlowGraphicsScene::load);
    auto e = OptionsMenu->addAction(QIcon(":/icons/icons/lock.png"),QObject::tr("Lock Dataflow"));
    QObject::connect(e, &QAction::triggered, this, [this, title, e](){
        auto& model = *_models.at(title);
        model.setNodesLocked(!model.getNodesLocked());
        e->setText(model.getNodesLocked()?QObject::tr("Unlock Dataflow"):QObject::tr("Lock Dataflow"));

    });

      // 安全添加到区域：优先使用当前聚焦区域，其次回退到中心区域
    if (m_DockManager && m_DockManager->focusedDockWidget() && m_DockManager->focusedDockWidget()->dockAreaWidget()) {
        m_DockManager->addDockWidgetTabToArea(DockWidget, m_DockManager->focusedDockWidget()->dockAreaWidget());
        m_DockManager->setWidgetFocus(DockWidget);
    } else {
        m_DockManager->addDockWidgetTab(ads::CenterDockWidgetArea, DockWidget);
        m_DockManager->setWidgetFocus(DockWidget);
    }

    emit createNewScene(title);
}

void DataflowViewsManger::addNewScene(const QString& title)
{
    if (!ensureModel(title)) {
        return;
    }
    if (wantsUi()) {
        createSceneUi(title);
    }
}

void DataflowViewsManger::attachSceneUi(const QString& title)
{
    createSceneUi(title);
}

void DataflowViewsManger::addNewSceneFromeModel(const QString& title, QJsonObject const &jsonDocument) {
    auto* model = ensureModel(title);
    if (!model) {
        return;
    }

    if (wantsUi()) {
        createSceneUi(title);
    }

    QObject::connect(model,
                     &CustomDataFlowGraphModel::loadProgress,
                     this,
                     [this, title](const QString& phase, int current, int total) {
                         Q_EMIT loadProgress(title, phase, current, total);
                     });
    model->load(jsonDocument);
    // if (_models.count(title)) return;
    // _models.emplace(title, std::unique_ptr<CustomDataFlowGraphModel>(model));
    //
    // ads::CDockWidget* DockWidget = new ads::CDockWidget(m_DockManager,title);
    //
    // _DockWidget[title] = DockWidget;
    //
    // auto view  = new CustomGraphicsView(DockWidget);
    //
    // auto scene = new CustomFlowGraphicsScene(*model, view);
    // // 说明：确保 Scene 作为 QObject 的父对象是 View，从而在 DockWidget 关闭时按父子链自动析构
    // scene->setParent(view);
    // view->setScene(scene);
    //
    // // 便捷行为：加载后居中显示
    // QObject::connect(scene, &CustomFlowGraphicsScene::sceneLoaded, view, &GraphicsView::centerScene);
    //
    // DockWidget->setWidget(view);
    // DockWidget->setIcon(QIcon(":/icons/icons/genealogy.png"));
    // QMenu* OptionsMenu = new QMenu(DockWidget);
    // OptionsMenu->setTitle(QObject::tr("Options"));
    // OptionsMenu->setToolTip(OptionsMenu->title());
    // OptionsMenu->setIcon(QIcon(":/icons/icons/options.png"));
    // auto MenuAction = OptionsMenu->menuAction();
    // // The object name of the action will be set for the QToolButton that
    // // is created in the dock area title bar. You can use this name for CSS
    // // styling
    // MenuAction->setObjectName("optionsMenu");
    // DockWidget->setTitleBarActions({OptionsMenu->menuAction()});
    // auto a = OptionsMenu->addAction(QIcon(":/icons/icons/clear.png"),QObject::tr("Clear Dataflow"));
    // // c->connect(a, SIGNAL(triggered()), SLOT(clear()));
    // QObject::connect(a, &QAction::triggered, scene, &CustomFlowGraphicsScene::clear);
    // auto b = OptionsMenu->addAction(QIcon(":/icons/icons/save.png"),QObject::tr("Save Child Dataflow"));
    // QObject::connect(b, &QAction::triggered, scene, &CustomFlowGraphicsScene::save);
    // auto c = OptionsMenu->addAction(QIcon(":/icons/icons/delete_database.png"),QObject::tr("Delete Dataflow"));
    // QObject::connect(c, &QAction::triggered, this, [this, DockWidget, title](){
    //     m_DockManager->removeDockWidget(DockWidget);
    //     _models.erase(title);
    //     emit removeScene(title);
    // });
    // auto d = OptionsMenu->addAction(QIcon(":/icons/icons/folder.png"),QObject::tr("Load Child Dataflow"));
    // QObject::connect(d, &QAction::triggered, scene, &CustomFlowGraphicsScene::load);
    // auto e = OptionsMenu->addAction(QIcon(":/icons/icons/lock.png"),QObject::tr("Lock Dataflow"));
    // QObject::connect(e, &QAction::triggered, this, [this, title, e](){
    //     auto& model = *_models.at(title);
    //     model.setNodesLocked(!model.getNodesLocked());
    //     e->setText(model.getNodesLocked()?QObject::tr("Unlock Dataflow"):QObject::tr("Lock Dataflow"));
    //
    // });
    //
    // if (m_DockManager) {
    //     m_DockManager->addDockWidgetTab(ads::CenterDockWidgetArea, DockWidget);
    //     m_DockManager->setWidgetFocus(DockWidget);
    // }
    // emit createNewScene(title);

}

void DataflowViewsManger::clearAllScenes() {
    GraphSnapshotBridge::instance()->clearAll();
    // 函数级注释：
    // 作用：批量关闭并删除所有 DockWidget（连带 View/Scene 按父子关系销毁）， subsequent safe release model;
    // 关键：阻塞 DockManager 信号，避免批量关闭时触发重布局和回调造成重入/性能问题
     for (auto& kv : _models) {
        emit removeScene(kv.first);
    }
    if (!m_DockManager) {
        _DockWidget.clear();
        _models.clear();
        return;
    }
    const QSignalBlocker blocker(m_DockManager);

    std::vector<ads::CDockWidget*> docks;
    docks.reserve(_DockWidget.size());
    for (auto& kv : _DockWidget) {
        if (kv.second) docks.push_back(kv.second);
    }
    _DockWidget.clear();

    for (auto* dock : docks) {
        m_DockManager->removeDockWidget(dock);
        delete dock;
    }

    _models.clear();
}

void DataflowViewsManger::load(QJsonObject const &nodeJson) {
    clearAllScenes();
    if (nodeJson.contains("tabs")) {
        QJsonArray tabsArray = nodeJson["tabs"].toArray();
        for (const auto& tabObj : tabsArray) {
            if (tabObj.isObject()) {
                QJsonObject tab = tabObj.toObject();
                for (auto it = tab.begin(); it != tab.end(); ++it) {
                    QString tabTitle = it.key();
                    QJsonObject tabJson = it.value().toObject();
                    addNewSceneFromeModel(tabTitle,tabJson);
                }
            }
        }
    }
}

QJsonObject DataflowViewsManger::save() const {
    QJsonObject root;
    QJsonArray tabsArray;

    for (const auto& mod : _models) {
        QJsonObject tabObj;
        tabObj.insert(mod.first, mod.second->save());
        tabsArray.append(tabObj);   // 关键：把每个 tabObj 加到数组里
    }

    root.insert("tabs", tabsArray);  // 关键：把数组写进 root
    return root;
}

std::map<QString, std::unique_ptr<CustomDataFlowGraphModel>> *DataflowViewsManger::getModel()
{
    return &_models;
}


void DataflowViewsManger::setSceneLocked(bool locked)
{
    for (auto& kv : _models) {
        kv.second->setNodesLocked(locked);
    }
}

QStringList DataflowViewsManger::sceneTitles() const
{
    // 函数级注释：
    // 说明：返回当前管理器维护的所有场景标题，便于外部（如 NodeListWidget）构建场景选择列表。
    QStringList titles;
    for (const auto& kv : _models) {
        titles << kv.first;
    }
    return titles;
}

CustomFlowGraphicsScene* DataflowViewsManger::sceneByTitle(const QString& title) const
{
    // 函数级注释：
    // 说明：通过 DockWidget -> CustomGraphicsView -> scene 链获取对应的 CustomFlowGraphicsScene 指针。
    auto it = _DockWidget.find(title);
    if (it == _DockWidget.end()) return nullptr;
    ads::CDockWidget* dock = it->second;
    if (!dock) return nullptr;
    auto view = qobject_cast<CustomGraphicsView*>(dock->widget());
    if (!view) return nullptr;
    return qobject_cast<CustomFlowGraphicsScene*>(view->scene());
}

CustomDataFlowGraphModel* DataflowViewsManger::modelByTitle(const QString& title) const
{
    // 函数级注释：
    // 说明：返回指定标题对应的数据流模型的裸指针，若不存在返回 nullptr。
    auto it = _models.find(title);
    if (it == _models.end()) return nullptr;
    return it->second.get();
}

QString DataflowViewsManger::currentFocusedSceneTitle() const
{
    if (m_DockManager && m_DockManager->focusedDockWidget()) {
        for (const auto& kv : _DockWidget) {
            if (kv.second == m_DockManager->focusedDockWidget()) {
                return kv.first;
            }
        }
    }
    if (!_models.empty()) {
        return _models.begin()->first;
    }
    return {};
}

void DataflowViewsManger::focusedSceneTitle()
{
    // 仅在聚焦的 Dock 为 dataflow 场景页时通知外部，避免焦点切到节点列表等面板时误触发重建
    if (!m_DockManager) {
        return;
    }
    ads::CDockWidget* focused = m_DockManager->focusedDockWidget();
    if (!focused) {
        return;
    }
    for (const auto& kv : _DockWidget) {
        if (kv.second == focused) {
            emit sceneIsActive(kv.first);
            return;
        }
    }
}

void DataflowViewsManger::refreshAllScenes()
{
    // 函数级注释：
    // 说明：强制刷新所有场景和视图，用于主题切换后更新显示
    auto const &flowViewStyle = StyleCollection::flowViewStyle();
    for (auto& kv : _DockWidget) {
        ads::CDockWidget* dock = kv.second;
        if (!dock) continue;
        auto view = qobject_cast<CustomGraphicsView*>(dock->widget());
        if (view) {
            // 更新背景颜色
            view->setBackgroundBrush(flowViewStyle.BackgroundColor);
            // 强制更新视图（背景等）
            view->update();
            if (view->scene()) {
                // 强制更新场景项
                view->scene()->update();
            }
        }
    }
}
