#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include <QDebug>
#include <QSignalBlocker>

#include <QtNodes/NodeDelegateModel>
#include "NodeDataList.hpp"
#include <QtCore/qglobal.h>
#include "LTCGeneratorInterface.h"
#include <QComboBox>
#include <QJsonObject>
#include "PluginDefinition.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
#include "LTCGeneratorWorker.hpp"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

struct GlobalEvent;

namespace Nodes {
    /**
     * @brief LTC生成节点模型
     * - 输出 LTC 音频；START/VOLUME/RESET/offset 可外部控制（OSC / WebSocket）
     */
    class LTCGeneratorDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
        Q_PROPERTY(double volume READ volume WRITE setVolume NOTIFY volumeChanged)
        Q_PROPERTY(int offset READ offset WRITE setOffset NOTIFY offsetChanged)
        Q_PROPERTY(bool reset READ reset WRITE setReset NOTIFY resetChanged)

    public:
        LTCGeneratorDataModel()
            : _label(new TimeCodeInterface())
            , _worker(new LTCGeneratorWorker())
            , _workerThread(new QThread(this))
        {
            qRegisterMetaType<TimeCodeType>("TimeCodeType");
            InPortCount = 3;   // START, VOLUME, RESET
            OutPortCount = 3;  // AUDIO, START, VOLUME
            CaptionVisible = true;
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = false;
            Caption = PLUGIN_NAME;

            _volumeDb = static_cast<double>(_label->volumeSlider->value());
            _offset = _label->timeCodeOffsetSpinBox->value();
            _running = false;

            registerBindings();

            _worker->moveToThread(_workerThread);

            connect(_worker, &LTCGeneratorWorker::timeCodeFrameGenerated,
                    this, &LTCGeneratorDataModel::onReceivedTimecodeFrame);
            connect(_worker, &LTCGeneratorWorker::processingStatusChanged,
                    this, &LTCGeneratorDataModel::onProcessingStatusChanged);

            connect(_label, &TimeCodeInterface::runningToggled, this, &LTCGeneratorDataModel::setRunning);
            connect(_label, &TimeCodeInterface::resetRequested, this, [this]() {
                setReset(true);
            });
            connect(_label, &TimeCodeInterface::timeCodeTypeChanged, this, [this](TimeCodeType type) {
                QMetaObject::invokeMethod(_worker, "setTimeCodeType", Qt::QueuedConnection, Q_ARG(TimeCodeType, type));
            });
            connect(_label, &TimeCodeInterface::volumeChanged, this, [this](float volume) {
                setVolume(static_cast<double>(volume));
            });
            connect(_label->timeCodeOffsetSpinBox, &IntDragValueWidget::valueChanged,
                    this, &LTCGeneratorDataModel::setOffset);

            _workerThread->start();

            QMetaObject::invokeMethod(
                _worker,
                "setTimeCodeType",
                Qt::QueuedConnection,
                Q_ARG(TimeCodeType, timecode_type_from_label(_label->timeCodeTypeComboBox->currentText(), TimeCodeType::PAL)));
            QMetaObject::invokeMethod(
                _worker,
                "setVolume",
                Qt::QueuedConnection,
                Q_ARG(float, static_cast<float>(_volumeDb)));
        }

        ~LTCGeneratorDataModel() override
        {
            GlobalEventBus::instance()->unsubscribe(this);
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

        bool running() const { return _running; }
        double volume() const { return _volumeDb; }
        int offset() const { return _offset; }
        bool reset() const { return _reset; }

        void setRunning(bool running)
        {
            if (_running == running) {
                _label->setRunningChecked(running);
                return;
            }
            _running = running;
            _label->setRunningChecked(running);
            if (running) {
                QMetaObject::invokeMethod(_worker, "startProcessing", Qt::QueuedConnection);
            } else {
                QMetaObject::invokeMethod(_worker, "stopProcessing", Qt::QueuedConnection);
            }
            emit runningChanged(_running);
            emit dataUpdated(1);
        }

        void setVolume(double volumeDb)
        {
            if (qFuzzyCompare(1.0 + _volumeDb, 1.0 + volumeDb)) {
                {
                    QSignalBlocker blocker(_label->volumeSlider);
                    _label->volumeSlider->setValue(volumeDb);
                }
                return;
            }
            _volumeDb = volumeDb;
            {
                QSignalBlocker blocker(_label->volumeSlider);
                _label->volumeSlider->setValue(_volumeDb);
            }
            QMetaObject::invokeMethod(_worker, "setVolume", Qt::QueuedConnection,
                                      Q_ARG(float, static_cast<float>(_volumeDb)));
            emit volumeChanged(_volumeDb);
            emit dataUpdated(2);
        }

        void setOffset(int value)
        {
            if (_offset == value) {
                return;
            }
            _offset = value;
            {
                QSignalBlocker blocker(_label->timeCodeOffsetSpinBox);
                _label->timeCodeOffsetSpinBox->setValue(_offset);
            }
            emit offsetChanged(_offset);
        }

        void setReset(bool value)
        {
            if (!value || _reset) {
                return;
            }
            _reset = true;
            emit resetChanged(true);
            QMetaObject::invokeMethod(_worker, "resetTimecode", Qt::QueuedConnection);
            _reset = false;
            emit resetChanged(false);
        }

        NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override
        {
            switch (portType) {
                case PortType::In:
                    return VariableData().type();
                case PortType::Out:
                    if (portIndex == 0) {
                        return AudioData().type();
                    }
                    return VariableData().type();
                default:
                    return VariableData().type();
            }
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            switch (portType) {
                case PortType::In:
                    if (portIndex == 0) return QStringLiteral("START");
                    if (portIndex == 1) return QStringLiteral("VOLUME");
                    if (portIndex == 2) return QStringLiteral("RESET");
                    return {};
                case PortType::Out:
                    if (portIndex == 0) return QStringLiteral("AUDIO");
                    if (portIndex == 1) return QStringLiteral("START");
                    if (portIndex == 2) return QStringLiteral("VOLUME");
                    return {};
                default:
                    return {};
            }
        }

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            switch (port) {
                case 0: {
                    auto audioData = std::make_shared<AudioData>();
                    audioData->setSharedAudioBuffer(_worker->getOutputBuffer());
                    return audioData;
                }
                case 1:
                    return std::make_shared<VariableData>(_running);
                case 2:
                    return std::make_shared<VariableData>(_volumeDb);
                default:
                    return std::make_shared<VariableData>();
            }
        }

