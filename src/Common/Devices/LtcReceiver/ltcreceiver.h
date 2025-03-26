#ifndef LTCRECEIVER_H
#define LTCRECEIVER_H

#include <QObject>

#include "TimeCodeMessage.h"
#include "../../Common/Devices/LtcDecoder/ltcdecoder.h"
#include "portaudio.h"

class LTCReceiver: public QObject
{
Q_OBJECT

public:
  LTCReceiver(QObject *parent=nullptr);
  ~LTCReceiver();

public slots:
  void start(int deviceIndex);
  void stop();

signals:
  void newFrame(TimeCodeFrame frame);
  void statusChanged(bool error, QString message);

private:
  // PortAudio 回调函数
  static int paCallback(const void* inputBuffer, void* outputBuffer,
                       unsigned long framesPerBuffer,
                       const PaStreamCallbackTimeInfo* timeInfo,
                       PaStreamCallbackFlags statusFlags,
                       void* userData);

  // 处理音频数据
  void processAudioData(const char* buffer, size_t size);

private:
  LTCDecoder* _decoder {nullptr};
  PaStream* _paStream {nullptr};
  bool _isRunning {false};

  // 音频参数
  static constexpr int SAMPLE_RATE = 48000;
  static constexpr int FRAMES_PER_BUFFER = 512;
  static constexpr int NUM_CHANNELS = 1;
};

#endif // LTCRECEIVER_H
