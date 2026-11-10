#pragma once

#include <QtCore/QObject>
#include <QtWidgets/QLabel>
#include <QTimer>
#include <QDebug>
#include <QSignalBlocker>

#include <QtNodes/NodeDelegateModel>
#include <QtNodes/NodeDelegateModelRegistry>
#include "NodeDataList.hpp"
#include "TimestampGenerator/TimestampGenerator.hpp"
#include <QtCore/qglobal.h>
#include "MTCDecoderInterface.h"
#include <QComboBox>
#include <QJsonObject>
#include "PluginDefinition.hpp"
#include "Common/BaseClass/AbstractDelegateModel.h"
#include "Common/Devices/MtcReceiver/mtcreceiver.h"
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
     * @brief MTC解码节点模型
     * - 从指定 MIDI 输入设备接收 MTC，解码为时间码
     * - Out 0~4：FRAMES/SECONDS/MINUTES/HOURS/TOTAL（VariableData）
     * - Out 5：SYNC 时钟源（TC 含用户偏移映射为内部秒，可驱动 AudioDecoder 等从机节点）
     * - >500ms 无新帧 → SYNC 置无效（跟随端保持定格，TC 恢复自动重新锁定）
     */
    class MTCDecoderDataModel : public AbstractDelegateModel
    {
        Q_OBJECT
        Q_PROPERTY(QString device READ device WRITE setDevice NOTIFY deviceChanged)
        Q_PROPERTY(int offset READ offset WRITE setOffset NOTIFY offsetChanged)

    public:
        MTCDecoderDataModel()
            : _label(new MTCDecoderInterface())
            , _receiver(new MtcReceiver(this))
        {
            InPortCount = 0;
            OutPortCount = 6;
            CaptionVisible = true;
            Resizable = false;
            PortEditable = false;
            WidgetEmbeddable = false;
            Caption = PLUGIN_NAME;

            _device = _label->midiDeviceComboBox->currentText();

            // 同步源状态：对外以 SyncData（内部秒）发布，图内同步统一走全局时钟
            m_syncState = std::make_shared<SyncSharedState>(QStringLiteral("MTCDecoder"));

            // TC 失联看门狗：>500ms 无新帧则置无效（跟随端据此联动停止）
            m_mtcWatchdog = new QTimer(this);
            m_mtcWatchdog->setInterval(200);
            connect(m_mtcWatchdog, &QTimer::timeout, this, [this]() {
                if (m_lastFrameTickMs > 0
                    && QDateTime::currentMSecsSinceEpoch() - m_lastFrameTickMs > 500) {
                    m_syncState->setActive(false);
                }
            });
            m_mtcWatchdog->start();

            // MTC 接收：MtcReceiver 内部持有解码器，主线程信号直达
            connect(_receiver, &MtcReceiver::newFrame,
                    this, &MTCDecoderDataModel::onReceivedTimecodeFrame);
            // 帧率格式自动识别（来自 MTC 码流 piece7 / Full-Frame 高位标志）
            connect(_receiver, &MtcReceiver::frameRateDetected,
                    this, [this](TimeCodeType type) {
                        m_detectedFps = timecode_frames_per_sec(type);
                    });
            connect(_receiver, &MtcReceiver::statusChanged, this, [this](bool error, const QString &msg) {
                if (msg == QStringLiteral("listening")) {
                    _label->setStatus(false, QStringLiteral("Listening"));
                } else if (msg == QStringLiteral("stopped")) {
                    _label->setStatus(true, QStringLiteral("Idle"));
                } else {
                    _label->setStatus(error, msg);
                }
            });

            // 设备切换
            connect(_label, &MTCDecoderInterface::midiDeviceChanged, this, &MTCDecoderDataModel::setDevice);

            // 立即打开默认设备（若有）
            if (!_device.isEmpty()) {
                _receiver->start(_device);
            }

            registerBindings();
        }

        ~MTCDecoderDataModel() override
        {
            GlobalEventBus::instance()->unsubscribe(this);
            if (_receiver) {
                // 先断开信号再 stop：stop() 会 emit statusChanged，
                // 而 embeddedWidget(_label) 的归属是节点图形项（QGraphicsProxyWidget），
                // 删除节点时场景可能已先销毁 _label，此时 emit 触发的槽会访问悬挂指针
                _receiver->disconnect(this);
                _receiver->stop();
            }
        }

        QString device() const { return _device; }
        int offset() const { return _offset; }

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
            if (_receiver) {
                _receiver->stop();
                if (!_device.isEmpty()) {
                    _receiver->start(_device);
                }
            }
            emit deviceChanged(_device);
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

        NodeDataType dataType(PortType const portType, PortIndex const portIndex) const override
        {
            Q_UNUSED(portType);
            return (portIndex == 5) ? SyncData().type() : VariableData().type();
        }

        QString portCaption(QtNodes::PortType portType, QtNodes::PortIndex portIndex) const override
        {
            if (portType != PortType::Out) {
                return {};
            }
            switch (portIndex) {
                case 0: return QStringLiteral("FRAMES");
                case 1: return QStringLiteral("SECONDS");
                case 2: return QStringLiteral("MINUTES");
                case 3: return QStringLiteral("HOURS");
                case 4: return QStringLiteral("TOTAL");
                case 5: return QStringLiteral("SYNC");
                default: return {};
            }
        }

        std::shared_ptr<NodeData> outData(PortIndex const port) override
        {
            switch (port) {
                case 5:
                    // SYNC 时钟源输出口：TC 已映射为内部秒（含用户偏移）
                    return std::make_shared<SyncData>(m_syncState);
                case 0:
                    return std::make_shared<VariableData>(_timeCodeFrame.frames);
                case 1:
                    return std::make_shared<VariableData>(_timeCodeFrame.seconds);
                case 2:
                    return std::make_shared<VariableData>(_timeCodeFrame.minutes);
                case 3:
                    return std::make_shared<VariableData>(_timeCodeFrame.hours);
                case 4:
                    return std::make_shared<VariableData>(timecode_frame_to_frames(_timeCodeFrame, _timeCodeFrame.type));
                default:
                    return std::make_shared<VariableData>();
            }
        }

        QWidget *embeddedWidget() override { return _label; }

        /** @brief 无输入端口（MTC 来自 MIDI 设备），空实现满足纯虚接口 */
        void setInData(std::shared_ptr<NodeData>, PortIndex const) override {}

        QJsonObject save() const override
        {
            QJsonObject modelJson = NodeDelegateModel::save();
            modelJson["offset"] = _offset;
            modelJson["device"] = _device;
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
        }

    signals:
        void deviceChanged(const QString &device);
        void offsetChanged(int offset);

    public slots:
        /**
         * @brief 接收到时间码帧的槽函数（主线程）
         * @param frame 解码得到的时间码帧
         */
        void onReceivedTimecodeFrame(TimeCodeFrame frame)
        {
            _label->setTimeStamp(frame);
            _timeCodeFrame = timecode_frame_add(frame, _offset);
            _label->setStatus(false, m_detectedFps > 0
                ? QStringLiteral("Decoding · %1fps").arg(m_detectedFps)
                : QStringLiteral("Decoding"));

            // ---- 时钟源发布：TC（含用户偏移）→ 内部秒 → 共享同步状态 ----
            {
                const int fps = timecode_frames_per_sec(_timeCodeFrame.type);
                const double sec = _timeCodeFrame.hours * 3600.0
                    + _timeCodeFrame.minutes * 60.0
                    + _timeCodeFrame.seconds
                    + double(_timeCodeFrame.frames) / double(fps > 0 ? fps : 25);
                m_syncState->setActive(true);
                m_syncState->write(sec,
                                   TimestampGenerator::getInstance()->getCurrentFrameCount(),
                                   true);
                m_lastFrameTickMs = QDateTime::currentMSecsSinceEpoch();
            }

            // 通知输出端口数据更新
            emit dataUpdated(0);
            emit dataUpdated(1);
            emit dataUpdated(2);
            emit dataUpdated(3);
            emit dataUpdated(4);
            emit dataUpdated(5);
        }

        void onGlobalEvent(const GlobalEvent &ev)
        {
            if (ev.kind != GlobalEventKind::Command) {
                return;
            }

            const QString localPath = ev.address.mid(ev.address.lastIndexOf(QLatin1Char('/')) + 1);
            if (localPath == QLatin1String("device")) {
                setDevice(ev.payload.toString());
            } else if (localPath == QLatin1String("offset")) {
                setOffset(ev.payload.toInt());
            }
        }

    protected:
        void afterModelReady() override
        {
            AbstractDelegateModel::afterModelReady();
            auto *bus = GlobalEventBus::instance();
            bus->subscribe(makeFullOscAddress(QStringLiteral("/device")), this, SLOT(onGlobalEvent(GlobalEvent)));
            bus->subscribe(makeFullOscAddress(QStringLiteral("/offset")), this, SLOT(onGlobalEvent(GlobalEvent)));
        }

    private:
        void registerBindings()
        {
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "device";
                b.control = _label->midiDeviceComboBox;
                AbstractDelegateModel::registerExternalBinding(QStringLiteral("/device"), this, b);
            }
            {
                NodeDelegateModel::ExternalBinding b;
                b.member = "offset";
                b.control = _label->timeCodeOffsetSpinBox;
                AbstractDelegateModel::registerExternalBinding(QStringLiteral("/offset"), this, b);
            }
        }

        MTCDecoderInterface *_label = nullptr;                          ///< 时间码显示界面
        TimeCodeFrame _timeCodeFrame;                                   ///< 当前时间码帧
        MtcReceiver *_receiver = nullptr;                               ///< MTC 接收器（含解码器）

        // ---- SyncData 时钟源发布 ----
        std::shared_ptr<SyncSharedState> m_syncState;                   ///< 对外发布的同步状态（TC→内部秒）
        QTimer *m_mtcWatchdog = nullptr;                                ///< TC 失联看门狗
        qint64 m_lastFrameTickMs = 0;                                   ///< 最后收到 TC 帧的时刻（ms）
        int m_detectedFps = 0;                                          ///< 自动识别的帧率（0 = 尚未识别）

        QString _device;
        int _offset = 0;
    };
}
