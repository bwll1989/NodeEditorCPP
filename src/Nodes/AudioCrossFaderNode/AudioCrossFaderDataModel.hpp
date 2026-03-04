#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include <QTimer>
#include <QDebug>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>
#include "DataTypes/NodeDataList.hpp"
#include "DataTypes/AudioTimestampRingQueue.h"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include <QtCore/QDir>
#include <QtCore/QEvent>
#include <QtWidgets/QFileDialog>
#include <QtCore/qglobal.h>
#include "AudioCrossFaderInterface.h"
#include <QComboBox>
#include <QJsonArray>
#include <QJsonObject>
#include "PluginDefinition.hpp"
#include "AudioCrossFaderWorker.hpp"
#include "Common/BuildInNodes/AbstractDelegateModel.h"
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
     * @brief Audio Ducking Node Model
     */
    class AudioCrossFaderDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(double mix READ mix WRITE setMix NOTIFY mixChanged)
        Q_PROPERTY(double fadeMs READ fadeMs WRITE setFadeMs NOTIFY fadeMsChanged)
        Q_PROPERTY(int controlAction READ controlActionProperty WRITE setControlActionProperty NOTIFY controlActionChanged)
    
    public:
       AudioCrossFaderDataModel()
            : _worker(new AudioCrossFaderWorker())
            , _workerThread(new QThread(this))
        {
            InPortCount = 2; // 0: A, 1: B
            OutPortCount = 1; // Mixed Out
            widget = new AudioCrossFaderInterface();
            CaptionVisible = true;
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = false;
            Caption = "Audio Cross Fader";
            AbstractDelegateModel::registerExternalControl("/mix", widget->mixSpin);
            AbstractDelegateModel::registerExternalControl("/fade_ms", widget->fadeDurationSpin);
            AbstractDelegateModel::registerExternalControl("/action", widget->actionCombo);
            // Initialize buffers in worker
            _worker->initializeBuffers(InPortCount, OutPortCount);
            
            _worker->moveToThread(_workerThread);

            connect(_workerThread, &QThread::started, this, [this]() {
                QMetaObject::invokeMethod(_worker, "startProcessing", Qt::QueuedConnection);
            });
            connect(_workerThread, &QThread::finished, _worker, &AudioCrossFaderWorker::stopProcessing);
            
            connect(widget->mixSpin, &FloatDragValueWidget::valueChanged,
                    this, &AudioCrossFaderDataModel::setMix);
            connect(this, &AudioCrossFaderDataModel::mixChanged, this, [this](double){
                {
                    QSignalBlocker blocker(widget->mixSpin);
                    widget->mixSpin->setValue(m_mix);
                }
                QMetaObject::invokeMethod(
                    _worker,
                    "setMix",
                    Qt::QueuedConnection,
                    Q_ARG(double, m_mix)
                );
                AbstractDelegateModel::stateFeedBack("/mix", m_mix);
            });

            connect(widget->fadeDurationSpin, &FloatDragValueWidget::valueChanged,
                    this, &AudioCrossFaderDataModel::setFadeMs);
            connect(this, &AudioCrossFaderDataModel::fadeMsChanged, this, [this](double){
                {
                    QSignalBlocker blocker(widget->fadeDurationSpin);
                    widget->fadeDurationSpin->setValue(m_fadeMs);
                }
                QMetaObject::invokeMethod(
                    _worker,
                    "setFadeDuration",
                    Qt::QueuedConnection,
                    Q_ARG(double, m_fadeMs)
                );
                AbstractDelegateModel::stateFeedBack("/fade_ms", m_fadeMs);
            });
            connect(widget->actionCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
                    this, &AudioCrossFaderDataModel::onActionComboIndexChanged);
            connect(this, &AudioCrossFaderDataModel::controlActionChanged, this, [this](int){
                QSignalBlocker blocker(widget->actionCombo);
                widget->actionCombo->setCurrentIndex(m_controlAction);
                AbstractDelegateModel::stateFeedBack("/action", m_controlAction);
            });
            
            _workerThread->start();
        }
    
        ~AudioCrossFaderDataModel()
        {
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
            Q_UNUSED(portIndex);
            return AudioData().type();
        }
        
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
                case PortType::In:
                    if (portIndex == 0) return "A";
                    if (portIndex == 1) return "B";
                    break;
                case PortType::Out:
                    return "Out";
                default:
                    break;
            }
            return "";
        }

        double mix() const
        {
            return m_mix;
        }

        void setMix(double value)
        {
            if (qFuzzyCompare(value + 1.0, m_mix + 1.0)) {
                return;
            }
            m_mix = value;
            Q_EMIT mixChanged(value);
        }

        double fadeMs() const
        {
            return m_fadeMs;
        }

        void setFadeMs(double ms)
        {
            if (qFuzzyCompare(ms + 1.0, m_fadeMs + 1.0)) {
                return;
            }
            m_fadeMs = ms;
            Q_EMIT fadeMsChanged(ms);
        }

        /**
         * 函数级注释：获取当前控制动作属性值（0=空闲，1=A->B，2=B->A，3=Reset）
         */
        int controlActionProperty() const
        {
            return m_controlAction;
        }

        /**
         * 函数级注释：设置控制动作属性，触发对应淡入淡出或重置操作
         */
        void setControlActionProperty(int action)
        {
            if (action < 0 || action > 2) {
                return;
            }
            m_controlAction = action;
           
            executeControlAction(m_controlAction);
           
            Q_EMIT controlActionChanged(m_controlAction);
        }
        
        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            if (port >= OutPortCount) {
                return std::make_shared<AudioData>();
            }

            std::shared_ptr<AudioData> outputData = std::make_shared<AudioData>();
            std::shared_ptr<AudioTimestampRingQueue> outputBuffer = _worker->getOutputBuffer(port);
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
            
            auto audioData = std::dynamic_pointer_cast<AudioData>(nodeData);
            std::shared_ptr<AudioTimestampRingQueue> audioBuffer = nullptr;
            
            if (audioData && audioData->isConnectedToSharedBuffer()) {
                audioBuffer = audioData->getSharedAudioBuffer();
            }
            
            QMetaObject::invokeMethod(_worker, "setInputBuffer", 
                                    Qt::QueuedConnection,
                                    Q_ARG(int, port),
                                    Q_ARG(std::shared_ptr<AudioTimestampRingQueue>, audioBuffer));
        }
    
        QWidget *embeddedWidget() override { return widget; }
        
        QJsonObject save() const override
        {
            QJsonObject modelJson;
            modelJson["mix"] = mix();
            modelJson["fade_ms"] = fadeMs();
            return modelJson;
        }

        void load(QJsonObject const &p) override
        {
            if (p.contains("mix")) {
                setMix(p["mix"].toDouble());
            }
            if (p.contains("fade_ms")) {
                setFadeMs(p["fade_ms"].toDouble());
            }
        }

    public slots:
        /**
         * @brief 重新发送当前混合比例到工作线程，效果等同于 mixSpin 的 valueChanged
         */
        void onResetButtonClicked() {
            double v = widget->mixSpin->value();
            QMetaObject::invokeMethod(_worker, "setMix", Qt::QueuedConnection, Q_ARG(double, v));
        }

        /**
         * 函数级注释：处理下拉菜单索引变化并通过属性系统触发控制动作
         */
        void onActionComboIndexChanged(int index)
        {
            if (index < 0 || index > 2) {
                return;
            }
            setControlActionProperty(index);
        }
    signals:
        void mixChanged(double value);
        void fadeMsChanged(double ms);
        /**
         * 函数级注释：控制动作属性变化信号，用于状态反馈与界面同步
         */
        void controlActionChanged(int action);

    protected:
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/mix"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/fade_ms"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/action"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
        }

    private Q_SLOTS:
        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }
            const QString addrMix = makeFullOscAddress("/mix");
            const QString addrFade = makeFullOscAddress("/fade_ms");
            const QString addrAction = makeFullOscAddress("/action");
            if (ev.address == addrMix) {
                setMix(ev.payload.toDouble());
            } else if (ev.address == addrFade) {
                setFadeMs(ev.payload.toDouble());
            } else if (ev.address == addrAction) {
                setControlActionProperty(ev.payload.toInt());
            }
        }

    private:
        AudioCrossFaderInterface *widget;
        AudioCrossFaderWorker* _worker;
        QThread* _workerThread;
        double m_mix = 0.5;
        double m_fadeMs = 0.0;
        int m_controlAction = 0;

        /**
         * 函数级注释：根据控制动作编码执行 A->B、B->A 或重置混合操作
         */
        void executeControlAction(int action)
        {
            switch (action) {
            case 0:
                setMix(m_mix);
                break;
            case 1:
                QMetaObject::invokeMethod(_worker, "startFadeAToB", Qt::QueuedConnection);
                break;
            case 2:
                QMetaObject::invokeMethod(_worker, "startFadeBToA", Qt::QueuedConnection);
                break;
            
            default:
                break;
            }
        }
    };
}
