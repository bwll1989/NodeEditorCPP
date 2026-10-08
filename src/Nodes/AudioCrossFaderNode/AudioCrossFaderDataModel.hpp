#pragma once

#include <QtCore/QObject>
#include <QThread>
#include <QSignalBlocker>
#include <QPushButton>
#include <QButtonGroup>
#include <QAbstractButton>
#include <QJsonObject>

#include <QtNodes/NodeDelegateModel>
#include "Common/DataTypes/NodeDataList.hpp"
#include "Common/DataTypes/AudioTimestampRingQueue.h"
#include "AudioCrossFaderInterface.h"
#include "PluginDefinition.hpp"
#include "AudioCrossFaderWorker.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

struct GlobalEvent;

namespace Nodes {
    /**
     * @brief 多通道交叉淡化节点
     * Channels 路 A/B 成对输入（A1…AN, B1…BN），输出 Out 1…N，共用 Mix
     * 默认 Mix=0（输出 A）；Action 为互斥按钮 A→B / B→A
     * 控制输入：SWTCH B（bool→淡入 A/B）、MIX（0~1 直接设 Mix）
     */
    class AudioCrossFaderDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(int channels READ channels WRITE setChannels NOTIFY channelsChanged)
        Q_PROPERTY(double mix READ mix WRITE setMix NOTIFY mixChanged)
        Q_PROPERTY(double fadeMs READ fadeMs WRITE setFadeMs NOTIFY fadeMsChanged)
        Q_PROPERTY(int controlAction READ controlActionProperty WRITE setControlActionProperty NOTIFY controlActionChanged)

    public:
        AudioCrossFaderDataModel()
            : _worker(new AudioCrossFaderWorker())
            , _workerThread(new QThread(this))
        {
            widget = new AudioCrossFaderInterface();
            m_channels = AudioCrossFaderInterface::kDefaultChannels;
            InPortCount = static_cast<unsigned int>(m_channels * 2 + 2); // + SWTCH B + MIX
            OutPortCount = static_cast<unsigned int>(m_channels);
            CaptionVisible = true;
            WidgetEmbeddable = false;
            Resizable = true;
            PortEditable = false;
            Caption = PLUGIN_NAME;

            m_mix = widget->mixSpin->value();
            m_fadeMs = widget->fadeDurationSpin->value();
            m_controlAction = AudioCrossFaderInterface::kActionToA;

            registerBindings();

            _worker->initializeBuffers(m_channels);
            _worker->moveToThread(_workerThread);

            connect(_workerThread, &QThread::started, this, [this]() {
                QMetaObject::invokeMethod(_worker, "startProcessing", Qt::QueuedConnection);
                pushParamsToWorker();
            });

            connect(widget->channelsSpin, &IntDragValueWidget::valueChanged,
                    this, &AudioCrossFaderDataModel::setChannels);
            connect(this, &AudioCrossFaderDataModel::channelsChanged, this, [this](int value) {
                widget->channelsSpin->setValue(value);
                applyChannelCount(value, true);
            });

            connect(widget->mixSpin, &FloatDragValueWidget::valueChanged,
                    this, &AudioCrossFaderDataModel::setMix);
            connect(this, &AudioCrossFaderDataModel::mixChanged, this, [this](double) {
                {
                    const QSignalBlocker blocker(widget->mixSpin);
                    widget->mixSpin->setValue(m_mix);
                }
                QMetaObject::invokeMethod(_worker, "setMix", Qt::QueuedConnection, Q_ARG(double, m_mix));
                syncActionButtonsFromMix();
            });

            connect(widget->fadeDurationSpin, &FloatDragValueWidget::valueChanged,
                    this, &AudioCrossFaderDataModel::setFadeMs);
            connect(this, &AudioCrossFaderDataModel::fadeMsChanged, this, [this](double) {
                {
                    const QSignalBlocker blocker(widget->fadeDurationSpin);
                    widget->fadeDurationSpin->setValue(m_fadeMs);
                }
                QMetaObject::invokeMethod(_worker, "setFadeDuration",
                                          Qt::QueuedConnection, Q_ARG(double, m_fadeMs));
            });

            connect(widget->actionGroup, &QButtonGroup::idClicked,
                    this, &AudioCrossFaderDataModel::setControlActionProperty);
            connect(this, &AudioCrossFaderDataModel::controlActionChanged, this, [this](int) {
                syncActionButtons();
            });

            _workerThread->start();
        }

