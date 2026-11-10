#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include <QDebug>
#include <QSignalBlocker>

#include <QtNodes/NodeDelegateModel>
#include "NodeDataList.hpp"
#include <QtCore/qglobal.h>
#include "MTCGeneratorInterface.h"
#include <QComboBox>
#include <QJsonObject>
#include "PluginDefinition.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "StatusContainer/GlobalEventBus.hpp"
#include "Common/Devices/MtcSender/mtcsender.h"

using QtNodes::NodeData;
using QtNodes::NodeDataType;
using QtNodes::NodeDelegateModel;
using QtNodes::PortIndex;
using QtNodes::PortType;
using namespace NodeDataTypes;

struct GlobalEvent;

namespace Nodes {
    /**
     * @brief MTC 生成节点模型
     * - 向指定 MIDI 输出端口发送 MTC（Quarter-Frame + Full-Frame 定位）
     * - START/RESET/offset 可外部控制（OSC / WebSocket）
     */
    class MTCGeneratorDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(bool running READ running WRITE setRunning NOTIFY runningChanged)
        Q_PROPERTY(int offset READ offset WRITE setOffset NOTIFY offsetChanged)
        Q_PROPERTY(bool reset READ reset WRITE setReset NOTIFY resetChanged)
        Q_PROPERTY(QString device READ device WRITE setDevice NOTIFY deviceChanged)
        Q_PROPERTY(double syncOffset READ syncOffset WRITE setSyncOffset NOTIFY syncOffsetChanged)

    public:
        MTCGeneratorDataModel()
            : _label(new MTCGeneratorInterface())
            , _sender(new MtcSender(this))
        {
            qRegisterMetaType<TimeCodeType>("TimeCodeType");
            InPortCount = 3;   // START, RESET, SYNC
            OutPortCount = 2;  // TIMECODE, START
            CaptionVisible = true;
            WidgetEmbeddable = false;
            Resizable = false;
            PortEditable = false;
            Caption = PLUGIN_NAME;

            _offset = _label->timeCodeOffsetSpinBox->value();
            _running = false;
            _device = _label->midiDeviceComboBox->currentText();

            registerBindings();

            connect(_sender, &MtcSender::timeCodeFrameGenerated,
                    this, &MTCGeneratorDataModel::onReceivedTimecodeFrame);
            connect(_sender, &MtcSender::statusChanged,
                    this, &MTCGeneratorDataModel::onSenderStatusChanged);

            connect(_label, &MTCGeneratorInterface::runningToggled, this, &MTCGeneratorDataModel::setRunning);
            connect(_label, &MTCGeneratorInterface::resetRequested, this, [this]() {
                setReset(true);
            });
            connect(_label, &MTCGeneratorInterface::timeCodeTypeChanged, this, [this](TimeCodeType type) {
                _type = type;
                applySenderConfig();
            });
            connect(_label, &MTCGeneratorInterface::midiDeviceChanged, this, &MTCGeneratorDataModel::setDevice);
            connect(_label->timeCodeOffsetSpinBox, &IntDragValueWidget::valueChanged,
                    this, &MTCGeneratorDataModel::setOffset);

            applySenderConfig();
        }

        ~MTCGeneratorDataModel() override
        {
            GlobalEventBus::instance()->unsubscribe(this);
            if (_sender) {
                // 先断开信号再 stop：stop() 会 emit statusChanged，
                // 而 embeddedWidget(_label) 的归属是节点图形项（QGraphicsProxyWidget），
                // 删除节点时场景可能已先销毁 _label，此时 emit 触发的槽会访问悬挂指针
                _sender->disconnect(this);
                _sender->stop();
            }
        }

        bool running() const { return _running; }
        int offset() const { return _offset; }
        bool reset() const { return _reset; }
        QString device() const { return _device; }
        double syncOffset() const { return _syncOffsetSec; }

        void setRunning(bool running)
        {
            if (_running == running) {
                _label->setRunningChecked(running);
                return;
            }
            _running = running;
            _label->setRunningChecked(running);
            _sender->setPlaying(running);
            emit runningChanged(_running);
            emit dataUpdated(1);
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
            // 注意：reset 是瞬时命令（true 立即执行归零后自动回 false），
            // _reset 仅作同步执行防重入的守卫；Q_PROPERTY 保留供 OSC /reset 反射绑定
            if (!value || _reset) {
                return;
            }
            _reset = true;
            emit resetChanged(true);
            // 归零：时间码复位到 00:00:00:00，并立即发 Full-Frame 定位
            _sender->setTimecode(TimeCodeFrame{0, 0, 0, 0, _type});
            _sender->sendFullFrame();
            _label->setTimeStamp(TimeCodeFrame{0, 0, 0, 0, _type});
            _reset = false;
            emit resetChanged(false);
        }

        void setDevice(const QString &device)
        {
            if (_device == device) {
                return;
            }
            _device = device;
            {
                QSignalBlocker blocker(_label->midiDeviceComboBox);
                const int idx = _label->midiDeviceComboBox->findText(device);
                if (idx >= 0) {
                    _label->midiDeviceComboBox->setCurrentIndex(idx);
                }
            }
            applySenderConfig();
            emit deviceChanged(_device);
        }

