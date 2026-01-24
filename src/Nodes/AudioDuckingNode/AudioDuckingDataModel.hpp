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
#include "AudioDuckingInterface.h"
#include <QComboBox>
#include <QJsonArray>
#include <QJsonObject>
#include "PluginDefinition.hpp"
#include "AudioDuckingWorker.hpp"
#include "ConstantDefines.h"
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
    class AudioDuckingDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(double threshold READ threshold WRITE setThreshold NOTIFY thresholdChanged)
        Q_PROPERTY(double ratio READ ratio WRITE setRatio NOTIFY ratioChanged)
        Q_PROPERTY(double attack READ attack WRITE setAttack NOTIFY attackChanged)
        Q_PROPERTY(double release READ release READ release WRITE setRelease NOTIFY releaseChanged)
        Q_PROPERTY(double makeupGain READ makeupGain WRITE setMakeupGain NOTIFY makeupGainChanged)
        Q_PROPERTY(double sidechainGain READ sidechainGain WRITE setSidechainGain NOTIFY sidechainGainChanged)
        Q_PROPERTY(double depth READ depth WRITE setDepth NOTIFY depthChanged)
    
    public:
        /**
         * @brief 构造函数，初始化音频闪避节点并绑定属性与控件
         */
       AudioDuckingDataModel()
            : _worker(new AudioDuckingWorker())
            , _workerThread(new QThread(this))
        {
            InPortCount = 2; // 0: Music, 1: Sidechain
            OutPortCount = 1; // Ducked Music
            widget = new AudioDuckingInterface();
            CaptionVisible = true;
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = false;
            Caption = "Audio Ducking";
            
            // 初始化属性默认值
            m_threshold = widget->thresholdSpin->value();
            m_ratio = widget->ratioSpin->value();
            m_attack = widget->attackSpin->value();
            m_release = widget->releaseSpin->value();
            m_makeupGain = widget->makeupGainSpin->value();
            m_sidechainGain = widget->sidechainGainSpin->value();
            m_depth = widget->depthSpin->value();

            // 注册外部控制控件
            AbstractDelegateModel::registerExternalControl("/threshold", widget->thresholdSpin);
            AbstractDelegateModel::registerExternalControl("/ratio", widget->ratioSpin);
            AbstractDelegateModel::registerExternalControl("/attack", widget->attackSpin);
            AbstractDelegateModel::registerExternalControl("/release", widget->releaseSpin);
            AbstractDelegateModel::registerExternalControl("/makeup", widget->makeupGainSpin);
            AbstractDelegateModel::registerExternalControl("/sidechain_gain", widget->sidechainGainSpin);
            AbstractDelegateModel::registerExternalControl("/depth", widget->depthSpin);

            // Initialize buffers in worker
            _worker->initializeBuffers(InPortCount, OutPortCount);
            
            _worker->moveToThread(_workerThread);
            
            // Connect signals
            connect(_workerThread, &QThread::started, this, [this]() {
                QMetaObject::invokeMethod(_worker, "startProcessing", Qt::QueuedConnection);
            });
            connect(_workerThread, &QThread::finished, _worker, &AudioDuckingWorker::stopProcessing);
            
            connect(widget->thresholdSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                    this, &AudioDuckingDataModel::setThreshold);
            connect(this, &AudioDuckingDataModel::thresholdChanged, this, [this](double value){
                {
                    QSignalBlocker blocker(widget->thresholdSpin);
                    widget->thresholdSpin->setValue(value);
                }
                QMetaObject::invokeMethod(_worker, "setThreshold",
                                          Qt::QueuedConnection,
                                          Q_ARG(double, value));
                AbstractDelegateModel::stateFeedBack("/threshold", value);
            });

            connect(widget->ratioSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                    this, &AudioDuckingDataModel::setRatio);
            connect(this, &AudioDuckingDataModel::ratioChanged, this, [this](double value){
                {
                    QSignalBlocker blocker(widget->ratioSpin);
                    widget->ratioSpin->setValue(value);
                }
                QMetaObject::invokeMethod(_worker, "setRatio",
                                          Qt::QueuedConnection,
                                          Q_ARG(double, value));
                AbstractDelegateModel::stateFeedBack("/ratio", value);
            });

            connect(widget->attackSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                    this, &AudioDuckingDataModel::setAttack);
            connect(this, &AudioDuckingDataModel::attackChanged, this, [this](double value){
                {
                    QSignalBlocker blocker(widget->attackSpin);
                    widget->attackSpin->setValue(value);
                }
                QMetaObject::invokeMethod(_worker, "setAttack",
                                          Qt::QueuedConnection,
                                          Q_ARG(double, value));
                AbstractDelegateModel::stateFeedBack("/attack", value);
            });

            connect(widget->releaseSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                    this, &AudioDuckingDataModel::setRelease);
            connect(this, &AudioDuckingDataModel::releaseChanged, this, [this](double value){
                {
                    QSignalBlocker blocker(widget->releaseSpin);
                    widget->releaseSpin->setValue(value);
                }
                QMetaObject::invokeMethod(_worker, "setRelease",
                                          Qt::QueuedConnection,
                                          Q_ARG(double, value));
                AbstractDelegateModel::stateFeedBack("/release", value);
            });

            connect(widget->makeupGainSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                    this, &AudioDuckingDataModel::setMakeupGain);
            connect(this, &AudioDuckingDataModel::makeupGainChanged, this, [this](double value){
                {
                    QSignalBlocker blocker(widget->makeupGainSpin);
                    widget->makeupGainSpin->setValue(value);
                }
                QMetaObject::invokeMethod(_worker, "setMakeupGain",
                                          Qt::QueuedConnection,
                                          Q_ARG(double, value));
                AbstractDelegateModel::stateFeedBack("/makeup", value);
            });

            connect(widget->sidechainGainSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                    this, &AudioDuckingDataModel::setSidechainGain);
            connect(this, &AudioDuckingDataModel::sidechainGainChanged, this, [this](double value){
                {
                    QSignalBlocker blocker(widget->sidechainGainSpin);
                    widget->sidechainGainSpin->setValue(value);
                }
                QMetaObject::invokeMethod(_worker, "setSidechainGain",
                                          Qt::QueuedConnection,
                                          Q_ARG(double, value));
                AbstractDelegateModel::stateFeedBack("/sidechain_gain", value);
            });

            connect(widget->depthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                    this, &AudioDuckingDataModel::setDepth);
            connect(this, &AudioDuckingDataModel::depthChanged, this, [this](double value){
                {
                    QSignalBlocker blocker(widget->depthSpin);
                    widget->depthSpin->setValue(value);
                }
                QMetaObject::invokeMethod(_worker, "setDepth",
                                          Qt::QueuedConnection,
                                          Q_ARG(double, value));
                AbstractDelegateModel::stateFeedBack("/depth", value);
            });
            
            connect(_worker, &AudioDuckingWorker::processingStatusChanged, this, &AudioDuckingDataModel::onProcessingStatusChanged);
            
            _workerThread->start();
        }
    
        /**
         * @brief 析构函数，安全停止工作线程并清理资源
         */
        ~AudioDuckingDataModel()
        {
            if (_workerThread && _workerThread->isRunning()) {
                _workerThread->quit();
                _workerThread->wait(3000);
            }
            if (_worker) {
                _worker->deleteLater();
            }
        }

        /**
         * @brief 获取端口数据类型
         * @param portType 端口类型
         * @param portIndex 端口索引
         * @return 数据类型
         */
        NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override
        {
            Q_UNUSED(portIndex);
            return AudioData().type();
        }
        
        /**
         * @brief 获取端口标题
         * @param portType 端口类型
         * @param portIndex 端口索引
         * @return 端口标题字符串
         */
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
                case PortType::In:
                    if (portIndex == 0) return "Music";
                    if (portIndex == 1) return "Sidechain";
                    break;
                case PortType::Out:
                    return "Out";
                default:
                    break;
            }
            return "";
        }

        /**
         * 函数级注释：获取当前压限阈值属性（单位 dB）
         */
        double threshold() const
        {
            return m_threshold;
        }

        /**
         * 函数级注释：设置压限阈值属性并触发状态反馈
         */
        void setThreshold(double value)
        {
            if (qFuzzyCompare(value + 1.0, m_threshold + 1.0)) {
                return;
            }
            m_threshold = value;
            Q_EMIT thresholdChanged(value);
        }

        /**
         * 函数级注释：获取压缩比属性
         */
        double ratio() const
        {
            return m_ratio;
        }

        /**
         * 函数级注释：设置压缩比属性并触发状态反馈
         */
        void setRatio(double value)
        {
            if (qFuzzyCompare(value + 1.0, m_ratio + 1.0)) {
                return;
            }
            m_ratio = value;
            Q_EMIT ratioChanged(value);
        }

        /**
         * 函数级注释：获取攻击时间属性（毫秒）
         */
        double attack() const
        {
            return m_attack;
        }

        /**
         * 函数级注释：设置攻击时间属性并触发状态反馈
         */
        void setAttack(double value)
        {
            if (qFuzzyCompare(value + 1.0, m_attack + 1.0)) {
                return;
            }
            m_attack = value;
            Q_EMIT attackChanged(value);
        }

        /**
         * 函数级注释：获取释放时间属性（毫秒）
         */
        double release() const
        {
            return m_release;
        }

        /**
         * 函数级注释：设置释放时间属性并触发状态反馈
         */
        void setRelease(double value)
        {
            if (qFuzzyCompare(value + 1.0, m_release + 1.0)) {
                return;
            }
            m_release = value;
            Q_EMIT releaseChanged(value);
        }

        /**
         * 函数级注释：获取化妆增益属性（单位 dB）
         */
        double makeupGain() const
        {
            return m_makeupGain;
        }

        /**
         * 函数级注释：设置化妆增益属性并触发状态反馈
         */
        void setMakeupGain(double value)
        {
            if (qFuzzyCompare(value + 1.0, m_makeupGain + 1.0)) {
                return;
            }
            m_makeupGain = value;
            Q_EMIT makeupGainChanged(value);
        }

        /**
         * 函数级注释：获取侧链增益属性（单位 dB）
         */
        double sidechainGain() const
        {
            return m_sidechainGain;
        }

        /**
         * 函数级注释：设置侧链增益属性并触发状态反馈
         */
        void setSidechainGain(double value)
        {
            if (qFuzzyCompare(value + 1.0, m_sidechainGain + 1.0)) {
                return;
            }
            m_sidechainGain = value;
            Q_EMIT sidechainGainChanged(value);
        }

        /**
         * 函数级注释：获取压低深度属性（单位 dB）
         */
        double depth() const
        {
            return m_depth;
        }

        /**
         * 函数级注释：设置压低深度属性并触发状态反馈
         */
        void setDepth(double value)
        {
            if (qFuzzyCompare(value + 1.0, m_depth + 1.0)) {
                return;
            }
            m_depth = value;
            Q_EMIT depthChanged(value);
        }
        
        /**
         * @brief 获取输出数据
         * @param port 端口索引
         * @return 输出数据
         */
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
    
        /**
         * @brief 设置输入数据
         * @param nodeData 输入节点数据
         * @param port 端口索引
         */
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
    
        /**
         * 函数级注释：返回嵌入式界面控件
         */
        QWidget *embeddedWidget() override { return widget; }
        
        /**
         * @brief 保存节点配置
         */
        QJsonObject save() const override
        {
            QJsonObject modelJson;
            modelJson["threshold"] = threshold();
            modelJson["ratio"] = ratio();
            modelJson["attack"] = attack();
            modelJson["release"] = release();
            modelJson["makeup"] = makeupGain();
            modelJson["sidechain_gain"] = sidechainGain();
            modelJson["depth"] = depth();
            return modelJson;
        }

        /**
         * @brief 加载节点配置
         */
        void load(QJsonObject const &p) override
        {
            if (p.contains("threshold")) setThreshold(p["threshold"].toDouble());
            if (p.contains("ratio")) setRatio(p["ratio"].toDouble());
            if (p.contains("attack")) setAttack(p["attack"].toDouble());
            if (p.contains("release")) setRelease(p["release"].toDouble());
            if (p.contains("makeup")) setMakeupGain(p["makeup"].toDouble());
            if (p.contains("sidechain_gain")) setSidechainGain(p["sidechain_gain"].toDouble());
            if (p.contains("depth")) setDepth(p["depth"].toDouble());
        }

    public slots:
        /**
         * @brief 处理工作线程状态变化，可用于更新界面状态
         */
        void onProcessingStatusChanged(bool isProcessing) {
            Q_UNUSED(isProcessing);
        }

    signals:
        /**
         * 函数级注释：当阈值属性发生变化时发出的通知信号
         */
        void thresholdChanged(double value);
        /**
         * 函数级注释：当压缩比属性发生变化时发出的通知信号
         */
        void ratioChanged(double value);
        /**
         * 函数级注释：当攻击时间属性发生变化时发出的通知信号
         */
        void attackChanged(double value);
        /**
         * 函数级注释：当释放时间属性发生变化时发出的通知信号
         */
        void releaseChanged(double value);
        /**
         * 函数级注释：当化妆增益属性发生变化时发出的通知信号
         */
        void makeupGainChanged(double value);
        /**
         * 函数级注释：当侧链增益属性发生变化时发出的通知信号
         */
        void sidechainGainChanged(double value);
        /**
         * 函数级注释：当压低深度属性发生变化时发出的通知信号
         */
        void depthChanged(double value);

    protected:
        /**
         * 函数级注释：模型就绪后订阅全局事件总线，使用包含正确节点ID的完整地址
         */
        void afterModelReady() override
        {
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/threshold"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/ratio"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/attack"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/release"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/makeup"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/sidechain_gain"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
            GlobalEventBus::instance()->subscribe(
                makeFullOscAddress("/depth"),
                this,
                SLOT(onGlobalEvent(GlobalEvent))
            );
        }

    private Q_SLOTS:
        /**
         * 函数级注释：处理来自全局事件总线的外部命令，更新压缩参数属性
         */
        void onGlobalEvent(const GlobalEvent& ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }
            const QString addrThreshold = makeFullOscAddress("/threshold");
            const QString addrRatio = makeFullOscAddress("/ratio");
            const QString addrAttack = makeFullOscAddress("/attack");
            const QString addrRelease = makeFullOscAddress("/release");
            const QString addrMakeup = makeFullOscAddress("/makeup");
            const QString addrSidechain = makeFullOscAddress("/sidechain_gain");
            const QString addrDepth = makeFullOscAddress("/depth");

            if (ev.address == addrThreshold) {
                setThreshold(ev.payload.toDouble());
            } else if (ev.address == addrRatio) {
                setRatio(ev.payload.toDouble());
            } else if (ev.address == addrAttack) {
                setAttack(ev.payload.toDouble());
            } else if (ev.address == addrRelease) {
                setRelease(ev.payload.toDouble());
            } else if (ev.address == addrMakeup) {
                setMakeupGain(ev.payload.toDouble());
            } else if (ev.address == addrSidechain) {
                setSidechainGain(ev.payload.toDouble());
            } else if (ev.address == addrDepth) {
                setDepth(ev.payload.toDouble());
            }
        }

    private:
        AudioDuckingInterface *widget;
        AudioDuckingWorker* _worker;
        QThread* _workerThread;
        double m_threshold = 0.0;
        double m_ratio = 0.0;
        double m_attack = 0.0;
        double m_release = 0.0;
        double m_makeupGain = 0.0;
        double m_sidechainGain = 0.0;
        double m_depth = 0.0;
    };
}
