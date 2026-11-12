#pragma once

#include "NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>
#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QThread>
#include <QtCore/QMetaObject>
#include <QtCore/QSignalBlocker>
#include <iostream>
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include "OpcUaServerWorker.hpp"
#include "OpcUaServerInterface.hpp"

using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;
using namespace QtNodes;
namespace Nodes
{
    // OPC UA 服务器节点：把 Flow 图的 8 个输入值暴露为 ns=1;s=<TagName> 变量。
    // 图 → 变量：In 端口写入；变量 → 图：外部客户端写入时经 onWrite 回调流出到 Out 端口。
    // open62541 服务器跑在独立 Worker 线程（20ms 非阻塞 iterate），GUI 不阻塞。
    class OpcUaServerDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(int port READ getPort WRITE setPort NOTIFY portChanged)
        Q_PROPERTY(QString tags READ getTags WRITE setTags NOTIFY tagsChanged)

    public:
        OpcUaServerDataModel()
        {
            InPortCount = 8;
            OutPortCount = 8;
            CaptionVisible = true;
            Caption = "OPC UA Server";
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = false;
            for (int i = 0; i < 8; ++i) {
                m_tagValues.append(QVariant(0.0));
                m_outData.append(std::make_shared<VariableData>(0.0));
            }

            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "port";
                b.control = widget->portSpinBox;
                AbstractDelegateModel::registerExternalBinding("/port", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "tags";
                b.control = widget->tagsEdit;
                AbstractDelegateModel::registerExternalBinding("/tags", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "start";
                b.control = widget->startButton;
                AbstractDelegateModel::registerExternalBinding("/start", this, b);
            }

            connect(widget->portSpinBox, &IntDragValueWidget::valueChanged, this, &OpcUaServerDataModel::setPort);
            connect(widget->tagsEdit, &QLineEdit::editingFinished, this, [this]() { setTags(widget->tagsEdit->text()); });
            connect(widget->startButton, &QPushButton::clicked, this, &OpcUaServerDataModel::toggleRunning, Qt::QueuedConnection);

            m_thread = new QThread(this);
            m_worker = new OpcUaServerWorker();
            m_worker->moveToThread(m_thread);
            connect(m_thread, &QThread::finished, m_worker, &QObject::deleteLater);
            connect(m_worker, &OpcUaServerWorker::runningChanged, this, &OpcUaServerDataModel::onRunningChanged, Qt::QueuedConnection);
            connect(m_worker, &OpcUaServerWorker::tagWritten, this, &OpcUaServerDataModel::onTagWritten, Qt::QueuedConnection);
            connect(m_worker, &OpcUaServerWorker::flowWriteAccepted, this, &OpcUaServerDataModel::onFlowWriteAccepted, Qt::QueuedConnection);
            m_thread->start();

            m_restartDebounce = new QTimer(this);
            m_restartDebounce->setSingleShot(true);
            m_restartDebounce->setInterval(400);
            connect(m_restartDebounce, &QTimer::timeout, this, &OpcUaServerDataModel::restartNow);

            m_restartDebounce->start(); // 构造后自动启动服务器
        }

        ~OpcUaServerDataModel() override
        {
            if (m_thread) {
                m_thread->quit();
                m_thread->wait(2000);
            }
        }

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            Q_UNUSED(portType)
            return VariableData().type();
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            const QString tag = (portIndex >= 0 && portIndex < m_tagNames.size())
                                    ? m_tagNames.at(portIndex)
                                    : QStringLiteral("Tag%1").arg(portIndex + 1);
            return tag;
        }

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            if (port < 0 || port >= m_outData.size()) return nullptr;
            return m_outData.at(port);
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (data == nullptr || portIndex < 0 || portIndex >= 8) return;
            auto v = std::dynamic_pointer_cast<VariableData>(data);
            if (!v) return;
            if (!m_worker) return;
            QMetaObject::invokeMethod(m_worker, "writeTag", Qt::QueuedConnection,
                                      Q_ARG(int, static_cast<int>(portIndex)),
                                      Q_ARG(QVariant, v->value()));
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson = NodeDelegateModel::save();
            QJsonObject values;
            values["port"] = m_port;
            values["tags"] = m_tags;
            modelJson["values"] = values;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            QJsonValue v = p["values"];
            if (!v.isUndefined() && v.isObject()) {
                QJsonObject values = v.toObject();
                if (values.contains("port")) setPort(values["port"].toInt());
                if (values.contains("tags")) setTags(values["tags"].toString());
                m_restartDebounce->start();
            }
        }

        QWidget *embeddedWidget() override
        {
            return widget;
        }

        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex index) const override
        {
            Q_UNUSED(index)
            return portType == PortType::In ? ConnectionPolicy::Many : ConnectionPolicy::Many;
        }

        void afterModelReady() override
        {
            AbstractDelegateModel::afterModelReady();
            auto bus = GlobalEventBus::instance();
            bus->subscribe(makeFullOscAddress("/port"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/tags"), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress("/start"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

    public slots:
        void scheduleRestart()
        {
            if (!m_restartDebounce) {
                restartNow();
                return;
            }
            m_restartDebounce->start();
        }

        void restartNow()
        {
            if (!m_worker) return;
            if (!m_running) {
                startPending();
                return;
            }
            m_pendingRestart = true;
            QMetaObject::invokeMethod(m_worker, "stopServer", Qt::QueuedConnection);
        }

        void toggleRunning()
        {
            if (m_running) {
                m_userStopped = true;
                QMetaObject::invokeMethod(m_worker, "stopServer", Qt::QueuedConnection);
            } else {
                m_userStopped = false;
                startPending();
            }
        }

        void onRunningChanged(bool running, const QString &message)
        {
            m_running = running;
            widget->startButton->setText(running ? "Stop" : "Start");
            widget->startButton->setChecked(running);
            widget->startButton->setStyleSheet(running ? "color: green; font-weight: bold;"
                                                       : "color: red; font-weight: bold;");
            widget->startButton->setToolTip(message);
            if (m_reportedRunningState != running) {
                m_reportedRunningState = running;
                stateFeedBack("/start", running);
            }
            if (running && m_pendingRestart) {
                m_pendingRestart = false;
                startPending(); // 旧实例已停，用新配置启动
            } else if (!running && m_pendingRestart && !m_userStopped) {
                startPending();
            }
        }

        // 外部 OPC UA 客户端写入了变量 → 流出到图
        void onTagWritten(int index, const QVariant &value)
        {
            if (index < 0 || index >= 8) return;
            m_tagValues[index] = value;
            m_outData[index] = std::make_shared<VariableData>(value);
            Q_EMIT dataUpdated(index);
        }

        // Flow 图写入被服务器接受（保持输出端口与变量同步）
        void onFlowWriteAccepted(int index, const QVariant &value)
        {
            if (index < 0 || index >= 8) return;
            m_tagValues[index] = value;
            m_outData[index] = std::make_shared<VariableData>(value);
            Q_EMIT dataUpdated(index);
        }

    public:
        // Property Getters and Setters
        int getPort() const { return m_port; }
        void setPort(int value)
        {
            value = qBound(1, value, 65535);
            if (m_port == value) return;
            m_port = value;
            if (widget && widget->portSpinBox->value() != value) {
                QSignalBlocker blocker(widget->portSpinBox);
                widget->portSpinBox->setValue(value);
            }
            Q_EMIT portChanged(value);
            AbstractDelegateModel::stateFeedBack("/port", value);
            scheduleRestart();
        }

        QString getTags() const { return m_tags; }
        void setTags(const QString &value)
        {
            if (m_tags == value) return;
            m_tags = value;
            if (widget && widget->tagsEdit->text() != value) {
                QSignalBlocker blocker(widget->tagsEdit);
                widget->tagsEdit->setText(value);
            }
            rebuildTagNames();
            Q_EMIT tagsChanged(value);
            AbstractDelegateModel::stateFeedBack("/tags", value);
            scheduleRestart(); // 变量集变化需要重建地址空间
        }

    Q_SIGNALS:
        void portChanged(int value);
        void tagsChanged(QString value);

    private Q_SLOTS:
        void onGlobalEvent(const GlobalEvent &ev)
        {
            QString localPath = ev.address.mid(ev.address.lastIndexOf("/") + 1);
            if (localPath == "port") setPort(ev.payload.toInt());
            else if (localPath == "tags") setTags(ev.payload.toString());
            else if (localPath == "start") toggleRunning();
        }

    private:
        void startPending()
        {
            if (!m_worker || m_port <= 0) return;
            QMetaObject::invokeMethod(m_worker, "startServer", Qt::QueuedConnection,
                                      Q_ARG(int, m_port), Q_ARG(QStringList, m_tagNames));
        }

        void rebuildTagNames()
        {
            m_tagNames.clear();
            const QStringList parts = m_tags.split(QLatin1Char(','), Qt::SkipEmptyParts);
            for (QString p : parts) {
                p = p.trimmed();
                if (p.isEmpty()) continue;
                // NodeId 字符串里不允许出现英文分号，替换掉防解析错乱
                p.replace(QLatin1Char(';'), QLatin1Char('_'));
                m_tagNames.append(p);
            }
            if (m_tagNames.isEmpty()) {
                for (int i = 0; i < 8; ++i) m_tagNames.append(QStringLiteral("Tag%1").arg(i + 1));
            }
            while (m_tagNames.size() < 8) m_tagNames.append(m_tagNames.last());
        }

        OpcUaServerInterface *widget = new OpcUaServerInterface();
        OpcUaServerWorker *m_worker = nullptr;
        QThread *m_thread = nullptr;
        QTimer *m_restartDebounce = nullptr;

        QVector<QVariant> m_tagValues;
        QVector<std::shared_ptr<VariableData>> m_outData;
        QStringList m_tagNames;

        int m_port = 4840;
        QString m_tags = QStringLiteral("Tag1,Tag2,Tag3,Tag4,Tag5,Tag6,Tag7,Tag8");
        bool m_running = false;
        bool m_pendingRestart = false;
        bool m_userStopped = false;
        bool m_reportedRunningState = false;
    };
}
