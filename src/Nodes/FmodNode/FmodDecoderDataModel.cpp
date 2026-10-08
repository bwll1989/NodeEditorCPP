/**
 * @file FmodDecoderDataModel.cpp
 * @brief FmodDecoderDataModel 实现
 */

#include "FmodDecoderDataModel.hpp"

#include <QDebug>
#include <QFileDialog>
#include <QHash>
#include <QJsonObject>
#include <QLineEdit>
#include <QPushButton>
#include <QSignalBlocker>

namespace Nodes {

FmodDecoderDataModel::FmodDecoderDataModel()
{
    qRegisterMetaType<std::vector<std::shared_ptr<AudioTimestampRingQueue>>>(
        "std::vector<std::shared_ptr<AudioTimestampRingQueue>>");
    qRegisterMetaType<Nodes::FmodParamDesc>("Nodes::FmodParamDesc");
    qRegisterMetaType<QVector<Nodes::FmodParamDesc>>("QVector<Nodes::FmodParamDesc>");

    InPortCount = 0;
    OutPortCount = 12;
    CaptionVisible = true;
    Caption = QStringLiteral("Fmod Node");
    WidgetEmbeddable = false;
    Resizable = true;
    PortEditable = false;

    widget_ = new FmodDecoderInterface();

    connect(widget_->fileSelectComboBox, &QLineEdit::textChanged,
            this, &FmodDecoderDataModel::setBankPath);
    connect(widget_->selectButton, &QPushButton::clicked,
            this, &FmodDecoderDataModel::selectBankFolder, Qt::QueuedConnection);

    connect(widget_, &FmodDecoderInterface::parameterChanged,
            this, &FmodDecoderDataModel::onParameterChanged);
    connect(widget_, &FmodDecoderInterface::eventTriggered,
            this, &FmodDecoderDataModel::playEventPath);

    outputBuffers_.resize(OutPortCount);
    for (int i = 0; i < OutPortCount; ++i) {
        outputBuffers_[i] = std::make_shared<AudioTimestampRingQueue>();
    }

    worker_ = new FmodDecoderWorker();
    workerThread_ = new QThread(this);
    worker_->moveToThread(workerThread_);

    connect(workerThread_, &QThread::started, worker_, &FmodDecoderWorker::startProcessing);
    connect(workerThread_, &QThread::finished, worker_, &FmodDecoderWorker::stopProcessing);
    connect(worker_, &FmodDecoderWorker::eventCatalogUpdated,
            this, &FmodDecoderDataModel::onEventCatalogUpdated);
    connect(worker_, &FmodDecoderWorker::errorOccurred, this, [](const QString& msg) {
        qWarning() << "FmodDecoderWorker Error:" << msg;
    });

    workerThread_->start();
    QMetaObject::invokeMethod(
        worker_,
        "initialize",
        Qt::QueuedConnection,
        Q_ARG(std::vector<std::shared_ptr<AudioTimestampRingQueue>>, outputBuffers_));
}

FmodDecoderDataModel::~FmodDecoderDataModel()
{
    if (workerThread_ && workerThread_->isRunning()) {
        workerThread_->quit();
        workerThread_->wait();
    }
    delete worker_;
    worker_ = nullptr;
}

NodeDataType FmodDecoderDataModel::dataType(PortType portType, PortIndex portIndex) const
{
    Q_UNUSED(portIndex)
    switch (portType) {
    case PortType::In:
        return VariableData().type();
    case PortType::Out:
        return AudioData().type();
    case PortType::None:
        break;
    }
    return VariableData().type();
}

std::shared_ptr<NodeData> FmodDecoderDataModel::outData(PortIndex port)
{
    auto audioData = std::make_shared<AudioData>();
    if (port >= 0 && static_cast<size_t>(port) < outputBuffers_.size()) {
        audioData->setSharedAudioBuffer(outputBuffers_[static_cast<size_t>(port)]);
    }
    return audioData;
}

QJsonObject FmodDecoderDataModel::save() const
{
    QJsonObject modelJson = NodeDelegateModel::save();
    if (!bankPath_.isEmpty()) {
        modelJson.insert(QStringLiteral("path"), bankPath_);
    }

    QJsonObject params;
    for (auto it = paramValues_.cbegin(); it != paramValues_.cend(); ++it) {
        params.insert(it.key(), QJsonValue::fromVariant(it.value()));
    }
    if (!params.isEmpty()) {
        modelJson.insert(QStringLiteral("params"), params);
    }
    return modelJson;
}

void FmodDecoderDataModel::load(QJsonObject const& p)
{
    AbstractDelegateModel::load(p);

    const QJsonObject params = p.value(QStringLiteral("params")).toObject();
    for (auto it = params.begin(); it != params.end(); ++it) {
        paramValues_.insert(it.key(), it.value().toVariant());
    }

    QString path = p.value(QStringLiteral("path")).toString();
    if (path.isEmpty() && widget_ && widget_->fileSelectComboBox) {
        path = widget_->fileSelectComboBox->text();
    }
    if (!path.isEmpty()) {
        setBankPath(path);
    }
}

QString FmodDecoderDataModel::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    if (portType == QtNodes::PortType::Out) {
        return QStringLiteral("Out %1").arg(portIndex + 1);
    }
    if (portType == QtNodes::PortType::In
        && portIndex >= 0
        && portIndex < inPorts_.size()) {
        return inPorts_.at(portIndex).caption;
    }
    return QStringLiteral("In %1").arg(portIndex + 1);
}

