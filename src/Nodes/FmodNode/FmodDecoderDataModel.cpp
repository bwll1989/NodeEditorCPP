#include "FmodDecoderDataModel.hpp"
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <set>
#include "TimestampGenerator/TimestampGenerator.hpp"

namespace Nodes {

FmodDecoderDataModel::FmodDecoderDataModel()
{
    qRegisterMetaType<std::vector<std::shared_ptr<AudioTimestampRingQueue>>>("std::vector<std::shared_ptr<AudioTimestampRingQueue>>");

    InPortCount = 2;
    OutPortCount = 12;
    CaptionVisible = true;
    Caption = "Fmod Node";
    WidgetEmbeddable = false;
    Resizable = false;
    PortEditable = false;
    
    // Create widget
    widget = new FmodDecoderInterface();
    
    // Initialize buffers
    outputBuffers_.resize(OutPortCount);
    for (int i = 0; i < OutPortCount; ++i) {
        outputBuffers_[i] = std::make_shared<AudioTimestampRingQueue>();
    }
    
    // Setup Worker Thread
    worker_ = new FmodDecoderWorker();
    workerThread_ = new QThread(this);
    worker_->moveToThread(workerThread_);
    
    // Connect Signals
    connect(workerThread_, &QThread::started, worker_, &FmodDecoderWorker::startProcessing);
    connect(workerThread_, &QThread::finished, worker_, &FmodDecoderWorker::stopProcessing);
    
    connect(worker_, &FmodDecoderWorker::eventListUpdated, this, &FmodDecoderDataModel::updateEventListUI);
    connect(worker_, &FmodDecoderWorker::errorOccurred, this, [](const QString& msg){
        qWarning() << "FmodDecoderWorker Error:" << msg;
    });
    
    // UI Connections
    connect(widget->selectButton, &QPushButton::clicked, this, &FmodDecoderDataModel::select_audio_file, Qt::QueuedConnection);
    
    // OSC
    AbstractDelegateModel::registerOSCControl("/select", widget->selectButton);
    AbstractDelegateModel::registerOSCControl("/file", widget->fileSelectComboBox);
    
    // Start Thread
    workerThread_->start();
    
    // Initialize Worker with buffers
    QMetaObject::invokeMethod(worker_, "initialize", Qt::QueuedConnection, Q_ARG(std::vector<std::shared_ptr<AudioTimestampRingQueue>>, outputBuffers_));
}

FmodDecoderDataModel::~FmodDecoderDataModel()
{
    if (workerThread_ && workerThread_->isRunning()) {
        workerThread_->quit();
        workerThread_->wait();
    }
    if (worker_) {
        delete worker_;
    }
}

NodeDataType FmodDecoderDataModel::dataType(PortType portType, PortIndex portIndex) const
{
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
    if (port < outputBuffers_.size()) {
        audioData->setSharedAudioBuffer(outputBuffers_[port]);
    }
    return audioData;
}

/**
 * @brief 保存节点状态（当前选中的 FMOD Bank 路径）
 */
QJsonObject FmodDecoderDataModel::save() const
{
    QJsonObject modelJson = NodeDelegateModel::save();
    QString pathText = widget && widget->fileSelectComboBox ? widget->fileSelectComboBox->text() : QString();
    if (!pathText.isEmpty()) {
        modelJson["path"] = pathText;
    }
    return modelJson;
}

QString FmodDecoderDataModel::portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const
{
    if (portType == QtNodes::PortType::Out) {
        return QString("Out %1").arg(portIndex + 1);
    }
    if (portType == QtNodes::PortType::In) {
        switch (portIndex) {
            case 0:
                return QString("Event");
            case 1:
                return QString("Index");
        }
    }
    return QString("In %1").arg(portIndex + 1);
}

void FmodDecoderDataModel::select_audio_file()
{
    QString path = QFileDialog::getExistingDirectory(nullptr, "Select FMOD Bank Folder", "");
    if (!path.isEmpty()) {
        widget->fileSelectComboBox->setText(path);
        // Call worker to load banks
        QMetaObject::invokeMethod(worker_, "loadBanks", Qt::QueuedConnection, Q_ARG(QString, path));
    }
}

void FmodDecoderDataModel::updateEventListUI(const QStringList& events)
{
    // 更新内部事件列表缓存
    availableEvents_ = events;
    // Clear existing buttons
    QLayoutItem *item;
    while ((item = widget->buttonLayout->takeAt(0)) != nullptr) {
        if (item->widget()) {
            delete item->widget();
        }
        delete item;
    }

    for (const QString& eventPath : events) {
        QString btnText = eventPath;
        if (btnText.startsWith("event:/")) {
             btnText = btnText.mid(7);

        }
        
        QPushButton* btn = new QPushButton(btnText);
        AbstractDelegateModel::registerOSCControl("/"+btnText, btn);
        btn->setToolTip(eventPath);
        // Style the button to look better

        
        connect(btn, &QPushButton::clicked, this, [this, eventPath]() {
            onEventSelected(eventPath);
        });
        
        widget->buttonLayout->addWidget(btn);
    }
}

/**
 * @brief 处理输入端口
 * - 端口0：事件文本，直接播放对应事件路径或GUID
 * - 端口1：事件索引，按当前事件列表中的索引播放
 */
void FmodDecoderDataModel::setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex)
{
    if (!data) return;
    auto variableData = std::dynamic_pointer_cast<VariableData>(data);
    if (!variableData) return;

    switch (portIndex) {
    case 0: { // 文本事件触发
        const QString eventPath = variableData->value().toString();
        if (!eventPath.isEmpty()) {
            QMetaObject::invokeMethod(worker_, "playEvent", Qt::QueuedConnection, Q_ARG(QString, eventPath));
        }
        break;
    }
    case 1: { // 索引事件触发
        const int idx = variableData->value().toInt();
        if (idx >= 0 && idx < availableEvents_.size()) {
            const QString& eventPath = availableEvents_.at(idx);
            QMetaObject::invokeMethod(worker_, "playEvent", Qt::QueuedConnection, Q_ARG(QString, eventPath));
        }
        break;
    }
    default:
        break;
    }
}

void FmodDecoderDataModel::onEventSelected(const QString& eventPath)
{
    // Call worker to play event
    QMetaObject::invokeMethod(worker_, "playEvent", Qt::QueuedConnection, Q_ARG(QString, eventPath));
}

void FmodDecoderDataModel::load(QJsonObject const &p)
{
    AbstractDelegateModel::load(p);
    // 优先从保存的 JSON 中恢复路径，其次使用 UI 文本
    QString path;
    QJsonValue v = p["path"];
    if (!v.isUndefined() && v.isString()) {
        path = v.toString();
    }
    if (path.isEmpty() && widget && widget->fileSelectComboBox) {
        path = widget->fileSelectComboBox->text();
    }
    if (!path.isEmpty()) {
        widget->fileSelectComboBox->setText(path);
        QMetaObject::invokeMethod(worker_, "loadBanks", Qt::QueuedConnection, Q_ARG(QString, path));
    }
}

} // namespace Nodes
