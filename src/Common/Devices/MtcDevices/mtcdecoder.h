#ifndef MTCDECODER_H
#define MTCDECODER_H

#include <QObject>
#include <QByteArray>

#include "Common/AppConfig/TimeCodeDefines.h"

// MtcDevices 以 DLL 形式提供（与其他设备库一致）：编译本 DLL 时导出，使用者导入
#if defined(MTCDEVICES_LIBRARY)
#define MTCDEVICES_EXPORT Q_DECL_EXPORT
#else
#define MTCDEVICES_EXPORT Q_DECL_IMPORT
#endif

/**
 * @brief MTC（MIDI Time Code）解码器
 *
 * 将 MIDI 字节流解析为 TimeCodeFrame。支持两种消息：
 *  1. Quarter-Frame（0xF1 + 数据字节）：8 个 QF 打包 2 帧时间。
 *     piece 0~3 顺序到达（前向）携带 帧低位/帧高位/秒低位/秒高位，
 *     piece 4~7 顺序到达携带 分低位/分高位/时低位/时高位。
 *     前向：收到 piece 3 即可用"本组 0~3 + 上一组 4~7"组出完整时间码；
 *     反向：收到 piece 7 即可用"本组 4~7 + 上一组 0~3"组出完整时间码。
 *  2. Full-Frame SysEx（F0 7F 7F 01 01 hh mm ss ff F7）：一次携带完整时间，
 *     通常用于定位/跳转，直接发 newFrame。
 *
 * 该类只做解析，不持有任何设备 —— 字节由 MtcReceiver（RtMidiIn 回调线程）
 * 经 Qt 信号队列投递到 feedMidi()。
 */
class MTCDEVICES_EXPORT MtcDecoder : public QObject {
    Q_OBJECT

public:
    explicit MtcDecoder(QObject *parent = nullptr);

    /** @brief 设置回退帧率类型（仅在尚未收到任何帧率标志时使用；收到 QF/Full-Frame 后以自动识别为准） */
    void setTimecodeType(TimeCodeType type) { _type = type; }

    /** @brief 当前生效的帧率类型：已识别 → 按流内标志，否则回退到手动设置 */
    TimeCodeType effectiveType() const
    {
        return _frameRateFlag >= 0 ? flagToType(_frameRateFlag) : _type;
    }

    /** @brief MTC 帧率标志（piece7 / Full-Frame 高 2 位）→ 时间码类型 */
    static TimeCodeType flagToType(int flag)
    {
        switch (flag & 0x03) {
            case 0:  return TimeCodeType::Film;     // 24fps
            case 1:  return TimeCodeType::PAL;      // 25fps
            case 2:  return TimeCodeType::NTSC_DF;  // 30DF
            default: return TimeCodeType::NTSC;     // 30fps
        }
    }

    /** @brief 读取 QF piece7 中编码的帧率标志（-1 = 尚未收到） */
    int lastFrameRateFlag() const { return _frameRateFlag; }

public slots:
    /** @brief 喂入原始 MIDI 字节流（可包含任意消息，内部只挑 MTC 相关的） */
    void feedMidi(const QByteArray &data);

    /** @brief 直接喂入一个 QF（piece: 0~7, value: 低 4 位数据），便于测试 */
    void feedQuarterFrame(int piece, int value);

    /** @brief 直接喂入一条 Full-Frame SysEx 数据体（去掉 F0/F7 的 7 字节） */
    void feedFullFrame(int hours, int minutes, int seconds, int frames, int rateFlag);

    /** @brief 复位状态机（换设备/换源时调用） */
    void reset();

signals:
    /** @brief 组出一帧完整时间码（QF 组帧成功或收到 Full-Frame） */
    void newFrame(TimeCodeFrame frame);

    /** @brief 从码流中识别到（或变更）帧率格式时发出，便于 UI/节点展示 */
    void frameRateDetected(TimeCodeType type);

private:
    void emitAssembled(bool forward);

    TimeCodeType _type {TimeCodeType::PAL};

    // QF 状态机
    int _nibbles[8] {0, 0, 0, 0, 0, 0, 0, 0}; ///< piece -> 数据
    int _lastPiece {-1};                       ///< 上一个到达的 piece（-1 = 无）
    bool _haveHighGroup {false};               ///< 已有 minutes/hours（piece 4~7）
    bool _haveLowGroup {false};                ///< 已有 frames/seconds（piece 0~3）
    int _frameRateFlag {-1};                   ///< piece7 高位帧率标志：0=24,1=25,2=30DF,3=30

    // 原始字节流解析（SysEx 可能分多个 MIDI 消息到达）
    QByteArray _sysexBuffer;
    bool _inSysex {false};
};

#endif // MTCDECODER_H
