#include "mtcsender.h"

#include <RtMidi.h>
#include <QDebug>
#include <cmath>
#include <vector>

namespace {
constexpr int kTickIntervalMs = 5; ///< 定时器粒度；发送数量按耗时折算，与精度无关
constexpr int kMaxQfPerTick = 64;  ///< 单次 tick 追赶上限，防止挂起后猛发
} // namespace

MtcSender::MtcSender(QObject *parent)
    : QObject(parent)
{
    _timer = new QTimer(this);
    _timer->setTimerType(Qt::PreciseTimer);
    _timer->setInterval(kTickIntervalMs);
    connect(_timer, &QTimer::timeout, this, &MtcSender::tick);
}

MtcSender::~MtcSender()
{
    stop();
}

QStringList MtcSender::availableDevices()
{
    QStringList list;
    try {
        RtMidiOut midi;
        const unsigned int count = midi.getPortCount();
        for (unsigned int i = 0; i < count; ++i) {
            list << QString("%1: %2").arg(i).arg(QString::fromStdString(midi.getPortName(i)));
        }
    } catch (const RtMidiError &e) {
        qWarning() << "MtcSender: 枚举 MIDI 输出设备失败:" << QString::fromStdString(e.getMessage());
    }
    return list;
}

int MtcSender::rateBits() const
{
    // MTC 帧率标志（piece7 / Full-Frame 高位）：00=24, 01=25, 10=30DF, 11=30
    switch (_type) {
        case TimeCodeType::Film:    return 0;
        case TimeCodeType::Film_DF: return 0;
        case TimeCodeType::PAL:     return 1;
        case TimeCodeType::HD:      return 1;
        case TimeCodeType::NTSC_DF: return 2;
        case TimeCodeType::NTSC:    return 3;
        default:                    return 1;
    }
}

void MtcSender::start(const QString &deviceName, TimeCodeType type)
{
    stop();

    // 空设备名防御："".split(":")[0].toInt() == 0 会误开端口 0
    if (deviceName.trimmed().isEmpty()) {
        _deviceName = deviceName;
        emit statusChanged(true, QStringLiteral("未选择 MIDI 输出设备"));
        return;
    }

    _type = type;
    _base = TimeCodeFrame{0, 0, 0, 0, _type};
    _qfCount = 0;
    _deviceName = deviceName;

    const int portIndex = deviceName.split(":").value(0).toInt();

    try {
        _midiOut = new RtMidiOut();
        const unsigned int count = _midiOut->getPortCount();
        if (portIndex < 0 || static_cast<unsigned int>(portIndex) >= count) {
            emit statusChanged(true, QStringLiteral("找不到指定的 MIDI 输出设备"));
            delete _midiOut;
            _midiOut = nullptr;
            return;
        }
        _midiOut->openPort(static_cast<unsigned int>(portIndex), "Flow-MTC-Out");
    } catch (const RtMidiError &e) {
        emit statusChanged(true, QStringLiteral("打开 MIDI 设备失败: %1")
                                      .arg(QString::fromStdString(e.getMessage())));
        delete _midiOut;
        _midiOut = nullptr;
        return;
    }

    // 定时器常开：自由模式下 _playing=false 时 tick 直接返回；
    // SYNC 模式下源暂停时仍需轮询以恢复跟随
    _timer->start();

    emit statusChanged(false, QStringLiteral("ready"));
}

void MtcSender::stop()
{
    _timer->stop();
    _playing = false;

    if (_midiOut) {
        try {
            _midiOut->closePort();
        } catch (...) {
        }
        delete _midiOut;
        _midiOut = nullptr;
        emit statusChanged(true, QStringLiteral("stopped"));
    }
}

void MtcSender::setTimecode(const TimeCodeFrame &frame)
{
    _base = frame;
    _base.type = _type;
    _qfCount = 0;
    _clock.restart();
}

void MtcSender::setSyncSource(std::shared_ptr<NodeDataTypes::SyncSharedState> src)
{
    const bool hadSource = (_syncSource != nullptr);
    _syncSource = std::move(src);
    // 状态切换：清 hold 标记，让下一轮 followSync 重新对齐
    _syncHold = false;

    // 源被移除（SYNC 连接断开）：立即定格并停发，避免按旧时钟"幽灵自跑"
    if (!_syncSource && hadSource) {
        _playing = false;
        sendFullFrame();   // 定位到当前定格位置，便于下游对齐
    }

    if (_timer && _syncSource && !_timer->isActive()) {
        _timer->start(); // 同步模式下定时器常开用于轮询源状态
    }
}

void MtcSender::sendFullFrame()
{
    if (!_midiOut) {
        return;
    }

    // Universal Realtime SysEx：F0 7F <device> <sub-ID 01 MTC> <sub-ID 02 FullFrame> hh mm ss ff F7
    // 复用成员缓冲，避免每次堆分配（仅主线程 tick/followSync 路径使用）
    const int rb = rateBits();
    _msgBuffer.clear();
    _msgBuffer.reserve(10);
    _msgBuffer.push_back(0xF0);
    _msgBuffer.push_back(0x7F);
    _msgBuffer.push_back(0x7F); // device: all-calls
    _msgBuffer.push_back(0x01); // sub-ID 1: MIDI Time Code
    _msgBuffer.push_back(0x01); // sub-ID 2: Full Frame
    _msgBuffer.push_back(static_cast<unsigned char>((_base.hours & 0x1F) | (rb << 5)));
    _msgBuffer.push_back(static_cast<unsigned char>(_base.minutes & 0x3F));
    _msgBuffer.push_back(static_cast<unsigned char>(_base.seconds & 0x3F));
    _msgBuffer.push_back(static_cast<unsigned char>(_base.frames & 0x1F));
    _msgBuffer.push_back(0xF7);

    try {
        _midiOut->sendMessage(&_msgBuffer);
    } catch (const RtMidiError &e) {
        qWarning() << "MtcSender: 发送 Full-Frame 失败:" << QString::fromStdString(e.getMessage());
    }
}

