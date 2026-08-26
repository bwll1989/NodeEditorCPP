/**
 * @file PingDataModel.hpp
 * @brief Ping 连通性节点：ENABLE 开启后每 10s ping 一次，输出 CONNECTED
 */
#pragma once

#include <atomic>
#include <memory>

#include <QFutureWatcher>
#include <QSignalBlocker>
#include <QTimer>

#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/DataTypes/NodeDataList.hpp"
#include "PingInterface.hpp"
#include "PluginDefinition.hpp"

#ifdef Q_OS_WIN
#include <utility>
#endif

#ifndef Q_OS_WIN
#include <QProcess>
#endif

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
        static constexpr unsigned kPingTimeoutMs = 3000;

        enum InputPort : PortIndex { EnablePort = 0 };
        enum OutputPort : PortIndex { ConnectedPort = 0 };

        Q_PROPERTY(QString host READ host WRITE setHost NOTIFY hostChanged)
        Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
        Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)

    public:
        PingDataModel();
        ~PingDataModel() override;

        NodeDataType dataType(PortType, PortIndex) const override;
        QString portCaption(PortType portType, PortIndex portIndex) const override;
        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override;
        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override;
        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex) const override;
        QJsonObject save() const override;
        void load(const QJsonObject &p) override;
        QWidget *embeddedWidget() override;

        QString host() const { return m_host; }
        bool enabled() const { return m_enabled; }
        bool connected() const { return m_connected; }

        void setHost(const QString &host);
        void setEnabled(bool enabled);

    Q_SIGNALS:
        void hostChanged(const QString &host);
        void enabledChanged(bool enabled);
        void connectedChanged(bool connected);

    private Q_SLOTS:
        void startPing();
#ifdef Q_OS_WIN
        void onIcmpPingFinished();
#else
        void onPingProcessFinished(int exitCode, QProcess::ExitStatus exitStatus);
#endif

    private:
        void invalidatePendingPing();
        void setConnected(bool connected);
        static bool isBoolTrue(const VariableData &data);

        PingInterface *widget = nullptr;
        QTimer *m_timer = nullptr;

#ifdef Q_OS_WIN
        using PingResult = std::pair<int, bool>;
        QFutureWatcher<PingResult> *m_pingWatcher = nullptr;
        std::atomic<int> m_pingGeneration{0};
        std::atomic<bool> m_pingInFlight{false};
#else
        QProcess *m_process = nullptr;
#endif

        std::shared_ptr<VariableData> m_connectedOutput;
        QString m_host;
        bool m_enabled = false;
        bool m_connected = false;
    };
}
