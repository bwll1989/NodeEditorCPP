#include "ContainerDataModel.hpp"

#include "VariableInDataModel.hpp"
#include "VariableOutDataModel.hpp"
#include "ImageInDataModel.hpp"
#include "ImageOutDataModel.hpp"
#include "AudioInDataModel.hpp"
#include "AudioOutDataModel.hpp"

#include "Widget/NodeWidget/CustomDataFlowGraphModel.h"
#include "Common/DataTypes/NodeDataList.hpp"

#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>
#include <QtCore/QDebug>
#include <QtCore/QDir>
#include <QtCore/QFile>
#include <QtCore/QHash>
#include <QtCore/QPointF>
#include <QtCore/QPointer>
#include <QtCore/QRegularExpression>
#include <QtCore/QTimer>

#include <QtWidgets/QFileDialog>
#include <QtWidgets/QMessageBox>
#include <QtWidgets/QWidget>

#include <QtNodes/Definitions>
#include <QtNodes/AbstractGraphModel>

#include <algorithm>

namespace Nodes {
namespace {

struct SyncGuard {
    bool &flag;
    explicit SyncGuard(bool &f)
        : flag(f)
    {
        flag = true;
    }
    ~SyncGuard() { flag = false; }
};

QString firstLineRemarks(NodeDelegateModel const *model, QString const &fallback)
{
    if (!model)
        return fallback;
    QString base = model->getRemarks().trimmed();
    if (base.isEmpty())
        base = fallback;
    return base.section(QLatin1Char('\n'), 0, 0).trimmed();
}

} // namespace

std::shared_ptr<NodeDelegateModelRegistry> ContainerDataModel::s_registry;

QString ContainerDataModel::portLabelFromRemarks(NodeDelegateModel const *model,
                                                  QString const &fallback,
                                                  PortIndex localPort,
                                                  bool disambiguate,
                                                  NodeId nodeId)
{
    QString base = firstLineRemarks(model, fallback);
    if (disambiguate)
        return QStringLiteral("%1#%2.%3").arg(base).arg(nodeId).arg(localPort);
    return QStringLiteral("%1.%2").arg(base).arg(localPort);
}

bool ContainerDataModel::isInterfaceIn(NodeDelegateModel const *model)
{
    return dynamic_cast<VariableInDataModel const *>(model)
           || dynamic_cast<ImageInDataModel const *>(model)
           || dynamic_cast<AudioInDataModel const *>(model);
}

bool ContainerDataModel::isInterfaceOut(NodeDelegateModel const *model)
{
    return dynamic_cast<VariableOutDataModel const *>(model)
           || dynamic_cast<ImageOutDataModel const *>(model)
           || dynamic_cast<AudioOutDataModel const *>(model);
}

ContainerDataModel::ContainerDataModel()
{
    Caption = QStringLiteral("Container");
    CaptionVisible = true;
    WidgetEmbeddable = false;
    Resizable = false;
    PortEditable = false;
    InPortCount = 0;
    OutPortCount = 0;
    setRemarks(QStringLiteral("Container"));
}

ContainerDataModel::~ContainerDataModel()
{
    _suspendInterfaceSync = true;
    unbindAllExports();
    for (auto const &c : _innerModelConnections)
        QObject::disconnect(c);
    _innerModelConnections.clear();
    _innerSignalsConnected = false;
    _inMaps.clear();
    _outMaps.clear();
    _outCache.clear();
    // 显式先销毁子图，避免析构顺序中仍有信号回入本对象
    _innerModel.reset();
}

void ContainerDataModel::setSharedRegistry(std::shared_ptr<NodeDelegateModelRegistry> registry)
{
    s_registry = std::move(registry);
}

void ContainerDataModel::afterModelReady()
{
    // 延迟到事件循环，确保 GraphModel 已接好 portsInserted 等信号。
    // QPointer：清空/加载工程时节点可能在排队期间已被销毁。
    QPointer<ContainerDataModel> self(this);
    QTimer::singleShot(0, this, [self]() {
        if (!self)
            return;
        // load() 已写入 inner-scene 时不要再 seed
        if (self->_innerModel && !self->_innerModel->allNodeIds().empty()) {
            self->refreshInnerModelAlias();
            return;
        }
        self->ensureInnerModel();
        self->seedDefaultInterfaceNodes();
        self->refreshInnerModelAlias();
    });
}

void ContainerDataModel::refreshInnerModelAlias()
{
    if (!_innerModel)
        return;

    if (getNodeID() == QtNodes::InvalidNodeId)
        return;

    QString const parent = getParentAlias().trimmed();
    QString alias;
    if (parent.isEmpty())
        alias = QString::number(getNodeID());
    else
        alias = parent + QLatin1Char('/') + QString::number(getNodeID());

    _innerModel->setModelAlias(alias);

    for (NodeId id : _innerModel->allNodeIds()) {
        if (auto *m = _innerModel->delegateModel<NodeDelegateModel>(id)) {
            m->setParentAlias(alias);
            if (auto *child = dynamic_cast<ContainerDataModel *>(m))
                child->refreshInnerModelAlias();
        }
    }
}

CustomDataFlowGraphModel &ContainerDataModel::ensureInnerModel()
{
    if (!_innerModel) {
        auto registry = s_registry;
        if (!registry)
            registry = std::make_shared<NodeDelegateModelRegistry>();
        _innerModel = std::make_unique<CustomDataFlowGraphModel>(std::move(registry));
    }
    refreshInnerModelAlias();
    connectInnerModelSignals();
    return *_innerModel;
}

void ContainerDataModel::connectInnerModelSignals()
{
    if (!_innerModel || _innerSignalsConnected)
        return;

    auto resync = [this](NodeId) { syncInterfaceFromInner(); };

    _innerModelConnections.push_back(
        connect(_innerModel.get(), &QtNodes::AbstractGraphModel::nodeCreated, this, resync));
    _innerModelConnections.push_back(
        connect(_innerModel.get(), &QtNodes::AbstractGraphModel::nodeDeleted, this, resync));
    _innerModelConnections.push_back(
        connect(_innerModel.get(),
                &QtNodes::AbstractGraphModel::nodeUpdated,
                this,
                [this](NodeId id) {
                    if (!_innerModel)
                        return;
                    auto *m = _innerModel->delegateModel<NodeDelegateModel>(id);
                    if (isInterfaceIn(m) || isInterfaceOut(m))
                        syncInterfaceFromInner();
                }));

    _innerSignalsConnected = true;
}

void ContainerDataModel::seedDefaultInterfaceNodes()
{
    auto &model = ensureInnerModel();
    if (!model.allNodeIds().empty())
        return;

    NodeId const inId = model.addNode(QStringLiteral("Variable In"));
    NodeId const outId = model.addNode(QStringLiteral("Variable Out"));

    model.setNodeData(inId, QtNodes::NodeRole::Position, QPointF(50, 120));
    model.setNodeData(outId, QtNodes::NodeRole::Position, QPointF(420, 120));

    syncInterfaceFromInner();
}

std::vector<std::pair<NodeId, NodeDelegateModel *>> ContainerDataModel::sortedInNodes() const
{
    std::vector<std::pair<NodeId, NodeDelegateModel *>> result;
    if (!_innerModel)
        return result;
    for (NodeId id : _innerModel->allNodeIds()) {
        if (auto *m = _innerModel->delegateModel<NodeDelegateModel>(id)) {
            if (isInterfaceIn(m))
                result.emplace_back(id, m);
        }
    }
    std::sort(result.begin(), result.end(),
              [](auto const &a, auto const &b) { return a.first < b.first; });
    return result;
}

std::vector<std::pair<NodeId, NodeDelegateModel *>> ContainerDataModel::sortedOutNodes() const
{
    std::vector<std::pair<NodeId, NodeDelegateModel *>> result;
    if (!_innerModel)
        return result;
    for (NodeId id : _innerModel->allNodeIds()) {
        if (auto *m = _innerModel->delegateModel<NodeDelegateModel>(id)) {
            if (isInterfaceOut(m))
                result.emplace_back(id, m);
        }
    }
    std::sort(result.begin(), result.end(),
              [](auto const &a, auto const &b) { return a.first < b.first; });
    return result;
}

void ContainerDataModel::unbindAllExports()
{
    for (auto const &c : _exportConnections)
        QObject::disconnect(c);
    _exportConnections.clear();
}

void ContainerDataModel::bindExportRelay(NodeDelegateModel *exportModel, PortIndex globalBase)
{
    if (!exportModel)
        return;

    _exportConnections.push_back(connect(
        exportModel,
        &NodeDelegateModel::dataUpdated,
        this,
        [this, exportModel, globalBase](PortIndex localPort) {
            PortIndex const globalPort = globalBase + localPort;
            _outCache[globalPort] = exportModel->outData(localPort);
            Q_EMIT dataUpdated(globalPort);
        }));
}

void ContainerDataModel::flushExportOutputs()
{
    for (PortIndex i = 0; i < static_cast<PortIndex>(_outMaps.size()); ++i) {
        PortMap const &m = _outMaps[i];
        if (!m.model)
            continue;
        _outCache[i] = m.model->outData(m.localPort);
        Q_EMIT dataUpdated(i);
    }
}

void ContainerDataModel::applyPortCountChange(PortType portType,
                                               unsigned int oldCount,
                                               unsigned int newCount)
{
    if (newCount == oldCount)
        return;

    if (newCount > oldCount) {
        Q_EMIT portsAboutToBeInserted(portType, oldCount, newCount - 1);
        if (portType == PortType::In)
            InPortCount = newCount;
        else
            OutPortCount = newCount;
        Q_EMIT portsInserted();
    } else {
        Q_EMIT portsAboutToBeDeleted(portType, newCount, oldCount - 1);
        if (portType == PortType::In)
            InPortCount = newCount;
        else
            OutPortCount = newCount;
        Q_EMIT portsDeleted();
    }
}

void ContainerDataModel::syncInterfaceFromInner()
{
    if (!_innerModel)
        return;

    if (_suspendInterfaceSync)
        return;

    // 必须用实例标志：嵌套 Container 时 static/thread_local 会挡住内层 sync
    if (_isSyncing)
        return;
    SyncGuard guard(_isSyncing);

    std::vector<PortMap> inMaps;
    std::vector<PortMap> outMaps;

    auto const inNodes = sortedInNodes();
    auto const outNodes = sortedOutNodes();

    QHash<QString, int> inRemarkCount;
    QHash<QString, int> outRemarkCount;
    for (auto const &[id, model] : inNodes) {
        Q_UNUSED(id);
        inRemarkCount[firstLineRemarks(model, QStringLiteral("In"))]++;
    }
    for (auto const &[id, model] : outNodes) {
        Q_UNUSED(id);
        outRemarkCount[firstLineRemarks(model, QStringLiteral("Out"))]++;
    }

    for (auto const &[id, model] : inNodes) {
        QString const base = firstLineRemarks(model, QStringLiteral("In"));
        bool const clash = inRemarkCount.value(base) > 1;
        unsigned const n = model->nPorts(PortType::Out);
        for (PortIndex p = 0; p < n; ++p) {
            PortMap m;
            m.model = model;
            m.localPort = p;
            m.label = portLabelFromRemarks(model, QStringLiteral("In"), p, clash, id);
            inMaps.push_back(m);
        }
    }

    PortIndex outBase = 0;
    struct ExportBind {
        NodeDelegateModel *model = nullptr;
        PortIndex base = 0;
    };
    std::vector<ExportBind> binds;

    for (auto const &[id, model] : outNodes) {
        QString const base = firstLineRemarks(model, QStringLiteral("Out"));
        bool const clash = outRemarkCount.value(base) > 1;
        binds.push_back({model, outBase});
        unsigned const n = model->nPorts(PortType::In);
        for (PortIndex p = 0; p < n; ++p) {
            PortMap m;
            m.model = model;
            m.localPort = p;
            m.label = portLabelFromRemarks(model, QStringLiteral("Out"), p, clash, id);
            outMaps.push_back(m);
        }
        outBase = static_cast<PortIndex>(outMaps.size());
    }

    unsigned const newIn = static_cast<unsigned>(inMaps.size());
    unsigned const newOut = static_cast<unsigned>(outMaps.size());
    unsigned const oldIn = InPortCount;
    unsigned const oldOut = OutPortCount;

    _inMaps = std::move(inMaps);
    _outMaps = std::move(outMaps);

    applyPortCountChange(PortType::In, oldIn, newIn);
    applyPortCountChange(PortType::Out, oldOut, newOut);

    if (newIn == oldIn && newOut == oldOut)
        Q_EMIT embeddedWidgetSizeUpdated();

    unbindAllExports();
    for (auto const &b : binds)
        bindExportRelay(b.model, b.base);

    for (auto it = _outCache.begin(); it != _outCache.end();) {
        if (it->first >= newOut)
            it = _outCache.erase(it);
        else
            ++it;
    }

    // load 期间子图内部已算完当前值，但重绑中继时错过了 dataUpdated；这里主动刷出
    flushExportOutputs();
}

QString ContainerDataModel::portCaption(PortType portType, PortIndex portIndex) const
{
    if (portType == PortType::In) {
        if (portIndex < _inMaps.size())
            return _inMaps[portIndex].label;
    } else if (portType == PortType::Out) {
        if (portIndex < _outMaps.size())
            return _outMaps[portIndex].label;
    }
    return NodeDelegateModel::portCaption(portType, portIndex);
}

QJsonObject ContainerDataModel::save() const
{
    QJsonObject modelJson = NodeDelegateModel::save();
    if (_innerModel)
        modelJson[QStringLiteral("inner-scene")] = _innerModel->save();
    else
        modelJson[QStringLiteral("inner-scene")] = QJsonObject{};
    return modelJson;
}

void ContainerDataModel::load(QJsonObject const &p)
{
    NodeDelegateModel::load(p);

    QJsonObject inner = p.value(QStringLiteral("inner-scene")).toObject();
    auto &model = ensureInnerModel();

    // 整图重建会连续触发 nodeDeleted/nodeCreated；若每次都 sync，外壳端口会先缩到 0
    // 再扩张，外层连到 Container 的连线会被 portsAboutToBeDeleted 清掉。
    _suspendInterfaceSync = true;
    if (!inner.isEmpty()) {
        try {
            model.load(inner);
        } catch (std::exception const &e) {
            qWarning() << "Container inner load failed:" << e.what();
        }
    } else {
        seedDefaultInterfaceNodes();
    }
    _suspendInterfaceSync = false;

    refreshInnerModelAlias();
    syncInterfaceFromInner();
}

bool ContainerDataModel::exportChildFlow(QWidget *parent) const
{
    if (!_innerModel)
        return false;

    QString baseName = firstLineRemarks(this, QStringLiteral("Container"));
    // 文件名去掉路径非法字符
    baseName.replace(QRegularExpression(QStringLiteral(R"([\\/:*?"<>|])")), QStringLiteral("_"));
    if (baseName.isEmpty())
        baseName = QStringLiteral("Container");

    QString fileName = QFileDialog::getSaveFileName(
        parent,
        tr("Export Child Flow"),
        QDir::homePath() + QLatin1Char('/') + baseName + QStringLiteral(".childflow"),
        tr("Flow Scene Files (*.childflow)"));

    if (fileName.isEmpty())
        return false;

    if (!fileName.endsWith(QStringLiteral("childflow"), Qt::CaseInsensitive))
        fileName += QStringLiteral(".childflow");

    QFile file(fileName);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::warning(parent,
                             tr("导出失败"),
                             tr("无法写入文件 %1:\n%2").arg(fileName, file.errorString()));
        return false;
    }

    file.write(QJsonDocument(_innerModel->save()).toJson());
    return true;
}

unsigned int ContainerDataModel::nPorts(PortType portType) const
{
    switch (portType) {
    case PortType::In:
        return InPortCount;
    case PortType::Out:
        return OutPortCount;
    default:
        return 0;
    }
}

NodeDataType ContainerDataModel::dataType(PortType portType, PortIndex portIndex) const
{
    if (portType == PortType::In) {
        if (portIndex < _inMaps.size() && _inMaps[portIndex].model) {
            auto const &m = _inMaps[portIndex];
            return m.model->dataType(PortType::Out, m.localPort);
        }
    } else if (portType == PortType::Out) {
        if (portIndex < _outMaps.size() && _outMaps[portIndex].model) {
            auto const &m = _outMaps[portIndex];
            return m.model->dataType(PortType::In, m.localPort);
        }
    }
    return NodeDataTypes::VariableData().type();
}

std::shared_ptr<NodeData> ContainerDataModel::outData(PortIndex port)
{
    auto it = _outCache.find(port);
    if (it != _outCache.end())
        return it->second;
    return nullptr;
}

void ContainerDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex portIndex)
{
    if (portIndex >= _inMaps.size())
        return;
    PortMap const &m = _inMaps[portIndex];
    if (m.model)
        m.model->setInData(data, m.localPort);
}

QWidget *ContainerDataModel::embeddedWidget()
{
    return nullptr;
}

} // namespace Nodes
