//
// Created by 吴斌 on 2024/1/16.
//

#include "CustomDataFlowGraphModel.h"

#include <stdexcept>
#include <QMessageBox>
#include "QtNodes/internal/ConnectionIdHash.hpp"
#include "QtNodes/internal/GroupIdUtils.hpp"
#include "Widget/PortEditWidget/PortEditAddRemoveWidget.hpp"
#include <QJsonArray>
#include <QToolBox>
#include "DataTypes/NodeDataList.hpp"
using QtNodes::InvalidNodeId;
using QtNodes::ConnectionPolicy;
using QtNodes::NodeDataType;
using QtNodes:: StyleCollection;
using QtNodes::NodeFlag;
using QtNodes::fromJson;
using QtNodes::groupToJson;
using QtNodes::NodeData;
using namespace NodeDataTypes;
CustomDataFlowGraphModel::CustomDataFlowGraphModel(std::shared_ptr<NodeDelegateModelRegistry> registry)
        : _registry(std::move(registry))
        , _nextNodeId{0}
{}

std::unordered_set<NodeId> CustomDataFlowGraphModel::allNodeIds() const
{
    std::unordered_set<NodeId> nodeIds;
    for_each(_models.begin(), _models.end(), [&nodeIds](auto const &p) { nodeIds.insert(p.first); });

    return nodeIds;
}

std::unordered_set<ConnectionId> CustomDataFlowGraphModel::allConnectionIds(NodeId const nodeId) const
{
    std::unordered_set<ConnectionId> result;

    std::copy_if(_connectivity.begin(),
                 _connectivity.end(),
                 std::inserter(result, std::end(result)),
                 [&nodeId](ConnectionId const &cid) {
                     return cid.inNodeId == nodeId || cid.outNodeId == nodeId;
                 });

    return result;
}

std::unordered_set<GroupId> CustomDataFlowGraphModel::allGroupIds() const
{
    return _groups;
}

std::unordered_set<ConnectionId> CustomDataFlowGraphModel::connections(NodeId nodeId,
                                                                 PortType portType,
                                                                 PortIndex portIndex) const
{
    std::unordered_set<ConnectionId> result;

    std::copy_if(_connectivity.begin(),
                 _connectivity.end(),
                 std::inserter(result, std::end(result)),
                 [&portType, &portIndex, &nodeId](ConnectionId const &cid) {
                     return (getNodeId(portType, cid) == nodeId &&
                             getPortIndex(portType, cid) == portIndex);
                 });

    return result;
}

bool CustomDataFlowGraphModel::connectionExists(ConnectionId const connectionId) const
{
    return (_connectivity.find(connectionId) != _connectivity.end());
}

NodeId CustomDataFlowGraphModel::addNode(QString const nodeType)
{
    std::unique_ptr<NodeDelegateModel> model = _registry->create(nodeType);

    if (model) {
        NodeId newId = newNodeId();
        model->setNodeID(newId);
        connect(model.get(),
                &NodeDelegateModel::dataUpdated,
                [newId, this](PortIndex const portIndex) {
                    onOutPortDataUpdated(newId, portIndex);
                });

        connect(model.get(),
                &NodeDelegateModel::portsAboutToBeDeleted,
                this,
                [newId, this](PortType const portType, PortIndex const first, PortIndex const last) {
                    portsAboutToBeDeleted(newId, portType, first, last);
                });

        connect(model.get(),
                &NodeDelegateModel::portsDeleted,
                this,
                &CustomDataFlowGraphModel::portsDeleted);

        connect(model.get(),
                &NodeDelegateModel::portsAboutToBeInserted,
                this,
                [newId, this](PortType const portType, PortIndex const first, PortIndex const last) {
                    portsAboutToBeInserted(newId, portType, first, last);
                });

        connect(model.get(),
                &NodeDelegateModel::portsInserted,
                this,
                &CustomDataFlowGraphModel::portsInserted);

        _models[newId] = std::move(model);

        Q_EMIT nodeCreated(newId);
        return newId;
    }

    return InvalidNodeId;
}

