#pragma once

#include <QtCore/QObject>
#include <QtCore/qglobal.h>

#include "NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>

#include "RangerMiniInterface.hpp"
#include "RangerMiniController.h"
#include "Common/Devices/StatusContainer/GlobalEventBus.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"

using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;

using namespace NodeDataTypes;
using namespace QtNodes;

namespace Nodes
{
    /**
     * @brief RANGER MINI 3.0 数据流节点模型
     *
     * 职责：
     * - 承载节点 UI（RangerMiniInterface）与协议控制器（RangerMiniController）
     * - 将图端口 / 属性 / OSC 命令映射到控制器
     * - 把底盘 STATUS / FEEDBACK 推到输出端口
     *
     * 端口：
     * - 输入 0 VX(m/s) / 1 STEER(rad) / 2 SPIN(rad/s) / 3 ENABLE(bool)
     * - 输出 0 STATUS / 1 FEEDBACK
     *
     * 线程：UI 在主线程；对控制器的调用经 QueuedConnection，与 CanBus Worker 解耦。
     */
    class RangerMiniDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        // 供属性树 / OSC / 外部绑定读写的 Q_PROPERTY
        Q_PROPERTY(int deviceIndex READ getDeviceIndex WRITE setDeviceIndex NOTIFY deviceIndexChanged)
        Q_PROPERTY(int motionMode READ getMotionMode WRITE setMotionMode NOTIFY motionModeChanged)
        Q_PROPERTY(double linearVelocity READ getLinearVelocity WRITE setLinearVelocity NOTIFY linearVelocityChanged)
        Q_PROPERTY(double steerRadians READ getSteerRadians WRITE setSteerRadians NOTIFY steerRadiansChanged)
        Q_PROPERTY(double spinRate READ getSpinRate WRITE setSpinRate NOTIFY spinRateChanged)
        Q_PROPERTY(bool controlEnabled READ getControlEnabled WRITE setControlEnabled NOTIFY controlEnabledChanged)

