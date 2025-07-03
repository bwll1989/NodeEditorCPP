#ifndef LTCRECEIVER_H
#define LTCRECEIVER_H

#include <QObject>

#include "TimeCodeDefines.h"
#include "../../Common/Devices/LtcDecoder/ltcdecoder.h"
#include "portaudio.h"
#include "pa_asio.h"

class LTCReceiver: public QObject
{
Q_OBJECT

public:
  explicit LTCReceiver(QObject *parent = nullptr);
  ~LTCReceiver();

public slots:
  void start(QString deviceName);
  void stop();
  void setChannel(int channel) { 
    _selectedChannel = channel;
    if (_isRunning && _paStream) {
      // 如果是 ASIO 设备，需要重新启动流
      const PaStreamInfo* info = Pa_GetStreamInfo(_paStream);
      if (info && _isASIO) {
        stop();
        start(_currentDeviceName);
      }
    }
  }
  int getDeviceIndex() const { return _currentDeviceIndex; }
  QString getDeviceName() const { return _currentDeviceName; }
  int getChannel() const { return _selectedChannel; }
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
  bool isASIODevice(int deviceIndex) const;
  void setupASIOStream(PaStreamParameters& params);

private:
  LtcDecoder* _decoder {nullptr};
  PaStream* _paStream {nullptr};
  bool _isRunning {false};
  int _selectedChannel {0};
  bool _isASIO {false};
  int _currentDeviceIndex {0};
  QString _currentDeviceName;
  // 音频参数
  static constexpr int SAMPLE_RATE = 44100;
  static constexpr int FRAMES_PER_BUFFER = 512;
  static constexpr int NUM_CHANNELS = 2;
};

#endif // LTCRECEIVER_H
