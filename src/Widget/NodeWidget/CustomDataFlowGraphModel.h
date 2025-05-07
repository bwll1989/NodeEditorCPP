//
// Created by 吴斌 on 2024/1/16.
//

#ifndef NODEEDITORCPP_CUSTOMDATAFLOWGRAPHMODEL_H
#define NODEEDITORCPP_CUSTOMDATAFLOWGRAPHMODEL_H
#pragma once

#include <memory>

#include "QtNodes/AbstractGraphModel"
#include "QtNodes/ConnectionIdUtils"
#include "QtNodes/internal/Export.hpp"
#include "QtNodes/NodeDelegateModelRegistry"
#include "QtNodes/internal/Serializable.hpp"
#include "QtNodes/StyleCollection"
#include "Widget/PortEditWidget/PortEditAddRemoveWidget.hpp"
#include <QJsonObject>
using QtNodes::AbstractGraphModel;
using QtNodes::Serializable;
using QtNodes::NodeDelegateModelRegistry;
using QtNodes::NodeId;
using QtNodes::GroupId;
using QtNodes::ConnectionId;
using QtNodes::PortType;
using QtNodes::PortIndex;
using QtNodes::NodeRole;
using QtNodes::NodeFlags;
using QtNodes::PortRole;
using QtNodes::NodeDelegateModel;
class PortEditAddRemoveWidget;
class CustomDataFlowGraphModel: public AbstractGraphModel, public Serializable
{
    Q_OBJECT

    public:
    /**
     * 节点几何数据
     */
    struct NodeGeometryData
    {
        QSize size;
        QPointF pos;
    };

    public:
    /**
     * 构造函数
     * @param std::shared_ptr<NodeDelegateModelRegistry> registry 注册表
     */
    CustomDataFlowGraphModel(std::shared_ptr<NodeDelegateModelRegistry> registry);

    /**
     * 数据模型注册表
     * @return std::shared_ptr<NodeDelegateModelRegistry> 注册表
     */
    std::shared_ptr<NodeDelegateModelRegistry> dataModelRegistry() { return _registry; }

    public:
    /**
     * 所有节点ID
     * @return std::unordered_set<NodeId> 节点ID集合
     */
    std::unordered_set<NodeId> allNodeIds() const override;
    /**
     * 所有连接ID
     * @param NodeId const nodeId 节点ID
     * @return std::unordered_set<ConnectionId> 连接ID集合
     */
    std::unordered_set<ConnectionId> allConnectionIds(NodeId const nodeId) const override;

    std::unordered_set<GroupId> allGroupIds() const override;

    std::unordered_set<ConnectionId> connections(NodeId nodeId,
                                                 PortType portType,
                                                 PortIndex portIndex) const override;
    /**
     * 连接是否存在
     * @param ConnectionId const connectionId 连接ID
     * @return bool 是否存在
     */
    bool connectionExists(ConnectionId const connectionId) const override;
    /**
     * 添加节点
     * @param QString const nodeType 节点类型
     */
    NodeId addNode(QString const nodeType) override;
    
    bool connectionPossible(ConnectionId const connectionId) const override;
    /**
     * 分离是否可能
     * @param ConnectionId const 连接ID
     */
    bool detachPossible(ConnectionId const) const override;
    /**
     * 设置分离是否可能
     * @param bool d 是否可能
     */
    void setDetachPossible(bool d = true);
    /**
     * 移除端口
     * @param NodeId nodeId 节点ID
     * @param PortType portType 端口类型
     * @param PortIndex portIndex 端口索引
     */
    void removePort(NodeId nodeId, PortType portType, PortIndex portIndex);
    /**
     * 添加端口
     * @param NodeId nodeId 节点ID
     * @param PortType portType 端口类型
     * 
     */
    void addPort(NodeId nodeId, PortType portType, PortIndex portIndex);
    /**
     * 节点标志
     * @param NodeId nodeId 节点ID
     * @return NodeFlags 节点标志
     */
    NodeFlags nodeFlags(NodeId nodeId) const override;
    /**
     * 设置节点锁定
     * @param bool b 是否锁定
     */
    void setNodesLocked(bool b = true);
    /**
     * 获取节点锁定
     * @return bool 是否锁定
     */
    bool getNodesLocked() const;
    /**
     * 添加连接
     * @param ConnectionId const connectionId 连接ID
     */
    void addConnection(ConnectionId const connectionId) override;

    void addGroup(GroupId const groupId) override;

    bool nodeExists(NodeId const nodeId) const override;
    /**
     * 节点数据
     * @param NodeId nodeId 节点ID
     * @param NodeRole role 节点角色
     * @return QVariant 节点数据
     */
    QVariant nodeData(NodeId nodeId, NodeRole role) const override;