    public:
        RangerMiniDataModel()
        {
            // —— 节点外观与端口数量 ——
            InPortCount = 4;
            OutPortCount = 2;
            CaptionVisible = true;
            PortEditable = false;
            Caption = PLUGIN_NAME;
            WidgetEmbeddable = false;
            Resizable = false;

            m_statusOut = std::make_shared<VariableData>();
            m_feedbackOut = std::make_shared<VariableData>();

            // —— 注册外部绑定：OSC 地址 ↔ 属性/控件 ——
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "deviceIndex";
                b.control = widget->deviceCombo;
                AbstractDelegateModel::registerExternalBinding("/deviceIndex", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "motionMode";
                b.control = widget->motionModeCombo;
                AbstractDelegateModel::registerExternalBinding("/motionMode", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "linearVelocity";
                b.control = widget->linearSpin;
                AbstractDelegateModel::registerExternalBinding("/linearVelocity", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "steerRadians";
                b.control = widget->steerSpin;
                AbstractDelegateModel::registerExternalBinding("/steerRadians", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "spinRate";
                b.control = widget->spinRateSpin;
                AbstractDelegateModel::registerExternalBinding("/spinRate", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "controlEnabled";
                b.control = widget->enableButton;
                AbstractDelegateModel::registerExternalBinding("/controlEnabled", this, b);
            }

            // —— 模型 → 控制器（排队到控制器所在线程上下文）——
            connect(this, &RangerMiniDataModel::openDeviceRequest, controller, &RangerMiniController::openDevice, Qt::QueuedConnection);
            connect(this, &RangerMiniDataModel::closeDeviceRequest, controller, &RangerMiniController::closeDevice, Qt::QueuedConnection);
            connect(this, &RangerMiniDataModel::setMotionModeRequest, controller, &RangerMiniController::setMotionMode, Qt::QueuedConnection);
            connect(this, &RangerMiniDataModel::setLinearRequest, controller, &RangerMiniController::setLinearVelocity, Qt::QueuedConnection);
            connect(this, &RangerMiniDataModel::setSteerRequest, controller, &RangerMiniController::setSteerRadians, Qt::QueuedConnection);
            connect(this, &RangerMiniDataModel::setSpinRequest, controller, &RangerMiniController::setSpinRadPerSec, Qt::QueuedConnection);
            connect(this, &RangerMiniDataModel::setControlRequest, controller, &RangerMiniController::setControlEnabled, Qt::QueuedConnection);
            connect(this, &RangerMiniDataModel::clearFaultRequest, controller, &RangerMiniController::clearFaults, Qt::QueuedConnection);

            // —— 控制器 → UI / 输出端口 ——
            connect(controller, &RangerMiniController::isReady, this, [this](bool ready) {
                m_busReady = ready;
                widget->setBusConnected(ready);
                if (!ready) {
                    setControlEnabled(false);
                }
            }, Qt::QueuedConnection);

            connect(controller, &RangerMiniController::controlEnabledChanged, this, [this](bool enabled) {
                m_controlEnabled = enabled;
                widget->setControlArmed(enabled);
                emit controlEnabledChanged(enabled);
            }, Qt::QueuedConnection);

            // STATUS 输出：更新端口数据，并刷新界面状态摘要
            connect(controller, &RangerMiniController::statusUpdated, this, [this](const QVariantMap &status) {
                m_statusOut = std::make_shared<VariableData>(status);
                const int mode = status.value(QStringLiteral("control_mode")).toInt();
                const double batt = status.value(QStringLiteral("battery_v")).toDouble();
                const bool estop = status.value(QStringLiteral("estop")).toBool();
                widget->statusLabel->setText(
                    QStringLiteral("模式=%1  电压=%2V  急停=%3")
                        .arg(mode)
                        .arg(batt, 0, 'f', 1)
                        .arg(estop ? QStringLiteral("是") : QStringLiteral("否")));
                Q_EMIT dataUpdated(0);
            }, Qt::QueuedConnection);

            // FEEDBACK 输出
            connect(controller, &RangerMiniController::feedbackUpdated, this, [this](const QVariantMap &fb) {
                m_feedbackOut = std::make_shared<VariableData>(fb);
                Q_EMIT dataUpdated(1);
            }, Qt::QueuedConnection);

            // —— UI 交互 → 属性 setter ——
            connect(widget->deviceCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
                setDeviceIndex(widget->deviceCombo->currentData().toInt());
            });
            connect(widget->connectButton, &QPushButton::clicked, this, [this](bool) {
                // 已连接则断开，否则打开当前选中设备
                if (m_busReady) {
                    emit closeDeviceRequest();
                } else {
                    openCurrentDevice();
                }
            });
            connect(widget->motionModeCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this](int) {
                setMotionMode(widget->motionModeCombo->currentData().toInt());
            });
            connect(widget->linearSpin, &FloatDragValueWidget::valueChanged, this, &RangerMiniDataModel::setLinearVelocity);
            connect(widget->steerSpin, &FloatDragValueWidget::valueChanged, this, &RangerMiniDataModel::setSteerRadians);
            connect(widget->spinRateSpin, &FloatDragValueWidget::valueChanged, this, &RangerMiniDataModel::setSpinRate);
            connect(widget->clearFaultButton, &QPushButton::clicked, this, [this]() {
                emit clearFaultRequest();
            });
            connect(widget->enableButton, &QPushButton::clicked, this, [this](bool checked) {
                setControlEnabled(checked);
            });

            m_deviceIndex = widget->deviceCombo->currentData().toInt();
            m_motionMode = widget->motionModeCombo->currentData().toInt();
        }

        ~RangerMiniDataModel() override
        {
            if (controller) {
                controller->setControlEnabled(false);
                controller->closeDevice();
                delete controller;
                controller = nullptr;
            }
        }

        /** USB-CAN 设备索引（下拉框 itemData） */
        int getDeviceIndex() const { return m_deviceIndex; }
        void setDeviceIndex(int index)
        {
            if (m_deviceIndex == index) {
                return;
            }
            m_deviceIndex = index;
            {
                QSignalBlocker blocker(widget->deviceCombo);
                const int comboIndex = widget->deviceCombo->findData(m_deviceIndex);
                if (comboIndex >= 0) {
                    widget->deviceCombo->setCurrentIndex(comboIndex);
                }
            }
            // 已连接时切换设备：立即重开新索引
            if (m_busReady) {
                openCurrentDevice();
            }
            emit deviceIndexChanged(m_deviceIndex);
        }

        /** 运动模型 0~3，见 RangerMiniController::MotionMode */
        int getMotionMode() const { return m_motionMode; }
        void setMotionMode(int mode)
        {
            if (m_motionMode == mode) {
                return;
            }
            m_motionMode = mode;
            {
                QSignalBlocker blocker(widget->motionModeCombo);
                const int comboIndex = widget->motionModeCombo->findData(m_motionMode);
                if (comboIndex >= 0) {
                    widget->motionModeCombo->setCurrentIndex(comboIndex);
                }
            }
            emit setMotionModeRequest(m_motionMode);
            emit motionModeChanged(m_motionMode);
        }

        /** 线速度，单位 m/s（节点间）；CAN 边界再 ×1000 → mm/s */
        double getLinearVelocity() const { return m_linearVelocity; }
        void setLinearVelocity(double v)
        {
            m_linearVelocity = v;
            {
                QSignalBlocker blocker(widget->linearSpin);
                widget->linearSpin->setValue(m_linearVelocity);
            }
            emit setLinearRequest(m_linearVelocity);
            emit linearVelocityChanged(m_linearVelocity);
        }

        /** 转向角，单位 rad，左转为正（与说明书一致；CAN 下发 ×1000） */
        double getSteerRadians() const { return m_steerRadians; }
        void setSteerRadians(double rad)
        {
            m_steerRadians = rad;
            {
                QSignalBlocker blocker(widget->steerSpin);
                widget->steerSpin->setValue(m_steerRadians);
            }
            emit setSteerRequest(m_steerRadians);
            emit steerRadiansChanged(m_steerRadians);
        }

        /** 自旋角速度，单位 rad/s，逆时针为正 */
        double getSpinRate() const { return m_spinRate; }
        void setSpinRate(double rate)
        {
            m_spinRate = rate;
            {
                QSignalBlocker blocker(widget->spinRateSpin);
                widget->spinRateSpin->setValue(m_spinRate);
            }
            emit setSpinRequest(m_spinRate);
            emit spinRateChanged(m_spinRate);
        }

        /** 是否使能运动心跳（Control ON） */
        bool getControlEnabled() const { return m_controlEnabled; }
        void setControlEnabled(bool enabled)
        {
            // 实际结果以控制器 controlEnabledChanged 回传为准
            emit setControlRequest(enabled);
        }

        /**
         * 模型 ID / 父级路径就绪后订阅全局事件总线，
         * 使 OSC / 外部命令可按完整地址写入上述属性。
         */
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/deviceIndex"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/motionMode"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/linearVelocity"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/steerRadians"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/spinRate"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/controlEnabled"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

        /** 端口显示名称 */
        QString portCaption(PortType portType, PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                switch (portIndex) {
                case 0: return QStringLiteral("VX");      // m/s
                case 1: return QStringLiteral("STEER");   // rad
                case 2: return QStringLiteral("SPIN");    // rad/s
                case 3: return QStringLiteral("ENABLE");  // bool
                default: break;
                }
                break;
            case PortType::Out:
                switch (portIndex) {
                case 0: return QStringLiteral("STATUS");
                case 1: return QStringLiteral("FEEDBACK");
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
            case 0: return m_statusOut;
            case 1: return m_feedbackOut;
            default: return nullptr;
            }
        }

        /**
         * 输入端口数据到达：
         * - VX / STEER / SPIN：更新目标值（心跳帧会带上最新值）
         * - ENABLE：开启或关闭运动控制
         */
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
            case 0:
                setLinearVelocity(var->asNumber());
                break;
            case 1:
                setSteerRadians(var->asNumber());
                break;
            case 2:
                setSpinRate(var->asNumber());
                break;
            case 3:
                setControlEnabled(var->asBool());
                break;
            default:
                break;
            }
        }

