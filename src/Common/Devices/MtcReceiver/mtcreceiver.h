#ifndef MTCRECEIVER_H
#define MTCRECEIVER_H

#include <QObject>
#include <QStringList>

#include <RtMidi.h>

#include "Common/AppConfig/TimeCodeDefines.h"
#include "../MtcDecoder/mtcdecoder.h"

/**
 * @brief MTC 接收器：封装 RtMidiIn，把指定 MIDI 输入设备的字节流喂给 MtcDecoder
 *
 * 设备名格式与 LTCReceiver 一致："索引: 名称"（如 "0: loopMIDI Port"）。
 * RtMidiIn 回调运行在其内部输入线程，字节经 Qt 队列信号投递到解码器（主线程）。
 */
class MtcReceiver : public QObject {
    Q_OBJECT

public:
    explicit MtcReceiver(QObject *parent = nullptr);
    ~MtcReceiver() override;

    /** @brief 枚举系统当前可用的 MIDI 输入端口，返回 "索引: 名称" 列表 */
    static QStringList availableDevices();

public slots:
    void start(QString deviceName);
    void stop();

    int getPortIndex() const { return _portIndex; }
    QString getDeviceName() const { return _currentDeviceName; }

signals:
    void newFrame(TimeCodeFrame frame);
    void statusChanged(bool error, QString message);
    /** @brief 转发自 MtcDecoder：码流中识别到（或变更）帧率格式 */
    void frameRateDetected(TimeCodeType type);

private:
    static void midiCallback(double timeStamp, std::vector<unsigned char> *message, void *userData);

    RtMidiIn *_midiIn {nullptr};
    MtcDecoder *_decoder {nullptr};
    bool _isRunning {false};
    int _portIndex {-1};
    QString _currentDeviceName;
};

#endif // MTCRECEIVER_H
