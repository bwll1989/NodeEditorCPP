#pragma once

#include <QTabWidget>
#include <memory>
#include <vector>
#include "DockManager.h"
#include "CustomDataFlowGraphModel.h"
#include "CustomFlowGraphicsScene.h"
#include <QtNodes/GraphicsView>
#include <QtNodes/NodeDelegateModelRegistry>

#include "CustomGraphicsView.h"
#include "ModelDataBridge/ModelDataBridge.hpp"

class DataflowViewsManger : public QObject

{
public:
    explicit DataflowViewsManger( ads::CDockManager* dockManager,QObject* parent = nullptr);
    
    ~DataflowViewsManger();
    /**
     * @brief 设置DockManager指针
     *
     * @param dockManager DockManager指针
     */
    void setDockManager(ads::CDockManager* dockManager);
    /**
     * @brief 创建新空场景
     *
     * @param title 场景标题
     */
    void addNewScene(const QString& title = QString());
    /**
     * @brief 从已有的模型创建新场景
     *
     * @param title 场景标题
     * @param model 数据流程模型指针
     */
    void addNewSceneFromeModel(const QString& title = QString(), CustomDataFlowGraphModel* model = nullptr);

    /**
     * @brief 保存所有场景的状态到JSON对象
     *
     * @return QJsonObject JSON对象
     */
    QJsonObject save() const;
    /**
     * @brief 从JSON加载场景
     *
     * @param nodeJson JSON对象
     */
    void load(QJsonObject const &nodeJson);
    /**
     * @brief 获取所有模型的引用
     *
     * @return std::map<QString, std::unique_ptr<CustomDataFlowGraphModel>>& 模型映射
     */
    std::map<QString, std::unique_ptr<CustomDataFlowGraphModel>> *getModel();
public Q_SLOTS:
    /**
     * @brief 批量设置所有模型的节点锁定状态
     *
     * @param locked 是否锁定节点
     */
    void setSceneLocked(bool locked);
    /**
     * @brief 清空所有场景，包括模型、视图和停靠窗口
     */
    void clearAllScenes();
private:
    // 键：标题（addNewScene 传入的 title）；值：对应的数据流模型
    std::map<QString, std::unique_ptr<CustomDataFlowGraphModel>> _models;
    // 保存所有已创建的ads::CDockWidget指针，键：标题（addNewScene 传入的 title）；值：指针
    std::map<QString, QPointer<ads::CDockWidget>> _DockWidget;
    // 默认注册器（可选），用于创建未指定注册器的场景
    std::shared_ptr<QtNodes::NodeDelegateModelRegistry> _defaultRegistry;
    // ads::CDockManager指针，用于创建和管理DockWidget,来自主窗口
    ads::CDockManager* m_DockManager;

};