        QWidget *embeddedWidget() override { return widget; }

        /** 保存设备索引、运动模式与运动参数（不保存瞬时连接/使能状态） */
        QJsonObject save() const override
        {
            QJsonObject values;
            values[QStringLiteral("DeviceIndex")] = m_deviceIndex;
            values[QStringLiteral("MotionMode")] = m_motionMode;
            values[QStringLiteral("LinearVelocity")] = m_linearVelocity;
            values[QStringLiteral("SteerRadians")] = m_steerRadians;
            values[QStringLiteral("SpinRate")] = m_spinRate;

            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson[QStringLiteral("values")] = values;
            return modelJson;
        }

        /** 加载工程时恢复参数；加载后需用户手动连接并 Control ON */
        void load(const QJsonObject &p) override
        {
            const QJsonValue v = p[QStringLiteral("values")];
            if (!v.isObject()) {
                return;
            }
            widget->refreshDevices();
            setDeviceIndex(v[QStringLiteral("DeviceIndex")].toInt());
            setMotionMode(v[QStringLiteral("MotionMode")].toInt());
            setLinearVelocity(v[QStringLiteral("LinearVelocity")].toDouble());
            setSteerRadians(v[QStringLiteral("SteerRadians")].toDouble());
            setSpinRate(v[QStringLiteral("SpinRate")].toDouble());
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

    public slots:
        /** 处理全局事件总线发来的 Command（OSC / 外部控制） */
        void onGlobalEvent(const GlobalEvent &ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }
            const QString localPath = ev.address.mid(ev.address.lastIndexOf(QLatin1Char('/')) + 1);
            if (localPath == QLatin1String("deviceIndex")) {
                setDeviceIndex(ev.payload.toInt());
            } else if (localPath == QLatin1String("motionMode")) {
                setMotionMode(ev.payload.toInt());
            } else if (localPath == QLatin1String("linearVelocity")) {
                setLinearVelocity(ev.payload.toDouble());
            } else if (localPath == QLatin1String("steerRadians")) {
                setSteerRadians(ev.payload.toDouble());
            } else if (localPath == QLatin1String("spinRate")) {
                setSpinRate(ev.payload.toDouble());
            } else if (localPath == QLatin1String("controlEnabled")) {
                setControlEnabled(ev.payload.toBool());
            }
        }

