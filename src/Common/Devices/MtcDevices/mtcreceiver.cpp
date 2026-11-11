#include "mtcreceiver.h"

#include <RtMidi.h>
#include <QDebug>
#include <QMetaObject>

MtcReceiver::MtcReceiver(QObject *parent)
    : QObject(parent)
{
    _decoder = new MtcDecoder(this);
    connect(_decoder, &MtcDecoder::newFrame, this, &MtcReceiver::newFrame);
    connect(_decoder, &MtcDecoder::frameRateDetected, this, &MtcReceiver::frameRateDetected);
}

MtcReceiver::~MtcReceiver()
{
    stop();
}

QStringList MtcReceiver::availableDevices()
{
    QStringList list;
    try {
        RtMidiIn midi;
        const unsigned int count = midi.getPortCount();
        for (unsigned int i = 0; i < count; ++i) {
            list << QString("%1: %2").arg(i).arg(QString::fromStdString(midi.getPortName(i)));
        }
    } catch (const RtMidiError &e) {
        qWarning() << "MtcReceiver: 枚举 MIDI 输入设备失败:" << QString::fromStdString(e.getMessage());
    }
    return list;
}

void MtcReceiver::start(QString deviceName)
{
    stop();

    // 空设备名防御："" .split(":")[0].toInt() == 0 会误开端口 0
    if (deviceName.trimmed().isEmpty()) {
        _currentDeviceName = deviceName;
        emit statusChanged(true, QStringLiteral("未选择 MIDI 输入设备"));
        return;
    }

    _currentDeviceName = deviceName;
    _portIndex = deviceName.split(":").value(0).toInt();

    // 换源时清掉上一设备的 QF 组帧残留，避免错组时间码
    _decoder->reset();

    try {
        _midiIn = new RtMidiIn();
        // MTC Full-Frame 是 SysEx：必须打开 SysEx 透传（默认被 RtMidi 忽略）。
        // 注意：midiTime 标志不仅过滤 0xF8 时钟，还会把 MTC Quarter-Frame(0xF1)
        // 一并丢弃（见 RtMidi.cpp WinMM 回调 status==0xF1 分支），因此必须为 false。
        _midiIn->ignoreTypes(false /*sysex*/, false /*time: 含 MTC QF!*/, true /*sensing*/);
        _midiIn->setCallback(&MtcReceiver::midiCallback, this);

        const unsigned int count = _midiIn->getPortCount();
        if (_portIndex < 0 || static_cast<unsigned int>(_portIndex) >= count) {
            emit statusChanged(true, QStringLiteral("找不到指定的 MIDI 输入设备"));
            delete _midiIn;
            _midiIn = nullptr;
            return;
        }

        _midiIn->openPort(static_cast<unsigned int>(_portIndex),
                          "Flow-MTC-In");
    } catch (const RtMidiError &e) {
        emit statusChanged(true, QStringLiteral("打开 MIDI 设备失败: %1")
                                      .arg(QString::fromStdString(e.getMessage())));
        delete _midiIn;
        _midiIn = nullptr;
        return;
    }

    _isRunning = true;
    emit statusChanged(false, QStringLiteral("listening"));
}

void MtcReceiver::stop()
{
    if (!_isRunning && !_midiIn) {
        return;
    }

    if (_midiIn) {
        try {
            _midiIn->closePort();
        } catch (...) {
        }
        delete _midiIn; // RtMidiIn 析构会取消回调并 join 输入线程
        _midiIn = nullptr;
    }

    _isRunning = false;
    emit statusChanged(true, QStringLiteral("stopped"));
}

void MtcReceiver::midiCallback(double /*timeStamp*/,
                               std::vector<unsigned char> *message,
                               void *userData)
{
    auto *self = static_cast<MtcReceiver *>(userData);
    if (!self || !self->_decoder || !message || message->empty()) {
        return;
    }

    // 回调线程 → 解码器（主线程）：经队列连接跨线程投递
    QByteArray data(reinterpret_cast<const char *>(message->data()),
                    static_cast<int>(message->size()));
    QMetaObject::invokeMethod(self->_decoder, "feedMidi", Qt::QueuedConnection,
                              Q_ARG(QByteArray, data));
}