bool CustomDataFlowGraphModel::connectionPossible(ConnectionId const connectionId) const
{
//    端口类型是否相同
    auto getDataType = [&](PortType const portType) {
        return portData(getNodeId(portType, connectionId),
                        portType,
                        getPortIndex(portType, connectionId),
                        PortRole::DataType)
                .value<NodeDataType>();
    };

// 输入端口是否为空
    auto portVacant = [&](PortType const portType) {
        NodeId const nodeId = getNodeId(portType, connectionId);
        PortIndex const portIndex = getPortIndex(portType, connectionId);
        auto const connected = connections(nodeId, portType, portIndex);

        auto policy = portData(nodeId, portType, portIndex, PortRole::ConnectionPolicyRole)
                .value<ConnectionPolicy>();
        return connected.empty() || (policy == ConnectionPolicy::Many);

    };

//
    if (portVacant(PortType::Out) && portVacant(PortType::In)){
//        接口未占用时，判断数据类型是否一致，或端口类型为万能类
        return getDataType(PortType::Out).id == getDataType(PortType::In).id or
           getDataType(PortType::In).id == VariableData().type().id;

    } else{
//        返回不允许连接
        return false;
    }

//    输入接口未被
}

void CustomDataFlowGraphModel::addConnection(ConnectionId const connectionId)
{
    _connectivity.insert(connectionId);

    sendConnectionCreation(connectionId);

    QVariant const portDataToPropagate = portData(connectionId.outNodeId,
                                                  PortType::Out,
                                                  connectionId.outPortIndex,
                                                  PortRole::Data);

    setPortData(connectionId.inNodeId,
                PortType::In,
                connectionId.inPortIndex,
                portDataToPropagate,
                PortRole::Data);
//    qDebug()<<"add connection:"+QString::number(connectionId.outNodeId)+" to "+QString::number(connectionId.inNodeId);
}

void CustomDataFlowGraphModel::addGroup(GroupId const groupId)
{
//    // 濡傛灉缁勮妭鐐规暟閲忓皬浜?锛屼笉鍒涘缓缁?//
    if (groupId.nodeIds.size() ==0) {
        //qDebug() << "Group not added: Less than 2 nodes";
        return;
    }
    
    // 妫€鏌ユ槸鍚﹀凡缁忓瓨鍦ㄥ畬鍏ㄧ浉鍚岀殑缁?
     for (const auto& existingGroup : _groups) {
        // 棣栧厛姣旇緝鑺傜偣鏁伴噺鏄惁鐩稿悓
        if (existingGroup.nodeIds.size() == groupId.nodeIds.size()) {
            // 妫€鏌ユ槸鍚﹀寘鍚畬鍏ㄧ浉鍚岀殑鑺傜偣锛堝拷鐣ラ『搴忥級
            std::vector<NodeId> sortedNewNodes = groupId.nodeIds;
            std::vector<NodeId> sortedExistingNodes = existingGroup.nodeIds;
            std::sort(sortedNewNodes.begin(), sortedNewNodes.end());
            std::sort(sortedExistingNodes.begin(), sortedExistingNodes.end());
            
            if (sortedNewNodes == sortedExistingNodes) {
                qDebug() << "Group not added: Identical group already exists";
                return;
            }
        }
    }
    
    // 妫€鏌ョ粍閲嶅彔
    for (const auto& existingGroup : _groups) {
        // 璁＄畻鑺傜偣浜ら泦
        std::vector<NodeId> intersection;
        for (const auto& nodeId : groupId.nodeIds) {
            if (std::find(existingGroup.nodeIds.begin(), existingGroup.nodeIds.end(), nodeId) != existingGroup.nodeIds.end()) {
                intersection.push_back(nodeId);
            }
        }
        
        // 濡傛灉鏈変换浣曢噸鍙犺妭鐐癸紝涓嶆坊鍔犳柊缁?
         if (!intersection.empty()) {
            qDebug() << "Group not added: Node overlap detected";
            return;
        }
    }

    // 娣诲姞鏂扮粍
//    qDebug() << "Adding group with" << groupId.nodeIds.size() << "nodes";
    _groups.insert(groupId);
    Q_EMIT groupCreated(groupId);
}