        void setInData(std::shared_ptr<NodeData> nodeData, PortIndex const port) override
        {
            if (!nodeData) {
                return;
            }
            auto varData = std::dynamic_pointer_cast<VariableData>(nodeData);
            if (!varData) {
                return;
            }

            switch (port) {
                case 0:
                    setRunning(varData->asBool());
                    break;
                case 1:
                    setVolume(varData->asNumber());
                    break;
                case 2:
                    if (varData->asBool()) {
                        setReset(true);
                    }
                    break;
                default:
                    break;
            }
        }

        QWidget *embeddedWidget() override { return _label; }

        QJsonObject save() const override
        {
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["offset"] = _offset;
            modelJson["timecodeType"] = _label->timeCodeTypeComboBox->currentText();
            modelJson["volume"] = _volumeDb;
            modelJson["running"] = _running;
            return modelJson;
        }

        void load(QJsonObject const &jsonObj) override
        {
            if (jsonObj.contains("offset")) {
                setOffset(jsonObj["offset"].toInt());
            }
            if (jsonObj.contains("timecodeType")) {
                const QString label = jsonObj["timecodeType"].toString();
                const int idx = _label->timeCodeTypeComboBox->findText(label);
                if (idx >= 0) {
                    _label->timeCodeTypeComboBox->setCurrentIndex(idx);
                } else {
                    _label->timeCodeTypeComboBox->setCurrentText(timecode_type_to_label(TimeCodeType::PAL));
                }
                QMetaObject::invokeMethod(_worker, "setTimeCodeType", Qt::QueuedConnection,
                                          Q_ARG(TimeCodeType, timecode_type_from_label(label, TimeCodeType::PAL)));
            }
            if (jsonObj.contains("volume")) {
                setVolume(jsonObj["volume"].toDouble());
            }
            if (jsonObj.contains("running") && jsonObj["running"].toBool()) {
                setRunning(true);
            }
        }

    signals:
        void runningChanged(bool running);
        void volumeChanged(double volume);
        void offsetChanged(int offset);
        void resetChanged(bool reset);

    public slots:
        void onReceivedTimecodeFrame(TimeCodeFrame frame)
        {
            _label->setTimeStamp(frame);
            _timeCodeFrame = timecode_frame_add(frame, _offset);
            _label->setStatus(false, QStringLiteral("Generating"));
            emit dataUpdated(0);
        }

        void onProcessingStatusChanged(bool isProcessing)
        {
            if (_running != isProcessing) {
                _running = isProcessing;
                emit runningChanged(_running);
                emit dataUpdated(1);
            }
            _label->setRunningChecked(isProcessing);
            if (isProcessing) {
                _label->setStatus(false, QStringLiteral("Processing"));
            } else {
                _label->setStatus(true, QStringLiteral("Idle"));
            }
        }

        void onGlobalEvent(const GlobalEvent &ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }

            const QString localPath = ev.address.mid(ev.address.lastIndexOf(QLatin1Char('/')) + 1);
            if (localPath == QLatin1String("start")) {
                setRunning(ev.payload.toBool());
            } else if (localPath == QLatin1String("volume")) {
                setVolume(ev.payload.toDouble());
            } else if (localPath == QLatin1String("offset")) {
                setOffset(ev.payload.toInt());
            } else if (localPath == QLatin1String("reset")) {
                setReset(ev.payload.toBool());
            }
        }

    protected:
        void afterModelReady() override
        {
            AbstractDelegateModel::afterModelReady();
            auto *bus = GlobalEventBus::instance();
            bus->subscribe(makeFullOscAddress(QStringLiteral("/start")), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress(QStringLiteral("/volume")), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress(QStringLiteral("/offset")), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress(QStringLiteral("/reset")), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

    private:
        void registerBindings()
        {
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "running";
                b.control = _label->startButton;
                AbstractDelegateModel::registerExternalBinding(QStringLiteral("/start"), this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "volume";
                b.control = _label->volumeSlider;
                AbstractDelegateModel::registerExternalBinding(QStringLiteral("/volume"), this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "offset";
                b.control = _label->timeCodeOffsetSpinBox;
                AbstractDelegateModel::registerExternalBinding(QStringLiteral("/offset"), this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "reset";
                b.control = _label->resetButton;
                AbstractDelegateModel::registerExternalBinding(QStringLiteral("/reset"), this, b);
            }
        }

        TimeCodeInterface *_label = nullptr;
        TimeCodeFrame _timeCodeFrame;
        LTCGeneratorWorker *_worker = nullptr;
        QThread *_workerThread = nullptr;
        bool _running = false;
        double _volumeDb = -25.0;
        int _offset = 0;
        bool _reset = false;
    };
}
