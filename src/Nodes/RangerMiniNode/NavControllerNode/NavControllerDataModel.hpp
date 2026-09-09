#pragma once

#include <QtCore/QObject>
#include <QtCore/qglobal.h>
#include <QtCore/QJsonArray>
#include <QtCore/QVariantList>
#include <QtCore/QDateTime>
#include <cmath>
#include <algorithm>
#include <limits>

#include "NodeDataList.hpp"
#include <QtNodes/NodeDelegateModel>

#include "NavControllerInterface.hpp"
#include "RsPath.hpp"
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
     * @brief 导航控制器（RangerMiniNode 子插件）— Reeds–Shepp + Pure Pursuit
     *
     * ---------------------------------------------------------------------------
     * 职责
     * ---------------------------------------------------------------------------
     * 1) 对当前位姿 → GOAL=[x,y,yaw] 做 RS 最短路径规划（含倒车圆弧）
     * 2) Pure Pursuit 跟踪路径采样点，输出 VX / STEER / ENABLE / ARRIVED
     *
     * 推荐连线：Aurora 位姿 → 本节点；Mission.GOAL/ACTIVE → GOAL/ENABLE；
     *           VX/STEER/ENABLE → Ranger；ARRIVED → Mission
     *
     * 单位：m / rad / m/s。L_eff = L/2（前后对向阿克曼）。
     * R_min = L_eff / tan(MaxSteer)，保证规划曲率不超过底盘转角能力。
     */
    class NavControllerDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        // 目标位姿统一为向量属性 [x, y, yaw]，避免拆成多个浮点属性造成中间态抖动
        Q_PROPERTY(QVariantList goal READ getGoal WRITE setGoal NOTIFY goalChanged)
        Q_PROPERTY(double maxVx READ getMaxVx WRITE setMaxVx NOTIFY maxVxChanged)
        Q_PROPERTY(double maxSteer READ getMaxSteer WRITE setMaxSteer NOTIFY maxSteerChanged)
        Q_PROPERTY(double arriveDist READ getArriveDist WRITE setArriveDist NOTIFY arriveDistChanged)
        Q_PROPERTY(bool enable READ getEnable WRITE setEnable NOTIFY enableChanged)

    public:
        NavControllerDataModel()
        {
            InPortCount = 5;
            OutPortCount = 5;
            CaptionVisible = true;
            PortEditable = false;
            Caption = QStringLiteral("Nav Controller");
            WidgetEmbeddable = false;
            Resizable = false;

            m_vxOut = std::make_shared<VariableData>(0.0);
            m_steerOut = std::make_shared<VariableData>(0.0);
            m_enableOut = std::make_shared<VariableData>(false);
            m_statusOut = std::make_shared<VariableData>();
            m_arrivedOut = std::make_shared<VariableData>(false);

            // 默认开启镜像：Aurora 地图 +Y 右 → Ranger 车体 +Y 左 / STEER 左正
            // 等价于平面变换 (y, yaw) → (-y, -yaw)；整车左右反了再关掉
            widget->mirrorFrameCheck->setChecked(true);

            // OSC / 属性绑定：目标 /goal = [x,y,yaw]
            registerBinding("/goal", "goal", nullptr);
            registerBinding("/maxVx", "maxVx", widget->maxVxSpin);
            registerBinding("/maxSteer", "maxSteer", widget->maxSteerSpin);
            registerBinding("/arriveDist", "arriveDist", widget->arriveSpin);
            registerBinding("/enable", "enable", nullptr);

            // 面板 Goal X/Y/Yaw 分开编辑，写入时合成向量走 applyGoal
            connect(widget->goalXSpin, &FloatDragValueWidget::valueChanged, this, [this](double x) {
                applyGoal(x, m_goalY, m_goalYaw);
            });
            connect(widget->goalYSpin, &FloatDragValueWidget::valueChanged, this, [this](double y) {
                applyGoal(m_goalX, y, m_goalYaw);
            });
            connect(widget->goalYawSpin, &FloatDragValueWidget::valueChanged, this, [this](double yaw) {
                applyGoal(m_goalX, m_goalY, yaw);
            });
            connect(widget->maxVxSpin, &FloatDragValueWidget::valueChanged, this, &NavControllerDataModel::setMaxVx);
            connect(widget->maxSteerSpin, &FloatDragValueWidget::valueChanged, this, &NavControllerDataModel::setMaxSteer);
            connect(widget->arriveSpin, &FloatDragValueWidget::valueChanged, this, &NavControllerDataModel::setArriveDist);
            connect(widget->arriveHystSpin, &FloatDragValueWidget::valueChanged, this, [this](double) { recompute(); });
            connect(widget->slowDistSpin, &FloatDragValueWidget::valueChanged, this, [this](double) { recompute(); });
            connect(widget->yawTolSpin, &FloatDragValueWidget::valueChanged, this, [this](double) { recompute(); });
            connect(widget->lookAheadSpin, &FloatDragValueWidget::valueChanged, this, [this](double) { recompute(); });
            connect(widget->wheelbaseSpin, &FloatDragValueWidget::valueChanged, this, [this](double) {
                m_needReplan = true; // 转弯半径变了，需重规划
                recompute();
            });
            connect(widget->kpSteerSpin, &FloatDragValueWidget::valueChanged, this, [this](double) { recompute(); });
            connect(widget->mirrorFrameCheck, &QCheckBox::toggled, this, [this](bool) {
                m_needReplan = true;
                recompute();
            });

            m_goalX = widget->goalXSpin->value();
            m_goalY = widget->goalYSpin->value();
            m_goalYaw = widget->goalYawSpin->value();
            m_maxVx = widget->maxVxSpin->value();
            m_maxSteer = widget->maxSteerSpin->value();
            m_arriveDist = widget->arriveSpin->value();
        }

        QVariantList getGoal() const
        {
            return QVariantList{m_goalX, m_goalY, m_goalYaw};
        }

        /** 向量写目标：[x,y,yaw]；缺省维保持原值 */
        void setGoal(const QVariantList &g)
        {
            const double x = g.size() > 0 ? g.at(0).toDouble() : m_goalX;
            const double y = g.size() > 1 ? g.at(1).toDouble() : m_goalY;
            const double yaw = g.size() > 2 ? g.at(2).toDouble() : m_goalYaw;
            applyGoal(x, y, yaw);
        }

        double getMaxVx() const { return m_maxVx; }
        void setMaxVx(double v)
        {
            m_maxVx = v;
            QSignalBlocker b(widget->maxVxSpin);
            widget->maxVxSpin->setValue(m_maxVx);
            emit maxVxChanged(m_maxVx);
            recompute();
        }

        double getMaxSteer() const { return m_maxSteer; }
        void setMaxSteer(double v)
        {
            m_maxSteer = v;
            QSignalBlocker b(widget->maxSteerSpin);
            widget->maxSteerSpin->setValue(m_maxSteer);
            emit maxSteerChanged(m_maxSteer);
            m_needReplan = true; // 转弯半径变了，需重规划
            recompute();
        }

        double getArriveDist() const { return m_arriveDist; }
        void setArriveDist(double v)
        {
            m_arriveDist = v;
            QSignalBlocker b(widget->arriveSpin);
            widget->arriveSpin->setValue(m_arriveDist);
            emit arriveDistChanged(m_arriveDist);
            recompute();
        }

        bool getEnable() const { return m_enable; }
        void setEnable(bool v)
        {
            if (v && !m_enable) {
                m_needReplan = true; // 重新使能时规划一次
            }
            m_enable = v;
            emit enableChanged(m_enable);
            recompute();
        }

        /** 模型就绪后订阅 OSC 全局命令（与 registerBinding 路径对应） */
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/goal"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/maxVx"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/maxSteer"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/arriveDist"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/enable"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

        QString portCaption(PortType portType, PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In:
                switch (portIndex) {
                case 0: return QStringLiteral("POSITION");
                case 1: return QStringLiteral("ORIENTATION");
                case 2: return QStringLiteral("LOCALIZATION");
                case 3: return QStringLiteral("GOAL");
                case 4: return QStringLiteral("ENABLE");
                default: break;
                }
                break;
            case PortType::Out:
                switch (portIndex) {
                case 0: return QStringLiteral("VX");
                case 1: return QStringLiteral("STEER");
                case 2: return QStringLiteral("ENABLE");
                case 3: return QStringLiteral("STATUS");
                case 4: return QStringLiteral("ARRIVED");
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
            case 0: return m_vxOut;
            case 1: return m_steerOut;
            case 2: return m_enableOut;
            case 3: return m_statusOut;
            case 4: return m_arrivedOut;
            default: return nullptr;
            }
        }

        /**
         * @brief 输入口回调：更新位姿 / 目标 / 使能后统一 recompute
         *
         * POSITION 取前两维；ORIENTATION 优先 rad 键，长度≥3 取下标 2 为 yaw；
         * 否则把标量当 yaw。GOAL 走 applyGoal 一次写入 [x,y,yaw]。
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
            case 0: { // POSITION：[x,y,...] m；一阶低通抑制 SLAM 抖动
                const QVector<float> v = var->asFloats(2);
                const double x = static_cast<double>(v.value(0));
                const double y = static_cast<double>(v.value(1));
                if (m_havePose) {
                    // alpha 越大越跟新样本；50Hz 下约几十 ms 量级平滑
                    constexpr double kAlpha = 0.35;
                    m_posX = kAlpha * x + (1.0 - kAlpha) * m_posX;
                    m_posY = kAlpha * y + (1.0 - kAlpha) * m_posY;
                } else {
                    m_posX = x;
                    m_posY = y;
                    m_havePose = true;
                }
                break;
            }
            case 1: { // ORIENTATION：rad=[roll,pitch,yaw] 或标量 yaw；角度同样低通
                QVector<float> v = var->asFloats(0, QStringLiteral("rad"));
                if (v.isEmpty()) {
                    v = var->asFloats(0);
                }
                double yawIn = 0.0;
                if (v.size() >= 3) {
                    yawIn = static_cast<double>(v.at(2));
                } else {
                    yawIn = v.isEmpty() ? 0.0 : static_cast<double>(v.at(0));
                }
                if (m_haveYaw) {
                    constexpr double kAlpha = 0.35;
                    const double err = wrapAngle(yawIn - m_yaw);
                    m_yaw = wrapAngle(m_yaw + kAlpha * err);
                } else {
                    m_yaw = yawIn;
                    m_haveYaw = true;
                }
                break;
            }
            case 2: // LOCALIZATION：仅记录，不参与是否跟踪
                m_localized = var->asBool();
                break;
            case 3: { // GOAL：[gx,gy,gyaw]；不足 3 维时 yaw 保持原值
                const QVector<float> v = var->asFloats(0);
                const double x = static_cast<double>(v.value(0));
                const double y = static_cast<double>(v.value(1));
                const double yaw = v.size() >= 3 ? static_cast<double>(v.at(2)) : m_goalYaw;
                applyGoal(x, y, yaw);
                return;
            }
            case 4: // ENABLE：导航总开关（setEnable 内已 recompute）
                setEnable(var->asBool());
                return;
            default:
                break;
            }
            recompute();
        }

        QWidget *embeddedWidget() override { return widget; }

        QJsonObject save() const override
        {
            QJsonObject values;
            values[QStringLiteral("Goal")] = QJsonArray{m_goalX, m_goalY, m_goalYaw};
            values[QStringLiteral("MaxVx")] = m_maxVx;
            values[QStringLiteral("MaxSteer")] = m_maxSteer;
            values[QStringLiteral("ArriveDist")] = m_arriveDist;
            values[QStringLiteral("ArriveHyst")] = widget->arriveHystSpin->value();
            values[QStringLiteral("SlowDist")] = widget->slowDistSpin->value();
            values[QStringLiteral("Wheelbase")] = widget->wheelbaseSpin->value();
            values[QStringLiteral("KpSteer")] = widget->kpSteerSpin->value();
            values[QStringLiteral("YawTol")] = widget->yawTolSpin->value();
            values[QStringLiteral("LookAhead")] = widget->lookAheadSpin->value();
            values[QStringLiteral("MirrorFrame")] = widget->mirrorFrameCheck->isChecked();

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
            const QJsonArray g = v[QStringLiteral("Goal")].toArray();
            applyGoal(g.at(0).toDouble(),
                      g.at(1).toDouble(),
                      g.size() >= 3 ? g.at(2).toDouble() : 0.0);
            setMaxVx(v[QStringLiteral("MaxVx")].toDouble(0.3));
            setMaxSteer(v[QStringLiteral("MaxSteer")].toDouble(0.5));
            setArriveDist(v[QStringLiteral("ArriveDist")].toDouble(0.15));
            widget->arriveHystSpin->setValue(v[QStringLiteral("ArriveHyst")].toDouble(0.05));
            widget->slowDistSpin->setValue(v[QStringLiteral("SlowDist")].toDouble(0.8));
            widget->wheelbaseSpin->setValue(v[QStringLiteral("Wheelbase")].toDouble(0.494));
            widget->kpSteerSpin->setValue(v[QStringLiteral("KpSteer")].toDouble(1.2));
            widget->yawTolSpin->setValue(v[QStringLiteral("YawTol")].toDouble(0.15));
            widget->lookAheadSpin->setValue(v[QStringLiteral("LookAhead")].toDouble(0.45));
            widget->mirrorFrameCheck->setChecked(v[QStringLiteral("MirrorFrame")].toBool(true));
            m_needReplan = true;
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
        /** OSC 命令入口：按地址末段分发到对应 setter */
        void onGlobalEvent(const GlobalEvent &ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }
            const QString localPath = ev.address.mid(ev.address.lastIndexOf(QLatin1Char('/')) + 1);
            if (localPath == QLatin1String("goal")) {
                // 向量目标：[x,y,yaw] 或 {x,y,yaw}
                if (ev.payload.canConvert<QVariantList>()) {
                    setGoal(ev.payload.toList());
                } else if (ev.payload.typeId() == QMetaType::QVariantMap) {
                    const QVariantMap m = ev.payload.toMap();
                    applyGoal(m.value(QStringLiteral("x"), m_goalX).toDouble(),
                              m.value(QStringLiteral("y"), m_goalY).toDouble(),
                              m.value(QStringLiteral("yaw"), m_goalYaw).toDouble());
                }
            } else if (localPath == QLatin1String("maxVx")) {
                setMaxVx(ev.payload.toDouble());
            } else if (localPath == QLatin1String("maxSteer")) {
                setMaxSteer(ev.payload.toDouble());
            } else if (localPath == QLatin1String("arriveDist")) {
                setArriveDist(ev.payload.toDouble());
            } else if (localPath == QLatin1String("enable")) {
                setEnable(ev.payload.toBool());
            }
        }

    signals:
        void goalChanged();
        void maxVxChanged(double);
        void maxSteerChanged(double);
        void arriveDistChanged(double);
        void enableChanged(bool);

    private:
        /**
         * @brief 原子更新目标位姿 [x,y,yaw]，并标记需要重新 RS 规划
         */
        void applyGoal(double x, double y, double yaw)
        {
            const bool sameX = qFuzzyCompare(m_goalX + 1.0, x + 1.0);
            const bool sameY = qFuzzyCompare(m_goalY + 1.0, y + 1.0);
            const bool sameYaw = qFuzzyCompare(m_goalYaw + 1.0, yaw + 1.0);
            if (sameX && sameY && sameYaw) {
                return;
            }

            m_arrivedFired = false;
            m_inArriveZone = false;
            m_needReplan = true;
            m_path = {};
            m_pathPlotDirty = true;
            m_goalX = x;
            m_goalY = y;
            m_goalYaw = yaw;

            {
                const QSignalBlocker bx(widget->goalXSpin);
                const QSignalBlocker by(widget->goalYSpin);
                const QSignalBlocker bz(widget->goalYawSpin);
                widget->goalXSpin->setValue(m_goalX);
                widget->goalYSpin->setValue(m_goalY);
                widget->goalYawSpin->setValue(m_goalYaw);
            }

            emit goalChanged();
            recompute();
        }

        void registerBinding(const QString &path, const char *member, QWidget *control)
        {
            NodeDelegateModel::ExternalBinding b;
            b.member = QString::fromUtf8(member);
            b.control = control;
            AbstractDelegateModel::registerExternalBinding(path, this, b);
        }

        /**
         * @brief 将角度折到 [-π, π]
         *
         * 用于航向误差，保证最短转向方向（|error| ≤ π）。
         */
        static double wrapAngle(double a)
        {
            constexpr double kPi = 3.14159265358979323846;
            // fmod 对负数余数可能落在负区间，需再抬到 [0, 2π) 再减 π
            a = std::fmod(a + kPi, 2.0 * kPi);
            if (a < 0.0) {
                a += 2.0 * kPi;
            }
            return a - kPi;
        }

        /// 控制系位姿：可选镜像 (y,yaw)→(-y,-yaw)
        Rs::Pose2d controlPose(double x, double y, double yaw) const
        {
            if (widget->mirrorFrameCheck->isChecked()) {
                return {x, -y, -yaw};
            }
            return {x, y, yaw};
        }

        /// R_min = L_eff / tan(δ_max)，L_eff = L/2（前后对向阿克曼）
        double turningRadius() const
        {
            const double wheelbase = std::max(0.05, widget->wheelbaseSpin->value());
            const double lEff = 0.5 * wheelbase;
            const double steer = std::max(0.05, m_maxSteer);
            return lEff / std::tan(steer);
        }

        /**
         * @brief Pure Pursuit：最近点 + 预瞄点 → STEER；gear 来自路径段
         * @param yawError 相对预瞄点航向误差（跟线用）
         * @param pathAlignError 相对路径最近点航向误差（重规划用）
         * @return gear：+1 前进，-1 倒车
         */
        int purePursuit(const Rs::Pose2d &pose, double lEff, double &vx, double &steer,
                        double &yawError, double &crossTrack, double &pathAlignError) const
        {
            vx = 0.0;
            steer = 0.0;
            yawError = 0.0;
            crossTrack = 0.0;
            pathAlignError = 0.0;
            if (m_path.samples.empty()) {
                return 1;
            }

            // 找路径上最近采样点，作为横向偏差、档位、路径航向对齐来源
            size_t nearest = 0;
            double bestD = std::numeric_limits<double>::infinity();
            for (size_t i = 0; i < m_path.samples.size(); ++i) {
                const double d = std::hypot(m_path.samples[i].x - pose.x,
                                           m_path.samples[i].y - pose.y);
                if (d < bestD) {
                    bestD = d;
                    nearest = i;
                }
            }
            crossTrack = bestD;
            const auto &np = m_path.samples[nearest];
            const int gear = np.gear >= 0 ? 1 : -1;
            pathAlignError = wrapAngle(np.yaw - pose.yaw);

            // 沿路径向前累加弧长，取预瞄距离处的点
            const double Ld = std::max(0.15, widget->lookAheadSpin->value());
            size_t target = nearest;
            double acc = 0.0;
            for (size_t i = nearest; i + 1 < m_path.samples.size(); ++i) {
                acc += std::hypot(m_path.samples[i + 1].x - m_path.samples[i].x,
                                  m_path.samples[i + 1].y - m_path.samples[i].y);
                target = i + 1;
                if (acc >= Ld) {
                    break;
                }
            }

            const auto &tp = m_path.samples[target];
            const double dx = tp.x - pose.x;
            const double dy = tp.y - pose.y;
            const double c = std::cos(pose.yaw);
            const double s = std::sin(pose.yaw);
            // 车体坐标：localX 前向，localY 左侧
            const double localX = c * dx + s * dy;
            const double localY = -s * dx + c * dy;
            // 倒车时改从车尾看预瞄点，避免 α≈±π 导致转角乱跳
            const double alpha = (gear >= 0)
                                     ? std::atan2(localY, localX)
                                     : std::atan2(-localY, -localX);
            yawError = wrapAngle(tp.yaw - pose.yaw);

            // 经典 Pure Pursuit：δ = atan(2 L_eff sin(α) / Ld)
            const double denom = std::max(0.15, Ld);
            steer = std::atan2(2.0 * lEff * std::sin(alpha), denom);
            if (gear < 0) {
                steer = -steer; // 与 VX<0 的自行车模型同号约定对齐
            }
            steer = std::clamp(steer, -m_maxSteer, m_maxSteer);

            // 近目标降速；大曲率（α 大）时再降一点
            const double distGoal = std::hypot(m_path.samples.back().x - pose.x,
                                               m_path.samples.back().y - pose.y);
            const double slowDist = std::max(0.05, widget->slowDistSpin->value());
            const double speedScale = std::clamp(distGoal / slowDist, 0.15, 1.0);
            constexpr double kPi = 3.14159265358979323846;
            const double turnScale = std::clamp(1.0 - 0.5 * (std::abs(alpha) / kPi), 0.25, 1.0);
            vx = static_cast<double>(gear) * m_maxVx * speedScale * turnScale;
            Q_UNUSED(localX);
            return gear;
        }

        /**
         * @brief 核心：RS 规划（按需）+ Pure Pursuit 跟踪 + 位姿到达判定
         */
        void recompute()
        {
            const bool active = m_enable && m_havePose && m_haveYaw;

            double vx = 0.0;
            double steer = 0.0;
            bool arrived = false;
            bool arrivedPulse = false;
            bool reversing = false;
            double distance = 0.0;
            double yawError = 0.0;
            double crossTrack = 0.0;
            double pathAlignError = 0.0;
            QString modeText = QStringLiteral("空闲");

            if (!active) {
                m_arrivedFired = false;
                m_inArriveZone = false;
                m_needReplan = true;
                m_path = {};
                m_lastReplanMs = 0;
                m_pathPlotDirty = true;
            } else {
                // 在控制系（可选镜像）下规划与跟踪
                const Rs::Pose2d start = controlPose(m_posX, m_posY, m_yaw);
                const Rs::Pose2d goal = controlPose(m_goalX, m_goalY, m_goalYaw);
                distance = std::hypot(goal.x - start.x, goal.y - start.y);
                const double yawErrGoal = wrapAngle(goal.yaw - start.yaw);
                yawError = yawErrGoal;

                const double leaveDist = m_arriveDist + std::max(0.0, widget->arriveHystSpin->value());
                const double yawTol = std::max(0.02, widget->yawTolSpin->value());
                const bool posOk = distance <= m_arriveDist;
                const bool yawOk = std::abs(yawErrGoal) <= yawTol;

                // 到达条件：位置 + 航向；滞回防抖（SLAM 边界抖动）
                if (!m_inArriveZone) {
                    if (posOk && yawOk) {
                        m_inArriveZone = true;
                    }
                } else if (distance > leaveDist || std::abs(yawErrGoal) > yawTol * 1.5) {
                    m_inArriveZone = false;
                    m_arrivedFired = false;
                }

                if (m_inArriveZone) {
                    arrived = true;
                    vx = 0.0;
                    steer = 0.0;
                    modeText = QStringLiteral("已到达");
                    if (!m_arrivedFired) {
                        m_arrivedFired = true;
                        arrivedPulse = true; // ARRIVED 口只脉冲一次 true
                    }
                } else {
                    const double lEff = 0.5 * std::max(0.05, widget->wheelbaseSpin->value());
                    const double rMin = turningRadius();

                    // 仅在 needReplan / 路径无效时规划。
                    // 注意：不要做“定时重规划”——SLAM 静止时位姿仍抖动，
                    // 每 0.5s 从新起点规划会表现为不停重规划。
                    if (m_needReplan || !m_path.ok || m_path.samples.empty()) {
                        m_path = Rs::plan(start, goal, rMin, 0.08);
                        m_needReplan = false;
                        m_lastReplanMs = QDateTime::currentMSecsSinceEpoch();
                        m_pathPlotDirty = true;
                    }

                    if (!m_path.ok || m_path.samples.empty()) {
                        modeText = QStringLiteral("规划失败");
                        vx = 0.0;
                        steer = 0.0;
                    } else {
                        const int gear = purePursuit(start, lEff, vx, steer, yawError, crossTrack,
                                                     pathAlignError);
                        reversing = gear < 0;
                        modeText = reversing ? QStringLiteral("RS倒车跟踪") : QStringLiteral("RS前进跟踪");

                        // 近终点：PP 航向弱，用慢速 + 航向 P 纠偏（阿克曼需 |v|>0 才能转 yaw）
                        if (distance < std::max(0.40, 2.5 * m_arriveDist)
                            && std::abs(yawErrGoal) > yawTol) {
                            const double creep = std::copysign(std::min(0.12, m_maxVx), vx == 0.0 ? 1.0 : vx);
                            const double thetaDot = 1.5 * yawErrGoal;
                            vx = creep;
                            steer = std::clamp(std::atan(thetaDot * lEff / creep), -m_maxSteer, m_maxSteer);
                            modeText = QStringLiteral("近点航向纠偏");
                        }

                        // 相对路径最近点：距离过大 或 航向差≥约90° → 重规划（冷却 1s）
                        const double crossTol = std::max(0.6, 3.0 * m_arriveDist);
                        constexpr double kPathYawReplanRad = 1.5707963267948966; // π/2
                        const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
                        const bool crossBad = crossTrack > crossTol;
                        const bool yawBad = std::abs(pathAlignError) > kPathYawReplanRad;
                        if ((crossBad || yawBad) && (nowMs - m_lastReplanMs) >= 1000) {
                            m_needReplan = true;
                            if (crossBad && yawBad) {
                                modeText = QStringLiteral("位姿偏差·重规划");
                            } else if (crossBad) {
                                modeText = QStringLiteral("横向偏差·重规划");
                            } else {
                                modeText = QStringLiteral("航向偏差·重规划");
                            }
                        }
                    }
                }
            }

            const bool outEnable = active && !arrived;

            m_vxOut = std::make_shared<VariableData>(vx);
            m_steerOut = std::make_shared<VariableData>(steer);
            m_enableOut = std::make_shared<VariableData>(outEnable);
            m_arrivedOut = std::make_shared<VariableData>(arrivedPulse);

            QVariantMap status;
            status.insert(QStringLiteral("active"), active);
            status.insert(QStringLiteral("arrived"), arrived);
            status.insert(QStringLiteral("arrived_pulse"), arrivedPulse);
            status.insert(QStringLiteral("tracking"), outEnable);
            status.insert(QStringLiteral("reversing"), reversing);
            status.insert(QStringLiteral("mode"), modeText);
            status.insert(QStringLiteral("plan_ok"), m_path.ok);
            status.insert(QStringLiteral("path_length_m"), m_path.length);
            status.insert(QStringLiteral("path_types"), QString::fromStdString(m_path.typeString));
            status.insert(QStringLiteral("sample_count"), static_cast<int>(m_path.samples.size()));
            status.insert(QStringLiteral("cross_track_m"), crossTrack);
            status.insert(QStringLiteral("path_align_yaw_rad"), pathAlignError);
            status.insert(QStringLiteral("r_min_m"), turningRadius());
            status.insert(QStringLiteral("l_eff_m"), 0.5 * widget->wheelbaseSpin->value());
            status.insert(QStringLiteral("distance_m"), distance);
            status.insert(QStringLiteral("yaw_error_rad"), yawError);
            status.insert(QStringLiteral("vx_m_s"), vx);
            status.insert(QStringLiteral("steer_rad"), steer);
            status.insert(QStringLiteral("localized"), m_localized);
            status.insert(QStringLiteral("pos_x"), m_posX);
            status.insert(QStringLiteral("pos_y"), m_posY);
            status.insert(QStringLiteral("goal_x"), m_goalX);
            status.insert(QStringLiteral("goal_y"), m_goalY);
            status.insert(QStringLiteral("goal_yaw"), m_goalYaw);
            status.insert(QStringLiteral("default"), distance);
            m_statusOut = std::make_shared<VariableData>(status);

            widget->setStatusText(
                QStringLiteral("d=%1 m  yawErr=%2  vx=%3  %4")
                    .arg(distance, 0, 'f', 3)
                    .arg(yawError, 0, 'f', 3)
                    .arg(vx, 0, 'f', 3)
                    .arg(modeText));

            refreshPathPlot(active);

            Q_EMIT dataUpdated(0);
            Q_EMIT dataUpdated(1);
            Q_EMIT dataUpdated(2);
            Q_EMIT dataUpdated(3);
            Q_EMIT dataUpdated(4);
        }

        /// 限频刷新路径缓存；弹窗打开时才 replot
        void refreshPathPlot(bool active)
        {
            const qint64 nowMs = QDateTime::currentMSecsSinceEpoch();
            // 始终限频更新缓存，保证打开弹窗时数据够新
            const bool due = m_pathPlotDirty || (nowMs - m_lastPathPlotMs) >= 100;
            if (!due) {
                return;
            }
            m_lastPathPlotMs = nowMs;

            const Rs::Pose2d pose = controlPose(m_posX, m_posY, m_yaw);
            const Rs::Pose2d goal = controlPose(m_goalX, m_goalY, m_goalYaw);
            const bool forceRescale = m_pathPlotDirty;
            m_pathPlotDirty = false;

            if (!active) {
                widget->clearPathPlot();
                return;
            }

            QVector<double> xs;
            QVector<double> ys;
            QVector<int> gears;
            if (m_path.ok && !m_path.samples.empty()) {
                xs.reserve(static_cast<int>(m_path.samples.size()));
                ys.reserve(static_cast<int>(m_path.samples.size()));
                gears.reserve(static_cast<int>(m_path.samples.size()));
                for (const auto &s : m_path.samples) {
                    xs.append(s.x);
                    ys.append(s.y);
                    gears.append(s.gear);
                }
            }

            widget->updatePathPlot(xs,
                                   ys,
                                   gears,
                                   pose.x,
                                   pose.y,
                                   pose.yaw,
                                   goal.x,
                                   goal.y,
                                   goal.yaw,
                                   forceRescale);
        }

        NavControllerInterface *widget = new NavControllerInterface();

        std::shared_ptr<VariableData> m_vxOut;
        std::shared_ptr<VariableData> m_steerOut;
        std::shared_ptr<VariableData> m_enableOut;
        std::shared_ptr<VariableData> m_statusOut;
        std::shared_ptr<VariableData> m_arrivedOut;

        double m_goalX = 0.0;
        double m_goalY = 0.0;
        double m_goalYaw = 0.0;
        double m_maxVx = 0.3;
        double m_maxSteer = 0.5;
        double m_arriveDist = 0.15;
        bool m_enable = false;

        double m_posX = 0.0;
        double m_posY = 0.0;
        double m_yaw = 0.0;
        bool m_havePose = false;
        bool m_haveYaw = false;
        bool m_localized = false;
        bool m_arrivedFired = false;  // ARRIVED 是否已对本目标脉冲过
        bool m_inArriveZone = false;  // 滞回到点区状态
        bool m_needReplan = true;     // 下次 recompute 重新跑 RS
        qint64 m_lastReplanMs = 0;    // 周期重规划时间戳（ms）
        bool m_pathPlotDirty = true;  // 路径变更后强制 rescale
        qint64 m_lastPathPlotMs = 0;  // 路径图限频
        Rs::PathResult m_path;        // 当前 RS 路径与采样
    };
}