void CustomDataFlowGraphModel::sendConnectionCreation(ConnectionId const connectionId)
{
    Q_EMIT connectionCreated(connectionId);

    auto iti = _models.find(connectionId.inNodeId);
    auto ito = _models.find(connectionId.outNodeId);
    if (iti != _models.end() && ito != _models.end()) {
        auto &modeli = iti->second;
        auto &modelo = ito->second;
        modeli->inputConnectionCreated(connectionId);
        modelo->outputConnectionCreated(connectionId);
    }
}

void CustomDataFlowGraphModel::sendConnectionDeletion(ConnectionId const connectionId)
{
    Q_EMIT connectionDeleted(connectionId);

    auto iti = _models.find(connectionId.inNodeId);
    auto ito = _models.find(connectionId.outNodeId);
    if (iti != _models.end() && ito != _models.end()) {
        auto &modeli = iti->second;
        auto &modelo = ito->second;
        modeli->inputConnectionDeleted(connectionId);
        modelo->outputConnectionDeleted(connectionId);
    }
}

bool CustomDataFlowGraphModel::nodeExists(NodeId const nodeId) const
{
    return (_models.find(nodeId) != _models.end());
}

QVariant CustomDataFlowGraphModel::nodeData(NodeId nodeId, NodeRole role) const
{
    QVariant result;

    auto it = _models.find(nodeId);
    if (it == _models.end())
        return result;

    auto &model = it->second;

    switch (role) {
        case NodeRole::Type:
            result = model->type();
            break;

        case NodeRole::Position:
            result = _nodeGeometryData[nodeId].pos;
            break;

        case NodeRole::Size:
            result = _nodeGeometryData[nodeId].size;
            break;

        case NodeRole::CaptionVisible:
            result = model->widgetEmbeddable() ? model->captionVisible() : true;
            break;

        case NodeRole::Caption:
            result = model->caption();
            break;

        case NodeRole::Style: {
            auto style = StyleCollection::nodeStyle();
            result = style.toJson().toVariantMap();
        } break;

        case NodeRole::InternalData: {
            QJsonObject nodeJson;

            nodeJson["internal-data"] = _models.at(nodeId)->save();

            result = nodeJson.toVariantMap();
            break;
        }

        case NodeRole::InPortCount:
            result = model->nPorts(PortType::In);
            break;

        case NodeRole::OutPortCount:
            result = model->nPorts(PortType::Out);
            break;

        case NodeRole::WidgetEmbeddable:
            result = model->widgetEmbeddable();
            break;

        case NodeRole::Widget: {
                switch (model->getWidgetType()) {
                case QtNodes::NodeWidgetType::InternalWidget: {
                        auto w = model->embeddedWidget();
                        result = QVariant::fromValue(w);
                }
                    break;
                case  QtNodes::NodeWidgetType::PortEditWidget: {
                        auto w = layoutWidget(nodeId);
                        result = QVariant::fromValue(w);
                }
                    break;
                default:
                    break;
                }

        } break;
        case NodeRole::PortEditable: {
            result = model->portEditable();
            break;
        }
        case NodeRole::PortEditWidget: {
            auto l = layoutWidget(nodeId);
            result = QVariant::fromValue(l);
            break;
        }
        case NodeRole::OSCAddress: {
            result = QVariant::fromValue(model->getOscMapping());
            // qDebug() << "getOscMapping" << model->getOscMapping().size();
            break;
        }

        case NodeRole::NodeID: {
            result = QVariant::fromValue(model->getNodeID());
            break;
        }
        case NodeRole::Remarks:{
            result = model->getRemarks();
            break;
        }
        case NodeRole::EmbeddWidgetType:
            result=static_cast<int>(model->getWidgetType());
            break;
        default:
            break;
    }

    return result;
}

