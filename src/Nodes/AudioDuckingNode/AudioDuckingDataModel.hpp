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

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

namespace Nodes {
    /**
     * @brief Audio Ducking Node Model
     */
    class AudioDuckingDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
    
    public:
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
            
            // Initialize buffers in worker
            _worker->initializeBuffers(InPortCount, OutPortCount);
            
            _worker->moveToThread(_workerThread);
            
            // Register OSC controls
            AbstractDelegateModel::registerOSCControl("/threshold", widget->thresholdSpin);
            AbstractDelegateModel::registerOSCControl("/ratio", widget->ratioSpin);
            AbstractDelegateModel::registerOSCControl("/attack", widget->attackSpin);
            AbstractDelegateModel::registerOSCControl("/release", widget->releaseSpin);
            AbstractDelegateModel::registerOSCControl("/makeup", widget->makeupGainSpin);
            AbstractDelegateModel::registerOSCControl("/sidechain_gain", widget->sidechainGainSpin);
            AbstractDelegateModel::registerOSCControl("/depth", widget->depthSpin);

            // Connect signals
            connect(_workerThread, &QThread::started, this, [this]() {
                QMetaObject::invokeMethod(_worker, "startProcessing", Qt::QueuedConnection);
            });
            connect(_workerThread, &QThread::finished, _worker, &AudioDuckingWorker::stopProcessing);
            
            connect(widget->thresholdSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
                    this, [this](double val){ 
                        QMetaObject::invokeMethod(_worker, "setThreshold", Qt::QueuedConnection, Q_ARG(double, val)); 
                    });
            connect(widget->ratioSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
                    this, [this](double val){ 
                        QMetaObject::invokeMethod(_worker, "setRatio", Qt::QueuedConnection, Q_ARG(double, val)); 
                    });
            connect(widget->attackSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
                    this, [this](double val){ 
                        QMetaObject::invokeMethod(_worker, "setAttack", Qt::QueuedConnection, Q_ARG(double, val)); 
                    });
            connect(widget->releaseSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
                    this, [this](double val){ 
                        QMetaObject::invokeMethod(_worker, "setRelease", Qt::QueuedConnection, Q_ARG(double, val)); 
                    });
            connect(widget->makeupGainSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
                    this, [this](double val){ 
                        QMetaObject::invokeMethod(_worker, "setMakeupGain", Qt::QueuedConnection, Q_ARG(double, val)); 
                    });
            connect(widget->sidechainGainSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
                    this, [this](double val){ 
                        QMetaObject::invokeMethod(_worker, "setSidechainGain", Qt::QueuedConnection, Q_ARG(double, val)); 
                    });
            connect(widget->depthSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
                    this, [this](double val){ 
                        QMetaObject::invokeMethod(_worker, "setDepth", Qt::QueuedConnection, Q_ARG(double, val)); 
                    });
            
            connect(_worker, &AudioDuckingWorker::processingStatusChanged, this, &AudioDuckingDataModel::onProcessingStatusChanged);
            
            _workerThread->start();
        }
    
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

        NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override
        {
            Q_UNUSED(portIndex);
            return AudioData().type();
        }
        
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
            modelJson["threshold"] = widget->thresholdSpin->value();
            modelJson["ratio"] = widget->ratioSpin->value();
            modelJson["attack"] = widget->attackSpin->value();
            modelJson["release"] = widget->releaseSpin->value();
            modelJson["makeup"] = widget->makeupGainSpin->value();
            modelJson["sidechain_gain"] = widget->sidechainGainSpin->value();
            modelJson["depth"] = widget->depthSpin->value();
            return modelJson;
        }

        void load(QJsonObject const &p) override
        {
            if (p.contains("threshold")) widget->thresholdSpin->setValue(p["threshold"].toDouble());
            if (p.contains("ratio")) widget->ratioSpin->setValue(p["ratio"].toDouble());
            if (p.contains("attack")) widget->attackSpin->setValue(p["attack"].toDouble());
            if (p.contains("release")) widget->releaseSpin->setValue(p["release"].toDouble());
            if (p.contains("makeup")) widget->makeupGainSpin->setValue(p["makeup"].toDouble());
            if (p.contains("sidechain_gain")) widget->sidechainGainSpin->setValue(p["sidechain_gain"].toDouble());
            if (p.contains("depth")) widget->depthSpin->setValue(p["depth"].toDouble());
        }

    public slots:
        void onProcessingStatusChanged(bool isProcessing) {
            // Optional: update UI state
        }

    private:
        AudioDuckingInterface *widget;
        AudioDuckingWorker* _worker;
        QThread* _workerThread;
    };
}
