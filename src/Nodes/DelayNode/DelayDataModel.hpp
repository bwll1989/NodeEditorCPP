#pragma once

#include "Common/DataTypes/NodeDataList.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "DelayInterface.hpp"

#include <QtNodes/NodeDelegateModel>

#include <QtCore/QObject>
#include <QtCore/QSignalBlocker>
#include <QtCore/qglobal.h>

#include <QElapsedTimer>
#include <QTimer>

#include <algorithm>

using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
    class DelayDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        DelayDataModel()
        {
            InPortCount = 1;
            OutPortCount = 2;
            CaptionVisible = true;
            Caption = QStringLiteral("Delay");
            WidgetEmbeddable = false;
            Resizable = true;
            PortEditable = false;

            widget->setRowCount(static_cast<int>(OutPortCount));

            timer->setParent(this);
            timer->setSingleShot(true);
            timer->setTimerType(Qt::PreciseTimer);

            progressTimer->setParent(this);
            progressTimer->setInterval(100);

            connect(timer, &QTimer::timeout, this, &DelayDataModel::onSingleTimerTimeout);
            connect(progressTimer, &QTimer::timeout, this, &DelayDataModel::updateProgress);
            connect(widget, &DelayInterface::rowAppended, this, &DelayDataModel::onRowAppended);
            connect(widget, &DelayInterface::rowRemoved, this, &DelayDataModel::onRowRemoved);
            connect(widget, &DelayInterface::listChanged, this, [this]() {
                Q_EMIT embeddedWidgetSizeUpdated();
            });

            updateProgress();
        }

        ~DelayDataModel() override
        {
            stopAndClearSchedule();
            deleteLater();
        }

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            Q_UNUSED(portIndex)
            Q_UNUSED(portType)
            return VariableData().type();
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                Q_UNUSED(portIndex)
                return QStringLiteral("TRIGGER");
            case PortType::Out:
                return QStringLiteral("OUTPUT %1").arg(portIndex);
            default:
                return {};
            }
        }

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            Q_UNUSED(port)
            return inData;
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (data == nullptr) {
                return;
            }
            auto var = std::dynamic_pointer_cast<VariableData>(data);
            if (!var) {
                return;
            }

            if (portIndex != 0) {
                inData = var;
                return;
            }

            const bool ignoreRepeat = widget && widget->isIgnoreRepeat();
            if (ignoreRepeat && (timer->isActive() || !m_schedule.isEmpty())) {
                return;
            }

            inData = var;
            if (!ignoreRepeat) {
                stopAndClearSchedule();
            }

            QVector<DelayRule> messages = widget->getDelayMessages();
            if (messages.isEmpty()) {
                stopAndClearSchedule();
                Q_EMIT dataUpdated(0);
                return;
            }

            std::sort(messages.begin(), messages.end(), [](const DelayRule &a, const DelayRule &b) {
                return a.time < b.time;
            });
            m_schedule = messages;
            m_cursor = 0;
            m_totalDurationMs = std::max(0, m_schedule.last().time);
            m_elapsed.restart();

            if (!progressTimer->isActive()) {
                progressTimer->start();
            }
            armNextFromElapsed();
            updateProgress();
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson1 = widget->exportRules();
            modelJson1.insert(QStringLiteral("ignoreRepeat"), widget->isIgnoreRepeat());
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson.insert(QStringLiteral("values"), modelJson1);
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            const QSignalBlocker widgetBlocker(widget);
            const QJsonValue v = p.value(QStringLiteral("values"));
            if (!v.isUndefined() && v.isObject()) {
                const QJsonObject values = v.toObject();
                widget->importRules(values);
                widget->setIgnoreRepeat(values.value(QStringLiteral("ignoreRepeat")).toBool(true));
            }
            OutPortCount = static_cast<unsigned int>(qMax(1, widget->rowCount()));
            widget->setRowCount(static_cast<int>(OutPortCount));
            Q_EMIT embeddedWidgetSizeUpdated();
        }

        QWidget *embeddedWidget() override
        {
            return widget;
        }

        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex index) const override
        {
            Q_UNUSED(index)
            switch (portType) {
            case PortType::In:
            case PortType::Out:
                return ConnectionPolicy::Many;
            default:
                return ConnectionPolicy::One;
            }
        }

    public slots:
        void updateProgress()
        {
            if (!widget) {
                return;
            }
            if (m_schedule.isEmpty() || m_cursor >= m_schedule.size()) {
                widget->setProgressIdle();
                return;
            }

            const int totalMs = std::max(0, m_totalDurationMs);
            const int nowMs = static_cast<int>(elapsedMs());
            if (totalMs <= 0) {
                widget->setProgressPercent(100);
                return;
            }
            const int percent = std::max(0, std::min(100, (nowMs * 100) / totalMs));
            widget->setProgressPercent(percent);
        }

        void onSingleTimerTimeout()
        {
            if (m_schedule.isEmpty() || m_cursor >= m_schedule.size()) {
                stopAndClearSchedule();
                return;
            }

            fireDueItems();
            if (m_cursor < m_schedule.size()) {
                armNextFromElapsed();
                updateProgress();
            } else {
                stopAndClearSchedule();
            }
        }

    private slots:
        void onRowAppended()
        {
            const unsigned int oldCount = OutPortCount;
            Q_EMIT portsAboutToBeInserted(PortType::Out, oldCount, oldCount);
            OutPortCount = oldCount + 1;
            Q_EMIT portsInserted();
            Q_EMIT embeddedWidgetSizeUpdated();
        }

        void onRowRemoved(int index)
        {
            if (index < 0 || OutPortCount <= 1) {
                return;
            }
            const auto portIndex = static_cast<PortIndex>(index);
            Q_EMIT portsAboutToBeDeleted(PortType::Out, portIndex, portIndex);
            OutPortCount -= 1;
            Q_EMIT portsDeleted();
            Q_EMIT embeddedWidgetSizeUpdated();
        }

    public:
        DelayInterface *widget = new DelayInterface();
        std::shared_ptr<VariableData> inData;
        QTimer *timer = new QTimer();
        QTimer *progressTimer = new QTimer();

    private:
        QElapsedTimer m_elapsed;
        int m_totalDurationMs = 0;
        QVector<DelayRule> m_schedule;
        int m_cursor = 0;

        void stopAndClearSchedule()
        {
            if (timer->isActive()) {
                timer->stop();
            }
            if (progressTimer->isActive()) {
                progressTimer->stop();
            }
            m_schedule.clear();
            m_cursor = 0;
            m_totalDurationMs = 0;
            m_elapsed.invalidate();
            if (widget) {
                widget->setProgressIdle();
            }
        }

        qint64 elapsedMs() const
        {
            return m_elapsed.isValid() ? m_elapsed.elapsed() : 0;
        }

        void fireDueItems()
        {
            const qint64 now = elapsedMs();
            while (m_cursor < m_schedule.size()) {
                const int dueTime = std::max(0, m_schedule[m_cursor].time);
                if (dueTime > now) {
                    break;
                }
                while (m_cursor < m_schedule.size()
                       && std::max(0, m_schedule[m_cursor].time) == dueTime) {
                    const int port = std::max(0, m_schedule[m_cursor].port);
                    if (static_cast<unsigned int>(port) < OutPortCount) {
                        Q_EMIT dataUpdated(static_cast<PortIndex>(port));
                    }
                    ++m_cursor;
                }
            }
        }

        void armNextFromElapsed()
        {
            fireDueItems();
            if (m_cursor >= m_schedule.size()) {
                stopAndClearSchedule();
                return;
            }
            const int nextAbs = std::max(0, m_schedule[m_cursor].time);
            const int waitMs = std::max(0, static_cast<int>(nextAbs - elapsedMs()));
            timer->setInterval(waitMs);
            timer->start();
        }
    };
}
