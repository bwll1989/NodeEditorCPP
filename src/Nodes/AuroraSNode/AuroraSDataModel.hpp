#pragma once

#include <QtCore/QObject>
#include <QtCore/QJsonObject>
#include <QtCore/QElapsedTimer>
#include <QtCore/QThread>
#include <QtCore/QSignalBlocker>
#include <QtCore/QPointer>
#include <QtWidgets/QLineEdit>

#include <atomic>
#include <memory>

#include "NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include "AuroraSInterface.hpp"
#include "AuroraSWorker.hpp"
#include "PluginDefinition.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"

using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
    /// Aurora S 节点数据模型：桥接 UI、输出端口与 worker 线程
    class AuroraSDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(QString host READ getHost WRITE setHost NOTIFY hostChanged)
        Q_PROPERTY(QString mapFilePath READ getMapFilePath WRITE setMapFilePath NOTIFY mapFilePathChanged)
        Q_PROPERTY(bool connected READ getConnected NOTIFY connectedChanged)
        Q_PROPERTY(bool reinitialize READ getReinitialize WRITE setReinitialize NOTIFY reinitializeChanged)

    public:
        AuroraSDataModel()
        {
            InPortCount = 1;
            OutPortCount = 4;
            CaptionVisible = true;
            PortEditable = false;
            Caption = PLUGIN_NAME;
            WidgetEmbeddable = false;
            Resizable = false;

            // 初始化四个输出端口的默认数据
            m_orientationData = std::make_shared<VariableData>(QVariantMap{});
            m_positionData = std::make_shared<VariableData>(QVariantMap{});
            m_connectionData = std::make_shared<VariableData>(false);
            m_localizationData = std::make_shared<VariableData>(QVariantMap{
                {QStringLiteral("state"), QStringLiteral("unknown")},
                {QStringLiteral("state_text"), QStringLiteral("未知")},
                {QStringLiteral("localized"), false},
            });

            // worker 运行在独立线程，避免阻塞 UI
            m_worker = new AuroraSWorker();
            m_workerThread = new QThread(this);
            m_worker->moveToThread(m_workerThread);

            connect(m_worker,
                    &AuroraSWorker::connectionChanged,
                    this,
                    &AuroraSDataModel::onConnectionChanged,
                    Qt::QueuedConnection);
            connect(m_worker,
                    &AuroraSWorker::poseSampleReady,
                    this,
                    &AuroraSDataModel::onPoseSampleReady,
                    Qt::QueuedConnection);
            connect(m_worker,
                    &AuroraSWorker::localizationStatusReady,
                    this,
                    &AuroraSDataModel::onLocalizationStatusReady,
                    Qt::QueuedConnection);
            connect(m_worker,
                    &AuroraSWorker::errorOccurred,
                    this,
                    &AuroraSDataModel::onWorkerError,
                    Qt::QueuedConnection);
            connect(m_worker,
                    &AuroraSWorker::sessionFinished,
                    this,
                    &AuroraSDataModel::onSessionFinished,
                    Qt::QueuedConnection);

            // 外部 OSC/绑定：host、mapFilePath、connected
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "host";
                b.control = widget->hostEdit;
                AbstractDelegateModel::registerExternalBinding("/host", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "mapFilePath";
                b.control = widget->mapFileSelector;
                AbstractDelegateModel::registerExternalBinding("/mapFilePath", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "connected";
                AbstractDelegateModel::registerExternalBinding("/connected", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "reinitialize";
                b.control = widget->reinitializeButton;
                AbstractDelegateModel::registerExternalBinding("/reinitialize", this, b);
            }

            connect(widget->hostEdit, &QLineEdit::editingFinished, this, [this]() {
                setHost(widget->hostEdit->text());
            });
            connect(widget->mapFileSelector, &SelectorComboBox::textChanged, this, &AuroraSDataModel::setMapFilePath);
            connect(widget->reinitializeButton, &QPushButton::clicked, this, [this]() {
                setReinitialize(true);
            });

            widget->hostEdit->setText(m_host);
            widget->mapFileSelector->setCurrentValue(m_mapFilePath);

            m_uiUpdateTimer.start();
            m_workerThread->start();
            restartMonitor();
        }

        ~AuroraSDataModel() override
        {
            m_shuttingDown = true;
            m_restartPending = false;
            GlobalEventBus::instance()->unsubscribe(this);

            if (m_worker) {
                disconnect(m_worker, nullptr, this, nullptr);
                m_worker->requestStop();

                // 等待 runSession 退出（最多约 8 秒）
                for (int i = 0; i < 800 && m_worker->isSessionActive(); ++i) {
                    QThread::msleep(10);
                }

                if (m_workerThread && m_workerThread->isRunning()) {
                    // 在 worker 线程内 delete worker，避免跨线程析构 SDK
                    QMetaObject::invokeMethod(
                        m_worker, "shutdownAndDelete", Qt::BlockingQueuedConnection);
                    m_worker = nullptr;
                    m_workerThread->quit();
                    if (!m_workerThread->wait(5000)) {
                        m_workerThread->terminate();
                        m_workerThread->wait(1000);
                    }
                } else if (m_worker) {
                    m_worker->shutdown();
                    m_worker = nullptr;
                }
            }
        }

        QString getHost() const { return m_host; }
        void setHost(const QString& host)
        {
            if (m_host == host) {
                return;
            }
            m_host = host;

            if (widget) {
                QSignalBlocker blocker(widget->hostEdit);
                widget->hostEdit->setText(m_host);
            }

            emit hostChanged(m_host);
            restartMonitor();
        }

        QString getMapFilePath() const { return m_mapFilePath; }
        void setMapFilePath(const QString& mapFilePath)
        {
            if (m_mapFilePath == mapFilePath) {
                return;
            }
            m_mapFilePath = mapFilePath;

            if (widget) {
                QSignalBlocker blocker(widget->mapFileSelector);
                widget->mapFileSelector->setCurrentValue(m_mapFilePath);
            }

            emit mapFilePathChanged(m_mapFilePath);
            restartMonitor();
        }

        bool getConnected() const { return m_connected; }
        bool getReinitialize() const { return m_reinitialize; }

        /// 脉冲触发：外部写入 true 时执行重新初始化，随后自动复位为 false
        void setReinitialize(bool value)
        {
            if (!value || m_reinitialize) {
                return;
            }
            m_reinitialize = true;
            emit reinitializeChanged(true);

            requestReinitialize();

            m_reinitialize = false;
            emit reinitializeChanged(false);
        }

        QString portCaption(PortType portType, PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                switch (portIndex) {
                case 0:
                    return QStringLiteral("HOST");
                default:
                    break;
                }
                break;
            case PortType::Out:
                switch (portIndex) {
                case 0:
                    return QStringLiteral("ORIENTATION");
                case 1:
                    return QStringLiteral("POSITION");
                case 2:
                    return QStringLiteral("CONNECTION");
                case 3:
                    return QStringLiteral("LOCALIZATION");
                default:
                    break;
                }
                break;
            case PortType::None:
                break;
            }
            return {};
        }

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex);
            Q_UNUSED(portType);
            return VariableData().type();
        }

        std::shared_ptr<NodeData> outData(PortIndex portIndex) override
        {
            switch (portIndex) {
            case 0:
                return m_orientationData;
            case 1:
                return m_positionData;
            case 2:
                return m_connectionData;
            case 3:
                return m_localizationData;
            default:
                return nullptr;
            }
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex portIndex) override
        {
            if (data == nullptr) {
                return;
            }

            auto variableData = std::dynamic_pointer_cast<VariableData>(data);
            if (!variableData) {
                return;
            }

            switch (portIndex) {
            case 0:
                setHost(variableData->value().toString());
                break;
            default:
                break;
            }
        }

        QWidget* embeddedWidget() override
        {
            return widget;
        }

        QJsonObject save() const override
        {
            QJsonObject values;
            values.insert(QStringLiteral("Host"), m_host);
            values.insert(QStringLiteral("MapFilePath"), m_mapFilePath);

            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson.insert(QStringLiteral("values"), values);
            return modelJson;
        }

        void load(const QJsonObject& json) override
        {
            const QJsonValue values = json.value(QStringLiteral("values"));
            if (values.isObject()) {
                setHost(values[QStringLiteral("Host")].toString(m_host));
                setMapFilePath(values[QStringLiteral("MapFilePath")].toString(m_mapFilePath));
            }
        }

        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex /*index*/) const override
        {
            switch (portType) {
            case PortType::In:
            case PortType::Out:
                return ConnectionPolicy::Many;
            case PortType::None:
                break;
            }
            return ConnectionPolicy::One;
        }

        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(
                AbstractDelegateModel::makeFullOscAddress("/host"),
                this,
                SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(
                AbstractDelegateModel::makeFullOscAddress("/mapFilePath"),
                this,
                SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(
                AbstractDelegateModel::makeFullOscAddress("/reinitialize"),
                this,
                SLOT(onGlobalEvent(GlobalEvent)));
        }

    public slots:
        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }

            const QString localPath = ev.address.mid(ev.address.lastIndexOf('/') + 1);
            if (localPath == QStringLiteral("host")) {
                setHost(ev.payload.toString());
            } else if (localPath == QStringLiteral("mapFilePath")) {
                setMapFilePath(ev.payload.toString());
            } else if (localPath == QStringLiteral("reinitialize")) {
                setReinitialize(ev.payload.toBool());
            }
        }

        void onConnectionChanged(bool connected)
        {
            if (m_shuttingDown) {
                return;
            }
            m_connected = connected;
            m_connectionData = std::make_shared<VariableData>(connected);
            widget->updateConnectionStatus(connected);
            emit connectedChanged(connected);

            if (connected) {
                widget->clearStatusMessage();
            }

            Q_EMIT dataUpdated(2);
        }

        void onPoseSampleReady(const QVariantMap& orientation, const QVariantMap& position)
        {
            if (m_shuttingDown) {
                return;
            }

            // 输出端口始终更新；UI 限流 100ms 刷新
            m_orientationData = std::make_shared<VariableData>(orientation);
            m_positionData = std::make_shared<VariableData>(position);
            Q_EMIT dataUpdated(0);
            Q_EMIT dataUpdated(1);

            if (!widget) {
                return;
            }

            if (m_uiUpdateTimer.elapsed() >= kUiUpdateIntervalMs) {
                m_uiUpdateTimer.restart();
                widget->updateOrientation(orientation);
                widget->updatePosition(position);
            }
        }

        void onLocalizationStatusReady(const QVariantMap& status)
        {
            if (m_shuttingDown || !widget) {
                return;
            }
            m_localizationData = std::make_shared<VariableData>(status);
            widget->updateLocalizationStatus(status);
            Q_EMIT dataUpdated(3);
        }

        void onWorkerError(const QString& message)
        {
            if (m_shuttingDown || !widget) {
                return;
            }
            if (message.isEmpty()) {
                widget->clearStatusMessage();
            } else {
                widget->showError(message);
            }
        }

        /// 会话因 restartMonitor 停止后，用 pending 参数重新启动
        void onSessionFinished()
        {
            if (m_shuttingDown || !m_restartPending) {
                return;
            }

            m_restartPending = false;
            if (!m_worker || !m_workerThread || !m_workerThread->isRunning()) {
                return;
            }

            QMetaObject::invokeMethod(
                m_worker,
                "runSession",
                Qt::QueuedConnection,
                Q_ARG(QString, m_pendingHost),
                Q_ARG(QString, m_pendingMapFilePath));
        }

    signals:
        void hostChanged(const QString& host);
        void mapFilePathChanged(const QString& mapFilePath);
        void connectedChanged(bool connected);
        void reinitializeChanged(bool value);

    private:
        /// host/地图变更时重启监控：若会话活跃则先 requestStop，等 sessionFinished 后再 runSession
        void restartMonitor()
        {
            if (m_shuttingDown || !m_worker || !m_workerThread || !m_workerThread->isRunning()) {
                return;
            }

            m_pendingHost = m_host;
            m_pendingMapFilePath = m_mapFilePath;

            if (m_worker->isSessionActive()) {
                m_restartPending = true;
                m_worker->requestStop();
                return;
            }

            QMetaObject::invokeMethod(
                m_worker,
                "runSession",
                Qt::QueuedConnection,
                Q_ARG(QString, m_host),
                Q_ARG(QString, m_mapFilePath));
        }

        void requestReinitialize()
        {
            if (m_shuttingDown || !m_worker || !m_workerThread || !m_workerThread->isRunning()) {
                return;
            }

            QMetaObject::invokeMethod(
                m_worker,
                "reinitializeSession",
                Qt::QueuedConnection,
                Q_ARG(QString, m_mapFilePath));
        }

        AuroraSInterface* widget = new AuroraSInterface();
        AuroraSWorker* m_worker = nullptr;
        QThread* m_workerThread = nullptr;

        QString m_host = QStringLiteral("192.168.11.1");
        QString m_pendingHost = m_host;
        QString m_mapFilePath = QStringLiteral("auroramap.stcm");
        QString m_pendingMapFilePath = m_mapFilePath;
        bool m_connected = false;
        bool m_reinitialize = false;
        bool m_restartPending = false;
        std::atomic<bool> m_shuttingDown{false};

        QElapsedTimer m_uiUpdateTimer;
        static constexpr int kUiUpdateIntervalMs = 100;

        std::shared_ptr<VariableData> m_orientationData;
        std::shared_ptr<VariableData> m_positionData;
        std::shared_ptr<VariableData> m_connectionData;
        std::shared_ptr<VariableData> m_localizationData;
    };
}
