/**
 * @file GraphSnapshotBridge.hpp
 * @brief 图快照桥接器
 *
 * 在 Widget 层（CustomDataFlowGraphModel / DataflowViewsManger）与插件层
 * （Snapshot 节点）之间提供解耦桥梁，避免节点插件直接依赖编辑器内部类型。
 *
 * 工作流程：
 * 1. DataflowViewsManger 创建 dataflow 时调用 registerScene() 注册三个回调
 * 2. Snapshot 节点通过 sceneTitle（= getParentAlias()）调用 captureNodes / applySnapshot
 * 3. dataflow 关闭或工程切换时 unregisterScene() / clearAll()
 *
 * 典型调用链：
 *   捕获：selectedNodes → captureNodes → CustomDataFlowGraphModel::captureSnapshotNodes
 *   召回：applySnapshot → CustomDataFlowGraphModel::applySnapshotNodes → NodeDelegateModel::load()
 */
#pragma once

#include <QObject>
#include <QHash>
#include <QJsonArray>
#include <QReadWriteLock>
#include <QString>
#include <QVector>
#include <functional>

#ifdef GRAPHSNAPSHOTBRIDGE_LIBRARY
#define GRAPHSNAPSHOTBRIDGE_EXPORT Q_DECL_EXPORT
#else
#define GRAPHSNAPSHOTBRIDGE_EXPORT Q_DECL_IMPORT
#endif

/** @brief 图快照使用的节点 ID 类型（与 QtNodes::NodeId 数值兼容） */
using GraphSnapshotNodeId = unsigned int;

/**
 * @class GraphSnapshotBridge
 * @brief 全局单例桥接器，按 dataflow 标题路由快照捕获与召回请求
 *
 * 每个 dataflow 场景注册一组钩子函数（SceneHooks），
 * 插件侧无需持有 CustomDataFlowGraphModel 指针。
 */
class GRAPHSNAPSHOTBRIDGE_EXPORT GraphSnapshotBridge : public QObject
{
    Q_OBJECT
public:
    /**
     * @brief 捕获回调：将指定节点序列化为 saveNode 格式的 JSON 数组
     * @param sceneTitle dataflow 标题（别名）
     * @param nodeIds    待捕获的节点 ID 列表
     */
    using CaptureFn = std::function<QJsonArray(const QString &sceneTitle, const QVector<GraphSnapshotNodeId> &nodeIds)>;

    /**
     * @brief 召回回调：将快照 JSON 应用到已存在的节点（调用 load，不创建新节点）
     * @param sceneTitle dataflow 标题
     * @param nodesJson  saveNode 格式的节点 JSON 数组
     * @return 是否至少成功应用一个节点
     */
    using ApplyFn = std::function<bool(const QString &sceneTitle, const QJsonArray &nodesJson)>;

    /**
     * @brief 选中节点查询回调：返回当前画布中选中的节点 ID
     * @param sceneTitle dataflow 标题
     */
    using SelectedFn = std::function<QVector<GraphSnapshotNodeId>(const QString &sceneTitle)>;

    /** @brief 获取全局单例（线程安全懒汉式） */
    static GraphSnapshotBridge *instance();

    /**
     * @brief 注册 dataflow 场景的快照钩子
     *
     * 同一场景重复注册会覆盖旧钩子。
     * 由 DataflowViewsManger::ensureModel() 在模型创建时调用。
     */
    void registerScene(const QString &sceneTitle, CaptureFn capture, ApplyFn apply, SelectedFn selected);

    /** @brief 注销单个场景（dataflow 删除时调用） */
    void unregisterScene(const QString &sceneTitle);

    /** @brief 清空所有场景注册（工程切换 / clearDataflow 时调用） */
    void clearAll();

    /**
     * @brief 捕获指定节点快照
     * @return saveNode 格式的 JSON 数组；场景未注册时返回空数组
     */
    QJsonArray captureNodes(const QString &sceneTitle, const QVector<GraphSnapshotNodeId> &nodeIds) const;

    /**
     * @brief 召回快照到已存在节点
     * @return 是否成功；场景未注册时返回 false
     */
    bool applySnapshot(const QString &sceneTitle, const QJsonArray &nodesJson) const;

    /**
     * @brief 获取场景中当前选中的节点 ID
     * @return 节点 ID 列表；无 UI 或未选中时返回空列表
     */
    QVector<GraphSnapshotNodeId> selectedNodes(const QString &sceneTitle) const;

private:
    explicit GraphSnapshotBridge(QObject *parent = nullptr);

    /** @brief 单个 dataflow 场景注册的回调集合 */
    struct SceneHooks {
        CaptureFn capture;   ///< 节点序列化
        ApplyFn apply;       ///< 快照召回
        SelectedFn selected; ///< 选中节点查询
    };

    mutable QReadWriteLock _lock;              ///< 保护 _scenes 的读写锁
    QHash<QString, SceneHooks> _scenes;        ///< key = dataflow 标题（别名）
};
