#pragma once

#include "Common/DataTypes/NodeDataList.hpp"
#include "Common/DataTypes/SyncData.h"
#include "Common/Devices/TimestampGenerator/TimestampGenerator.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"

#include <QtNodes/NodeDelegateModel>

#include <QElapsedTimer>
#include <QList>
#include <QPointer>
#include <QTimer>

// 一维曲线数据使用本插件 Core/ 下的独立 CurvePath（向心 Catmull-Rom 容器，源出 TimeLineNode 引擎、已整份复制自包含）；
// 分层：Curve1DCore = 核心采样算法（纯数据层，outData 用）；Curve1DEditor = 编辑器 GUI；
// Curve1DPanel / Curve1DDialog = 节点内嵌面板与编辑弹窗（纯 GUI 层，从本模型分离）。
#include "CurvePath.hpp"
#include "Curve1DCore.hpp"
#include "Curve1DPanel.hpp"
#include "Curve1DDialog.hpp"

#include <algorithm>
#include <cmath>

using QtNodes::ConnectionPolicy;
using QtNodes::NodeData;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes
{
    /**
     * @brief 一维值曲线节点（Curve1D，多曲线版）。
     *
     * 与 Curve3D/Curve2D 同模式：支持 1~16 条曲线，每条曲线独占一个输出端口。
     * 差异仅在于输出投影——本节点输出 **值 f(progress)**：X 轴即进度（0~1），
     * 输出 = 曲线在 X=progress 处的 Y 值（按 X 线性插值，端点外保持端值），
     * 适合标量包络、亮度/音量曲线、单参数渐变等场景。
     * 所有曲线共享同一播放引擎（progress / 周期 / 采样率 / 循环），同步推进。
     *
     * - 输入 START（真值触发启动）、STOP（真值触发停止）；
     * - 输入 SYNC（SyncData）：连接后播放头由同步源接管（播放态联动 + 位置按周期映射）；
     * - 输出：每个端口对应一条曲线的 Y 采样值（端口数 = 曲线数，随增删动态变化）。
     */
    class Curve1DDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        // progress 不做成 Q_PROPERTY：不进入属性/外部绑定系统（右键外部控制不显示、实例化不推送）；
        // 读写仍走 progress()/setProgress() 普通成员，progressChanged 信号保留备用
        Q_PROPERTY(double period READ period WRITE setPeriod NOTIFY periodChanged)
        Q_PROPERTY(bool loop READ loop WRITE setLoop NOTIFY loopChanged)
        Q_PROPERTY(bool play READ play WRITE setPlay NOTIFY playChanged)
        Q_PROPERTY(int sampleRate READ sampleRate WRITE setSampleRate NOTIFY sampleRateChanged)

        enum InputPort : PortIndex {
            StartPort = 0,
            StopPort = 1,
            SyncPort = 2
        };

        static constexpr int kMinCurves = 1;
        static constexpr int kMaxCurves = 16;

    public:
        Curve1DDataModel()
            : m_nodePanel(new Curve1DPanel(this))
        {
            InPortCount = 3;
            OutPortCount = 2;   // 端口 0 = SYNC 时钟源、端口 1 = PROGRESS 回放进度（曲线端口 2..N+1 随曲线数动态追加）
            CaptionVisible = true;
            Caption = QStringLiteral("Curve1D");
            WidgetEmbeddable = true;
            Resizable = false;
            PortEditable = false;
            // 双击节点保留库默认的展开/折叠行为；仅「编辑曲线」按钮弹出独立编辑窗口

            // 默认一条曲线（加载期不通知端口，同 AudioCrossFader 的 load 模式）
            applyCurveCount(kMinCurves, false);

            // 周期播放引擎：tick 间隔由采样率决定（1000/采样率，采样率 1~100 Hz，默认 30）
            m_playTimer = new QTimer(this);
            m_playTimer->setTimerType(Qt::PreciseTimer);
            m_playTimer->setInterval(int(1000.0 / m_sampleRate));
            connect(m_playTimer, &QTimer::timeout, this, &Curve1DDataModel::onPlaybackTick);

            // 同步跟随：25Hz 轮询同步源（播放态联动 + 位置纠偏），同 TimeLineNode 从机模式
            m_syncWatcher = new QTimer(this);
            m_syncWatcher->setInterval(40);
            connect(m_syncWatcher, &QTimer::timeout, this, &Curve1DDataModel::onSyncWatchTick);
            m_syncWatcher->start();

            // 初始输出推送（进度条/时间读数随 publishOutputs 同步，UI 侧细节见 Curve1DPanel）
            publishOutputs();

            // progress 不注册为外部控制属性：实例化不推送初始值、右键外部控制菜单不显示；
            // 外部 OSC 写 /progress 定位播放头仍有效（parseOSC → Command 广播 → onGlobalEvent 手动匹配）；
            // 下游取实时进度请连 PROGRESS 输出端口
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = QStringLiteral("period");
                AbstractDelegateModel::registerExternalBinding(QStringLiteral("/period"), this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = QStringLiteral("loop");
                b.control = m_nodePanel->loopBox();
                AbstractDelegateModel::registerExternalBinding(QStringLiteral("/loop"), this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = QStringLiteral("play");
                b.control = m_nodePanel->playButton();
                AbstractDelegateModel::registerExternalBinding(QStringLiteral("/play"), this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = QStringLiteral("sampleRate");
                AbstractDelegateModel::registerExternalBinding(QStringLiteral("/sampleRate"), this, b);
            }
        }

        ~Curve1DDataModel() override
        {
            GlobalEventBus::instance()->unsubscribe(this);
            if (m_playTimer) {
                m_playTimer->stop();
            }
            if (m_syncWatcher) {
                m_syncWatcher->stop();
            }
            if (m_dialog) {
                m_dialog->close();
                // 同步销毁：deleteLater 依赖事件循环，应用退出（closingDown）时不会执行，
                // 会导致顶层弹窗 + 编辑器泄漏；销毁后全部相关 QPointer 自动置空。
                delete m_dialog;
                m_dialog = nullptr;
            }
            // 立即删除曲线对象（QObject 析构时自动从 children 移除，避免依赖父类析构的滞后期）
            qDeleteAll(m_paths);
            m_paths.clear();
            m_nodePanel = nullptr;
        }

        QString portCaption(PortType portType, PortIndex portIndex) const override
        {
            if (portType == PortType::In) {
                switch (portIndex) {
                case StartPort: return QStringLiteral("START");
                case StopPort:  return QStringLiteral("STOP");
                case SyncPort:  return QStringLiteral("SYNC");
                default:        break;
                }
            } else if (portType == PortType::Out) {
                // 端口 0 = SYNC 时钟源、端口 1 = PROGRESS 回放进度；曲线端口从 2 开始（增删曲线只在末尾操作，前两端口连线稳定）
                if (portIndex == 0) {
                    return QStringLiteral("SYNC");
                }
                if (portIndex == 1) {
                    return QStringLiteral("PROGRESS");
                }
                const int index = int(portIndex) - 2;
                if (index >= 0 && index < m_paths.size()) {
                    return QStringLiteral("曲线 %1").arg(index + 1);
                }
            }
            return {};
        }

        NodeDataType dataType(PortType portType, PortIndex portIndex) const override
        {
            if (portType == PortType::In) {
                return portIndex == SyncPort ? SyncData().type() : VariableData().type();
            }
            // 输出端口 0 = SYNC 时钟源（播放头秒位置 + 播放态）、端口 1 = PROGRESS 回放进度，其余为各曲线采样输出
            return portIndex == 0 ? SyncData().type() : VariableData().type();
        }

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            if (int(port) == 0) {
                // SYNC 时钟源输出口：下游节点的 SYNC 端口接入后跟随本节点播放头
                return std::make_shared<SyncData>(m_syncState);
            }
            if (int(port) == 1) {
                // PROGRESS 输出口：回放进度（0~1），播放/拖拽/同步跟随均实时更新
                return std::make_shared<VariableData>(m_progress);
            }
            // 一维投影：X 轴即进度（0~1），输出 = 曲线在 X=progress 处的 Y 值
            // （按 X 线性插值，端点外保持端值——真"值 f(progress)"语义）
            const int index = int(port) - 2;
            CurvePath* path = (index >= 0 && index < m_paths.size())
                                  ? m_paths.at(index) : nullptr;
            return std::make_shared<VariableData>(curve1DValueAt(path, m_progress));
        }

        void setInData(std::shared_ptr<NodeData> data, PortIndex const portIndex) override
        {
            switch (portIndex) {
            case StartPort:
                if (asBool(data)) {
                    setPlay(true);
                }
                break;
            case StopPort:
                if (asBool(data)) {
                    setPlay(false);
                }
                break;
            case SyncPort: {
                // 连线即取得同步源共享句柄；断线（data == nullptr）则交还本地时钟
                auto sd = data ? std::dynamic_pointer_cast<SyncData>(data) : nullptr;
                m_syncSource = sd ? sd->getSharedState() : nullptr;
                break;
            }
            default:
                break;
            }
        }

        QWidget* embeddedWidget() override
        {
            return m_nodePanel;
        }

        QJsonObject save() const override
        {
            QJsonObject modelJson = NodeDelegateModel::save();
            QJsonObject values;
            QJsonArray curves;
            for (const CurvePath* path : m_paths) {
                QJsonObject curve;
                curve[QStringLiteral("points")] = path->toJson();
                curve[QStringLiteral("closed")] = path->isClosed();
                curves.append(curve);
            }
            values[QStringLiteral("curves")] = curves;
            values[QStringLiteral("progress")] = m_progress;
            values[QStringLiteral("period")] = m_period;
            values[QStringLiteral("loop")] = m_loop;
            values[QStringLiteral("sampleRate")] = m_sampleRate;
            modelJson[QStringLiteral("values")] = values;
            return modelJson;
        }

        void load(const QJsonObject& p) override
        {
            NodeDelegateModel::load(p);

            const QJsonValue v = p.value(QStringLiteral("values"));
            if (!v.isObject()) {
                return;
            }
            const QJsonObject values = v.toObject();

            if (values.contains(QStringLiteral("period"))) {
                setPeriod(values.value(QStringLiteral("period")).toDouble());
            }
            if (values.contains(QStringLiteral("loop"))) {
                setLoop(values.value(QStringLiteral("loop")).toBool(false));
            }
            if (values.contains(QStringLiteral("sampleRate"))) {
                setSampleRate(values.value(QStringLiteral("sampleRate")).toInt(30));
            }

            // 多曲线存档：values.curves = [{points, closed}, ...]
            const QJsonArray curves = values.value(QStringLiteral("curves")).toArray();
            if (!curves.isEmpty()) {
                const int target = qBound(kMinCurves, int(curves.size()), kMaxCurves);
                applyCurveCount(target, false);   // 加载期不通知端口
                for (int i = 0; i < target; ++i) {
                    const QJsonObject curve = curves.at(i).toObject();
                    const QJsonArray pts = curve.value(QStringLiteral("points")).toArray();
                    if (pts.isEmpty()) {
                        seedDefaultCurve(m_paths[i]);   // 空档回落 1D 斜坡（fromJson 空数组会回落自由曲线默认点）
                    } else {
                        m_paths[i]->fromJson(pts, curve.value(QStringLiteral("closed")).toBool(false));
                        normalizeCurve1D(m_paths[i]);   // 函数语义：X 升序 + 钳制 0~1 + 强制开放
                    }
                }
                m_activeCurve = 0;
            } else if (values.contains(QStringLiteral("points"))) {
                // 兼容旧版单曲线存档
                const QJsonArray pts = values.value(QStringLiteral("points")).toArray();
                if (pts.isEmpty()) {
                    seedDefaultCurve(m_paths.first());
                } else {
                    m_paths.first()->fromJson(pts, values.value(QStringLiteral("closed")).toBool(false));
                    normalizeCurve1D(m_paths.first());
                }
                m_activeCurve = 0;
            }

            if (values.contains(QStringLiteral("progress"))) {
                setProgress(values.value(QStringLiteral("progress")).toDouble());
            } else {
                publishOutputs();
            }
            if (m_dialog) {
                m_dialog->syncCurves();
                m_dialog->fitView();
                m_dialog->syncPointEditors();
            }
        }

    public slots:
        double progress() const { return m_progress; }

        double period() const { return m_period; }

        bool loop() const { return m_loop; }

        bool play() const { return m_play; }

        int sampleRate() const { return m_sampleRate; }

        int curveCount() const { return m_paths.size(); }

        int activeCurve() const { return m_activeCurve; }

        /** 曲线容器访问（GUI 层同步/编辑用；所有权仍在模型）。 */
        QList<CurvePath*> paths() const { return m_paths; }

        /** 当前选中曲线（GUI 层点编辑操作的目标；可能为空）。 */
        CurvePath* activePath() const
        {
            return (m_activeCurve >= 0 && m_activeCurve < m_paths.size())
                       ? m_paths.at(m_activeCurve) : nullptr;
        }

        /** 打开独立的一维曲线编辑弹窗（节点面板按钮共用入口）。 */
        Q_INVOKABLE void openEditor()
        {
            if (!m_dialog) {
                m_dialog = new Curve1DDialog(this);
            }
            if (!m_dialog) {
                return;
            }
            m_dialog->show();
            m_dialog->raise();
            m_dialog->activateWindow();
        }

        void setProgress(double v)
        {
            v = qBound(0.0, v, 1.0);
            if (qFuzzyCompare(v + 1.0, m_progress + 1.0)) {
                return;
            }
            m_progress = v;
            Q_EMIT progressChanged(m_progress);
            publishOutputs();
        }

        void setPeriod(double seconds)
        {
            seconds = qBound(0.1, seconds, 3600.0);
            if (qFuzzyCompare(seconds + 1.0, m_period + 1.0)) {
                return;
            }
            m_period = seconds;
            // 弹窗数值框 / 编辑器 X 轴刻度 / 点编辑行范围随周期刷新（弹窗存在时）
            if (m_dialog) {
                m_dialog->setPeriodView(m_period);
            }
            Q_EMIT periodChanged(m_period);
            if (m_nodePanel) {
                m_nodePanel->setTimeView(m_progress, m_period);
            }
        }

        void setLoop(bool loop)
        {
            if (m_loop == loop) {
                return;
            }
            m_loop = loop;
            // 面板复选框由 loopChanged 信号驱动同步（Curve1DPanel 内部处理）
            Q_EMIT loopChanged(loop);
        }

        void setPlay(bool playing)
        {
            if (m_play == playing) {
                return;
            }
            m_play = playing;
            if (m_play) {
                // 单次模式已停在末尾 → 从头重播（否则首个 tick 立即再次停止）
                if (!m_loop && m_progress >= 1.0) {
                    setProgress(0.0);
                }
                m_tickClock.restart();
                m_playTimer->start();
            } else {
                m_playTimer->stop();
                setProgress(0.0);   // 停止自动返回最开始
            }
            Q_EMIT playChanged(m_play);
        }

        /** 采样率：播放时每秒输出更新次数（1~100 Hz，默认 30）。 */
        void setSampleRate(int rate)
        {
            rate = qBound(1, rate, 100);
            if (m_sampleRate == rate) {
                return;
            }
            m_sampleRate = rate;
            if (m_playTimer) {
                m_playTimer->setInterval(int(1000.0 / m_sampleRate));
            }
            // 弹窗数值框由模型推送同步（OSC / load 改动采样率时）
            if (m_dialog) {
                m_dialog->setRateView(m_sampleRate);
            }
            Q_EMIT sampleRateChanged(m_sampleRate);
        }

        /** 新增一条曲线（输出端口随之 +1）。 */
        void addCurve()
        {
            if (m_paths.size() >= kMaxCurves) {
                return;
            }
            applyCurveCount(m_paths.size() + 1, true);
            setActiveCurve(m_paths.size() - 1);
        }

        /** 删除指定曲线（至少保留一条；对应输出端口上的连线由节点图自动移除）。 */
        void removeCurve(int index)
        {
            if (index < 0 || index >= m_paths.size() || m_paths.size() <= kMinCurves) {
                return;
            }
            CurvePath* path = m_paths.at(index);

            Q_EMIT portsAboutToBeDeleted(PortType::Out, PortIndex(index + 2), PortIndex(index + 2));
            m_paths.removeAt(index);
            OutPortCount = static_cast<unsigned int>(m_paths.size() + 2);
            delete path;   // path 的信号连接随析构自动断开（编辑器侧为 QPointer，悬空安全）
            Q_EMIT portsDeleted();

            if (m_activeCurve >= m_paths.size()) {
                m_activeCurve = m_paths.size() - 1;
            }
            if (m_dialog) {
                m_dialog->syncCurves();
                m_dialog->syncPointEditors();
            }
            publishOutputs();
        }

        /** 选中要编辑的曲线（编辑器高亮切换，不动 Y 轴视图）。 */
        void setActiveCurve(int index)
        {
            if (index < 0 || index >= m_paths.size()) {
                return;
            }
            m_activeCurve = index;
            if (m_dialog) {
                m_dialog->syncCurves();
                m_dialog->syncPointEditors();
            }
        }

        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }
            const QString addrProgress = makeFullOscAddress(QStringLiteral("/progress"));
            const QString addrPeriod = makeFullOscAddress(QStringLiteral("/period"));
            const QString addrLoop = makeFullOscAddress(QStringLiteral("/loop"));
            const QString addrPlay = makeFullOscAddress(QStringLiteral("/play"));
            const QString addrStop = makeFullOscAddress(QStringLiteral("/stop"));
            const QString addrRate = makeFullOscAddress(QStringLiteral("/sampleRate"));
            if (ev.address == addrProgress) {
                setProgress(ev.payload.toDouble());
            } else if (ev.address == addrPeriod) {
                setPeriod(ev.payload.toDouble());
            } else if (ev.address == addrLoop) {
                setLoop(ev.payload.toBool());
            } else if (ev.address == addrPlay) {
                setPlay(ev.payload.toBool());
            } else if (ev.address == addrStop) {
                setPlay(false);
            } else if (ev.address == addrRate) {
                setSampleRate(ev.payload.toInt());
            }
        }

    signals:
        void progressChanged(double v);
        void periodChanged(double seconds);
        void loopChanged(bool loop);
        void playChanged(bool playing);
        void sampleRateChanged(int rate);

    protected:
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(makeFullOscAddress(QStringLiteral("/progress")),
                                                  this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress(QStringLiteral("/period")),
                                                  this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress(QStringLiteral("/loop")),
                                                  this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress(QStringLiteral("/play")),
                                                  this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress(QStringLiteral("/stop")),
                                                  this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress(QStringLiteral("/sampleRate")),
                                                  this, SLOT(onGlobalEvent(GlobalEvent)));
        }

    private:
        static bool asBool(const std::shared_ptr<NodeData>& data)
        {
            auto value = std::dynamic_pointer_cast<VariableData>(data);
            return value ? value->asBool() : false;
        }

        /**
         * 调整曲线数量（输出端口数随之变化）。
         * notifyPorts=true 时通过 portsAboutToBeInserted/Deleted 系列信号通知节点图
         * 动态增删端口（模式同 AudioCrossFaderDataModel::applyPortCount）。
         */
        void applyCurveCount(int target, bool notifyPorts)
        {
            target = qBound(kMinCurves, target, kMaxCurves);
            while (m_paths.size() < target) {
                auto* path = new CurvePath(this);
                seedDefaultCurve(path);   // 覆盖 CurvePath 面向自由曲线的默认点 → 1D 斜坡
                connect(path, &CurvePath::curveChanged, this, [this] {
                    publishOutputs();
                });
                connect(path, &CurvePath::selectionChanged, this, [this] {
                    if (m_dialog) {
                        m_dialog->syncPointEditors();
                    }
                });
                if (notifyPorts) {
                    // 曲线端口插在 SYNC/PROGRESS（端口 0/1）之后：端口 index = 现有曲线数 + 2
                    const unsigned int insertAt = static_cast<unsigned int>(m_paths.size()) + 2;
                    Q_EMIT portsAboutToBeInserted(PortType::Out, PortIndex(insertAt), PortIndex(insertAt));
                    m_paths.append(path);
                    OutPortCount = static_cast<unsigned int>(m_paths.size() + 2);
                    Q_EMIT portsInserted();
                } else {
                    m_paths.append(path);
                    OutPortCount = static_cast<unsigned int>(m_paths.size() + 2);
                }
            }
            while (m_paths.size() > target) {
                const int index = m_paths.size() - 1;
                CurvePath* path = m_paths.at(index);
                if (notifyPorts) {
                    Q_EMIT portsAboutToBeDeleted(PortType::Out, PortIndex(index + 2), PortIndex(index + 2));
                }
                m_paths.removeAt(index);
                OutPortCount = static_cast<unsigned int>(m_paths.size() + 2);
                delete path;
                if (notifyPorts) {
                    Q_EMIT portsDeleted();
                }
            }
            if (m_activeCurve >= m_paths.size()) {
                m_activeCurve = m_paths.size() - 1;
            }
            if (m_dialog) {
                m_dialog->syncCurves();
            }
        }

        /** 新建曲线的一维默认形状：0→1 线性斜坡（CurvePath 构造默认是自由曲线点，不合函数语义）。 */
        static void seedDefaultCurve(CurvePath* path)
        {
            QJsonArray points;
            auto append = [&points](double x, double y) {
                QJsonArray item;
                item.append(x);
                item.append(y);
                item.append(0.0);
                points.append(item);
            };
            append(0.0, 0.0);
            append(1.0, 1.0);
            path->fromJson(points, false);
        }

        void publishOutputs()
        {
            // 同步源发布：当前播放头位置（秒）+ 锚点帧 + 播放态，供下游 SYNC 从机外推跟随
            if (m_syncState) {
                auto* generator = TimestampGenerator::getInstance();
                m_syncState->write(m_progress * qMax(0.1, m_period),
                                   generator ? generator->getCurrentFrameCount() : 0,
                                   m_play);
            }

            for (unsigned int i = 0; i < OutPortCount; ++i) {
                Q_EMIT dataUpdated(static_cast<PortIndex>(i));
            }

            // UI 同步（进度条/时间读数/编辑器播放头）：面板与弹窗各自处理显示细节
            if (m_dialog) {
                m_dialog->setPlaybackFraction(m_progress);
                m_dialog->setProgressView(m_progress);
            }
            if (m_nodePanel) {
                m_nodePanel->setProgressView(m_progress);
                m_nodePanel->setTimeView(m_progress, m_period);
            }
        }

        /**
         * 周期播放引擎 tick：按采样网格推进并输出。
         * 进度量化到 1/(周期×采样率) 的格点，保证一个周期的更新次数
         * 恰好 = 周期 × 采样率（如 5s × 30Hz = 150 次）。
         * 循环模式格点序号取模回绕；非循环到达末尾自动停止。
         *
         * 追帧补点：拖拽窗口、弹窗开关等动作会短暂阻塞事件循环，QTimer 超时
         * 被合并——恢复后的一次 tick 可能跨越多个采样格点。此时从上一已输出
         * 格点逐格补推输出（而非直接跳到最新格点），阻塞期间缺失的采样点
         * 得以补全，输出点序列保持完整。
         *
         * SYNC 从机模式：播放头位置取自同步源时钟（连续外推），但输出仍由
         * 本定时器按本机采样网格量化 + 追帧补点推进——从机一个周期的输出
         * 点数与本地播放一致（周期 × 采样率），同周期同采样率时与主机逐点
         * 对齐。若走 40ms 轮询直接输出（≈25Hz 且无补点），从机输出点会
         * 比主机少很多。播放态起停联动在 onSyncWatchTick（轮询只做状态桥接）。
         */
        void onPlaybackTick()
        {
            if (!m_play) {
                return;
            }
            // SYNC 已连接：位置跟随同步源时钟，输出走本机采样网格（含追帧补点）
            if (m_syncSource) {
                const SyncState snap = m_syncSource->read();
                if (!snap.active || !snap.playing) {
                    return;
                }
                double frac = m_syncSource->expectedNowSec() / qMax(0.1, m_period);
                if (m_loop) {
                    frac = std::fmod(frac, 1.0);
                } else {
                    frac = qBound(0.0, frac, 1.0);
                }
                followSyncToFraction(frac);
                return;
            }
            if (!m_tickClock.isValid()) {
                m_tickClock.start();
                return;
            }
            const double dt = double(m_tickClock.elapsed()) / 1000.0;
            m_tickClock.restart();
            if (dt <= 0.0) {
                return;
            }
            const double period = qMax(0.1, m_period);
            const qint64 grid = qint64(std::llround(double(m_sampleRate) * period));

            // 游标对齐：外部（OSC / 数值框）改动 progress 后，连续游标与当前进度失配
            // 则重对齐到当前格点（循环比较相位，非循环比较钳制值）
            const qint64 currentIndex = qint64(std::llround(m_progress * double(grid)));
            const qint64 cursorPhase = m_loop ? (m_slotCursor % grid)
                                              : qBound<qint64>(0, m_slotCursor, grid);
            if (m_slotCursor < 0 || cursorPhase != currentIndex) {
                m_slotCursor = currentIndex;
            }

            // 连续格点游标推进：本次 tick 跨越的格点数 = dt × 采样率（事件循环被
            // 阻塞后 >1，逐格补推）；限幅最多补一个周期——循环模式下更长阻塞
            // 期间的输出本来就周期重复，无信息损失
            qint64 target = m_slotCursor + qint64(std::floor(dt * double(m_sampleRate) + 0.5));
            if (target - m_slotCursor > grid) {
                target = m_slotCursor + grid;
            }
            for (qint64 s = m_slotCursor + 1; s <= target; ++s) {
                if (!m_loop && s >= grid) {
                    setProgress(1.0);
                    setPlay(false);
                    return;
                }
                setProgress(double(s % grid) / double(grid));
            }
            m_slotCursor = target;
        }

        /**
         * SYNC 从机格点跟随：把同步源位置（连续外推值）映射到本机采样网格，
         * 从上一已输出格点逐格补推到当前格点——从机输出点数与本地播放一致
         * （一个周期 = 周期 × 采样率），轮询/事件阻塞造成的缺口也被补全。
         * 源位置跳变（主机拖进度条 / OSC 定位）幅度超过本 tick 实际流逝时间
         * 可解释的格数（dt × 采样率 + 2）时，判定为跳变：重对齐游标并直接
         * 定位，不做整段扫掠补推（与本地引擎外部改动 progress 的语义一致）。
         */
        void followSyncToFraction(double frac)
        {
            const double period = qMax(0.1, m_period);
            const qint64 grid = qint64(std::llround(double(m_sampleRate) * period));
            if (grid <= 0) {
                setProgress(frac);
                return;
            }
            const qint64 current = qint64(std::llround(frac * double(grid)));

            // 本 tick 实际流逝时间最多可解释 dt × 采样率（+2 容差）格的推进
            double dt = 0.0;
            if (m_tickClock.isValid()) {
                dt = double(m_tickClock.elapsed()) / 1000.0;
            }
            m_tickClock.restart();
            const qint64 maxAdvance = qint64(dt * double(m_sampleRate)) + 2;

            if (m_loop) {
                const qint64 curPhase = ((current % grid) + grid) % grid;
                if (m_slotCursor < 0) {
                    // 首次对齐：直接输出当前格点
                    m_slotCursor = curPhase;
                    setProgress(double(curPhase) / double(grid));
                    return;
                }
                qint64 delta = curPhase - (m_slotCursor % grid);
                if (delta < 0) {
                    delta += grid;         // 源已回绕
                }
                if (delta <= maxAdvance && delta > 0) {
                    // 正常推进/阻塞追帧：从上一格逐格补推
                    for (qint64 s = 1; s <= delta; ++s) {
                        setProgress(double((m_slotCursor + s) % grid) / double(grid));
                    }
                } else if (delta > 0) {
                    // 源跳变：重对齐游标并直接定位，不扫掠
                    setProgress(double(curPhase) / double(grid));
                }
                m_slotCursor += delta;
            } else {
                const qint64 target = qBound<qint64>(0, current, grid);
                if (m_slotCursor < 0) {
                    // 首次对齐：直接输出当前位置
                    m_slotCursor = target;
                    setProgress(frac);
                    return;
                }
                const qint64 cursor = qBound<qint64>(0, m_slotCursor, grid);
                if (target < cursor) {
                    // 源回退（主机定位/回绕）：重对齐并直接跟随
                    m_slotCursor = target;
                    setProgress(frac);
                    return;
                }
                if (target == cursor) {
                    return;                // 同格去重
                }
                if (target - cursor > maxAdvance) {
                    // 源前向跳变：重对齐，不扫掠
                    m_slotCursor = target;
                    setProgress(frac);
                    return;
                }
                for (qint64 s = cursor + 1; s <= target; ++s) {
                    setProgress(double(s) / double(grid));
                }
                m_slotCursor = target;
            }
        }

        /**
         * SYNC 跟随（从机模式，同 TimeLineNode）：
         * - 播放态联动：源起播则起播、源停止则停止（非循环且源已超周期时不起播，定格待命）；
         * - 位置跟随：由播放定时器按本机采样网格执行（onPlaybackTick 的 SYNC 分支 +
         *   followSyncToFraction），本轮询只做播放态桥接，不直接输出位置。
         */
        void onSyncWatchTick()
        {
            if (!m_syncSource) {
                return;
            }
            const SyncState snap = m_syncSource->read();
            if (!snap.active) {
                return;
            }
            if (snap.playing != m_play) {
                bool allowStart = true;
                if (snap.playing && !m_loop && m_syncSource->expectedNowSec() >= m_period) {
                    allowStart = false;
                }
                if (allowStart) {
                    setPlay(snap.playing);
                }
            }
            if (!snap.playing) {
                return;
            }
            // 位置跟随不在轮询里直接输出：轮询 40ms（≈25Hz）低于采样率且无
            // 追帧补点，直接 setProgress 会让从机输出点比主机少很多。
            // 位置输出由播放定时器按本机采样网格执行（onPlaybackTick SYNC 分支）。
        }

        QList<CurvePath*> m_paths;              // 每条曲线一个输出端口
        int m_activeCurve = 0;                  // 当前编辑选中的曲线
        QPointer<Curve1DPanel> m_nodePanel;     // 节点体内嵌面板（GUI 类，节点图拥有）
        QPointer<Curve1DDialog> m_dialog;       // 编辑弹窗（GUI 类，懒创建；模型析构时同步销毁）
        QTimer* m_playTimer = nullptr;          // 播放推进
        QTimer* m_syncWatcher = nullptr;        // 同步源轮询（40ms）
        QElapsedTimer m_tickClock;              // 播放步长计时
        qint64 m_slotCursor = -1;               // 连续采样格点游标（不回绕；-1=未对齐），阻塞后逐格补推用
        std::shared_ptr<SyncSharedState> m_syncSource;  // SYNC 输入共享句柄（从机跟随源）
        std::shared_ptr<SyncSharedState> m_syncState = std::make_shared<SyncSharedState>(QStringLiteral("Curve1D")); // 本节点作为同步源的状态（SYNC 输出端口）
        double m_progress = 0.0;
        double m_period = 5.0;                  // 一个完整周期（秒）
        bool m_loop = false;                    // 默认单次播放，勾选循环后才回绕
        bool m_play = false;
        int m_sampleRate = 30;                  // 每秒输出更新次数（1~100 Hz）
    };
}