NodeFlags CustomDataFlowGraphModel::nodeFlags(NodeId nodeId) const
{
    auto it = _models.find(nodeId);
    auto basicFlags = AbstractGraphModel::nodeFlags(nodeId);
    if (_nodesLocked) {
        basicFlags |= NodeFlag::Locked;

        return basicFlags;
    }
    if (it != _models.end() && it->second->widgetEmbeddable() && it->second->resizable())
        return NodeFlag::Resizable;

    return NodeFlag::NoFlags;
}

bool CustomDataFlowGraphModel::setNodeData(NodeId nodeId, NodeRole role, QVariant value)
{


    bool result = false;

    switch (role) {
        case NodeRole::Type:
            break;
        case NodeRole::Position: {
            _nodeGeometryData[nodeId].pos = value.value<QPointF>();
            Q_EMIT nodePositionUpdated(nodeId);
            result = true;
        } break;

        case NodeRole::Size: {
            _nodeGeometryData[nodeId].size = value.value<QSize>();
            result = true;
        } break;

        case NodeRole::CaptionVisible: {
            _models[nodeId]->CaptionVisible= value.value<bool>();
            Q_EMIT nodeUpdated(nodeId);
            result = true;
        }
            break;

        case NodeRole::Caption: {
            _models[nodeId]->Caption = value.value<QString>();
            Q_EMIT nodeUpdated(nodeId);
            result = true;
        }
            break;

        case NodeRole::Style:
            break;

        case NodeRole::InternalData:
            break;
        case NodeRole::PortEditable: {
            _models[nodeId]->PortEditable=value.toBool();
            Q_EMIT nodeUpdated(nodeId);
            result = true;
        }
            break;
        case NodeRole::InPortCount:{
            _models[nodeId]->InPortCount=value.toUInt();
            layoutWidget(nodeId)->populateButtons(PortType::In, value.toUInt());
//            更新端口设置中的布局
            Q_EMIT nodeUpdated(nodeId);
            result = true;
        }break;

        case NodeRole::OutPortCount:
        {
            _models[nodeId]->OutPortCount=value.toUInt();
            layoutWidget(nodeId)->populateButtons(PortType::Out, value.toUInt());
//            更新端口设置中的布局
            Q_EMIT nodeUpdated(nodeId);
            result = true;
        }
            break;

        case NodeRole::WidgetEmbeddable: {
            auto it = _models.find(nodeId);
            auto &model = it->second;
            model->WidgetEmbeddable=value.toBool();
            model->embeddedWidgetSizeUpdated();
            Q_EMIT nodeUpdated(nodeId);
            result = true;
        }
            break;
        case NodeRole::OSCAddress:
        {
            auto it = _models.find(nodeId);
            auto &model = it->second;
            model->getWidgetFromOSCAddress(value.toString());
            Q_EMIT nodeUpdated(nodeId);
            result = true;
        }
            break; 
        case NodeRole::Widget:
            break;
        case NodeRole::NodeID:
        { 
            auto it = _models.find(nodeId);
            auto &model = it->second;
            model->setNodeID(value.toInt());
            break;
        }
        case NodeRole::Remarks:{ 
            auto it = _models.find(nodeId);
            auto &model = it->second;
            model->setRemarks(value.toString());
            Q_EMIT nodeUpdated(nodeId);
            result = true;
        }
            break;
        case NodeRole::EmbeddWidgetType: {
            auto it = _models.find(nodeId);
            auto &model = it->second;
            model->setEmbeddWidgetType(static_cast<decltype(model->getWidgetType())>(value.toInt())); // 显式类型转换
            Q_EMIT nodeWidgetUpdated(nodeId);
            result=true;
        }
            break;
        case NodeRole::PortEditWidget:
            break;
        default:
            break;
    }

    return result;
}

QVariant CustomDataFlowGraphModel::portData(NodeId nodeId,
                                      PortType portType,
                                      PortIndex portIndex,
                                      PortRole role) const
{
    QVariant result;

    auto it = _models.find(nodeId);
    if (it == _models.end())
        return result;

    auto &model = it->second;

    switch (role) {
        case PortRole::Data:
            if (portType == PortType::Out)
                result = QVariant::fromValue(model->outData(portIndex));
            break;

        case PortRole::DataType:
            result = QVariant::fromValue(model->dataType(portType, portIndex));
            break;

        case PortRole::ConnectionPolicyRole:
            result = QVariant::fromValue(model->portConnectionPolicy(portType, portIndex));
            break;

        case PortRole::CaptionVisible:
            result = model->portCaptionVisible(portType, portIndex);
            break;

        case PortRole::Caption:
            result = model->portCaption(portType, portIndex);
            break;

        default:
            break;
    }

    return result;
}

