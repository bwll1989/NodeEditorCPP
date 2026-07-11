/**
 * @file GraphSnapshotBridge.cpp
 * @brief GraphSnapshotBridge 实现
 */
#include "GraphSnapshotBridge.hpp"

GraphSnapshotBridge *GraphSnapshotBridge::instance()
{
    static GraphSnapshotBridge inst;
    return &inst;
}

GraphSnapshotBridge::GraphSnapshotBridge(QObject *parent)
    : QObject(parent)
{
}

void GraphSnapshotBridge::registerScene(const QString &sceneTitle,
                                        CaptureFn capture,
                                        ApplyFn apply,
                                        SelectedFn selected)
{
    if (sceneTitle.isEmpty()) {
        return;
    }
    QWriteLocker locker(&_lock);
    _scenes.insert(sceneTitle, SceneHooks{std::move(capture), std::move(apply), std::move(selected)});
}

void GraphSnapshotBridge::unregisterScene(const QString &sceneTitle)
{
    QWriteLocker locker(&_lock);
    _scenes.remove(sceneTitle);
}

void GraphSnapshotBridge::clearAll()
{
    QWriteLocker locker(&_lock);
    _scenes.clear();
}

QJsonArray GraphSnapshotBridge::captureNodes(const QString &sceneTitle,
                                             const QVector<GraphSnapshotNodeId> &nodeIds) const
{
    QReadLocker locker(&_lock);
    const auto it = _scenes.constFind(sceneTitle);
    if (it == _scenes.end() || !it->capture) {
        return {};
    }
    // 转发到 Widget 层注册的 capture 回调（通常为 captureSnapshotNodes）
    return it->capture(sceneTitle, nodeIds);
}

bool GraphSnapshotBridge::applySnapshot(const QString &sceneTitle, const QJsonArray &nodesJson) const
{
    QReadLocker locker(&_lock);
    const auto it = _scenes.constFind(sceneTitle);
    if (it == _scenes.end() || !it->apply) {
        return false;
    }
    // 转发到 Widget 层注册的 apply 回调（通常为 applySnapshotNodes → load）
    return it->apply(sceneTitle, nodesJson);
}

QVector<GraphSnapshotNodeId> GraphSnapshotBridge::selectedNodes(const QString &sceneTitle) const
{
    QReadLocker locker(&_lock);
    const auto it = _scenes.constFind(sceneTitle);
    if (it == _scenes.end() || !it->selected) {
        return {};
    }
    // 转发到场景侧注册的 selected 回调（通常为 CustomFlowGraphicsScene::selectedNodes）
    return it->selected(sceneTitle);
}
