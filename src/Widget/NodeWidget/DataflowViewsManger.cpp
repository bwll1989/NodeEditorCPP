#include "DataflowViewsManger.hpp"

#include "ContainerDataModel.hpp"
#include "GraphSnapshotBridge/GraphSnapshotBridge.hpp"

#include <QJsonArray>
#include <QJsonObject>
#include <QMenu>
#include <QAction>
#include "Widget/ExternalControl/ExportActionDialog.h"
#include <QMessageBox>
#include <QVBoxLayout>
#include <QIcon>
#include <QPixmap>
#include <QDebug>

#include "DockWidget.h"
#include "Widget/ExternalControl/HttpServer.hpp"
#include "QtNodes/internal/BasicGraphicsScene.hpp"
#include "QtNodes/internal/PluginsManager.hpp"
#include "QtNodes/StyleCollection"

using namespace QtNodes;
using Nodes::ContainerDataModel;

namespace {

void registerSnapshotHooks(DataflowViewsManger *self,
                           const QString &key,
                           CustomDataFlowGraphModel *model)
{
    if (!self || !model || key.isEmpty())
        return;

    GraphSnapshotBridge::instance()->registerScene(
        key,
        [model](const QString &, const QVector<NodeId> &nodeIds) {
            return model->captureSnapshotNodes(nodeIds);
        },
        [model](const QString &, const QJsonArray &nodesJson) {
            return model->applySnapshotNodes(nodesJson);
        },
        [self, key](const QString &) {
            QVector<NodeId> ids;
            // 仅当该 key 对应当前层时返回选中节点
            if (self->currentModel()
                && self->snapshotKeyFor(self->currentModel()) == key) {
                if (CustomFlowGraphicsScene *scene = self->currentScene()) {
                    const auto selected = scene->selectedNodes();
                    ids.reserve(static_cast<int>(selected.size()));
                    for (NodeId id : selected)
                        ids.append(id);
                }
            }
            return ids;
        });
}

QIcon titleBarNavIcon(const QString &resourcePath)
{
    return QIcon(QPixmap(resourcePath).scaled(16, 16, Qt::KeepAspectRatio, Qt::SmoothTransformation));
}

} // namespace

DataflowViewsManger::DataflowViewsManger(ads::CDockManager *dockManager,
                                         QObject *parent,
                                         DataflowPresentationMode mode)
    : QObject(parent)
    , _mode(mode)
    , m_DockManager(dockManager)
{
    if (wantsUi())
        ensureEditorUi();
}

DataflowViewsManger::~DataflowViewsManger()
{
    clearDataflow();
}

QString DataflowViewsManger::snapshotKeyFor(CustomDataFlowGraphModel *model) const
{
    if (!model)
        return {};
    QString const alias = model->modelAlias().trimmed();
    return alias.isEmpty() ? QStringLiteral("dataflow") : alias;
}

void DataflowViewsManger::registerSnapshotForModel(CustomDataFlowGraphModel *model)
{
    registerSnapshotHooks(this, snapshotKeyFor(model), model);
}

void DataflowViewsManger::setDockManager(ads::CDockManager *dockManager)
{
    m_DockManager = dockManager;
    if (wantsUi())
        ensureEditorUi();
}