        void setSyncOffset(double sec)
        {
            if (qFuzzyCompare(1.0 + _syncOffsetSec, 1.0 + sec)) {
                return;
            }
            _syncOffsetSec = sec;
            _sender->setSyncOffsetSec(_syncOffsetSec);
            emit syncOffsetChanged(_syncOffsetSec);
        }

        NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override
        {
            switch (portType) {
                case PortType::In:
                    return (portIndex == 2) ? SyncData().type() : VariableData().type();
                case PortType::Out:
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
                    if (portIndex == 1) return QStringLiteral("RESET");
                    if (portIndex == 2) return QStringLiteral("SYNC");
                    return {};
                case PortType::Out:
                    if (portIndex == 0) return QStringLiteral("TIMECODE");
                    if (portIndex == 1) return QStringLiteral("START");
                    return {};
                default:
                    return {};
            }
        }

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            switch (port) {
                case 0:
                    return std::make_shared<VariableData>(_lastTimecodeString);
                case 1:
                    return std::make_shared<VariableData>(_running);
                default:
                    return std::make_shared<VariableData>();
            }
        }

        void setInData(std::shared_ptr<NodeData> nodeData, PortIndex const port) override
        {
            // SYNC 输入：绑定/解绑跟随源（断开时传 nullptr 清源）
            if (port == 2) {
                auto sd = std::dynamic_pointer_cast<SyncData>(nodeData);
                std::shared_ptr<SyncSharedState> src = sd ? sd->getSharedState() : nullptr;
                _sender->setSyncSource(src);
                return;
            }

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
            modelJson["device"] = _device;
            modelJson["timecodeType"] = _label->timeCodeTypeComboBox->currentText();
            modelJson["running"] = _running;
            modelJson["syncOffset"] = _syncOffsetSec;
            return modelJson;
        }

        void load(QJsonObject const &jsonObj) override
        {
            if (jsonObj.contains("offset")) {
                setOffset(jsonObj["offset"].toInt());
            }
            if (jsonObj.contains("device")) {
                setDevice(jsonObj["device"].toString());
            }
            if (jsonObj.contains("timecodeType")) {
                const QString label = jsonObj["timecodeType"].toString();
                const int idx = _label->timeCodeTypeComboBox->findText(label);
                if (idx >= 0) {
                    _label->timeCodeTypeComboBox->setCurrentIndex(idx);
                } else {
                    _label->timeCodeTypeComboBox->setCurrentText(_label->timecodeTypeToLabel(TimeCodeType::PAL));
                }
                _type = _label->timecodeTypeFromLabel(label, TimeCodeType::PAL);
                applySenderConfig();
            }
            if (jsonObj.contains("running") && jsonObj["running"].toBool()) {
                setRunning(true);
            }
            if (jsonObj.contains("syncOffset")) {
                setSyncOffset(jsonObj["syncOffset"].toDouble());
            }
        }

    signals:
        void runningChanged(bool running);
        void offsetChanged(int offset);
        void resetChanged(bool reset);
        void deviceChanged(const QString &device);
        void syncOffsetChanged(double syncOffset);

    public slots:
        void onReceivedTimecodeFrame(TimeCodeFrame frame)
        {
            _label->setTimeStamp(frame);
            _lastTimecodeString = timecode_frame_to_string(timecode_frame_add(frame, _offset), _type);
            _label->setStatus(false, QStringLiteral("Generating"));
            emit dataUpdated(0);
        }

        void onSenderStatusChanged(bool error, QString message)
        {
            _label->setStatus(error, message);
        }

        void onGlobalEvent(const GlobalEvent &ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }

            const QString localPath = ev.address.mid(ev.address.lastIndexOf(QLatin1Char('/')) + 1);
            if (localPath == QLatin1String("start")) {
                setRunning(ev.payload.toBool());
            } else if (localPath == QLatin1String("offset")) {
                setOffset(ev.payload.toInt());
            } else if (localPath == QLatin1String("reset")) {
                setReset(ev.payload.toBool());
            } else if (localPath == QLatin1String("device")) {
                setDevice(ev.payload.toString());
            } else if (localPath == QLatin1String("syncOffset")) {
                setSyncOffset(ev.payload.toDouble());
            }
        }

    protected:
        void afterModelReady() override
        {
            AbstractDelegateModel::afterModelReady();
            auto *bus = GlobalEventBus::instance();
            bus->subscribe(makeFullOscAddress(QStringLiteral("/start")), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress(QStringLiteral("/offset")), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress(QStringLiteral("/reset")), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress(QStringLiteral("/device")), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress(QStringLiteral("/syncOffset")), this, SLOT(onGlobalEvent(GlobalEvent)));
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

        /** @brief 按当前 device/type 重建发送器状态（保留播放状态） */
        void applySenderConfig()
        {
            const bool wasRunning = _running;
            if (wasRunning) {
                _sender->stop();
            }
            _sender->start(_device, _type);
            if (wasRunning) {
                _sender->setPlaying(true);
            }
        }

        MTCGeneratorInterface *_label = nullptr;
        MtcSender *_sender = nullptr;
        TimeCodeType _type = TimeCodeType::PAL;
        QString _lastTimecodeString = QStringLiteral("00:00:00:00");
        QString _device;
        bool _running = false;
        int _offset = 0;
        double _syncOffsetSec = 0.0;
        bool _reset = false;
    };
}