void FmodDecoderDataModel::selectBankFolder()
{
    const QString path = QFileDialog::getExistingDirectory(
        nullptr, QStringLiteral("Select FMOD Bank Folder"), QString());
    if (!path.isEmpty()) {
        setBankPath(path);
    }
}

void FmodDecoderDataModel::setBankPath(const QString& path)
{
    const QString trimmed = path.trimmed();
    if (trimmed == bankPath_) {
        return;
    }
    bankPath_ = trimmed;

    if (widget_ && widget_->fileSelectComboBox) {
        QSignalBlocker blocker(widget_->fileSelectComboBox);
        widget_->fileSelectComboBox->setText(bankPath_);
    }

    if (!bankPath_.isEmpty() && worker_) {
        QMetaObject::invokeMethod(
            worker_,
            "loadBanks",
            Qt::QueuedConnection,
            Q_ARG(QString, bankPath_));
    }
}

void FmodDecoderDataModel::onEventCatalogUpdated(const QStringList& events,
                                                 const QVector<Nodes::FmodParamDesc>& params)
{
    availableEvents_ = events;
    availableParams_ = params;

    for (const FmodParamDesc& p : availableParams_) {
        const QString key = FmodDecoderInterface::makeKey(p.eventPath, p.paramName);
        if (!paramValues_.contains(key)) {
            paramValues_.insert(key, p.defaultValue);
        }
    }

    rebuildInPorts();
    syncInputPortCount();
    if (widget_) {
        widget_->rebuildCatalog(availableEvents_, availableParams_, paramValues_);
    }

    if (worker_) {
        QHash<QString, QVariantMap> byEvent;
        for (const FmodParamDesc& p : availableParams_) {
            const QString key = FmodDecoderInterface::makeKey(p.eventPath, p.paramName);
            byEvent[p.eventPath].insert(p.paramName, paramValues_.value(key, p.defaultValue));
        }
        for (auto it = byEvent.begin(); it != byEvent.end(); ++it) {
            QMetaObject::invokeMethod(
                worker_,
                "setEventParameterMap",
                Qt::QueuedConnection,
                Q_ARG(QString, it.key()),
                Q_ARG(QVariantMap, it.value()));
        }
    }

    Q_EMIT embeddedWidgetSizeUpdated();
}

void FmodDecoderDataModel::rebuildInPorts()
{
    inPorts_.clear();

    QHash<QString, QVector<FmodParamDesc>> paramsByEvent;
    for (const FmodParamDesc& p : availableParams_) {
        paramsByEvent[p.eventPath].append(p);
    }

    for (const QString& eventPath : availableEvents_) {
        InPortDesc trigger;
        trigger.kind = InPortKind::Trigger;
        trigger.eventPath = eventPath;
        trigger.caption = FmodDecoderInterface::eventDisplayName(eventPath);
        inPorts_.append(trigger);

        for (const FmodParamDesc& p : paramsByEvent.value(eventPath)) {
            InPortDesc param;
            param.kind = InPortKind::Parameter;
            param.eventPath = p.eventPath;
            param.paramName = p.paramName;
            param.caption = p.caption;
            inPorts_.append(param);
        }
    }
}

void FmodDecoderDataModel::syncInputPortCount()
{
    const unsigned int newCount = static_cast<unsigned int>(inPorts_.size());
    const unsigned int oldCount = InPortCount;
    if (newCount == oldCount) {
        return;
    }

    if (newCount > oldCount) {
        Q_EMIT portsAboutToBeInserted(PortType::In, oldCount, newCount - 1);
        InPortCount = newCount;
        Q_EMIT portsInserted();
    } else {
        Q_EMIT portsAboutToBeDeleted(PortType::In, newCount, oldCount - 1);
        InPortCount = newCount;
        Q_EMIT portsDeleted();
    }
}

void FmodDecoderDataModel::onParameterChanged(const QString& eventPath,
                                              const QString& paramName,
                                              float value)
{
    paramValues_.insert(FmodDecoderInterface::makeKey(eventPath, paramName), value);
    if (!worker_) {
        return;
    }
    QMetaObject::invokeMethod(
        worker_,
        "setEventParameter",
        Qt::QueuedConnection,
        Q_ARG(QString, eventPath),
        Q_ARG(QString, paramName),
        Q_ARG(float, value));
}

void FmodDecoderDataModel::playEventPath(const QString& eventPath)
{
    if (eventPath.isEmpty() || !worker_) {
        return;
    }
    QMetaObject::invokeMethod(
        worker_,
        "playEvent",
        Qt::QueuedConnection,
        Q_ARG(QString, eventPath));
}

void FmodDecoderDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex)
{
    if (!data) {
        return;
    }
    const auto variableData = std::dynamic_pointer_cast<VariableData>(data);
    if (!variableData) {
        return;
    }
    if (portIndex < 0 || portIndex >= inPorts_.size()) {
        return;
    }

    const InPortDesc& port = inPorts_.at(portIndex);
    if (port.kind == InPortKind::Trigger) {
        if (variableData->asBool()) {
            playEventPath(port.eventPath);
        }
        return;
    }

    const float value = static_cast<float>(variableData->asNumber());
    onParameterChanged(port.eventPath, port.paramName, value);
    if (widget_) {
        widget_->setParameterValue(port.eventPath, port.paramName, value);
    }
}

} // namespace Nodes