    bool setNodeData(NodeId nodeId, NodeRole role, QVariant value) override;
    /**
     * 端口数据
     * @param NodeId nodeId 节点ID
     * @param PortType portType 端口类型
     * @param PortIndex portIndex 端口索引
     * @param PortRole role 端口角色
     * @return QVariant 端口数据
     */
    QVariant portData(NodeId nodeId,
                      PortType portType,
                      PortIndex portIndex,
                      PortRole role) const override;
    /**
     * 设置端口数据
     * @param NodeId nodeId 节点ID
     * @param PortType portType 端口类型
     * @param PortIndex portIndex 端口索引
     * @param QVariant value 端口数据
     * @param PortRole role 端口角色
     * @return bool 是否设置成功
     */
    bool setPortData(NodeId nodeId,
                     PortType portType,
                     PortIndex portIndex,
                     QVariant const &value,
                     PortRole role = PortRole::Data) override;
    /**
     * 删除连接
     * @param ConnectionId const connectionId 连接ID
     * @return bool 是否删除成功
     */
    bool deleteConnection(ConnectionId const connectionId) override;
    /**
     * 删除节点
     * @param NodeId const nodeId 节点ID
     * @return bool 是否删除成功
     */
    bool deleteNode(NodeId const nodeId) override;

    bool deleteGroup(GroupId const groupId) override;

    void updateGroup(GroupId const oldGroupId,GroupId const newGroupId) override;

    QJsonObject saveNode(NodeId const) const override;
    /**
     * 保存
     * @return QJsonObject 节点数据
     */
    QJsonObject save() const override;
    /**
     * 加载节点
     * @param QJsonObject const &nodeJson 节点数据
     */ 
    void loadNode(QJsonObject const &nodeJson) override;
    /**
     * 加载
     */
    void load(QJsonObject const &json) override;

    /**
     * 获取节点代理模型
     * @param NodeId const nodeId 节点ID
     * @return NodeDelegateModelType * 节点代理模型
     */
    template<typename NodeDelegateModelType>
    /**
     * 获取节点代理模型
     * @param NodeId const nodeId 节点ID
     * @return NodeDelegateModelType * 节点代理模型
     */
    NodeDelegateModelType *delegateModel(NodeId const nodeId)
    {
        auto it = _models.find(nodeId);
        if (it == _models.end())
            return nullptr;

        auto model = dynamic_cast<NodeDelegateModelType *>(it->second.get());

        return model;
    }
Q_SIGNALS:
    /**
     * 端口数据被设置
     * @param NodeId const nodeId 节点ID
     * @param PortType const portType 端口类型
     * @param PortIndex const portIndex 端口索引
     */
    void inPortDataWasSet(NodeId const, PortType const, PortIndex const);

private:
    /**
     * 新节点ID
     * @return NodeId 新节点ID
     */
    NodeId newNodeId() override { return _nextNodeId++; }
    /**
     * 发送连接创建
     * @param ConnectionId const connectionId 连接ID
     */
    void sendConnectionCreation(ConnectionId const connectionId);
    /**
     * 发送连接删除
     * @param ConnectionId const connectionId 连接ID
     */
    void sendConnectionDeletion(ConnectionId const connectionId);

    private Q_SLOTS:
/**
 * Fuction is called in three cases:
 *
 * - By underlying NodeDelegateModel when a node has new data to propagate.
 *   @see DataFlowGraphModel::addNode
 * - When a new connection is created.
 *   @see DataFlowGraphModel::addConnection
 * - When a node restored from JSON an needs to send data downstream.
 *   @see DataFlowGraphModel::loadNode
 */
    /**
     * 发送端口数据更新
     * @param NodeId const nodeId 节点ID
     * @param PortIndex const portIndex 端口索引
     */
    void onOutPortDataUpdated(NodeId const nodeId, PortIndex const portIndex);

    /**
     * 发送空数据到
     * @param NodeId const nodeId 节点ID
     * @param PortIndex const portIndex 端口索引
     */
    void propagateEmptyDataTo(NodeId const nodeId, PortIndex const portIndex);


private:
    //注册节点模型
    std::shared_ptr<NodeDelegateModelRegistry> _registry;
    //布局控件
    PortEditAddRemoveWidget  *layoutWidget(NodeId) const;
    //节点部件
    mutable std::unordered_map<NodeId, PortEditAddRemoveWidget *> _nodeWidgets;
    //下一个节点ID
    NodeId _nextNodeId;
    //分离是否可能
    bool _detachPossible=true;
    //节点锁定
    bool _nodesLocked= false;
    //节点模型
    std::unordered_map<NodeId, std::unique_ptr<NodeDelegateModel>> _models;
    //连接
    std::unordered_set<ConnectionId> _connectivity;

    std::unordered_set<GroupId> _groups;

    mutable std::unordered_map<NodeId, NodeGeometryData> _nodeGeometryData;
    };

#endif //NODEEDITORCPP_CUSTOMDATAFLOWGRAPHMODEL_H