bool CustomDataFlowGraphModel::setPortData(
        NodeId nodeId, PortType portType, PortIndex portIndex, QVariant const &value, PortRole role)
{
//    Q_UNUSED(nodeId);

    QVariant result;
//    qDebug()<<nodeId<<portType<<portIndex<<value<<role;
    auto it = _models.find(nodeId);
    if (it == _models.end())
    {
        return false;
    }


    auto &model = it->second;
    switch (role) {
        case PortRole::Data:
            if (portType == PortType::In) {
                model->setInData(value.value<std::shared_ptr<NodeData>>(), portIndex);

                // Triggers repainting on the scene.
                Q_EMIT inPortDataWasSet(nodeId, portType, portIndex);
            }
            break;

        default:
            break;
    }

    return false;
}

bool CustomDataFlowGraphModel::deleteConnection(ConnectionId const connectionId)
{



    bool disconnected = false;

    auto it = _connectivity.find(connectionId);

    if (it != _connectivity.end()&&_detachPossible) {
        disconnected = true;
//        qDebug()<<"delete connect:"+QString::number(connectionId.outNodeId)+" to "+QString::number(connectionId.inNodeId);
        _connectivity.erase(it);
    }

    if (disconnected) {
        sendConnectionDeletion(connectionId);

        propagateEmptyDataTo(getNodeId(PortType::In, connectionId),
                             getPortIndex(PortType::In, connectionId));
    }

    return disconnected;
}

bool CustomDataFlowGraphModel::deleteNode(NodeId const nodeId)
{
    // Delete connections to this node first.
    auto connectionIds = allConnectionIds(nodeId);
    for (auto &cId : connectionIds) {
        deleteConnection(cId);
    }

    _nodeGeometryData.erase(nodeId);
    _models.erase(nodeId);

    Q_EMIT nodeDeleted(nodeId);
//    qDebug()<<"delete node ID:"+QString::number(nodeId);
    return true;
}
void CustomDataFlowGraphModel::updateGroup(const GroupId oldGroupId, const GroupId newGroupId)
{
    // 鍒犻櫎鏃х粍
    if (auto it = _groups.find(oldGroupId); it != _groups.end()) {
        _groups.erase(it);
        _groups.insert(newGroupId);
        Q_EMIT groupUpdated(oldGroupId);
    }

}
bool CustomDataFlowGraphModel::deleteGroup(GroupId const groupId)
{
    auto it = _groups.find(groupId);
    if (it!= _groups.end()) {
        _groups.erase(it);
        Q_EMIT groupDeleted(groupId);
        return true;
    }
    return false;
}

QJsonObject CustomDataFlowGraphModel::saveNode(NodeId const nodeId) const
{
    QJsonObject nodeJson;

    nodeJson["id"] = static_cast<qint64>(nodeId);

    nodeJson["internal-data"] = _models.at(nodeId)->save();

    
    nodeJson["type"] = _models.at(nodeId)->type();
    // 保存备注
    nodeJson["remarks"] = _models.at(nodeId)->getRemarks();

    nodeJson["input-count"] = nodeData(nodeId, NodeRole::InPortCount).toInt();
    nodeJson["output-count"] = nodeData(nodeId, NodeRole::OutPortCount).toInt();
    nodeJson["port-editable"] = nodeData(nodeId, NodeRole::PortEditable).toBool();

    {
        QPointF const pos = nodeData(nodeId, NodeRole::Position).value<QPointF>();

        QJsonObject posJson;
        posJson["x"] = pos.x();
        posJson["y"] = pos.y();
        nodeJson["position"] = posJson;
        

    }

    return nodeJson;
}

