#include "mtcdecoder.h"

#include <QDebug>

MtcDecoder::MtcDecoder(QObject *parent)
    : QObject(parent)
{
}

void MtcDecoder::reset()
{
    for (int &n : _nibbles) {
        n = 0;
    }
    _lastPiece = -1;
    _haveHighGroup = false;
    _haveLowGroup = false;
    _frameRateFlag = -1;
    _sysexBuffer.clear();
    _inSysex = false;
}

void MtcDecoder::feedMidi(const QByteArray &data)
{
    const unsigned char *bytes = reinterpret_cast<const unsigned char *>(data.constData());
    const int size = static_cast<int>(data.size());

    for (int i = 0; i < size; ++i) {
        const unsigned char b = bytes[i];

        if (_inSysex) {
            if (b == 0xF7) {
                // SysEx 结束：尝试按 Full-Frame 解析
                _inSysex = false;
                // Universal Realtime SysEx: F0 7F <device 7F> <sub-ID 01> <MTC type 01> hh mm ss ff
                if (_sysexBuffer.size() == 8
                    && static_cast<unsigned char>(_sysexBuffer.at(0)) == 0x7F
                    && static_cast<unsigned char>(_sysexBuffer.at(1)) == 0x01
                    && static_cast<unsigned char>(_sysexBuffer.at(2)) == 0x01) {
                    const int hr = _sysexBuffer.at(3) & 0xFF;
                    feedFullFrame(hr & 0x1F,
                                  _sysexBuffer.at(4),
                                  _sysexBuffer.at(5),
                                  _sysexBuffer.at(6),
                                  (hr >> 5) & 0x03);
                }
                _sysexBuffer.clear();
            } else if (b >= 0x80) {
                // SysEx 中途被其他状态字节打断：放弃
                _inSysex = false;
                _sysexBuffer.clear();
                // 该字节本身可能是新消息的起始，落到下方统一处理
                if (b == 0xF1 && i + 1 < size) {
                    feedQuarterFrame((bytes[i + 1] >> 4) & 0x07, bytes[i + 1] & 0x0F);
                    ++i;
                }
            } else {
                _sysexBuffer.append(static_cast<char>(b));
                if (_sysexBuffer.size() > 64) {
                    // 异常超长，放弃以免堆积
                    _inSysex = false;
                    _sysexBuffer.clear();
                }
            }
            continue;
        }

        if (b == 0xF0) {
            // SysEx 开始
            _inSysex = true;
            _sysexBuffer.clear();
            continue;
        }

        if (b == 0xF1) {
            // Quarter-Frame：紧跟一个数据字节
            if (i + 1 < size) {
                feedQuarterFrame((bytes[i + 1] >> 4) & 0x07, bytes[i + 1] & 0x0F);
                ++i;
            }
            continue;
        }

        if (b >= 0x80) {
            // 其他消息（Note/CC/Clock...）：忽略。实时 MIDI 流是按消息完整投递的，
            // 正常不会出现消息数据字节在此被误判（RtMidi 按消息切分）。
            continue;
        }
    }
}

void MtcDecoder::feedQuarterFrame(int piece, int value)
{
    if (piece < 0 || piece > 7) {
        return;
    }

    if (piece == _lastPiece) {
        // piece 未递增：传输停在原地（暂停/挂起），不重复组帧
        return;
    }

    _nibbles[piece] = value & 0x0F;

    if (piece == 7) {
        const int flag = (value >> 1) & 0x03;
        if (flag != _frameRateFlag) {
            _frameRateFlag = flag;
            emit frameRateDetected(flagToType(flag));
        }
    }

    if (piece >= 4) {
        _haveHighGroup = true;
    } else {
        _haveLowGroup = true;
    }

    // 前向：收到 piece 3 → 本组帧/秒 + 上一组分/时
    if (piece == 3 && _haveHighGroup) {
        emitAssembled(true);
        _lastPiece = piece;
        return;
    }
    // 反向：收到 piece 7 → 本组分/时 + 上一组帧/秒
    if (piece == 7 && _haveLowGroup) {
        emitAssembled(false);
        _lastPiece = piece;
        return;
    }

    _lastPiece = piece;
}

void MtcDecoder::feedFullFrame(int hours, int minutes, int seconds, int frames, int rateFlag)
{
    if (rateFlag != _frameRateFlag) {
        _frameRateFlag = rateFlag;
        emit frameRateDetected(flagToType(rateFlag));
    }

    TimeCodeFrame frame;
    frame.type = effectiveType();
    frame.hours = hours & 0x1F;
    frame.minutes = minutes & 0x3F;
    frame.seconds = seconds & 0x3F;
    frame.frames = frames & 0x1F;

    emit newFrame(frame);
}

void MtcDecoder::emitAssembled(bool forward)
{
    TimeCodeFrame frame;
    frame.type = effectiveType();   // 自动识别：来自 piece7 / Full-Frame 的帧率标志

    if (forward) {
        frame.frames = (_nibbles[1] << 4) | _nibbles[0];
        frame.seconds = (_nibbles[3] << 4) | _nibbles[2];
        frame.minutes = (_nibbles[5] << 4) | _nibbles[4];
        frame.hours = (_nibbles[7] << 4) | _nibbles[6];
        frame.hours &= 0x1F;
    } else {
        // 反向（seek/回扫）：piece 4~7 是新位置的分/时，piece 0~3 是上一位置的帧/秒
        frame.minutes = (_nibbles[5] << 4) | _nibbles[4];
        frame.hours = (_nibbles[7] << 4) | _nibbles[6];
        frame.hours &= 0x1F;
        frame.frames = (_nibbles[1] << 4) | _nibbles[0];
        frame.seconds = (_nibbles[3] << 4) | _nibbles[2];
    }

    emit newFrame(frame);
}