void DataflowViewsManger::ensureEditorUi()
{
    if (!wantsUi() || !m_DockManager || _dockWidget)
        return;

    _dockWidget = m_DockManager->createDockWidget(QStringLiteral("节点编辑"));
    _dockWidget->setObjectName(QStringLiteral("dataflowEditor"));
    _dockWidget->setIcon(QIcon(":/icons/icons/genealogy.png"));

    _hostWidget = new QWidget(_dockWidget);
    auto *layout = new QVBoxLayout(_hostWidget);
    layout->setContentsMargins(0, 0, 0, 0);
    layout->setSpacing(0);

    _view = new CustomGraphicsView(_hostWidget);
    connect(_view, &GraphicsView::scaleChanged, this, [this](double) { rememberCurrentViewport(); });
    connect(_view, &GraphicsView::viewportChanged, this, [this]() { rememberCurrentViewport(); });
    layout->addWidget(_view, 1);
    _dockWidget->setWidget(_hostWidget);

    // 导航挂到 ADS Dock 标题栏：地址 → 上一级 → 主页 → Options
    _pathMenu = new QMenu(_dockWidget);
    _pathAction = new QAction(QStringLiteral("dataflow"), _dockWidget);
    _pathAction->setObjectName(QStringLiteral("dataflowNavPath"));
    _pathAction->setMenu(_pathMenu);
    _pathAction->setToolTip(QStringLiteral("当前数据流路径"));

    _backAction = new QAction(titleBarNavIcon(QStringLiteral(":/icons/icons/arror_left.png")),
                              QString(),
                              _dockWidget);
    _backAction->setObjectName(QStringLiteral("dataflowNavBack"));
    _backAction->setToolTip(QStringLiteral("上一级"));
    _backAction->setVisible(false);
    connect(_backAction, &QAction::triggered, this, &DataflowViewsManger::goBack);

    _homeAction = new QAction(titleBarNavIcon(QStringLiteral(":/icons/icons/home.png")),
                              QString(),
                              _dockWidget);
    _homeAction->setObjectName(QStringLiteral("dataflowNavHome"));
    _homeAction->setToolTip(QStringLiteral("返回主页"));
    _homeAction->setVisible(false);
    connect(_homeAction, &QAction::triggered, this, &DataflowViewsManger::goHome);

    QMenu *optionsMenu = new QMenu(_dockWidget);
    optionsMenu->setTitle(QObject::tr("Options"));
    optionsMenu->setToolTip(optionsMenu->title());
    optionsMenu->setIcon(QIcon(":/ads_icons/ads_icons/custom-menu-button.svg"));
    auto *menuAction = optionsMenu->menuAction();
    menuAction->setObjectName(QStringLiteral("optionsMenu"));

    _dockWidget->setTitleBarActions({_pathAction, _backAction, _homeAction, menuAction});

    auto *clearAct = optionsMenu->addAction(QIcon(":/icons/icons/clear.png"),
                                            QObject::tr("Clear Dataflow"));
    connect(clearAct, &QAction::triggered, this, [this]() {
        if (auto *scene = currentScene())
            scene->clearScene();
    });

    auto *lockAct = optionsMenu->addAction(QIcon(":/icons/icons/lock.png"),
                                           QObject::tr("Lock Dataflow"));
    connect(lockAct, &QAction::triggered, this, [this, lockAct]() {
        auto *model = currentModel();
        if (!model)
            return;
        model->setNodesLocked(!model->getNodesLocked());
        lockAct->setText(model->getNodesLocked() ? QObject::tr("Unlock Dataflow")
                                                 : QObject::tr("Lock Dataflow"));
    });

    auto *searchAct = optionsMenu->addAction(QIcon(":/icons/icons/search.png"),
                                             QObject::tr("Search Node"));
    connect(searchAct, &QAction::triggered, this, [this]() {
        if (auto *scene = currentScene())
            scene->showSearchNodeBar();
    });

    updateBreadcrumb();

    if (m_DockManager->focusedDockWidget()
        && m_DockManager->focusedDockWidget()->dockAreaWidget()) {
        m_DockManager->addDockWidgetTabToArea(_dockWidget,
                                              m_DockManager->focusedDockWidget()->dockAreaWidget());
    } else {
        m_DockManager->addDockWidgetTab(ads::CenterDockWidgetArea, _dockWidget);
    }
    m_DockManager->setWidgetFocus(_dockWidget);
}

void DataflowViewsManger::destroyScene(CustomFlowGraphicsScene *scene)
{
    if (!scene)
        return;
    _sceneViewports.remove(scene);
    if (_view && _view->scene() == scene)
        _view->setScene(nullptr);
    QObject::disconnect(&scene->graphModel(), nullptr, scene, nullptr);
    QObject::disconnect(scene, nullptr, this, nullptr);
    scene->setParent(nullptr);
    delete scene;
}

CustomFlowGraphicsScene *DataflowViewsManger::findSceneForModel(CustomDataFlowGraphModel *model) const
{
    if (!model)
        return nullptr;
    for (CustomFlowGraphicsScene *s : findChildren<CustomFlowGraphicsScene *>()) {
        if (s && &s->graphModel() == model)
            return s;
    }
    return nullptr;
}

void DataflowViewsManger::clearStackAndScenes()
{
    _isClearing = true;

    // Dock 可能已先于本对象销毁，_view 必须用 QPointer 判断
    if (_view)
        _view->setScene(nullptr);

    _stack.clear();
    _sceneViewports.clear();

    // 绝不能 clearScene()：会 deleteNode，嵌套 Container 的 inner model
    // 可能先于其 scene 被销毁。递归查找以免 reparent 后漏删。
    const auto scenes = findChildren<CustomFlowGraphicsScene *>();
    for (CustomFlowGraphicsScene *s : scenes)
        destroyScene(s);

    _rootModel.reset();
    _isClearing = false;
}

