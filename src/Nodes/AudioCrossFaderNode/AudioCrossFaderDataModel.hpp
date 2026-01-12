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
    class AudioCrossFaderDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
    
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
            AudioCrossFaderDataModel::registerOSCControl("/A-B",widget->fadeAToBButton);
           AudioCrossFaderDataModel::registerOSCControl("/B-A",widget->fadeBToAButton);
           AudioCrossFaderDataModel::registerOSCControl("/reset",widget->resetButton);
           AudioCrossFaderDataModel::registerOSCControl("/time",widget->fadeDurationSpin);
           AudioCrossFaderDataModel::registerOSCControl("/mix",widget->mixSpin);
            // Initialize buffers in worker
            _worker->initializeBuffers(InPortCount, OutPortCount);
            
            _worker->moveToThread(_workerThread);
            
            // Register OSC controls
            AbstractDelegateModel::registerOSCControl("/mix", widget->mixSpin);
            AbstractDelegateModel::registerOSCControl("/fade_ms", widget->fadeDurationSpin);

            // Connect signals
            connect(_workerThread, &QThread::started, this, [this]() {
                QMetaObject::invokeMethod(_worker, "startProcessing", Qt::QueuedConnection);
            });
            connect(_workerThread, &QThread::finished, _worker, &AudioCrossFaderWorker::stopProcessing);
            
            connect(widget->mixSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged), 
                    this, [this](double val){ 
                        QMetaObject::invokeMethod(_worker, "setMix", Qt::QueuedConnection, Q_ARG(double, val)); 
                    });
            connect(widget->fadeDurationSpin, QOverload<double>::of(&QDoubleSpinBox::valueChanged),
                    this, [this](double ms){
                        QMetaObject::invokeMethod(_worker, "setFadeDuration", Qt::QueuedConnection, Q_ARG(double, ms));
                    });
            connect(widget->fadeAToBButton, &QPushButton::clicked,
                    this, [this](){
                        QMetaObject::invokeMethod(_worker, "startFadeAToB", Qt::QueuedConnection);
                    });
            connect(widget->fadeBToAButton, &QPushButton::clicked,
                    this, [this](){
                        QMetaObject::invokeMethod(_worker, "startFadeBToA", Qt::QueuedConnection);
                    });
            connect(widget->resetButton, &QPushButton::clicked,
                    this, &AudioCrossFaderDataModel::onResetButtonClicked);
            
            connect(_worker, &AudioCrossFaderWorker::processingStatusChanged, this, &AudioCrossFaderDataModel::onProcessingStatusChanged);
            
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
            modelJson["mix"] = widget->mixSpin->value();
            modelJson["fade_ms"] = widget->fadeDurationSpin->value();
            return modelJson;
        }

        void load(QJsonObject const &p) override
        {
            if (p.contains("mix")) widget->mixSpin->setValue(p["mix"].toDouble());
            if (p.contains("fade_ms")) widget->fadeDurationSpin->setValue(p["fade_ms"].toDouble());
        }

    public slots:
        void onProcessingStatusChanged(bool isProcessing) {
            // Optional: update UI state
        }
        /**
         * @brief 重新发送当前混合比例到工作线程，效果等同于 mixSpin 的 valueChanged
         */
        void onResetButtonClicked() {
            double v = widget->mixSpin->value();
            QMetaObject::invokeMethod(_worker, "setMix", Qt::QueuedConnection, Q_ARG(double, v));
        }

    private:
        AudioCrossFaderInterface *widget;
        AudioCrossFaderWorker* _worker;
        QThread* _workerThread;
    };
}
