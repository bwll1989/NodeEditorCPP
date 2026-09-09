#pragma once

#include <QtCore/QObject>
#include <QtCore/QTimer>
#include <QtCore/QJsonArray>
#include <QtCore/QJsonObject>
#include <QtCore/QJsonDocument>
#include <QtCore/QJsonParseError>
#include <QtCore/QVariantList>
#include <cmath>

#include "NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>

#include "MissionSequencerInterface.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"

using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
    /**
     * @brief Nav Missions：导航任务序列（任务层）
     *
     * 输入：
     *   ENABLE  — 任务总使能
     *   ARRIVED — Nav 到达 trigger（true 一次）→ 导航步推进
     *
     * 输出 GOAL/ACTIVE → Nav；ACTION 仅在「动作」步输出 JSON（到达后执行），
     * 不含 navigate/wait 类型字符串。
     */
    class MissionSequencerDataModel : public AbstractDelegateModel
    {
        Q_OBJECT

    public:
        MissionSequencerDataModel()
        {
            InPortCount = 2;
            OutPortCount = 6;
            CaptionVisible = true;
            PortEditable = false;
            Caption = QStringLiteral("Nav Missions");
            WidgetEmbeddable = false;
            Resizable = true;

            m_goalOut = std::make_shared<VariableData>(QVariantList{0.0, 0.0, 0.0});
            m_activeOut = std::make_shared<VariableData>(false);
            m_actionOut = std::make_shared<VariableData>();
            m_stepOut = std::make_shared<VariableData>(0);
            m_finishedOut = std::make_shared<VariableData>(false);
            m_statusOut = std::make_shared<VariableData>();

            m_waitTimer.setSingleShot(true);
            connect(&m_waitTimer, &QTimer::timeout, this, [this]() {
                if (m_running && m_enable) {
                    advanceStep();
                }
            });

            connect(widget, &MissionSequencerInterface::stepsChanged, this, [this]() {
                m_steps = widget->steps();
                if (m_running && m_stepIndex >= m_steps.size()) {
                    finishMission();
                } else {
                    publishOutputs();
                }
            });

            QJsonArray demo;
            {
                QJsonObject s;
                s.insert(QStringLiteral("type"), QStringLiteral("navigate"));
                s.insert(QStringLiteral("x"), 1.0);
                s.insert(QStringLiteral("y"), 0.0);
                s.insert(QStringLiteral("yaw"), 0.0);
                s.insert(QStringLiteral("label"), QStringLiteral("点 A"));
                demo.append(s);
            }
            {
                QJsonObject s;
                s.insert(QStringLiteral("type"), QStringLiteral("wait"));
                s.insert(QStringLiteral("seconds"), 2.0);
                s.insert(QStringLiteral("label"), QStringLiteral("停顿"));
                demo.append(s);
            }
            {
                QJsonObject s;
                s.insert(QStringLiteral("type"), QStringLiteral("navigate"));
                s.insert(QStringLiteral("x"), 0.0);
                s.insert(QStringLiteral("y"), 0.0);
                s.insert(QStringLiteral("yaw"), 0.0);
                s.insert(QStringLiteral("label"), QStringLiteral("回原点"));
                demo.append(s);
            }
            widget->setSteps(demo);
            m_steps = demo;
            publishOutputs();
        }

        QString portCaption(PortType portType, PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                switch (portIndex) {
                case 0: return QStringLiteral("ENABLE");
                case 1: return QStringLiteral("ARRIVED");
                default: break;
                }
                break;
            case PortType::Out:
                switch (portIndex) {
                case 0: return QStringLiteral("GOAL");
                case 1: return QStringLiteral("ACTIVE");
                case 2: return QStringLiteral("ACTION");
                case 3: return QStringLiteral("STEP");
                case 4: return QStringLiteral("FINISHED");
                case 5: return QStringLiteral("STATUS");
                default: break;
                }
                break;
            default:
                break;
            }
            return {};
        }

        NodeDataType dataType(PortType, PortIndex) const override
        {
            return VariableData().type();
        }

        std::shared_ptr<NodeData> outData(PortIndex const portIndex) override
        {
            switch (portIndex) {
            case 0: return m_goalOut;
            case 1: return m_activeOut;
            case 2: return m_actionOut;
            case 3: return m_stepOut;
            case 4: return m_finishedOut;
            case 5: return m_statusOut;
            default: return nullptr;
            }
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            if (!data) {
                return;
            }
            auto var = std::dynamic_pointer_cast<VariableData>(data);
            if (!var) {
                return;
            }

            switch (portIndex) {
            case 0: { // ENABLE
                const bool en = var->asBool();
                if (en == m_enable) {
                    break;
                }
                m_enable = en;
                if (m_enable) {
                    startMission();
                } else {
                    resetMission();
                }
                break;
            }
            case 1: { // ARRIVED：Nav trigger，收到 true 即推进导航步
                if (!var->asBool()) {
                    break;
                }
                if (m_enable && m_running && currentType() == QStringLiteral("navigate")) {
                    advanceStep();
                } else {
                    publishOutputs();
                }
                break;
            }
            default:
                break;
            }
        }

        QWidget *embeddedWidget() override { return widget; }

        QJsonObject save() const override
        {
            QJsonObject values;
            values[QStringLiteral("steps")] = m_steps;
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson[QStringLiteral("values")] = values;
            return modelJson;
        }

        void load(const QJsonObject &p) override
        {
            const QJsonValue v = p[QStringLiteral("values")];
            if (!v.isObject()) {
                return;
            }
            const QJsonArray steps = v[QStringLiteral("steps")].toArray();
            widget->setSteps(steps);
            m_steps = steps;
            m_enable = false;
            resetMission();
        }

        ConnectionPolicy portConnectionPolicy(PortType portType, PortIndex) const override
        {
            switch (portType) {
            case PortType::In:
            case PortType::Out:
                return ConnectionPolicy::Many;
            default:
                break;
            }
            return ConnectionPolicy::One;
        }

    private:
        static QString stepType(const QJsonObject &step)
        {
            QString type = step.value(QStringLiteral("type")).toString(QStringLiteral("navigate"));
            if (type == QStringLiteral("emit")) {
                return QStringLiteral("action"); // 兼容旧工程
            }
            return type;
        }

        QString currentType() const
        {
            return stepType(currentStep());
        }

        QJsonObject currentStep() const
        {
            if (m_stepIndex < 0 || m_stepIndex >= m_steps.size()) {
                return {};
            }
            return m_steps.at(m_stepIndex).toObject();
        }

        /// 仅动作步：打包 { "action": "...", "params": ... }；无动作时返回空
        static std::shared_ptr<VariableData> makeActionJson(const QJsonObject &step)
        {
            QString name = step.value(QStringLiteral("action")).toString().trimmed();
            if (name.isEmpty()) {
                name = step.value(QStringLiteral("data")).toString().trimmed(); // 旧字段
            }
            if (name.isEmpty()) {
                return std::make_shared<VariableData>();
            }

            QJsonObject obj;
            obj.insert(QStringLiteral("action"), name);

            const QString paramsRaw = step.value(QStringLiteral("params")).toString().trimmed();
            if (!paramsRaw.isEmpty()) {
                QJsonParseError err{};
                const QJsonDocument doc = QJsonDocument::fromJson(paramsRaw.toUtf8(), &err);
                if (err.error == QJsonParseError::NoError && doc.isObject()) {
                    obj.insert(QStringLiteral("params"), doc.object());
                } else if (err.error == QJsonParseError::NoError && doc.isArray()) {
                    obj.insert(QStringLiteral("params"), doc.array());
                } else {
                    obj.insert(QStringLiteral("params"),
                               QJsonValue::fromVariant(VariableData::fromDisplay(paramsRaw)));
                }
            }

            return std::make_shared<VariableData>(&obj);
        }

        void startMission()
        {
            m_waitTimer.stop();
            if (m_steps.isEmpty()) {
                finishMission();
                return;
            }
            m_running = true;
            m_finished = false;
            m_stepIndex = 0;
            enterCurrentStep();
        }

        void resetMission()
        {
            m_waitTimer.stop();
            m_running = false;
            m_finished = false;
            m_stepIndex = 0;
            m_goalOut = std::make_shared<VariableData>(QVariantList{0.0, 0.0, 0.0});
            m_activeOut = std::make_shared<VariableData>(false);
            m_actionOut = std::make_shared<VariableData>();
            m_stepOut = std::make_shared<VariableData>(0);
            m_finishedOut = std::make_shared<VariableData>(false);
            publishOutputs();
        }

        void finishMission()
        {
            m_waitTimer.stop();
            m_running = false;
            m_finished = true;
            m_activeOut = std::make_shared<VariableData>(false);
            m_actionOut = std::make_shared<VariableData>(); // 不输出 finished 类型
            m_finishedOut = std::make_shared<VariableData>(true);
            publishOutputs();
        }

        void advanceStep()
        {
            if (!m_running || !m_enable) {
                return;
            }
            ++m_stepIndex;
            if (m_stepIndex >= m_steps.size()) {
                finishMission();
                return;
            }
            enterCurrentStep();
        }

        void enterCurrentStep()
        {
            m_waitTimer.stop();

            const QJsonObject step = currentStep();
            const QString type = stepType(step);

            m_stepOut = std::make_shared<VariableData>(m_stepIndex);
            m_finishedOut = std::make_shared<VariableData>(false);
            // ACTION 仅在动作步输出 JSON；导航/等待清空
            m_actionOut = std::make_shared<VariableData>();

            if (type == QStringLiteral("navigate")) {
                const double x = step.value(QStringLiteral("x")).toDouble();
                const double y = step.value(QStringLiteral("y")).toDouble();
                const double yaw = step.value(QStringLiteral("yaw")).toDouble(0.0);
                m_goalOut = std::make_shared<VariableData>(QVariantList{x, y, yaw});
                m_activeOut = std::make_shared<VariableData>(true);
                publishOutputs();
                return;
            }

            if (type == QStringLiteral("wait")) {
                m_activeOut = std::make_shared<VariableData>(false);
                const double sec = std::max(0.0, step.value(QStringLiteral("seconds")).toDouble(1.0));
                publishOutputs();
                m_waitTimer.start(static_cast<int>(std::lround(sec * 1000.0)));
                return;
            }

            // 到达后动作：ACTION = { action, params }
            m_actionOut = makeActionJson(step);
            m_activeOut = std::make_shared<VariableData>(false);
            publishOutputs();
            QTimer::singleShot(0, this, [this]() {
                if (m_running && m_enable && currentType() == QStringLiteral("action")) {
                    advanceStep();
                }
            });
        }

        void publishOutputs()
        {
            QVariantMap status;
            status.insert(QStringLiteral("enable"), m_enable);
            status.insert(QStringLiteral("running"), m_running);
            status.insert(QStringLiteral("finished"), m_finished);
            status.insert(QStringLiteral("step"), m_stepIndex);
            status.insert(QStringLiteral("step_count"), m_steps.size());
            status.insert(QStringLiteral("type"), currentType());
            status.insert(QStringLiteral("active"), m_activeOut ? m_activeOut->asBool() : false);
            if (m_actionOut && !m_actionOut->isEmpty()) {
                status.insert(QStringLiteral("action"), m_actionOut->asMap());
            }
            if (m_stepIndex >= 0 && m_stepIndex < m_steps.size()) {
                const QJsonObject step = currentStep();
                status.insert(QStringLiteral("label"), step.value(QStringLiteral("label")).toString());
                status.insert(QStringLiteral("goal_x"), step.value(QStringLiteral("x")).toDouble());
                status.insert(QStringLiteral("goal_y"), step.value(QStringLiteral("y")).toDouble());
                status.insert(QStringLiteral("goal_yaw"), step.value(QStringLiteral("yaw")).toDouble());
            }
            status.insert(QStringLiteral("default"), m_stepIndex);
            m_statusOut = std::make_shared<VariableData>(status);

            QString stateText = QStringLiteral("空闲");
            if (!m_enable) {
                stateText = QStringLiteral("未使能");
            } else if (m_finished) {
                stateText = QStringLiteral("全部完成");
            } else if (m_running) {
                stateText = QStringLiteral("执行中 步骤 %1/%2 (%3)")
                                .arg(m_stepIndex)
                                .arg(m_steps.size())
                                .arg(currentType());
            }
            widget->setStatusText(QStringLiteral("状态：%1").arg(stateText));
            widget->setCurrentStepHighlight(m_running ? m_stepIndex : -1);

            for (int i = 0; i < 6; ++i) {
                Q_EMIT dataUpdated(i);
            }
        }

        MissionSequencerInterface *widget = new MissionSequencerInterface();
        QTimer m_waitTimer;
        QJsonArray m_steps;

        bool m_enable = false;
        bool m_running = false;
        bool m_finished = false;
        int m_stepIndex = 0;

        std::shared_ptr<VariableData> m_goalOut;
        std::shared_ptr<VariableData> m_activeOut;
        std::shared_ptr<VariableData> m_actionOut;
        std::shared_ptr<VariableData> m_stepOut;
        std::shared_ptr<VariableData> m_finishedOut;
        std::shared_ptr<VariableData> m_statusOut;
    };
}
