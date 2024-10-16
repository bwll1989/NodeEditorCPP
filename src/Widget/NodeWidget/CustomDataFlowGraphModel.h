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

    struct NodeGeometryData
    {
        QSize size;
        QPointF pos;
    };

    public:
    CustomDataFlowGraphModel(std::shared_ptr<NodeDelegateModelRegistry> registry);

    std::shared_ptr<NodeDelegateModelRegistry> dataModelRegistry() { return _registry; }

    public:
    std::unordered_set<NodeId> allNodeIds() const override;

    std::unordered_set<ConnectionId> allConnectionIds(NodeId const nodeId) const override;

    std::unordered_set<ConnectionId> connections(NodeId nodeId,
                                                 PortType portType,
                                                 PortIndex portIndex) const override;

    bool connectionExists(ConnectionId const connectionId) const override;

    NodeId addNode(QString const nodeType) override;

    bool connectionPossible(ConnectionId const connectionId) const override;

    bool detachPossible(ConnectionId const) const override;

    void setDetachPossible(bool d = true);

    void removePort(NodeId nodeId, PortType portType, PortIndex portIndex);

    void addPort(NodeId nodeId, PortType portType, PortIndex portIndex);

    NodeFlags nodeFlags(NodeId nodeId) const override;

    void setNodesLocked(bool b = true);

    void addConnection(ConnectionId const connectionId) override;

    bool nodeExists(NodeId const nodeId) const override;

    QVariant nodeData(NodeId nodeId, NodeRole role) const override;

    bool setNodeData(NodeId nodeId, NodeRole role, QVariant value) override;

    QVariant portData(NodeId nodeId,
                      PortType portType,
                      PortIndex portIndex,
                      PortRole role) const override;

    bool setPortData(NodeId nodeId,
                     PortType portType,
                     PortIndex portIndex,
                     QVariant const &value,
                     PortRole role = PortRole::Data) override;

    bool deleteConnection(ConnectionId const connectionId) override;

    bool deleteNode(NodeId const nodeId) override;

    QJsonObject saveNode(NodeId const) const override;

    QJsonObject save() const override;

    void loadNode(QJsonObject const &nodeJson) override;

    void load(QJsonObject const &json) override;

    /**
     * Fetches the NodeDelegateModel for the given `nodeId` and tries to cast the
     * stored pointer to the given type
     */
    template<typename NodeDelegateModelType>
    NodeDelegateModelType *delegateModel(NodeId const nodeId)
    {
        auto it = _models.find(nodeId);
        if (it == _models.end())
            return nullptr;

        auto model = dynamic_cast<NodeDelegateModelType *>(it->second.get());

        return model;
    }
    Q_SIGNALS:
    void inPortDataWasSet(NodeId const, PortType const, PortIndex const);

    private:
    NodeId newNodeId() override { return _nextNodeId++; }

    void sendConnectionCreation(ConnectionId const connectionId);

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
    void onOutPortDataUpdated(NodeId const nodeId, PortIndex const portIndex);

/// Function is called after detaching a connection.
    void propagateEmptyDataTo(NodeId const nodeId, PortIndex const portIndex);


    private:
    std::shared_ptr<NodeDelegateModelRegistry> _registry;
    PortEditAddRemoveWidget  *layoutWidget(NodeId) const;

//    mutable std::unordered_map<NodeId, NodePortCount> _nodePortCounts;
    mutable std::unordered_map<NodeId, PortEditAddRemoveWidget *> _nodeWidgets;

    NodeId _nextNodeId;


    bool _detachPossible=true;

    bool _nodesLocked= false;
    std::unordered_map<NodeId, std::unique_ptr<NodeDelegateModel>> _models;

    std::unordered_set<ConnectionId> _connectivity;

    mutable std::unordered_map<NodeId, NodeGeometryData> _nodeGeometryData;
    };

#endif //NODEEDITORCPP_CUSTOMDATAFLOWGRAPHMODEL_H