    signals:
        void deviceIndexChanged(int);
        void motionModeChanged(int);
        void linearVelocityChanged(double);
        void steerRadiansChanged(double);
        void spinRateChanged(double);
        void controlEnabledChanged(bool);

        // 以下信号用于跨线程请求控制器执行（勿在 UI 线程直接阻塞调用 CAN）
        void openDeviceRequest(int deviceIndex);
        void closeDeviceRequest();
        void setMotionModeRequest(int mode);
        void setLinearRequest(double mps);
        void setSteerRequest(double radians);
        void setSpinRequest(double radPerSec);
        void setControlRequest(bool enabled);
        void clearFaultRequest();

    private:
        /** 按当前 m_deviceIndex 打开设备；无效索引则关闭 */
        void openCurrentDevice()
        {
            if (m_deviceIndex < 0) {
                emit closeDeviceRequest();
                return;
            }
            emit openDeviceRequest(m_deviceIndex);
        }

        RangerMiniInterface *widget = new RangerMiniInterface();     ///< 嵌入式控制面板
        RangerMiniController *controller = new RangerMiniController(); ///< 协议与心跳

        std::shared_ptr<VariableData> m_statusOut;   ///< 输出端口 0
        std::shared_ptr<VariableData> m_feedbackOut; ///< 输出端口 1

        int m_deviceIndex = -1;
        int m_motionMode = RangerMiniController::Ackermann;
        double m_linearVelocity = 0.0; ///< m/s
        double m_steerRadians = 0.0;   ///< rad
        double m_spinRate = 0.0;       ///< rad/s
        bool m_controlEnabled = false;
        bool m_busReady = false;
    };
}
