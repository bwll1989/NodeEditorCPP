#pragma once

#include <functional>
#include <memory>
#include <vector>

#include <QHash>
#include <QObject>
#include <QPointer>
#include <QString>

#include "DockManager.h"
#include "CustomDataFlowGraphModel.h"
#include "CustomFlowGraphicsScene.h"
#include "CustomGraphicsView.h"
#include <QtNodes/internal/GraphicsView.hpp>
#include <QtNodes/NodeDelegateModelRegistry>

class QAction;
class QMenu;
class QWidget;

namespace Flow { class NodeHttpServer; }

namespace Nodes {
class ContainerDataModel;
}

/** @brief 数据流呈现模式：完整 UI 或仅加载/运行模型 */
enum class DataflowPresentationMode {
    WithUi,    ///< 创建 Dock + View + Scene（GUI 主窗口）
    ModelOnly  ///< 仅维护 CustomDataFlowGraphModel，不实例化图形界面
};

/**
 * 单画布 + 导航栈：根 DataFlow 与 Container 内嵌 DataFlow 之间进入/返回。
 * 替代原先的多 Tab 分页。
 */
class DataflowViewsManger : public QObject
{
    Q_OBJECT
public:
    explicit DataflowViewsManger(ads::CDockManager *dockManager,
                                 QObject *parent = nullptr,
                                 DataflowPresentationMode mode = DataflowPresentationMode::WithUi);

    ~DataflowViewsManger() override;

    void setDockManager(ads::CDockManager *dockManager);
    DataflowPresentationMode presentationMode() const { return _mode; }

    /// 创建/重置根画布为空白图
    CustomFlowGraphicsScene *resetDataflow(const QString &title = QStringLiteral("dataflow"));

    CustomFlowGraphicsScene *currentScene() const;
    CustomDataFlowGraphModel *currentModel() const;
    CustomDataFlowGraphModel *rootModel() const { return _rootModel.get(); }
    CustomGraphicsView *view() const { return _view; }

    bool canGoBack() const { return _stack.size() > 1; }
    QString currentPath() const;

    QJsonObject save() const;
    void load(QJsonObject const &nodeJson);

    /** @brief 关联 HTTP 服务，用于节点 OSC 菜单发送到网页面板 */
    void setHttpServer(Flow::NodeHttpServer *server);

public Q_SLOTS:
    void goBack();
    void setDataflowLocked(bool locked);
    /// 销毁当前数据流（不自动重建；析构 / 关机时用）
    void clearDataflow();
    void refreshView();

Q_SIGNALS:
    /// 即将清空/重建数据流（NodeList 等应先解绑）
    void dataflowAboutToClear(QString path);
    void currentLevelChanged(QString path);

    void loadProgress(const QString &sceneTitle, const QString &phase, int current, int total);

public:
    /// Snapshot / OSC 场景键：根为 "dataflow"，嵌套为 modelAlias 路径
    QString snapshotKeyFor(CustomDataFlowGraphModel *model) const;

private:
    struct Level {
        CustomDataFlowGraphModel *model = nullptr;
        CustomFlowGraphicsScene *scene = nullptr;
        QString title;
        Nodes::ContainerDataModel *container = nullptr;
    };

    bool wantsUi() const { return _mode == DataflowPresentationMode::WithUi; }
    void ensureEditorUi();
    void pushLevel(Level level);
    void connectSceneNavigation(CustomFlowGraphicsScene *scene);
    void connectHttpServerToScene(CustomFlowGraphicsScene *scene);
    void onSendOscBindingToWebPanel(QJsonObject const &binding);
    void onNodeDoubleClicked(QtNodes::NodeId nodeId);
    void updateBreadcrumb();
    void rememberCurrentViewport();
    void restoreOrCenterViewport(CustomFlowGraphicsScene *scene, bool forceCenter = false);
    void switchToScene(CustomFlowGraphicsScene *scene);
    void runWithoutViewportRepaint(std::function<void()> fn);
    void notifyLevelChanged();
    void registerSnapshotForModel(CustomDataFlowGraphModel *model);
    void clearStackAndScenes();
    /// 断开导航/model 信号后删除 scene（不 clearScene，避免误删 model 节点）
    void destroyScene(CustomFlowGraphicsScene *scene);
    CustomFlowGraphicsScene *findSceneForModel(CustomDataFlowGraphModel *model) const;
    void lockModelRecursive(CustomDataFlowGraphModel *model, bool locked);
    /// 返回到导航栈指定层（含该层），供面包屑点击
    void goBackToLevel(int levelIndex);
    /// 直接返回根数据流
    void goHome();

    DataflowPresentationMode _mode = DataflowPresentationMode::WithUi;
    ads::CDockManager *m_DockManager = nullptr;
    /// 批量清空/重建期间禁止导航回调（避免删 scene/model 时重入）
    bool _isClearing = false;

    std::unique_ptr<CustomDataFlowGraphModel> _rootModel;
    std::vector<Level> _stack;

    /// 各 scene 独立的缩放/平移（与导航栈解耦，返回后子 scene 仍可复用视口）。
    QHash<CustomFlowGraphicsScene *, QtNodes::GraphicsView::ViewportState> _sceneViewports;
    bool _suppressViewportSave = false;

    QPointer<ads::CDockWidget> _dockWidget;
    QPointer<QWidget> _hostWidget;
    QPointer<QAction> _homeAction;
    QPointer<QAction> _backAction;
    QPointer<QAction> _pathAction;
    QPointer<QMenu> _pathMenu;
    QPointer<CustomGraphicsView> _view;
    Flow::NodeHttpServer *_httpServer = nullptr;
};
