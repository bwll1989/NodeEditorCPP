#include "FmodDecoderDataModel.hpp"
#include <QDebug>
#include <QDir>
#include <QFileInfo>
#include <QLineEdit>
#include <QSignalBlocker>
#include <set>
#include "TimestampGenerator/TimestampGenerator.hpp"
#include "StatusContainer/GlobalEventBus.hpp"

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

    // 注册外部控制并将 UI 文本作为属性入口
    AbstractDelegateModel::registerExternalControl("/file", widget->fileSelectComboBox);
    connect(widget->fileSelectComboBox, &QLineEdit::textChanged, this, &FmodDecoderDataModel::setBankPath);
    connect(this, &FmodDecoderDataModel::bankPathChanged, this, [this](const QString&){
        {
            QSignalBlocker blocker(widget->fileSelectComboBox);
            widget->fileSelectComboBox->setText(m_bankPath);
        }
        if (!m_bankPath.isEmpty() && worker_) {
            QMetaObject::invokeMethod(
                worker_,
                "loadBanks",
                Qt::QueuedConnection,
                Q_ARG(QString, m_bankPath)
            );
        }
        AbstractDelegateModel::stateFeedBack("/file", m_bankPath);
    });

    connect(this, &FmodDecoderDataModel::currentEventChanged, this, [this](const QString&){
        if (m_currentEvent.isEmpty() || !worker_) {
            return;
        }
        QMetaObject::invokeMethod(
            worker_,
            "playEvent",
            Qt::QueuedConnection,
            Q_ARG(QString, m_currentEvent)
        );
        AbstractDelegateModel::stateFeedBack("/event", m_currentEvent);
    });
    
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
    const QString pathText = bankPath();
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
        setBankPath(path);
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
        AbstractDelegateModel::registerExternalControl("/"+btnText, btn);
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
            setCurrentEvent(eventPath);
        }
        break;
    }
    case 1: { // 索引事件触发
        const int idx = variableData->value().toInt();
        if (idx >= 0 && idx < availableEvents_.size()) {
            const QString& eventPath = availableEvents_.at(idx);
            setCurrentEvent(eventPath);
        }
        break;
    }
    default:
        break;
    }
}

void FmodDecoderDataModel::onEventSelected(const QString& eventPath)
{
    setCurrentEvent(eventPath);
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
        setBankPath(path);
    }
}

QString FmodDecoderDataModel::bankPath() const
{
    return m_bankPath;
}

void FmodDecoderDataModel::setBankPath(const QString& path)
{
    const QString trimmed = path.trimmed();
    if (trimmed == m_bankPath) {
        return;
    }
    m_bankPath = trimmed;
    Q_EMIT bankPathChanged(trimmed);
}

QString FmodDecoderDataModel::currentEvent() const
{
    return m_currentEvent;
}

void FmodDecoderDataModel::setCurrentEvent(const QString& eventPath)
{
    const QString trimmed = eventPath.trimmed();
    if (trimmed == m_currentEvent) {
        return;
    }
    m_currentEvent = trimmed;
    Q_EMIT currentEventChanged(trimmed);
}

void FmodDecoderDataModel::afterModelReady()
{
    GlobalEventBus::instance()->subscribe(
        makeFullOscAddress("/file"),
        this,
        SLOT(onGlobalEvent(GlobalEvent))
    );
    GlobalEventBus::instance()->subscribe(
        makeFullOscAddress("/event"),
        this,
        SLOT(onGlobalEvent(GlobalEvent))
    );
}

void FmodDecoderDataModel::onGlobalEvent(const GlobalEvent& ev)
{
    if (ev.kind != GlobalEventKind::Command) {
        return;
    }

    const QString addrFile = makeFullOscAddress("/file");
    const QString addrEvent = makeFullOscAddress("/event");

    if (ev.address == addrFile) {
        setBankPath(ev.payload.toString());
    } else if (ev.address == addrEvent) {
        setCurrentEvent(ev.payload.toString());
    }
}

} // namespace Nodes