void MtcSender::setPlaying(bool playing)
{
    if (_playing == playing) {
        return;
    }
    _playing = playing;

    if (playing) {
        if (!_midiOut) {
            emit statusChanged(true, QStringLiteral("MIDI 输出设备未打开"));
            _playing = false;
            return;
        }
        _qfCount = 0;
        _clock.restart();
        if (!_syncSource) {
            sendFullFrame();   // 先定位，再进入 QF 流（同步模式下由 followSync 负责定位）
        }
        _timer->start();
    } else {
        if (!_syncSource) {
            _timer->stop();    // 同步模式下定时器保持运行（轮询源状态）
        }
        sendFullFrame();      // 停止时发最终定位，便于下游对齐
    }
}

TimeCodeFrame MtcSender::windowTime(qint64 qfIndex) const
{
    // 每 8 个 QF 覆盖 2 帧：window = qfIndex / 8
    const qint64 window = qfIndex / 8;
    return timecode_frame_add(_base, static_cast<int>(window * 2));
}

void MtcSender::tick()
{
    if (!_midiOut) {
        return;
    }

    // SYNC 跟随模式：先根据源状态/位置校正播放与时间码
    if (_syncSource) {
        followSync();
    }

    if (!_playing || !_midiOut) {
        return;
    }

    const int fps = timecode_frames_per_sec(_type);
    const qint64 elapsedMs = _clock.elapsed();
    const qint64 targetQf = static_cast<qint64>(std::floor(elapsedMs * fps * 4.0 / 1000.0));

    int guard = 0;
    while (_qfCount < targetQf && guard++ < kMaxQfPerTick) {
        const int piece = static_cast<int>(_qfCount % 8);
        const TimeCodeFrame t = windowTime(_qfCount);

        // 规范：piece 0~3 编码 2 帧窗口起始时间；piece 4~7 编码 +1 帧时间
        TimeCodeFrame encode = t;
        if (piece >= 4) {
            encode = timecode_frame_add(t, 1);
        }

        int value = 0;
        switch (piece) {
            case 0: value = encode.frames & 0x0F; break;
            case 1: value = (encode.frames >> 4) & 0x03; break;
            case 2: value = encode.seconds & 0x0F; break;
            case 3: value = (encode.seconds >> 4) & 0x07; break;
            case 4: value = encode.minutes & 0x0F; break;
            case 5: value = (encode.minutes >> 4) & 0x07; break;
            case 6: value = encode.hours & 0x0F; break;
            case 7: value = ((encode.hours >> 4) & 0x03) | (rateBits() << 1); break;
            default: break;
        }
        sendQuarterFrame(piece, value);

        // 每跨过完整 1 帧（4 个 QF）发一次 UI 更新：
        // 显示当前正在线上传输的帧号（= 真实位置取整），不超前 +1，
        // 否则面板会比实际发出的 MTC（以及 Decoder 端）凭空多 1 帧
        if (_qfCount % 4 == 3) {
            emit timeCodeFrameGenerated(timecode_frame_add(_base, static_cast<int>(_qfCount / 4)));
        }

        ++_qfCount;
    }
}

void MtcSender::followSync()
{
    const NodeDataTypes::SyncState snap = _syncSource->read();
    const bool active = snap.active && snap.playing;

    if (!active) {
        if (!_syncHold) {
            // 首次进入定格：时间码对齐到源位置并停发 QF
            double targetSec = _syncSource->expectedNowSec() + _syncOffsetSec;
            if (targetSec < 0.0) {
                targetSec = 0.0;
            }
            setTimecode(time_to_timecode_frame(targetSec, _type));
            sendFullFrame();
            _syncHold = true;
        }
        _playing = false;
        return;
    }
    _syncHold = false;
    _playing = true;

    // 外推期望位置 → 目标帧号，与当前编码帧号比较，超死区（1 帧）重新对齐
    const int fps = timecode_frames_per_sec(_type);
    double targetSec = _syncSource->expectedNowSec() + _syncOffsetSec;
    if (targetSec < 0.0) {
        targetSec = 0.0;
    }
    const qint64 targetFrames = static_cast<qint64>(std::floor(targetSec * fps + 0.5));
    const qint64 currentFrames = timecode_frame_to_frames(_base, _type) + _qfCount / 4;

    if (std::abs(targetFrames - currentFrames) > 1) {
        setTimecode(time_to_timecode_frame(targetSec, _type));
        sendFullFrame();
    }
}

void MtcSender::sendQuarterFrame(int piece, int value)
{
    // 复用成员缓冲：QF 以 4×帧率 频率发送（25fps 即 100 次/秒），避免每条堆分配
    _msgBuffer.clear();
    _msgBuffer.reserve(2);
    _msgBuffer.push_back(0xF1);
    _msgBuffer.push_back(static_cast<unsigned char>(((piece & 0x07) << 4) | (value & 0x0F)));
    try {
        _midiOut->sendMessage(&_msgBuffer);
    } catch (const RtMidiError &e) {
        qWarning() << "MtcSender: 发送 QF 失败:" << QString::fromStdString(e.getMessage());
    }
}