QJsonObject CustomDataFlowGraphModel::save() const
{
    QJsonObject sceneJson;

    QJsonArray nodesJsonArray;
    for (auto const nodeId : allNodeIds()) {
        nodesJsonArray.append(saveNode(nodeId));
    }
    sceneJson["nodes"] = nodesJsonArray;

    QJsonArray connJsonArray;
    for (auto const &cid : _connectivity) {
        connJsonArray.append(toJson(cid));
    }
    sceneJson["connections"] = connJsonArray;
    QJsonArray groupJsonArray;
    for (auto const &gid : _groups) {
        groupJsonArray.append(groupToJson(gid));
    }
    sceneJson["groups"] = groupJsonArray;
    return sceneJson;
}

void CustomDataFlowGraphModel::loadNode(QJsonObject const &nodeJson)
{
    // Possibility of the id clash when reading it from json and not generating a
    // new value.
    // 1. When restoring a scene from a file.
    // Conflict is not possible because the scene must be cleared by the time of
    // loading.
    // 2. When undoing the deletion command.  Conflict is not possible
    // because all the new ids were created past the removed nodes.

    NodeId restoredNodeId = nodeJson["id"].toInt();

    _nextNodeId = std::max(_nextNodeId, restoredNodeId + 1);

    QJsonObject const internalDataJson = nodeJson["internal-data"].toObject();

    QString delegateModelName =nodeJson["type"].toString();

    std::unique_ptr<NodeDelegateModel> model = _registry->create(delegateModelName);

    if (model) {
        connect(model.get(),
                &NodeDelegateModel::dataUpdated,
                [restoredNodeId, this](PortIndex const portIndex) {
                    onOutPortDataUpdated(restoredNodeId, portIndex);
                });
        model->setNodeID(restoredNodeId);
        _models[restoredNodeId] = std::move(model);

        Q_EMIT nodeCreated(restoredNodeId);

        QJsonObject posJson = nodeJson["position"].toObject();
        QPointF const pos(posJson["x"].toDouble(), posJson["y"].toDouble());

        setNodeData(restoredNodeId, NodeRole::Position, pos);
        //      设置位置
        setNodeData(restoredNodeId, NodeRole::Remarks, nodeJson["remarks"].toString());
        setNodeData(restoredNodeId, NodeRole::PortEditable, nodeJson["port-editable"].toBool());
        setNodeData(restoredNodeId, NodeRole::InPortCount, nodeJson["input-count"].toInt());
        setNodeData(restoredNodeId, NodeRole::OutPortCount, nodeJson["output-count"].toInt());
        _models[restoredNodeId]->load(internalDataJson);
    } else {
        //创建失败，抛出异常
        throw std::logic_error(std::string("No registered model with name ") +
                               delegateModelName.toLocal8Bit().data());
    }

}

void CustomDataFlowGraphModel::load(QJsonObject const &jsonDocument)
{
    // 清空现有数据
    auto existingNodes = allNodeIds();
    for (auto nodeId : existingNodes) {
        deleteNode(nodeId);
    }
    _groups.clear();
    _connectivity.clear();

    try { // 节点加载阶段
        QJsonArray nodesJsonArray = jsonDocument["nodes"].toArray();
        for (QJsonValueRef nodeJson : nodesJsonArray) {
            loadNode(nodeJson.toObject());
        }
    } catch (const std::exception& e) {
        qCritical() << tr("节点加载失败:\n%1").arg(e.what());
        return;
    }

    try { // 连接加载阶段
        QJsonArray connectionJsonArray = jsonDocument["connections"].toArray();
        for (QJsonValueRef connection : connectionJsonArray) {
            QJsonObject connJson = connection.toObject();
            ConnectionId connId = fromJson(connJson);
            addConnection(connId);
        }
    } catch (const std::exception& e) {
        qCritical() << tr("连接加载失败:\n%1").arg(e.what());
        return;
    }

    try { // 分组加载阶段
        std::vector<GroupId> allGroups;
        QJsonArray groupJsonArray = jsonDocument["groups"].toArray();
        for (QJsonValueRef group : groupJsonArray) {
            QJsonObject groupJson = group.toObject();
            GroupId groupId = QtNodes::fromJsonToGroup(groupJson);
            allGroups.push_back(groupId);
        }

        std::sort(allGroups.begin(), allGroups.end(),
            [](const GroupId& a, const GroupId& b) {
                return a.nodeIds.size() > b.nodeIds.size();
            });

        for (const auto& groupId : allGroups) {
            addGroup(groupId);
        }
    } catch (const std::exception& e) {
         qWarning() << tr("分组加载失败:\n%1").arg(e.what());

    }
}