CustomFlowGraphicsScene *DataflowViewsManger::resetDataflow(const QString &title)
{
    QString const oldPath = currentPath();

    // 先通知外围（如 NodeList）解绑旧 model/scene，再销毁
    if (!oldPath.isEmpty())
        emit dataflowAboutToClear(oldPath);

    GraphSnapshotBridge::instance()->clearAll();
    clearStackAndScenes();

    auto registry = PluginsManager::instance()->registry();
    ContainerDataModel::setSharedRegistry(registry);

    _rootModel = std::make_unique<CustomDataFlowGraphModel>(registry);
    // 根层 alias 为空：OSC 为 /dataflow/<nodeId>/...
    _rootModel->setModelAlias(QString());
    registerSnapshotForModel(_rootModel.get());

    if (wantsUi())
        ensureEditorUi();

    CustomFlowGraphicsScene *scene = nullptr;
    if (wantsUi()) {
        scene = new CustomFlowGraphicsScene(*_rootModel, this);
        connectSceneNavigation(scene);
    }

    Level root;
    root.model = _rootModel.get();
    root.scene = scene;
    root.title = title.isEmpty() ? QStringLiteral("dataflow") : title;
    root.container = nullptr;
    _stack.push_back(root);

    if (scene)
        switchToScene(scene);
    else
        updateBreadcrumb();

    notifyLevelChanged();
    return scene;
}

void DataflowViewsManger::connectSceneNavigation(CustomFlowGraphicsScene *scene)
{
    if (!scene)
        return;

    connectHttpServerToScene(scene);

    connect(scene,
            &BasicGraphicsScene::nodeDoubleClicked,
            this,
            &DataflowViewsManger::onNodeDoubleClicked);

    auto *gm = dynamic_cast<CustomDataFlowGraphModel *>(&scene->graphModel());
    if (!gm)
        return;

    // 删除 Container 前清掉其子 scene，并弹出导航栈中对应层
    connect(gm,
            &AbstractGraphModel::nodeAboutToBeDeleted,
            this,
            [this, gm](NodeId id) {
                if (_isClearing)
                    return;

                auto *container = gm->delegateModel<ContainerDataModel>(id);
                if (!container)
                    return;

                CustomDataFlowGraphModel *inner = container->innerModel();
                if (inner)
                    GraphSnapshotBridge::instance()->unregisterScene(snapshotKeyFor(inner));

                // 若当前正停留在该 Container（或其更深层），先退回安全层再删 scene
                for (size_t i = 0; i < _stack.size(); ++i) {
                    if (_stack[i].container == container) {
                        rememberCurrentViewport();
                        while (_stack.size() > i)
                            _stack.pop_back();
                        if (!_stack.empty())
                            switchToScene(_stack.back().scene);
                        else if (_view)
                            _view->setScene(nullptr);
                        updateBreadcrumb();
                        notifyLevelChanged();
                        break;
                    }
                }

                if (!inner)
                    return;

                // 同一 inner 可能只有一个 scene；逐个销毁以防万一
                while (CustomFlowGraphicsScene *s = findSceneForModel(inner))
                    destroyScene(s);
            });
}

void DataflowViewsManger::setHttpServer(Flow::NodeHttpServer *server)
{
    _httpServer = server;
    for (CustomFlowGraphicsScene *scene : findChildren<CustomFlowGraphicsScene *>())
        connectHttpServerToScene(scene);
}

void DataflowViewsManger::connectHttpServerToScene(CustomFlowGraphicsScene *scene)
{
    if (!scene)
        return;

    connect(scene,
            &BasicGraphicsScene::sendOscBindingToWebPanel,
            this,
            &DataflowViewsManger::onSendOscBindingToWebPanel,
            Qt::UniqueConnection);
}

void DataflowViewsManger::onSendOscBindingToWebPanel(QJsonObject const &binding)
{
    if (!_httpServer)
        return;

    QWidget *parent = _view ? static_cast<QWidget *>(_view.data()) : nullptr;
    Flow::ExportActionDraft draft;
    if (!Flow::ExportActionDialog::prompt(parent, binding, draft)) {
        return;
    }

    QJsonObject item = binding;
    item[QStringLiteral("entity")] = draft.entity;
    item[QStringLiteral("name")] = draft.name;
    item[QStringLiteral("suggestedName")] = draft.name;

    const QString entity = _httpServer->addAction(item);
    if (entity.isEmpty()) {
        QMessageBox::warning(parent, tr("网页面板"), tr("发送失败：地址无效。"));
    }
}

