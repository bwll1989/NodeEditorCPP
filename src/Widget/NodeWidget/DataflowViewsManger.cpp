// 文件：ViewsTabWidget.cpp（类 ViewsTabWidget 的成员实现）
// 依赖：QtNodes 的 DataFlowGraphicsScene / GraphicsView；QJsonObject / QJsonArray

#include "DataflowViewsManger.hpp"


#include "CustomFlowGraphicsScene.h"
#include "CustomGraphicsView.h"
#include <QJsonObject>
#include <QJsonArray>
#include <QSignalBlocker>
#include "DockWidget.h"
#include "QtNodes/internal/PluginsManager.hpp"

using namespace QtNodes;

DataflowViewsManger::DataflowViewsManger(ads::CDockManager* dockManager,QObject* parent)
    : QObject(parent),
    m_DockManager(dockManager){}

DataflowViewsManger::~DataflowViewsManger()
{
    // 函数级注释：
    // 说明：析构时确保场景和 DockWidget 全部关闭并释放，避免外部仍持有指向模型的引用导致悬挂指针。
    clearAllScenes();
}

void DataflowViewsManger::setDockManager(ads::CDockManager* dockManager)
{
    m_DockManager = dockManager;
}

void DataflowViewsManger::addNewScene(const QString& title)
{
    // 若标题已存在则直接返回，避免重复创建
    if (_models.count(title)) return;

    // 创建并以标题为键持久化模型
    _models.emplace(title, std::make_unique<CustomDataFlowGraphModel>(PluginsManager::instance()->registry()));
    auto& model = *_models.at(title);
    model.setModelAlias(title);
    ads::CDockWidget* DockWidget = new ads::CDockWidget(title);
    // 函数级注释：
    // // 作用：为 DockWidget 开启 DeleteOnClose，由 ADS 负责删除；管理器仅保存弱引用
    // DockWidget->setFeature(ads::CDockWidget::DockWidgetDeleteOnClose, true);

    // 以 DockWidget 为父对象，确保其关闭时 View/Scene 一并销毁
    auto view  = new CustomGraphicsView(DockWidget);
    auto scene = new CustomFlowGraphicsScene(model, view);
    // 函数级注释：
    // 说明：确保 Scene 作为 QObject 的父对象是 View，从而在 DockWidget 关闭时按父子链自动析构
    scene->setParent(view);
    view->setScene(scene);

    // 便捷行为：加载后居中显示
    QObject::connect(scene, &CustomFlowGraphicsScene::sceneLoaded, view, &GraphicsView::centerScene);

    // 仅保存弱引用，避免二次释放
    _DockWidget[title] = DockWidget;

    DockWidget->setWidget(view);
    DockWidget->setIcon(QIcon(":/icons/icons/dashboard.png"));

    // addAction(DockWidget->toggleViewAction());

    QMenu* OptionsMenu = new QMenu(DockWidget);
    OptionsMenu->setTitle(QObject::tr("Options"));
    OptionsMenu->setToolTip(OptionsMenu->title());
    OptionsMenu->setIcon(QIcon(":/icons/icons/options.png"));
    auto MenuAction = OptionsMenu->menuAction();
    // The object name of the action will be set for the QToolButton that
    // is created in the dock area title bar. You can use this name for CSS
    // styling
    MenuAction->setObjectName("optionsMenu");
    DockWidget->setTitleBarActions({OptionsMenu->menuAction()});
    auto a = OptionsMenu->addAction(QObject::tr("Clear Dataflow"));
    // c->connect(a, SIGNAL(triggered()), SLOT(clear()));
    QObject::connect(a, &QAction::triggered, scene, &CustomFlowGraphicsScene::clear);
    auto b = OptionsMenu->addAction(QObject::tr("Save Child Dataflow"));
    QObject::connect(b, &QAction::triggered, scene, &CustomFlowGraphicsScene::save);
    auto c = OptionsMenu->addAction(QObject::tr("Delete Dataflow"));
    QObject::connect(c, &QAction::triggered, this, [this, DockWidget, title](){
        m_DockManager->removeDockWidget(DockWidget);
        _models.erase(title);

    });
    auto d = OptionsMenu->addAction(QObject::tr("Load Child Dataflow"));
    QObject::connect(d, &QAction::triggered, scene, &CustomFlowGraphicsScene::load);
      // 安全添加到区域：优先使用当前聚焦区域，其次回退到中心区域
    if (m_DockManager && m_DockManager->focusedDockWidget() && m_DockManager->focusedDockWidget()->dockAreaWidget()) {
        m_DockManager->addDockWidgetTabToArea(DockWidget, m_DockManager->focusedDockWidget()->dockAreaWidget());
        m_DockManager->setWidgetFocus(DockWidget);
    } else {
        m_DockManager->addDockWidgetTab(ads::CenterDockWidgetArea, DockWidget);
        m_DockManager->setWidgetFocus(DockWidget);
    }
}