void CustomDataFlowGraphModel::onOutPortDataUpdated(NodeId const nodeId, PortIndex const portIndex)
{
    std::unordered_set<ConnectionId> const &connected = connections(nodeId,
                                                                    PortType::Out,
                                                                    portIndex);

    QVariant const portDataToPropagate = portData(nodeId, PortType::Out, portIndex, PortRole::Data);

    for (auto const &cn : connected) {
        setPortData(cn.inNodeId, PortType::In, cn.inPortIndex, portDataToPropagate, PortRole::Data);
    }
}

void CustomDataFlowGraphModel::propagateEmptyDataTo(NodeId const nodeId, PortIndex const portIndex)
{
    QVariant emptyData{};

    setPortData(nodeId, PortType::In, portIndex, emptyData, PortRole::Data);
}

bool CustomDataFlowGraphModel::detachPossible(ConnectionId const) const  { return _detachPossible; }

void CustomDataFlowGraphModel::setDetachPossible(bool d) {
    _detachPossible = d;
//    qDebug()<<"set scene connect DetachPossible: "+QString::number(d);
}

void CustomDataFlowGraphModel::setNodesLocked(bool b)
{
    _nodesLocked = b;

    for (NodeId nodeId : allNodeIds()) {
        setNodeData(nodeId,NodeRole::WidgetEmbeddable,false);
        Q_EMIT nodeFlagsUpdated(nodeId);
    }
    //锁定节点后也需锁定分组
    for(GroupId groupId : allGroupIds()){
        Q_EMIT groupFlagsUpdated(groupId);
    }
}

bool CustomDataFlowGraphModel::getNodesLocked() const
{
    return _nodesLocked;
}

void CustomDataFlowGraphModel::addPort(NodeId nodeId, PortType portType, PortIndex portIndex)
{
    // STAGE 1.
    // Compute new addresses for the existing connections that are shifted and
    // placed after the new ones
    PortIndex first = portIndex;
    PortIndex last = first;
    portsAboutToBeInserted(nodeId, portType, first, last);

//    // STAGE 2. Change the number of connections in your model
//
    if (portType == PortType::In)
        _models[nodeId]->InPortCount++;
    else
        _models[nodeId]->OutPortCount++;
//
//    // STAGE 3. Re-create previouly existed and now shifted connections
    portsInserted();
//    qDebug()<<2;
    Q_EMIT nodeUpdated(nodeId);
}

void CustomDataFlowGraphModel::removePort(NodeId nodeId, PortType portType, PortIndex portIndex)
{
    // STAGE 1.
    // Compute new addresses for the existing connections that are shifted upwards
    // instead of the deleted ports.
    PortIndex first = portIndex;
    PortIndex last = first;
    portsAboutToBeDeleted(nodeId, portType, first, last);

    // STAGE 2. Change the number of connections in your model
    if (portType == PortType::In) {
        _models[nodeId]->InPortCount--;
    }
    else {
        _models[nodeId]->OutPortCount--;
    }

    portsDeleted();
    Q_EMIT nodeUpdated(nodeId);
}

PortEditAddRemoveWidget *CustomDataFlowGraphModel::layoutWidget(NodeId nodeId) const
{
    auto it = _nodeWidgets.find(nodeId);

    if (it == _nodeWidgets.end()) {
        _nodeWidgets[nodeId] = new PortEditAddRemoveWidget(
                                                       nodeId,
                                                       *const_cast<CustomDataFlowGraphModel *>(this));
    }

    return _nodeWidgets[nodeId];
}
