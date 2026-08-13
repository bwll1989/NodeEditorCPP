/**
 * @file PingDataModel.hpp
 * @brief Ping 连通性节点：ENABLE 开启后每 10s ping 一次，输出 CONNECTED
 */
#pragma once

#include <QProcess>
#include <QSignalBlocker>
#include <QTimer>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/NodeDataList.hpp"
#include "PingInterface.hpp"
#include "PluginDefinition.hpp"

using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
    class PingDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

        static constexpr int kPingIntervalMs = 10000;

        enum InputPort : PortIndex { EnablePort = 0 };
        enum OutputPort : PortIndex { ConnectedPort = 0 };

        Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
        Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
        Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)

    public:
        PingDataModel()
        {
            InPortCount = 1;
            OutPortCount = 1;
            Caption = PLUGIN_NAME;
            CaptionVisible = true;
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = false;

            m_connectedOutput = std::make_shared<VariableData>(QVariant(false));
            m_host = widget->hostEdit->text().trimmed();
            widget->updateConnectionStatus(false);

            m_timer = new QTimer(this);
            m_timer->setInterval(kPingIntervalMs);
            connect(m_timer, &QTimer::timeout, this, &PingDataModel::startPing);

            m_process = new QProcess(this);
            connect(m_process,
                    QOverload<int, QProcess::ExitStatus>::of(&QProcess::finished),
                    this,
                    &PingDataModel::onPingFinished);
            connect(m_process, &QProcess::errorOccurred, this, [this](QProcess::ProcessError error) {
                if (error == QProcess::FailedToStart) {
                    setConnected(false);
                }
            });

            connect(widget->hostEdit, &QLineEdit::editingFinished, this, [this]() {
                setHost(widget->hostEdit->text());
            });
            connect(widget->enableCheckBox, &QCheckBox::toggled, this, [this](bool checked) {
                setEnabled(checked);
            });

            {
                NodeDelegateModel::ExternalBinding b;
                b.member = QStringLiteral("host");
                b.control = widget->hostEdit;
                AbstractDelegateModel::registerExternalBinding("/host", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = QStringLiteral("enabled");
                b.control = widget->enableCheckBox;
                AbstractDelegateModel::registerExternalBinding("/enable", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = QStringLiteral("connected");
                b.control = widget->connectionLabel;
                AbstractDelegateModel::registerExternalBinding("/connected", this, b);
            }
        }

        ~PingDataModel() override
        {
            m_timer->stop();
            if (m_process->state() != QProcess::NotRunning) {
                m_process->kill();
                m_process->waitForFinished(500);
            }
        }

        NodeDataType dataType(PortType, PortIndex) const override
        {
            return VariableData().type();
        }

        QString portCaption(PortType portType, PortIndex portIndex) const override
        {
            if (portType == PortType::In && portIndex == EnablePort) {
                return QStringLiteral("ENABLE");
            }
            if (portType == PortType::Out && portIndex == ConnectedPort) {
                return QStringLiteral("CONNECTED");
            }
            return {};
        }

        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            if (portIndex == ConnectedPort) {
                return m_connectedOutput;
            }
            return nullptr;
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (portIndex != EnablePort) {
                return;
            }
            auto var = std::dynamic_pointer_cast<VariableData>(data);
            if (!var) {
                setEnabled(false);
                return;
            }
            setEnabled(isBoolTrue(*var));
        }

        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex) const override
        {
            return (portType == PortType::In || portType == PortType::Out)
                ? ConnectionPolicy::Many
                : ConnectionPolicy::One;
        }

        QJsonObject save() const override
        {
            QJsonObject values;
            values[QStringLiteral("host")] = m_host;
            values[QStringLiteral("enabled")] = m_enabled;
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson[QStringLiteral("values")] = values;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            const QJsonValue v = p.value(QStringLiteral("values"));
            if (!v.isObject()) {
                return;
            }
            const QJsonObject values = v.toObject();
            setHost(values.value(QStringLiteral("host")).toString(m_host));
            if (values.contains(QStringLiteral("enabled"))) {
                setEnabled(values.value(QStringLiteral("enabled")).toBool());
            }
        }

        QWidget *embeddedWidget() override { return widget; }

        QString host() const { return m_host; }
        bool enabled() const { return m_enabled; }
        bool connected() const { return m_connected; }

        void setHost(const QString &host)
        {
            const QString trimmed = host.trimmed();
            if (m_host == trimmed) {
                return;
            }
            m_host = trimmed;
            {
                QSignalBlocker blocker(widget->hostEdit);
                widget->hostEdit->setText(m_host);
            }
            Q_EMIT hostChanged(m_host);
        }

        void setEnabled(bool enabled)
        {
            if (m_enabled == enabled) {
                return;
            }
            m_enabled = enabled;
            {
                QSignalBlocker blocker(widget->enableCheckBox);
                widget->enableCheckBox->setChecked(m_enabled);
            }
            Q_EMIT enabledChanged(m_enabled);

            if (m_enabled) {
                startPing();
                m_timer->start();
            } else {
                m_timer->stop();
                if (m_process->state() != QProcess::NotRunning) {
                    m_process->kill();
                }
                setConnected(false);
            }
        }

    Q_SIGNALS:
        void hostChanged(const QString &host);
        void enabledChanged(bool enabled);
        void connectedChanged(bool connected);

    private slots:
        void startPing()
        {
            if (m_process->state() != QProcess::NotRunning) {
                return;
            }

            const QString host = widget->hostEdit->text().trimmed();
            m_host = host;
            if (host.isEmpty()) {
                setConnected(false);
                return;
            }

            QString program;
            QStringList args;
#ifdef Q_OS_WIN
            program = QStringLiteral("ping");
            args << QStringLiteral("-n") << QStringLiteral("1")
                 << QStringLiteral("-w") << QStringLiteral("3000")
                 << host;
#else
            program = QStringLiteral("ping");
            args << QStringLiteral("-c") << QStringLiteral("1")
                 << QStringLiteral("-W") << QStringLiteral("3")
                 << host;
#endif
            m_process->start(program, args);
        }

        void onPingFinished(int exitCode, QProcess::ExitStatus exitStatus)
        {
            const bool ok = (exitStatus == QProcess::NormalExit && exitCode == 0);
            setConnected(ok);
        }

    private:
        void setConnected(bool connected)
        {
            if (m_connected == connected) {
                return;
            }
            m_connected = connected;
            widget->updateConnectionStatus(m_connected);
            m_connectedOutput = std::make_shared<VariableData>(QVariant(m_connected));
            Q_EMIT dataUpdated(ConnectedPort);
            Q_EMIT connectedChanged(m_connected);
        }

        static bool isBoolTrue(const VariableData &data)
        {
            const QVariant value = data.value();
            if (value.typeId() == QMetaType::Bool) {
                return value.toBool();
            }
            if (value.canConvert<int>()) {
                return value.toInt() != 0;
            }
            const QString text = value.toString().trimmed().toLower();
            return text == QLatin1String("true") || text == QLatin1String("1");
        }

        PingInterface *widget = new PingInterface();
        QTimer *m_timer = nullptr;
        QProcess *m_process = nullptr;

        std::shared_ptr<VariableData> m_connectedOutput;
        QString m_host;
        bool m_enabled = false;
        bool m_connected = false;
    };
}