void DataflowViewsManger::addNewSceneFromeModel(const QString& title, CustomDataFlowGraphModel* model) {

    if (_models.count(title)) return;
    _models.emplace(title, std::unique_ptr<CustomDataFlowGraphModel>(model));

    ads::CDockWidget* DockWidget = new ads::CDockWidget(title);

    _DockWidget[title] = DockWidget;

    auto view  = new CustomGraphicsView(DockWidget);
    auto scene = new CustomFlowGraphicsScene(*model, view);
    // 说明：确保 Scene 作为 QObject 的父对象是 View，从而在 DockWidget 关闭时按父子链自动析构
    scene->setParent(view);
    view->setScene(scene);

    // 便捷行为：加载后居中显示
    QObject::connect(scene, &CustomFlowGraphicsScene::sceneLoaded, view, &GraphicsView::centerScene);

    DockWidget->setWidget(view);
    DockWidget->setIcon(QIcon(":/icons/icons/dashboard.png"));
    QMenu* OptionsMenu = new QMenu(DockWidget);
    OptionsMenu->setTitle(QObject::tr("Options"));
    OptionsMenu->setToolTip(OptionsMenu->title());
    OptionsMenu->setIcon(QIcon(":/icons/icons/options.png"));
    auto MenuAction = OptionsMenu->menuAction();
    // The object name of the action will be set for the QToolButton that
    // is created in the dock area title bar. You can use this name for CSS
    // styling
    MenuAction->setObjectName("optionsMenu");
    DockWidget->setTitleBarActions({OptionsMenu->menuAction()});
    auto a = OptionsMenu->addAction(QObject::tr("Clear Dataflow"));
    // c->connect(a, SIGNAL(triggered()), SLOT(clear()));
    QObject::connect(a, &QAction::triggered, scene, &CustomFlowGraphicsScene::clear);
    auto b = OptionsMenu->addAction(QObject::tr("Save Child Dataflow"));
    QObject::connect(b, &QAction::triggered, scene, &CustomFlowGraphicsScene::save);
    auto c = OptionsMenu->addAction(QObject::tr("Delete Dataflow"));
    QObject::connect(c, &QAction::triggered, this, [this, DockWidget, title](){
        m_DockManager->removeDockWidget(DockWidget);
        _models.erase(title);

    });
    auto d = OptionsMenu->addAction(QObject::tr("Load child Dataflow"));
    QObject::connect(d, &QAction::triggered, scene, &CustomFlowGraphicsScene::load);
    if (m_DockManager) {
        m_DockManager->addDockWidgetTab(ads::CenterDockWidgetArea, DockWidget);
        m_DockManager->setWidgetFocus(DockWidget);
    }
}

void DataflowViewsManger::clearAllScenes() {
    // 函数级注释：
    // 作用：批量关闭并删除所有 DockWidget（连带 View/Scene 按父子关系销毁），随后安全释放模型；
    // 关键：阻塞 DockManager 信号，避免批量关闭时触发重布局和回调造成重入/性能问题
    if (!m_DockManager) {
        _DockWidget.clear();
        _models.clear();
        return;
    }
    const QSignalBlocker blocker(m_DockManager);

    for (auto& kv : _DockWidget) {
        ads::CDockWidget* dock = kv.second; // QPointer 直接取原始指针
        if (dock) {
            m_DockManager->removeDockWidget(dock);
            // 触发删除
        }
    }
    _DockWidget.clear();

    // 2) 再释放模型，确保 Scene/View 已销毁，不再引用模型
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
                    auto model= new CustomDataFlowGraphModel(PluginsManager::instance()->registry());
                    model->load(tabJson);
                    addNewSceneFromeModel(tabTitle,model);
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