void DataflowViewsManger::runWithoutViewportRepaint(std::function<void()> fn)
{
    if (!_view || !fn)
        return;

    _view->setUpdatesEnabled(false);
    _suppressViewportSave = true;
    fn();
    _suppressViewportSave = false;
    _view->setUpdatesEnabled(true);
    _view->viewport()->update();
}

void DataflowViewsManger::rememberCurrentViewport()
{
    if (_suppressViewportSave || !_view || _view->isRestoringViewport() || !_view->scene())
        return;

    auto *currentScene = qobject_cast<CustomFlowGraphicsScene *>(_view->scene());
    if (!currentScene)
        return;

    _sceneViewports.insert(currentScene, _view->viewportState());
}

void DataflowViewsManger::restoreOrCenterViewport(CustomFlowGraphicsScene *scene, bool forceCenter)
{
    if (!scene || !_view || _view->scene() != scene)
        return;

    if (!forceCenter && _sceneViewports.contains(scene)) {
        _view->setViewportState(_sceneViewports.value(scene));
        return;
    }

    _view->centerScene();
    _sceneViewports.insert(scene, _view->viewportState());
}

void DataflowViewsManger::switchToScene(CustomFlowGraphicsScene *scene)
{
    if (!scene || !_view)
        return;

    rememberCurrentViewport();

    runWithoutViewportRepaint([this, scene]() {
        _view->setScene(scene);
        restoreOrCenterViewport(scene);
    });

    updateBreadcrumb();
}

void DataflowViewsManger::pushLevel(Level level)
{
    _stack.push_back(level);
    switchToScene(level.scene);
    notifyLevelChanged();
}

void DataflowViewsManger::updateBreadcrumb()
{
    const bool canBack = canGoBack();
    if (_homeAction) {
        _homeAction->setEnabled(canBack);
        _homeAction->setVisible(canBack);
    }
    if (_backAction) {
        _backAction->setEnabled(canBack);
        _backAction->setVisible(canBack);
    }
    if (!_pathAction)
        return;

    if (_stack.empty()) {
        _pathAction->setText(QStringLiteral("dataflow"));
        _pathAction->setToolTip(QString());
        if (_pathMenu)
            _pathMenu->clear();
        return;
    }

    QStringList parts;
    parts.reserve(static_cast<int>(_stack.size()));
    for (auto const &lv : _stack)
        parts << lv.title;

    const QString pathText = parts.join(QStringLiteral(" > "));
    _pathAction->setText(pathText);
    _pathAction->setToolTip(pathText);

    if (!_pathMenu)
        return;

    _pathMenu->clear();
    const int n = static_cast<int>(_stack.size());
    for (int i = 0; i < n; ++i) {
        QAction *item = _pathMenu->addAction(_stack[static_cast<size_t>(i)].title);
        const bool isCurrent = (i == n - 1);
        item->setEnabled(!isCurrent);
        item->setCheckable(true);
        item->setChecked(isCurrent);
        if (!isCurrent) {
            connect(item, &QAction::triggered, this, [this, i]() {
                goBackToLevel(i);
            });
        }
    }
}

void DataflowViewsManger::goBackToLevel(int levelIndex)
{
    if (_isClearing || levelIndex < 0
        || levelIndex >= static_cast<int>(_stack.size()) - 1) {
        return;
    }

    rememberCurrentViewport();

    while (static_cast<int>(_stack.size()) - 1 > levelIndex) {
        if (_stack.back().container)
            _stack.back().container->syncInterfaceFromInner();
        _stack.pop_back();
    }
    switchToScene(_stack.back().scene);
    notifyLevelChanged();
}

void DataflowViewsManger::goHome()
{
    goBackToLevel(0);
}

QString DataflowViewsManger::currentPath() const
{
    if (_stack.empty())
        return {};
    QStringList parts;
    for (auto const &lv : _stack)
        parts << lv.title;
    return parts.join(QStringLiteral(" / "));
}

void DataflowViewsManger::notifyLevelChanged()
{
    emit currentLevelChanged(currentPath());
}

CustomFlowGraphicsScene *DataflowViewsManger::currentScene() const
{
    if (_stack.empty())
        return nullptr;
    return _stack.back().scene;
}

