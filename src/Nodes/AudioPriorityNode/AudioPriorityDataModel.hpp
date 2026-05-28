#pragma once

#include <iostream>

#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include <QTimer>
#include <QDebug>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>
#include "Common/DataTypes/NodeDataList.hpp"
#include "Common/DataTypes/AudioTimestampRingQueue.h"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include <QtCore/QDir>
#include <QtCore/QEvent>
#include <QtWidgets/QFileDialog>
#include <QtCore/qglobal.h>
#include <QJsonArray>
#include <QJsonObject>
#include "PluginDefinition.hpp"
#include "AudioPriorityWorker.hpp"
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
     * @brief Audio Ducking Node Model
     */
    class AudioPriorityDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(double threshold READ threshold WRITE setThreshold NOTIFY thresholdChanged)
        Q_PROPERTY(double ratio READ ratio WRITE setRatio NOTIFY ratioChanged)
        Q_PROPERTY(double attack READ attack WRITE setAttack NOTIFY attackChanged)
        Q_PROPERTY(double release READ release WRITE setRelease NOTIFY releaseChanged)
        Q_PROPERTY(double makeupGain READ makeupGain WRITE setMakeupGain NOTIFY makeupGainChanged)
        Q_PROPERTY(double sidechainGain READ sidechainGain WRITE setSidechainGain NOTIFY sidechainGainChanged)
        Q_PROPERTY(double depth READ depth WRITE setDepth NOTIFY depthChanged)
    
    public:
       /**
        * 函数级注释：构造函数，初始化音频优先级节点并绑定属性与控件
        */
       AudioPriorityDataModel()
            : _worker(new AudioPriorityWorker())
            , _workerThread(new QThread(this))
        {
            InPortCount = 2; // 0: Music, 1: Sidechain
            OutPortCount = 1; // Ducked Music
            CaptionVisible = true;
            WidgetEmbeddable = true;
            Resizable = false;
            PortEditable = false;
            Caption = "Audio Priority";

            m_threshold = -20.0;
            m_ratio = 4.0;
            m_attack = 10.0;
            m_release = 100.0;
            m_makeupGain = 0.0;
            m_depth = 24.0;
            m_sidechainGain = 0.0;

            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "threshold";
                AbstractDelegateModel::registerExternalBinding("/threshold", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "ratio";
                AbstractDelegateModel::registerExternalBinding("/ratio", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "attack";
                AbstractDelegateModel::registerExternalBinding("/attack", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "release";
                AbstractDelegateModel::registerExternalBinding("/release", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "makeupGain";
                AbstractDelegateModel::registerExternalBinding("/makeup", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "sidechainGain";
                AbstractDelegateModel::registerExternalBinding("/sidechain_gain", this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "depth";
                AbstractDelegateModel::registerExternalBinding("/depth", this, b);
            }

            _worker->initializeBuffers(InPortCount, OutPortCount);
            _worker->moveToThread(_workerThread);
            
            connect(_workerThread, &QThread::started, this, [this]() {
                QMetaObject::invokeMethod(_worker, "startProcessing", Qt::QueuedConnection);
            });
            connect(_workerThread, &QThread::finished, _worker, &AudioPriorityWorker::stopProcessing);

            connect(this, &AudioPriorityDataModel::thresholdChanged, this, [this](double value){
                QMetaObject::invokeMethod(_worker, "setThreshold",
                                          Qt::QueuedConnection,
                                          Q_ARG(double, value));
            });

            connect(this, &AudioPriorityDataModel::ratioChanged, this, [this](double value){
                QMetaObject::invokeMethod(_worker, "setRatio",
                                          Qt::QueuedConnection,
                                          Q_ARG(double, value));
            });

            connect(this, &AudioPriorityDataModel::attackChanged, this, [this](double value){
                QMetaObject::invokeMethod(_worker, "setAttack",
                                          Qt::QueuedConnection,
                                          Q_ARG(double, value));
            });

            connect(this, &AudioPriorityDataModel::releaseChanged, this, [this](double value){
                QMetaObject::invokeMethod(_worker, "setRelease",
                                          Qt::QueuedConnection,
                                          Q_ARG(double, value));
            });

            connect(this, &AudioPriorityDataModel::makeupGainChanged, this, [this](double value){
                QMetaObject::invokeMethod(_worker, "setMakeupGain",
                                          Qt::QueuedConnection,
                                          Q_ARG(double, value));
            });

            connect(this, &AudioPriorityDataModel::sidechainGainChanged, this, [this](double value){
                QMetaObject::invokeMethod(_worker, "setSidechainGain",
                                          Qt::QueuedConnection,
                                          Q_ARG(double, value));
            });

            connect(this, &AudioPriorityDataModel::depthChanged, this, [this](double value){
                QMetaObject::invokeMethod(_worker, "setDepth",
                                          Qt::QueuedConnection,
                                          Q_ARG(double, value));
            });
            
            connect(_worker, &AudioPriorityWorker::processingStatusChanged, this, &AudioPriorityDataModel::onProcessingStatusChanged);
            
            _workerThread->start();
        }
    
        /**
         * 函数级注释：析构函数，安全停止工作线程并释放资源
         */
        ~AudioPriorityDataModel()
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
         * 函数级注释：返回指定端口的数据类型
         */
        NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override
        {
            Q_UNUSED(portIndex);
            return AudioData().type();
        }
        
        /**
         * 函数级注释：返回端口标题字符串
         */
        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
                case PortType::In:
                    if (portIndex == 0) return "Low priority";
                    if (portIndex == 1) return "High priority";
                    break;
                case PortType::Out:
                    return "Out";
                default:
                    break;
            }
            return "";
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
    
        /**
         * 函数级注释：设置输入端口的音频缓冲区
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
         * 函数级注释：获取当前压限阈值属性（单位 dB）
         */
        double threshold() const
        {
            return m_threshold;
        }

        /**
         * 函数级注释：设置压限阈值属性并触发变化信号
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
         * 函数级注释：设置压缩比属性并触发变化信号
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
         * 函数级注释：设置攻击时间属性并触发变化信号
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
         * 函数级注释：设置释放时间属性并触发变化信号
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
         * 函数级注释：设置化妆增益属性并触发变化信号
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
         * 函数级注释：设置侧链增益属性并触发变化信号
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
         * 函数级注释：设置压低深度属性并触发变化信号
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
         * 函数级注释：返回嵌入式界面控件
         */
        
        /**
         * 函数级注释：保存节点配置为 JSON
         */
        QJsonObject save() const override
        {
            QJsonObject values;
            values["threshold"] = threshold();
            values["ratio"] = ratio();
            values["attack"] = attack();
            values["release"] = release();
            values["makeup"] = makeupGain();
            values["sidechain_gain"] = sidechainGain();
            values["depth"] = depth();

            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["values"] = values;
            return modelJson;
        }

        /**
         * 函数级注释：从 JSON 加载节点配置并更新属性
         */
        void load(QJsonObject const &p) override
        {
            NodeDelegateModel::load(p);

            QJsonObject values;
            const QJsonValue vv = p.value("values");
            if (vv.isObject()) {
                values = vv.toObject();
            } else {
                values = p;
            }

            if (values.contains("threshold")) setThreshold(values["threshold"].toDouble());
            if (values.contains("ratio")) setRatio(values["ratio"].toDouble());
            if (values.contains("attack")) setAttack(values["attack"].toDouble());
            if (values.contains("release")) setRelease(values["release"].toDouble());
            if (values.contains("makeup")) setMakeupGain(values["makeup"].toDouble());
            if (values.contains("sidechain_gain")) setSidechainGain(values["sidechain_gain"].toDouble());
            if (values.contains("depth")) setDepth(values["depth"].toDouble());
        }

    public slots:
        /**
         * 函数级注释：处理工作线程状态变化（预留界面更新扩展点）
         */
        void onProcessingStatusChanged(bool isProcessing) {
            Q_UNUSED(isProcessing);
        }

    signals:
        /**
         * 函数级注释：阈值属性变化通知信号
         */
        void thresholdChanged(double value);
        /**
         * 函数级注释：压缩比属性变化通知信号
         */
        void ratioChanged(double value);
        /**
         * 函数级注释：攻击时间属性变化通知信号
         */
        void attackChanged(double value);
        /**
         * 函数级注释：释放时间属性变化通知信号
         */
        void releaseChanged(double value);
        /**
         * 函数级注释：化妆增益属性变化通知信号
         */
        void makeupGainChanged(double value);
        /**
         * 函数级注释：侧链增益属性变化通知信号
         */
        void sidechainGainChanged(double value);
        /**
         * 函数级注释：压低深度属性变化通知信号
         */
        void depthChanged(double value);

    protected:
        /**
         * 函数级注释：模型就绪后订阅全局事件总线命令
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
         * 函数级注释：处理来自全局事件总线的外部控制命令
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
        AudioPriorityWorker* _worker;
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