        ~AudioCrossFaderDataModel() override
        {
            if (_worker) {
                _worker->stopProcessing();
            }
            if (_workerThread && _workerThread->isRunning()) {
                _workerThread->quit();
                _workerThread->wait(3000);
            }
            if (_worker) {
                _worker->deleteLater();
            }
        }

        NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override
        {
            if (portType == PortType::In) {
                const int idx = static_cast<int>(portIndex);
                if (idx == switchPortIndex() || idx == mixPortIndex()) {
                    return VariableData().type();
                }
            }
            return AudioData().type();
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
            case PortType::In: {
                const int idx = static_cast<int>(portIndex);
                if (idx == switchPortIndex()) {
                    return QStringLiteral("SWTCH B");
                }
                if (idx == mixPortIndex()) {
                    return QStringLiteral("MIX");
                }
                if (idx < m_channels) {
                    return QStringLiteral("A%1").arg(idx + 1);
                }
                if (idx < m_channels * 2) {
                    return QStringLiteral("B%1").arg(idx - m_channels + 1);
                }
                break;
            }
            case PortType::Out:
                if (m_channels == 1) {
                    return QStringLiteral("Out");
                }
                return QStringLiteral("Out %1").arg(portIndex + 1);
            default:
                break;
            }
            return {};
        }

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            if (port >= OutPortCount) {
                return std::make_shared<AudioData>();
            }
            auto outputData = std::make_shared<AudioData>();
            auto outputBuffer = _worker->getOutputBuffer(static_cast<int>(port));
            if (outputBuffer) {
                outputData->setSharedAudioBuffer(outputBuffer);
            }
            return outputData;
        }

        void setInData(std::shared_ptr<NodeData> nodeData, PortIndex const port) override
        {
            if (port >= InPortCount) {
                return;
            }

            // Variable：SWTCH B 淡入切换；MIX 直接设 0~1
            if (static_cast<int>(port) == switchPortIndex()) {
                if (!nodeData) {
                    return;
                }
                applySwitchB(isSwitchToB(nodeData));
                return;
            }
            if (static_cast<int>(port) == mixPortIndex()) {
                if (!nodeData) {
                    return;
                }
                applyMixPort(nodeData);
                return;
            }

            auto audioData = std::dynamic_pointer_cast<AudioData>(nodeData);
            std::shared_ptr<AudioTimestampRingQueue> audioBuffer;
            if (audioData && audioData->isConnectedToSharedBuffer()) {
                audioBuffer = audioData->getSharedAudioBuffer();
            }

            QMetaObject::invokeMethod(_worker, "setInputBuffer",
                                      Qt::QueuedConnection,
                                      Q_ARG(int, static_cast<int>(port)),
                                      Q_ARG(std::shared_ptr<AudioTimestampRingQueue>, audioBuffer));
        }

        QWidget *embeddedWidget() override { return widget; }

        int channels() const { return m_channels; }
        double mix() const { return m_mix; }
        double fadeMs() const { return m_fadeMs; }
        int controlActionProperty() const { return m_controlAction; }

        void setChannels(int value)
        {
            value = qBound(AudioCrossFaderInterface::kMinChannels,
                           value,
                           AudioCrossFaderInterface::kMaxChannels);
            if (value == m_channels) {
                return;
            }
            m_channels = value;
            Q_EMIT channelsChanged(value);
        }

        void setMix(double value)
        {
            value = qBound(0.0, value, 1.0);
            if (qFuzzyCompare(value + 1.0, m_mix + 1.0)) {
                return;
            }
            m_mix = value;
            Q_EMIT mixChanged(value);
        }

        void setFadeMs(double ms)
        {
            ms = qBound(10.0, ms, 600000.0);
            if (qFuzzyCompare(ms + 1.0, m_fadeMs + 1.0)) {
                return;
            }
            m_fadeMs = ms;
            Q_EMIT fadeMsChanged(ms);
        }

        void setControlActionProperty(int action)
        {
            action = normalizeAction(action);
            const bool same = (action == m_controlAction);
            m_controlAction = action;
            executeControlAction(m_controlAction);
            if (!same) {
                Q_EMIT controlActionChanged(m_controlAction);
            } else {
                syncActionButtons();
            }
        }

        QJsonObject save() const override
        {
            QJsonObject values;
            values["channels"] = channels();
            values["mix"] = mix();
            values["fade_ms"] = fadeMs();
            values["action"] = controlActionProperty();

            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["values"] = values;
            return modelJson;
        }

        void load(QJsonObject const &p) override
        {
            NodeDelegateModel::load(p);

            QJsonObject values = p.value(QStringLiteral("values")).toObject();
            if (values.isEmpty()) {
                values = p;
            }

            if (values.contains("channels")) {
                const int count = qBound(AudioCrossFaderInterface::kMinChannels,
                                         values["channels"].toInt(m_channels),
                                         AudioCrossFaderInterface::kMaxChannels);
                m_channels = count;
                widget->channelsSpin->setValue(count);
                applyChannelCount(count, false);
            }

            if (values.contains("fade_ms")) {
                setFadeMs(values.value("fade_ms").toDouble());
            }
            if (values.contains("action")) {
                // 加载时只同步按钮状态，不立刻触发淡入淡出；Mix 以 Action 为准对齐
                m_controlAction = normalizeAction(values.value("action").toInt());
                syncActionButtons();
                snapMixToAction(/*pushToWorker=*/true);
            } else if (values.contains("mix")) {
                setMix(values.value("mix").toDouble());
            }
            // 重开后控制口重连只作基线，不覆盖已恢复的 A/B / Mix
            m_loadedFromFile = true;
            m_switchSeen = false;
            m_mixPortSeen = false;
        }

    signals:
        void channelsChanged(int value);
        void mixChanged(double value);
        void fadeMsChanged(double ms);
        void controlActionChanged(int action);

    protected:
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/channels"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/mix"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/fade_ms"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/action"), this, SLOT(onGlobalEvent(GlobalEvent)));
            GlobalEventBus::instance()->subscribe(makeFullOscAddress("/switch_b"), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

    private Q_SLOTS:
        void onGlobalEvent(const GlobalEvent &ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }
            if (ev.address == makeFullOscAddress("/channels")) {
                setChannels(ev.payload.toInt());
            } else if (ev.address == makeFullOscAddress("/mix")) {
                setMix(ev.payload.toDouble());
            } else if (ev.address == makeFullOscAddress("/fade_ms")) {
                setFadeMs(ev.payload.toDouble());
            } else if (ev.address == makeFullOscAddress("/action")) {
                setControlActionProperty(ev.payload.toInt());
            } else if (ev.address == makeFullOscAddress("/switch_b")) {
                applySwitchB(variantIsSwitchToB(ev.payload));
            }
        }

    private:
        /** SWTCH B 保持在 2N，兼容旧工程连线；MIX 为新增末路 2N+1 */
        int switchPortIndex() const { return m_channels * 2; }
        int mixPortIndex() const { return m_channels * 2 + 1; }

        static bool variantIsSwitchToB(const QVariant &value)
        {
            if (!value.isValid()) {
                return false;
            }
            if (value.typeId() == QMetaType::Bool) {
                return value.toBool();
            }
            if (value.canConvert<double>()) {
                return !qFuzzyIsNull(value.toDouble());
            }
            const QString text = value.toString().trimmed().toLower();
            return text == QLatin1String("true") || text == QLatin1String("1");
        }

        static bool isSwitchToB(const std::shared_ptr<NodeData> &nodeData)
        {
            auto var = std::dynamic_pointer_cast<VariableData>(nodeData);
            if (!var) {
                return false;
            }
            return variantIsSwitchToB(var->value());
        }

        static bool tryParseMixValue(const QVariant &value, double &outMix)
        {
            if (!value.isValid()) {
                return false;
            }
            if (value.typeId() == QMetaType::Bool) {
                outMix = value.toBool() ? 1.0 : 0.0;
                return true;
            }
            if (value.canConvert<double>()) {
                outMix = value.toDouble();
                return true;
            }
            const QString text = value.toString().trimmed().toLower();
            if (text == QLatin1String("true")) {
                outMix = 1.0;
                return true;
            }
            if (text == QLatin1String("false")) {
                outMix = 0.0;
                return true;
            }
            bool ok = false;
            const double v = text.toDouble(&ok);
            if (!ok) {
                return false;
            }
            outMix = v;
            return true;
        }

        void applyMixPort(const std::shared_ptr<NodeData> &nodeData)
        {
            auto var = std::dynamic_pointer_cast<VariableData>(nodeData);
            if (!var) {
                return;
            }
            double mixValue = 0.0;
            if (!tryParseMixValue(var->value(), mixValue)) {
                return;
            }
            const double clamped = qBound(0.0, mixValue, 1.0);
            // 加载后首次 MIX 重连：只对齐基线，避免覆盖已恢复的 Mix
            if (m_loadedFromFile && !m_mixPortSeen) {
                m_mixPortSeen = true;
                m_lastMixPortValue = clamped;
                if (m_switchSeen) {
                    m_loadedFromFile = false;
                }
                return;
            }
            if (m_mixPortSeen && qFuzzyCompare(clamped + 1.0, m_lastMixPortValue + 1.0)) {
                return;
            }
            m_mixPortSeen = true;
            m_lastMixPortValue = clamped;
            setMix(clamped);
        }

        void applySwitchB(bool toB)
        {
            // 工程加载后首次 setInData：只记录基线，避免 SWTCH 当前值覆盖已恢复的选择
            if (m_loadedFromFile && !m_switchSeen) {
                m_switchSeen = true;
                m_lastSwitchToB = toB;
                // 等 MIX 与 SWTCH 都至少见到一次后再清 loaded 标记
                if (m_mixPortSeen) {
                    m_loadedFromFile = false;
                }
                return;
            }
            if (m_switchSeen && toB == m_lastSwitchToB) {
                return;
            }
            m_switchSeen = true;
            m_lastSwitchToB = toB;

            const int action = toB
                ? AudioCrossFaderInterface::kActionToB
                : AudioCrossFaderInterface::kActionToA;
            // 仅在状态变化时触发淡入淡出，避免重复 setInData 反复重启
            if (action == m_controlAction) {
                return;
            }
            setControlActionProperty(action);
        }

        void snapMixToAction(bool pushToWorker)
        {
            const double target = (m_controlAction == AudioCrossFaderInterface::kActionToB)
                ? 1.0
                : 0.0;
            m_mix = target;
            if (widget && widget->mixSpin) {
                const QSignalBlocker blocker(widget->mixSpin);
                widget->mixSpin->setValue(m_mix);
            }
            if (pushToWorker && _worker) {
                QMetaObject::invokeMethod(_worker, "setMix",
                                          Qt::QueuedConnection,
                                          Q_ARG(double, m_mix));
            }
        }

        static int normalizeAction(int action)
        {
            // 兼容旧 Combo：0=Reset→ToA，1=A→B，2=B→A→ToA
            if (action == 2) {
                return AudioCrossFaderInterface::kActionToA;
            }
            if (action == AudioCrossFaderInterface::kActionToB) {
                return AudioCrossFaderInterface::kActionToB;
            }
            return AudioCrossFaderInterface::kActionToA;
        }

        void syncActionButtons()
        {
            const QSignalBlocker blocker(widget->actionGroup);
            if (QAbstractButton *btn = widget->actionGroup->button(m_controlAction)) {
                btn->setChecked(true);
            }
        }

        void syncActionButtonsFromMix()
        {
            // Mix 拖到两端时同步互斥按钮选中态（不触发淡入淡出）
            if (m_mix <= 0.001) {
                if (m_controlAction != AudioCrossFaderInterface::kActionToA) {
                    m_controlAction = AudioCrossFaderInterface::kActionToA;
                    Q_EMIT controlActionChanged(m_controlAction);
                } else {
                    syncActionButtons();
                }
            } else if (m_mix >= 0.999) {
                if (m_controlAction != AudioCrossFaderInterface::kActionToB) {
                    m_controlAction = AudioCrossFaderInterface::kActionToB;
                    Q_EMIT controlActionChanged(m_controlAction);
                } else {
                    syncActionButtons();
                }
            }
        }

        void applyPortCount(PortType portType, unsigned int newCount, bool notifyPorts)
        {
            unsigned int &count = (portType == PortType::In) ? InPortCount : OutPortCount;
            const unsigned int oldCount = count;
            if (newCount == oldCount) {
                return;
            }
            if (notifyPorts) {
                if (newCount > oldCount) {
                    Q_EMIT portsAboutToBeInserted(portType, oldCount, newCount - 1);
                    count = newCount;
                    Q_EMIT portsInserted();
                } else {
                    Q_EMIT portsAboutToBeDeleted(portType, newCount, oldCount - 1);
                    count = newCount;
                    Q_EMIT portsDeleted();
                }
            } else {
                count = newCount;
            }
        }

        void applyChannelCount(int channelCount, bool notifyPorts)
        {
            const int audioCount = qBound(AudioCrossFaderInterface::kMinChannels,
                                          channelCount,
                                          AudioCrossFaderInterface::kMaxChannels);
            const unsigned int inCount = static_cast<unsigned int>(audioCount * 2 + 2); // + SWTCH B + MIX
            const unsigned int outCount = static_cast<unsigned int>(audioCount);

            applyPortCount(PortType::In, inCount, notifyPorts);
            applyPortCount(PortType::Out, outCount, notifyPorts);

            QMetaObject::invokeMethod(_worker, "initializeBuffers",
                                      Qt::QueuedConnection,
                                      Q_ARG(int, audioCount));

            if (notifyPorts) {
                for (unsigned int i = 0; i < OutPortCount; ++i) {
                    Q_EMIT dataUpdated(static_cast<PortIndex>(i));
                }
                Q_EMIT embeddedWidgetSizeUpdated();
            }
        }

        void registerBindings()
        {
            auto bind = [this](const char *member, const QString &address, QWidget *control) {
                NodeDelegateModel::ExternalBinding b;
                b.member = member;
                b.control = control;
                AbstractDelegateModel::registerExternalBinding(address, this, b);
            };
            bind("channels", QStringLiteral("/channels"), widget->channelsSpin);
            bind("mix", QStringLiteral("/mix"), widget->mixSpin);
            bind("fadeMs", QStringLiteral("/fade_ms"), widget->fadeDurationSpin);
            bind("controlAction", QStringLiteral("/action"), widget->fadeToAButton);
        }

        void pushParamsToWorker()
        {
            QMetaObject::invokeMethod(_worker, "setMix", Qt::QueuedConnection, Q_ARG(double, m_mix));
            QMetaObject::invokeMethod(_worker, "setFadeDuration", Qt::QueuedConnection, Q_ARG(double, m_fadeMs));
        }

        void executeControlAction(int action)
        {
            // 立刻回写目标 Mix，保证 save/UI 与最终输出一致；不经 setMix，以免取消正在进行的淡入淡出
            snapMixToAction(/*pushToWorker=*/false);
            m_lastSwitchToB = (action == AudioCrossFaderInterface::kActionToB);
            m_switchSeen = true;

            if (action == AudioCrossFaderInterface::kActionToB) {
                QMetaObject::invokeMethod(_worker, "startFadeAToB", Qt::QueuedConnection);
            } else {
                QMetaObject::invokeMethod(_worker, "startFadeBToA", Qt::QueuedConnection);
            }
        }

        AudioCrossFaderInterface *widget = nullptr;
        AudioCrossFaderWorker *_worker = nullptr;
        QThread *_workerThread = nullptr;
        int m_channels = AudioCrossFaderInterface::kDefaultChannels;
        double m_mix = 0.0;
        double m_fadeMs = 2000.0;
        int m_controlAction = AudioCrossFaderInterface::kActionToA;
        bool m_lastSwitchToB = false;
        bool m_switchSeen = false;
        bool m_mixPortSeen = false;
        double m_lastMixPortValue = 0.0;
        bool m_loadedFromFile = false;
    };
}