CustomDataFlowGraphModel *DataflowViewsManger::currentModel() const
{
    if (_stack.empty())
        return nullptr;
    return _stack.back().model;
}

void DataflowViewsManger::onNodeDoubleClicked(NodeId nodeId)
{
    if (_isClearing)
        return;

    auto *model = currentModel();
    auto *scene = currentScene();
    if (!model || !scene)
        return;

    auto *container = model->delegateModel<ContainerDataModel>(nodeId);
    if (!container)
        return;

    // 双击在库里会切换 WidgetEmbeddable；Container 不需要嵌入控件
    model->setNodeData(nodeId, NodeRole::WidgetEmbeddable, false);

    container->setParentAlias(model->modelAlias());
    container->setNodeID(nodeId);

    auto &inner = container->ensureInnerModel();
    // 仅空子图补默认 In/Out；已有内容时不要每次进入都 sync（会刷 dataUpdated）
    if (inner.allNodeIds().empty())
        container->seedDefaultInterfaceNodes();
    container->refreshInnerModelAlias();
    registerSnapshotForModel(&inner);

    for (auto const &lv : _stack) {
        if (lv.container == container && lv.scene)
            return; // 已在栈上，禁止重复进入
    }

    CustomFlowGraphicsScene *innerScene = findSceneForModel(&inner);
    if (!innerScene) {
        innerScene = new CustomFlowGraphicsScene(inner, this);
        connectSceneNavigation(innerScene);
    }

    QString title = container->getRemarks().trimmed();
    if (title.isEmpty())
        title = QStringLiteral("Container");

    Level level;
    level.model = &inner;
    level.scene = innerScene;
    level.title = title;
    level.container = container;
    pushLevel(level);
}

void DataflowViewsManger::goBack()
{
    if (_isClearing || !canGoBack())
        return;

    if (_stack.back().container)
        _stack.back().container->syncInterfaceFromInner();

    rememberCurrentViewport();

    _stack.pop_back();
    switchToScene(_stack.back().scene);
    notifyLevelChanged();
}

void DataflowViewsManger::clearDataflow()
{
    QString const oldPath = currentPath();
    if (!oldPath.isEmpty())
        emit dataflowAboutToClear(oldPath);

    GraphSnapshotBridge::instance()->clearAll();
    clearStackAndScenes();
    updateBreadcrumb();
}

void DataflowViewsManger::load(QJsonObject const &nodeJson)
{
    resetDataflow(QStringLiteral("dataflow"));
    if (!_rootModel)
        return;

    QJsonObject sceneJson = nodeJson.value(QStringLiteral("scene")).toObject();
    if (sceneJson.isEmpty())
        return;

    QObject::connect(_rootModel.get(),
                     &CustomDataFlowGraphModel::loadProgress,
                     this,
                     [this](const QString &phase, int current, int total) {
                         Q_EMIT loadProgress(QStringLiteral("dataflow"), phase, current, total);
                     });

    try {
        _rootModel->load(sceneJson);
    } catch (std::exception const &e) {
        qWarning() << "Dataflow load failed:" << e.what();
    }

    if (wantsUi() && !_stack.empty() && _stack.back().scene) {
        runWithoutViewportRepaint([this]() { restoreOrCenterViewport(_stack.back().scene, true); });
    }
}

QJsonObject DataflowViewsManger::save() const
{
    QJsonObject root;
    if (_rootModel)
        root[QStringLiteral("scene")] = _rootModel->save();
    else
        root[QStringLiteral("scene")] = QJsonObject{};
    return root;
}

void DataflowViewsManger::lockModelRecursive(CustomDataFlowGraphModel *model, bool locked)
{
    if (!model)
        return;
    model->setNodesLocked(locked);
    for (NodeId id : model->allNodeIds()) {
        if (auto *c = model->delegateModel<ContainerDataModel>(id)) {
            if (auto *inner = c->innerModel())
                lockModelRecursive(inner, locked);
        }
    }
}

void DataflowViewsManger::setDataflowLocked(bool locked)
{
    lockModelRecursive(_rootModel.get(), locked);
}

void DataflowViewsManger::refreshView()
{
    auto const &flowViewStyle = StyleCollection::flowViewStyle();
    if (!_view)
        return;
    _view->setBackgroundBrush(flowViewStyle.BackgroundColor);
    _view->update();
    if (_view->scene())
        _view->scene()->update();
}
